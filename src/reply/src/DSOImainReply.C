//////////////////////////////////////////////////////
//  FILE:  DSOImainReply.C
//
//  PURPOSE: To receive message from the ReplyToQ and
//	process them before sending them back to 
//	a 'Requestor' application.
//  
//  REVISION HISTORY:
//-----------------------------------------------
//  version	date		author	description	CR#
//-----------------------------------------------
//  1.00.00	06/24/97	lparks	main of the 
//					Reply for DSOI
//  2.00.01	09/24/97	lparks	initial release
//  2.00.02	10/09/97	lparks	removed parameter from putMsg
//					and return code from 
//					formatReport and changed
//					Critical error number
//  2.00.02	10/09/97	lparks	Updated 'USAGE'
//
//  6.00.00     08/04/99        rfuruta CR582, critical err msg should go to alarm file
//  6.00.00     08/04/99        rfuruta CR590, critical err msg should go to alarm file
//  7.00.00     12/23/99        rfuruta CR3126, changed DSOIrouteMQ to DSOIdistribute
//  7.01.00     07/03/00        rfuruta CR 4275, 4P connection
//  8.00.00     09/20/00        rfuruta CR 4767, remove 4P connection
//  8.03.00     06/07/01        rfuruta CR5391, translate the msgId to character
//  8.03.00     06/07/01        rfuruta CR5873, DSOIReply will continue to read until no more msgs
//  8.03.00     07/25/01        rfuruta CR6781, Select Section from service order
//  8.06.00     05/17/02        rfuruta CR7617, Assign a '88' section number to an invalid section name
//  8.06.00     06/07/02        rfuruta CR7697, Pass back SopId to SOM
//  8.08.00     01/02/03        rfuruta CR8000, upgrade to HP-UX 11.0
//  8.09.00     02/12/03        rfuruta CR8010, set waitInterval time
//  8.09.00     03/22/03        rfuruta CR8180, fix memory leaks
//  9.02.00     09/22/03        rfuruta CR8582, return SopId to requesting application
//  9.03.00     12/08/03        rfuruta CR 8761, New function codes(NQ, NT) for WebSOP
//  9.04.00     02/11/04        rfuruta CR 8911, set Expiry time on reply msg
//  9.06.00     08/17/04        rfuruta CR 9194, New function codes (NU, NR, NE)
//  9.08.00     01/03/05        rfuruta CR 9745, set Priority
//  9.08.00     01/19/05        rfuruta CR 9665, RSOLAR and SOLAR expanding the QS message
//  9.08.00     01/19/05        rfuruta CR 9667, new function 'SQ' for RSOLAR
//  9.08.00     02/15/05        rfuruta CR 9873, return CorrelId
//  9.10.00     05/16/05        rfuruta CR 9898, new function 'RQ' for RSOLAR
// 10.01.00     07/05/05        rfuruta CR10094, ECRIS error correction
// 10.04.00     02/01/07        rfuruta CR10999, migrate DSOI to Solaris
// 10.04.01     08/09/07        rfuruta CR11307, fix errors associated with migrating to Solaris
// 10.04.02     08/16/07        rfuruta CR11357, fix data conversion problem with the RQ function
//
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <iostream.h>
#include <cstring>
#include <cerrno>
#include <new>
//#include <libc.h> 		// used for exit() command
#include "DSOIlist.H"
#include "DSOIThrowError.H"
#include "DSOIThrowSendError.H"
#include "DSOIsendMQ.H"
#include "DSOIreceiveMQ.H"
#include "DSOImessageReply.H"
#include "DSOIlogMQ.H"
#include "DSOItoolbox.H"

unsigned char _msgId[DSOI_STRING_25];
static MQBYTE24 _correlId;
static char _tmpTime[DSOI_STRING_15];
static char _tmpApplDate[DSOI_STRING_9];
static char _tmpApplName[DSOI_STRING_29];
static char _tmpApplTime[DSOI_STRING_9];
static char sopId[4];


////////////////////////////////////////////////
//  FUNCTION: int main ( int argc, char* argv[])
//
//  PURPOSE: To process all messages on the queue
//	sent from a 'Requestor' application.
//
//  METHOD: Is triggered to start when there is one
//	message on the queue sent from a 'Requestor'
//	application.  Then
//	processes all messages on the queue until there
//	is no more messages.
//
//  PARAMETERS: int argc - the number of  command line parameters
//		char* argv[] - the command line parameters
//  
//  OTHER OUTPUTS: Error messages to the QL_DSOI_ERRORQ when
//	a message could not be processed.  Also sends an
//	error message back to the ReplyTQ when the
//	message could not be processed but received so that
//	a 'Requestor' application 
//	could correlate the message with a message
//	sent to DSOI.
//  
//  RETURNS: exits ONLY if there is an exception thrown.
// 
//  OTHER INPUTS: Requires a command line parameter as to
//	where the messages are received, the name of the 
//	MQSeries message queue.
//
//  KNOWN USERS: DSOI for all reply messages.
//
//  FUNCTIONS USED: DSOIsendMQ::putMsg(), DSOIreceiveMQ::getMsg(),
//	strstr(), new(), DSOIerrorHandler::sendErrorMessage(), strcpy()
//	strcat(), strcmp(), DSOIlist::is_present(), DSOIlist::insert(),
//	DSOIlist::get_reply(), try(), catch()
//
//  KNOWN DEFECTS:
//
//  REVISION HISTORY:
//
//////////////////////////////////////////////////////
int main ( int argc, char* argv[])
{
	List repList;
	Reply* queueReply;
	DSOIsendMQ *sendErrorQ='\0';
	DSOIsendMQ *reportQ;
        int waitIntervalTime = 0;
        int setExpiry = 0;
        int setPriority = 0;
        int maxMsgsReadin = 0;
        int msgCnt = 0;

        char* rqstSopId = new char[200];
        char* expryInfo = new char[200];

        char* newMessage = new char[1048576];
        char* msgTrunc   = new char[450];
	char  FunctionCodeValue[3];
	const char* FunctionCodeTag = "FunctionCode=";


	//DSOIsendMQ  *QL_DSOI_ReportQ;

	if(argc < 3 )	// executing with name of queue manager
	{
		cout << "Usage:  " << endl;
		cout << argv[0] ;
		cout << "  receiveQ  " <<    "  queueManager  " << endl; 
		exit(1);
	}

	DSOIerrorHandler mainErrObj;
	register int returnCode = 0;

	try{
		//The following code is to determine if this process is 
		// executed manually as a test or through the trigger monitor.

		register int x,y;

		if(strstr(argv[1], "TMC" ) == NULL) //not triggered
		{
			x = 1;
			y = 2;
		}
		else // triggered 
		{
			x = 2; y = 3;
		}

		try
		{
			//Queue to send errors
                        setExpiry = 0;
                        setPriority = 0;
			sendErrorQ = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_ERRORQ",argv[y]);

			if (sendErrorQ == 0)
			  {
				returnCode = -1;
				mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
				mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
				return returnCode;
			  }
		}

		catch (ThrowError err)
		{

#ifdef DEBUG
cout << "ERRORQ not available" << endl;
cout << "ERROR= " ;
cout << DSOIerrorHandler::_formatErrMsg << endl;
cout << endl;
#endif

			DSOIlogMQ*  logQ = new DSOIlogMQ();

			if(logQ == 0)
			{
				returnCode = -1;
				mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
				mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
				return returnCode;
			}

			logQ->logCriticalError(DSOIerrorHandler::_formatErrMsg);
                        if (logQ)            delete logQ;
			exit(1);
		}

		// the receive queue is from the command line parameter
		DSOIreceiveMQ* receiveQueue = new DSOIreceiveMQ(argv[x],argv[y]);
		if(receiveQueue == 0)
		{
			returnCode = -1;
			mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
			mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
			return returnCode;
		}

		// _requestQueue = SOP receive queue
                DSOImessageReply::_requestQueue = new char[strlen(argv[x]) + 1];
		if (DSOImessageReply::_requestQueue == 0)
		{ returnCode = -1;
		  mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", returnCode, (char *) "main",
                                         (char *) __FILE__, __LINE__);
		  mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
		  return returnCode;
	        }
                strcpy(DSOImessageReply::_requestQueue, argv[x]);

		// Queue to get messages from for correlation with MsgId
		DSOIreceiveMQ *receiveHoldQ = new DSOIreceiveMQ((char *) "QL_DSOI_HOLD",argv[y]);

		if(receiveHoldQ == 0)
		{
			returnCode = -1;
			mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
			mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
			return returnCode;
		}

                memset(_Date,0,11);
                calculateDate(_Date);

		//Queue to send report messages to
                setExpiry = 0;
                setPriority = 0;
		reportQ = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_REPORTQ",argv[y]);

		if(reportQ == 0)
		{
			returnCode = -1;
			mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
			mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
			return returnCode;
		}

                waitIntervalTime = 0;
                maxMsgsReadin = 0;
                returnCode = readParmFile((char*) "/opt/DSOI/etc/ParmFile", 
                                          &waitIntervalTime, &maxMsgsReadin);
                if (returnCode == 0)
                  { receiveQueue->setWaitInterval(waitIntervalTime);
                  }
                else
                  { receiveQueue->setWaitInterval(0);
                    maxMsgsReadin = 100;
                  }

                memset(rqstSopId,0,200);
		rqstSopId[0] = '\0';
                returnCode = readRqstSopId((char*) "/opt/DSOI/etc/rqstSopId", rqstSopId);
		
                memset(expryInfo,0,200);
		expryInfo[0] = '\0';
                returnCode = readExpryInfo((char*) "/opt/DSOI/etc/expryInfo", expryInfo);
		
		returnCode = 0;
		while((returnCode != MQRC_NO_MSG_AVAILABLE) && (msgCnt < maxMsgsReadin))
		{       msgCnt++;
			// set convert option on because DSOIReply reads
			// messages sent from MVS
			receiveQueue->setConvert(1);
			// get messages from the receiveing Queue
			DSOIreceiveMQ::_getCorrel = 0;
                        memset(_getTime,0,_sizeOfTime);
                        memset(_putApplName,0,28);
                        memset(_putApplDate,0,8);
                        memset(_putApplTime,0,6);
                        memset(_msgId,0,25);
                        memset(_correlId,0,24);
                        setExpiry = 0;
                        setPriority = 0;

	 		returnCode = receiveQueue->getMsg(_putApplName,_putApplDate,_putApplTime, _getTime,_msgId,(MQBYTE24*)_correlId, sendErrorQ);

			if(returnCode != 0 )
			{
				// could not get message

				if(returnCode != MQRC_NO_MSG_AVAILABLE)
				{
					// no messages on queue

					mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
					if(returnCode != MQRC_TRUNCATED_MSG_FAILED)
					{
						mainErrObj.formatErrMsg((char *) "DSOIREPLY:1005:CRITICAL:DSOImainReply() failed because MQGET() failed: DSOIREPLY exiting for ",argv[x], returnCode,(char *) "main",(char *)__FILE__,__LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
						exit(1);
					}
				}
			}

			else
			{
				//received a message

#ifdef DEBUG
cout << "Time = " << _getTime << " and Date " << _Date << endl;
cout << "_putApplName = " << _putApplName << endl;
cout << "message = " << receiveQueue->message  << endl;
#endif

				//check if ONS message

				if(strstr(_putApplName, "ONS" ) != NULL) 
			        {	DSOImessageReply::_ONSflag = 1;
#ifdef DEBUG
cout << "yes it IS ONS!" << endl;
#endif
				}
                                else
			        {	DSOImessageReply::_ONSflag = 0;
                                }

				// set the getCorrel flag to 1 which means that
				// a message with same msgId will be
				// received from the HOLDQ

				DSOIreceiveMQ::_getCorrel = 1;
                                memset(_tmpTime,0,11);
                                memset(_tmpApplName,0,28);
                                memset(_tmpApplDate,0,8);
                                memset(_tmpApplTime,0,6);
                                memset(_correlId,0,24);

	 		        returnCode = receiveHoldQ->getMsg(_tmpApplName,_tmpApplDate,
                                                                  _tmpApplTime,_tmpTime,_msgId,
                                                                  (MQBYTE24*)_correlId, sendErrorQ);

				if(returnCode != 0)
				{
					// there is no message correlated
					char id[49];
					char ptr[4];
					memset(id,0,49);
					id[0] = '\0';
					for (int i=0; i < 24; i++)
						{	memset(ptr,0,4);
							ptr[0] = '\0';
							int returnCode = sprintf(ptr,"%d",_msgId[i]);
							ptr[strlen(ptr)] = '\0';
							if (returnCode > 0)
								strcat(id, ptr);
                					else
                       						break;

						}

					mainErrObj.formatErrMsg((char *) "DSOIREPLY:1012:CRITICAL:errored because message could not be correlated:reply message will not be sent ! msgid = ", id, returnCode, (char *) "main", (char *) __FILE__, __LINE__);
					mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);

					returnCode = 0;
					continue;
				}

				else
				{
					// found correlated message

#ifdef DEBUG
cout << "message from HOLDQ= " << receiveHoldQ->message << endl;
#endif

					int ReturnCode = 0;

					if(!DSOImessageReply::_ONSflag)
						ReturnCode = DSOImessageReply::findNPANXX(receiveHoldQ->message);

					if(ReturnCode != 0)
					{
							mainErrObj.formatErrMsg((char *) "DSOIREPLY could not find an NPANXX value in the HOLDQ",(char *) "", returnCode,(char *) "main", (char *) __FILE__,__LINE__);
							mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
					}

					// process message

					memset(newMessage,0,1048576);
					newMessage[0] = '\0';
                                        memset(sopId,0,4);
					sopId[0] = '\0';
					returnCode = DSOImessageReply::messageMassage(receiveQueue->message,
                                               receiveHoldQ->message, receiveQueue->getMsgRecv(),sopId,sendErrorQ, newMessage);

					if(returnCode == -2)
					{
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
						exit(1);
					}

					else if(returnCode)
					{
						// messageMassage failed

						mainErrObj.formatErrMsg(DSOIerrorHandler::ReplyErrMsg,(char *) "", returnCode,(char *) "main", (char *) __FILE__,__LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
					        memset(newMessage,0,1048576);
					        newMessage[0] = '\0';
                                                memset(sopId,0,4);
					        sopId[0] = '\0';
						int length = strlen(DSOIerrorHandler::ReplyErrMsg);
						int returnCode = DSOImessageReply::messageMassage(DSOIerrorHandler::ReplyErrMsg,receiveHoldQ->message, length, sopId, sendErrorQ, newMessage);
						if(returnCode)
						{
							mainErrObj.formatErrMsg((char *) "DSOIREPLY could not send error to REQUESTOR",(char *) "", returnCode,(char *) "main", (char *) __FILE__,__LINE__);
							mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
				 		}

						mainErrObj.formatErrMsg(DSOIerrorHandler::ReplyErrMsg,(char *) "", returnCode, (char *) "main", (char *) __FILE__,__LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
					}	// end messageMassage failed

					char* tempSopId = rqstSopId;
					char rqstr[12];
					int passSopId = 0;
                                        int xyz = 1;

                                        while(xyz = 1)
				           { rqstr[0] = '\0';
                                             if (sscanf(tempSopId,"%s", rqstr) == EOF)   break;
					     if (strcmp(rqstr, "xxxxx") == NULL)         break;
                                             if (strstr(receiveHoldQ->replyToQ, rqstr))
                                               { passSopId = 1;
   				                 break;
				               }
					     tempSopId = tempSopId + strlen(rqstr) + 1;
                                           }

                                        char* tempExpry = expryInfo;
                                        setExpiry = 0;
                                        xyz = 1;

                                        while(xyz = 1)
				           { rqstr[0] = '\0';
                                             if (sscanf(tempExpry,"%s", rqstr) == EOF)   break;
                                             if (strcmp(rqstr, "xxxxx") == NULL)         break;
                                             if (strstr(receiveHoldQ->replyToQ, rqstr))
                                               { setExpiry = 1;
                                                 break;
                                               }
                                             tempExpry = tempExpry + strlen(rqstr) + 1;
                                           }

                                        if (!(strstr(newMessage, "SopId=")))
                                          { if((strstr(receiveHoldQ->replyToQ, "_SOSE_"))  ||
                                               (strstr(receiveHoldQ->replyToQ, "_SWIFT_")) ||
                                               (strstr(receiveHoldQ->replyToQ, "_SOM_"))   ||
                                               (strstr(receiveHoldQ->replyToQ, "_VCSR_"))  ||
                                               (passSopId == 1) )
                                              { int idx = 0;
                                                int idy = strlen(newMessage) + 10;
                                                int idz = strlen(newMessage);
                                                while (idx < 1) 
                                                  { newMessage[idy] = newMessage[idz];
                                                    idy = idy - 1;
                                                    idz = idz - 1;
                                                    if (idz < 0)    idx = 99;
                                                  }
                                                newMessage[0] = 'S';
                                                newMessage[1] = 'o';
                                                newMessage[2] = 'p';
                                                newMessage[3] = 'I';
                                                newMessage[4] = 'd';
                                                newMessage[5] = '=';
                                                newMessage[6] = sopId[0];
                                                newMessage[7] = sopId[1]; 
                                                newMessage[8] = sopId[2];
                                                newMessage[9] = '\xff';
                                              }
                                          }

#ifdef DEBUG
cout << "replyToQMgr in main after hold=" << receiveHoldQ->replyToQMgr <<  endl;
cout << "replyToQ in main after hold="    << receiveHoldQ->replyToQ    <<  endl;
// cout << "queueReply="                     << queueReply->replyQ        << endl;
cout << "receiveQueue->message=" << receiveQueue->message << endl;
cout << "           newMessage=" << newMessage << endl;
#endif
// cout << "test message=" << receiveQueue->message << endl;
// cout << "test  replyToQ=" << receiveHoldQ->replyToQ << endl;
// cout << "test  replyToQ=" << receiveHoldQ->replyToQMgr << endl;
// cout << "test  newMessage=" << newMessage   <<  endl;

                                        setPriority = 0;
					DSOIsendMQ* queueReply = new DSOIsendMQ(setExpiry, setPriority, receiveHoldQ->replyToQ,
                                                                                     receiveHoldQ->replyToQMgr);

					if(queueReply == 0)
					{
					 mainErrObj.formatErrMsg((char *) "DSOIREPLY:1008:CRITICAL:unable to allocate new memory",
                                                     receiveHoldQ->replyToQ, returnCode, (char *) "main", (char *) __FILE__,__LINE__);
				 	 mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
					 exit(1);
					}
#ifdef DEBUG
cout << "after sendQueue"  << endl;
#endif

                                        memset(_putTime,0,_sizeOfTime);

					// put message on ReplyToQ

					returnCode = queueReply->putMsg(_putTime, _msgId, _correlId, newMessage);

					if(returnCode)
					{
						mainErrObj.formatErrMsg((char *) "DSOIREPLY:1009:CRITICAL:errored because putMsg() failed on ReplyToQ=",
                                                            receiveHoldQ->replyToQ, returnCode, (char *) "main",(char *) __FILE__,__LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
						exit(1);
					}
		                        delete queueReply;
                                        setExpiry = 0;

					// format report message
					memset(msgTrunc,0,450);
					msgTrunc[0] = '\0';
                                        buildMsgTrunc(sopId, receiveHoldQ->message, newMessage, msgTrunc);

// cout << "test   after msgTrunc=" << msgTrunc <<  endl;

                                        memset(_reportMsg,0,1024);

					formatReport(_reportMsg,
							_Date,
							_putTime,
							_getTime,
							"DSOIREPLY",
							_putApplName,
							_putApplTime,
							_putApplDate,
							_msgId,
							msgTrunc);

					// send report message
					
                                        memset(_putTime,0,_sizeOfTime);
					returnCode = reportQ->putMsg(_putTime,_msgId,_correlId,_reportMsg);
					if(returnCode)
					{
						mainErrObj.formatErrMsg((char *) "DSOIREPLY:1007:CRITICAL:putMsg() failed on DSOI_REPORTQ:DSOIREPLY exiting ", 
                                                                        (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
						exit(1);
					}
                                        FunctionCodeValue[0] = '\0';
                                        findTagValue(receiveHoldQ->message,(char *)FunctionCodeTag,FunctionCodeValue);
                                        if (strcmp(FunctionCodeValue,"RQ")==0)
                                           {
					        delete receiveQueue;
						DSOIreceiveMQ* receiveQueue = new DSOIreceiveMQ(argv[x],argv[y]);
						if(receiveQueue == 0)
						{
							mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", 1, (char *) "main", (char *) __FILE__, __LINE__);
							mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
							exit(1);
						}
					        delete receiveHoldQ;
						DSOIreceiveMQ *receiveHoldQ = new DSOIreceiveMQ((char *) "QL_DSOI_HOLD",argv[y]);

						if(receiveHoldQ == 0)
						{
							mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", 1, (char *) "main", (char *) __FILE__, __LINE__);
							mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
							exit(1);
						}
					        delete reportQ;
                				setExpiry = 0;
                				setPriority = 0;
						reportQ = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_REPORTQ",argv[y]);

						if(reportQ == 0)
						{
							mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", 1, (char *) "main", (char *) __FILE__, __LINE__);
							mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
							exit(1);

                                           	}
                                           }

#ifdef DEBUG
cout << "_reportMsg " << _reportMsg << endl;
cout << "putAppl= " << _putApplName << endl;
#endif
// cout << "test  ReportMsg=" << _reportMsg  << endl;

				}	// end of else found correlated message

				DSOIreceiveMQ::_getCorrel = 0;

			}	// end of else there was no error

		}	// end of while

//              delete [] DSOImessageReply::_requestQueue;
//              if (sendErrorQ)      delete sendErrorQ;
//		if (receiveQueue)    delete receiveQueue;
//		if (receiveHoldQ)    delete receiveHoldQ;
//		if (reportQ)         delete reportQ;

	}	// end of try

	catch ( ThrowError& err )
	{

#ifdef DEBUG
cout << "ERROR=  ";
cout << DSOIerrorHandler::_formatErrMsg << endl;
cout << endl;
#endif

		exit(1);
	}

	catch (ThrowSendError& err)
	{

#ifdef DEBUG
cout << "in catch(...) " << endl;
#endif

		cerr << endl << endl;
		cerr << "ERROR SENDING TO ERRORQ, error=  ";
		cerr << DSOIerrorHandler::_formatErrMsg << endl;
		cerr << endl << endl;
		cerr << "ReplyErrorMessage=  ";
		cerr << DSOIerrorHandler::ReplyErrMsg << endl;

		DSOIlogMQ* logQ = new DSOIlogMQ();

		if(logQ == 0)
		{
			returnCode = -1;
			mainErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory", (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
			mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
			return returnCode;
		}

		try
		{
			logQ->logCriticalError(DSOIerrorHandler::_formatErrMsg);
		}

		catch( ThrowError& err )
		{
		        cout << "Error sending to log critical error!" << endl;
		        cout << "Error Message = " << DSOIerrorHandler::_formatErrMsg << endl;
		}

		delete logQ;
		exit(1);
	}

	catch(...)
	{

#ifdef DEBUG
cout << "in catch(...) " << endl;
#endif

		cerr << "EXCEPTION CAUGHT!" << endl;
		exit(1);
	}

	exit(0);
}


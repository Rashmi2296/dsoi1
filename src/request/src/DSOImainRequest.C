//////////////////////////////////////////////////////////
//  FILE: DSOImainRequest.C 
//
//  PURPOSE: The main function to receive requests from ICADS.
//  
//  REVISION HISTORY:
//--------------------------------------------------------
//  version	date		author	description
//  1.00.00	06/30/97	lparks	initial release IMA
//  2.00.01	09/24/97	lparks	initial release EASTERN
//  2.00.02	10/09/97	lparks	updated 'USAGE'
//  2.00.06     10/23/97  	lparks	updated errors sent to 'REQUESTOR'
//  6.00.00     08/04/99        rfuruta CR582, critical err msg should go to alarm file
//  6.00.00     08/04/99        rfuruta CR590, critical err msg should go to alarm file
//  7.00.00     12/23/99        rfuruta CR3123, changed DSOIrouteMQ to DSOIdistribute
//  7.01.00     07/03/00        rfuruta CR 4275, 4P connection
//  8.00.00     08/23/00        rfuruta CR 4767, remove 4P connection
//  8.03.00     06/07/01        rfuruta CR 5391, add LSRID to report
//  8.03.00     07/25/01        rfuruta CR 6781, Select Section from service order
//  8.04.00     02/14/02        rfuruta CR 7464, Remve edit chk for exchg for SBN to SOLAR
//  8.05.00     02/25/02        rfuruta CR 7495, Include original request msg to logfile report
//  8.05.00     03/19/02        rfuruta CR 7541, Compute OwnershipCode and pass to SOPs
//  8.08.00     01/02/03        rfuruta CR 8000, upgrade to HP-UX 11.0
//  8.09.00     01/12/03        rfuruta CR 8010, set waitInterval time
//  8.09.00     03/02/03        rfuruta CR 8037, Add owner code to logfile report
//  8.09.00     03/22/03        rfuruta CR 8180, fix memory leaks
//  8.09.00     03/27/03        rfuruta CR 8144, ROMS officeCd and typistId
//  9.03.00     12/08/03        rfuruta CR 8761, New function codes(NQ, NT) for WebSOP
//  9.04.00     02/08/04        rfuruta CR 8948, write 'IQ' txns to separate queue
//  9.04.00     02/11/04        rfuruta CR 8911, set Expiry time
//  9.04.00     03/10/04        rfuruta CR 9039, insert 'OWN' info to logfile rpt
//  9.06.00     05/10/04        rfuruta CR 9194, New function codes (NU, NR, NE)
//  9.06.00     08/16/04        rfuruta CR 9413, Check for other border towns, in addition to 208
//  9.08.00     01/03/05        rfuruta CR 9745, set Priority
//  9.08.00     01/18/05        rfuruta CR 9665, pass all QueryListTypes to SOLAR
//  9.08.00     01/20/05        rfuruta CR 9667, new function 'SQ' for RSOLAR
//  9.08.00     01/20/05        rfuruta CR 9791, new function 'NL' nolock for RSOLAR, SOLAR, SOPAD
//  9.08.00     02/15/05        rfuruta CR 9873, return CorrelId
//  9.09.00     03/15/05        rfuruta CR 9664, expand MAN# retrieval for SOLAR and RSOLAR
//  9.10.00     05/16/05        rfuruta CR 9898, new rate quote function 'RQ' for RSOLAR
// 10.01.00     07/05/05        rfuruta CR10094, ECRIS error correction
// 10.00.00     07/26/05        rfuruta CR10186, write 'CL' txns to separate queue
// 10.02.00     09/20/05        rfuruta CR10269, new function 'DX' for SOLAR- retrieve CSR data
// 10.04.00     02/01/07        rfuruta CR10999, migrate DSOI to Solaris
// 10.05.00     07/01/12        rfuruta CR11307, make the ServiceOrder tag/values optional on a DL function
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////////

#include <iostream.h>
#include <cstring>
#include <cerrno>
#include <new>
#include "DSOIThrowError.H"
#include "DSOIThrowSendError.H"
#include "DSOIsendMQ.H"
#include "DSOIreceiveMQ.H"
#include "DSOImessageRequest.H"
#include "DSOItoolbox.H"
#include "DSOIlogMQ.H"

int sendReport(DSOIsendMQ*, DSOIsendMQ*, char *);
int setExpiry = 0;
int setPriority = 0;
DSOIerrorHandler mainErrObj;
unsigned char _msgId[DSOI_STRING_25];
static MQBYTE24 _correlId;

char* functionCode= new char[3];
char* sopId=        new char[4];
char* sopToSend=    new char[48];
char* replyFromSop= new char[48];
char* ownerName=    new char[12];
char* ownerCode=    new char[2];
char* setPriortyA=  new char[2];
char* hldMsg=       new char[1048576];

////////////////////////////////////////////////
//  FUNCTION: int main ( int argc, char* argv[])
//
//  PURPOSE: To receive messages from the request queue
//	from ICADS.
//
//  METHOD: Creates an array to point to the SOPS and a send
//	queue for error messages only and a hold queue to send
//	messages that are correlated by the DSOIReply process.	
//	Messages are received until there are no more messages
//	to receive.
//
//  PARAMETERS: int argc - the number of command line parameters
//		char* argv[] - the command line parameters.
//  
//  OTHER OUTPUTS:  The message to send to the HOLDQ for correlation
//  
//  OTHER INPUTS: The reply to queue that is updated before the message
//	is sent to the hold q for correlation and the queue manager are both
//	command line parameters.
//
//  RETURNS: Exits with a return code of 1 if there is any catastrophic error
//	such as an object can't be created.
// 
//  FUNCTIONS USED: try(), catch(), strcpy(), strstr(), DSOIsendMQ::DSOIsendMQ(),
//	DSOIreceiveMQ::DSOIreceiveMQ(), DSOIreceiveMQ::getMsg()
//	DSOIerrorHandler::sendErrorMessage(), DSOIsendMQ::sendMsg()
//
//////////////////////////////////////////////////////

int main ( int argc, char* argv[])
{
        int waitIntervalTime = 0;
        int maxMsgsReadin = 0;
        int msgCnt = 0;

	if (argc < 3)
	  { cout << "Usage:  " << endl;
	    cout << argv[0]  << "    receiveQ   " << "   Qmanager" << endl;
	    exit(1);
	  }
	DSOIsendMQ* errorSendQ = '\0';
	DSOIsendMQ* reportQ;
	DSOIsendMQ* requestErrorReply =0; //queue of REQUESTOR
	register int returnCode =0;
	//this next check is for manual verses triggered
	// of this process.  MQSeries puts the trigger
	// message as the argv[1] argument.  It is not
	// necessary to use that argument.
	register int x= 0;
	register int y= 0;
        DSOIsendMQ* sendRSOdata;
        DSOIsendMQ* sendRSOquery;
        DSOIsendMQ* sendRSOiq;
        DSOIsendMQ* sendRSOcl;
        DSOIsendMQ* sendRSOdx;
	DSOIsendMQ* sendRSOATdata;
        DSOIsendMQ* sendESSdata;
        DSOIsendMQ* sendESSquery;
        DSOIsendMQ* sendESSiq;
        DSOIsendMQ* sendESScl;
        DSOIsendMQ* sendESSdx;
	DSOIsendMQ* sendESSATdata;
        DSOIsendMQ* sendESNdata;
        DSOIsendMQ* sendESNquery;
        DSOIsendMQ* sendESNiq;
        DSOIsendMQ* sendESNcl;
        DSOIsendMQ* sendESNdx;
	DSOIsendMQ* sendESNATdata;
        DSOIsendMQ* sendSOCdata;
        DSOIsendMQ* sendSOCATdata;
	DSOIsendMQ* sendSOCquery;
        DSOIsendMQ* sendSOCiq;
        DSOIsendMQ* sendSOCcl;
        DSOIsendMQ* sendSOCdx;
        DSOIsendMQ* sendSOUdata;
        DSOIsendMQ* sendSOUATdata;
	DSOIsendMQ* sendSOUquery;
        DSOIsendMQ* sendSOUiq;
        DSOIsendMQ* sendSOUcl;
        DSOIsendMQ* sendSOUdx;
        DSOIsendMQ* sendONSR;
        int openRSOdata  = 0;
        int openRSOquery = 0;
        int openRSOiq    = 0;
        int openRSOcl    = 0;
        int openRSOdx    = 0;
	int openRSOATdata  = 0;
        int openESSdata  = 0;
        int openESSquery = 0;
        int openESSiq    = 0;
        int openESScl    = 0;
        int openESSdx    = 0;
	int openESSATdata  = 0;
        int openESNdata  = 0;
        int openESNquery = 0;
        int openESNiq    = 0;
        int openESNcl    = 0;
        int openESNdx    = 0;
	int openESNATdata  = 0;
        int openSOCdata  = 0;
        int openSOCquery = 0;
        int openSOCATdata  = 0;
	int openSOCiq    = 0;
        int openSOCcl    = 0;
        int openSOCdx    = 0;
        int openSOUdata  = 0;
        int openSOUATdata  = 0;
	int openSOUquery = 0;
	int openSOUiq    = 0;
        int openSOUcl    = 0;
        int openSOUdx    = 0;
        int openONSR     = 0;

	if(strstr(argv[1], "TMC") != NULL)
	  { x = 2;
	    y = 3;
	  }
	else
	  { x = 1;
	    y = 2;
	  }

	try{
		try{
                        setExpiry = 0;
                        setPriority = 0;
			errorSendQ = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_ERRORQ",argv[y]);
		   }
		catch( ThrowError& err )
		   {
#ifdef DEBUG
cout << "In catch ThrowError " << endl;
cout << "lineNo= " << __LINE__ << endl;
cout << "ERRORQ not available!" << endl;
cout << "ERROR= "  ;
cout << DSOIerrorHandler::_formatErrMsg << endl;
cout << "   requestQueue=" << argv[x] << endl;
#endif
			DSOIlogMQ* logQ = new DSOIlogMQ();
			try{
			     logQ->logCriticalError(DSOIerrorHandler::_formatErrMsg);
			   }
			catch( ThrowError& err )
			   { cout << "Error sending to log critical error!" << endl;
			     cout << "Error Message = " << DSOIerrorHandler::_formatErrMsg << endl;
			   }
			delete logQ;
			if (errorSendQ !='\0')     delete errorSendQ;
			exit(1);
		   }
                memset(_getTime,0,_sizeOfTime);
                memset(_putApplName,0,28);
                memset(_putApplDate,0,8);
                memset(_putApplTime,0,6);
                memset(_msgId,0,25);
                memset(_correlId,0,24);

                // queue for receiving messages for routing with the last parameter
                // set to 1 so that the queue is opened in browse mode
                DSOIreceiveMQ  *dataRefQueue = new DSOIreceiveMQ((char *) "QL_DSOI_DATAREFQ",argv[y],1);

                returnCode = dataRefQueue->getMsg(_putApplName,_putApplDate,_putApplTime,
                                                  _getTime,_msgId,(MQBYTE24*)_correlId,errorSendQ);

                if (returnCode !=0)
                  { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:17:CRITICAL:getMsg() FAILED because MQGET() failed for the DATAREFQ:DSOIREQUEST is exiting",
                                            (char *) "QL_DSOI_DATAREFQ", returnCode,(char *) "main", (char *) __FILE__,__LINE__);
                    mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
                    exit(1);
                  }
                // need to reset the browse option so that messages are now taken
                // from the queue from now on
                dataRefQueue->setBrowse(0);

                returnCode = DSOImessageRequest::readDataRef(dataRefQueue->message);
                if (returnCode !=0)
                  { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:18:CRITICAL:readDataRef() FAILED for the DATAREFQ:DSOIREQUEST is exiting",
                                            (char *) "QL_DSOI_DATAREFQ", returnCode,(char *) "main", (char *) __FILE__,__LINE__);
                    mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
                    exit(1);
                  }
#ifdef DEBUG
cout << "main DATAREF+ " << dataRefQueue->message << endl;
cout << " requestQueue=" << argv[x] << endl;
#endif

                setExpiry = 0;
                setPriority = 0;
		DSOIsendMQ *sendHoldQ = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_HOLD",argv[y]);
		// queue for receiving messages from a requesting application
		DSOIreceiveMQ  *receiveQueue = new DSOIreceiveMQ(argv[x],argv[y]);

// The request queue name is in the form 'QC.NAME.DSOI.REQUEST'
// or in the form 'QL_NAME_DSOI_REQUEST'.  The ownerName is
// extracted from the request queue name and the '.' is
// replaced with '_'
                memset(ownerName,0,12);
                ownerName[0] = '_';
                int xyz = 1;
                for (int i=3; i<12; i++)
                   { if ((argv[x][i] == '.') || (argv[x][i] == '_'))
                          { ownerName[xyz] = '_';
                            i = 99;
                            xyz++;
                          }
                     else { ownerName[xyz] = argv[x][i];
                            xyz++;
                          }
                   }
                ownerName[xyz] = '\0';

		//Queue to send report messages to
                setExpiry = 0;
                setPriority = 0;
		reportQ = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_REPORTQ",argv[y]);

                memset(_Date,0,11);
                calculateDate(_Date);

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

                while((returnCode != MQRC_NO_MSG_AVAILABLE) && (msgCnt < maxMsgsReadin))
                  { msgCnt++;
                    memset(_getTime,0,_sizeOfTime);
                    memset(_putApplName,0,28);
                    memset(_putApplDate,0,8);
                    memset(_putApplTime,0,6);
                    memset(_msgId,0,25);
                    memset(_correlId,0,24);
                    setExpiry = 0;
                    setPriority = 0;
                    returnCode = 0;
                    returnCode = receiveQueue->getMsg(_putApplName,_putApplDate,_putApplTime,
                                                      _getTime,_msgId,(MQBYTE24*)_correlId,errorSendQ);
#ifdef DEBUG
   char messageId[49];
   char ptr[3];
   memset(messageId,0,49);
   for(int i=0; i < 24; i++)
      { memset(ptr,0,3);
        int rc = sprintf(ptr,"%02d",_msgId[i]);
        strcat(messageId, ptr);
      }
   cout << "Get msgId=" << messageId << endl;
   char correlationId[49];
   memset(correlationId,0,49);
   for(i=0; i < 24; i++)
      { memset(ptr,0,3);
        int rc = sprintf(ptr,"%02d",_correlId[i]);
        strcat(correlationId, ptr);
      }
   cout << "Get correlId=" << correlationId << endl;
#endif

		    if (returnCode != 0)
		      { //if message could not be received then exit because DSOI can't
			//do any processing
			if (returnCode != MQRC_NO_MSG_AVAILABLE)
			  { mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
			    if (returnCode != MQRC_TRUNCATED_MSG_FAILED)
			      { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:08:CRITICAL:getMsg() FAILED because MQGET() failed:DSOIREQUEST is exiting",
					(char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
				mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
				exit(1);
			      }//end of if(returnCode != MQRC_TRUNCATED_MSG_FAILED)
			  } // end of if(returnCode != MQRC_NO_MSG_AVAILABLE)
		      } // if(returnCode != 0) from getMsg()
		    else // message received and no errors
                      {
#ifdef DEBUG
cout << "received msg=" << receiveQueue->message << endl;
#endif
                        memset(DSOImessageRequest::_msgTrunc,0,450);
                        DSOImessageRequest::_msgTrunc[0] = '\0';
                        memset(sopToSend,0,48);
                        sopToSend[0] = '\0';
                        memset(replyFromSop,0,48);
                        replyFromSop[0] = '\0';
                        memset(functionCode,0,3);
                        functionCode[0] = '\0';
                        memset(sopId,0,4);
                        sopId[0] = '\0';
		        int returnCode = 0;
                        memset(ownerCode,0,2);
                        ownerCode[0] = '\0';
                        memset(setPriortyA,0,2);
                        setPriortyA[0] = '\0';
                        returnCode = 0;
		        returnCode = DSOImessageRequest::messageMassage(functionCode, receiveQueue->getMsgRecv(),errorSendQ,sopId,sopToSend,replyFromSop,ownerName,ownerCode,setPriortyA,receiveQueue->message);
#ifdef DEBUG
cout << "newMessage=" << DSOImessageRequest::newMessage << endl;
cout << " functionCode=" << functionCode << endl;
cout << " sopId="      << sopId << endl;
cout << "  ownerName=" << ownerName  << endl;
cout << " errorSendQ=" << errorSendQ << endl;
cout << "    sopToSend=" << sopToSend << endl;
cout << " replyFromSop=" << replyFromSop << endl;
#endif

		        if (returnCode != 0) //messageMassage()
			  { int rc = 0;
                            memset(_putTime,0,_sizeOfTime);
                            memset(hldMsg,0,1048576);
                            hldMsg[0] = '\0';
                            strcpy(hldMsg,receiveQueue->message);
			    rc = sendHoldQ->putMsg(_putTime, _msgId, _correlId, hldMsg);
			    if (rc !=0) //HOLDQ error
			      { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:12:CRITICAL:FAILED because can't put correlated error message(messageMassage() failed) on HOLDQ so no error will be sent back to the REQUESTOR",(char *) "", rc, (char *) "main", (char *) __FILE__, __LINE__);
			        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
			        exit(1);
			      }
		            if (requestErrorReply==0)
                              { setExpiry = 0;
                                setPriority = 0;
			        requestErrorReply = new DSOIsendMQ(setExpiry,setPriority, (char *) "QL_DSOI_ERROR_REPLY", argv[y]);
			      }
                            memset(_putTime,0,_sizeOfTime);
			    rc = 0;
			    rc = requestErrorReply->putMsg(_putTime,_msgId,_correlId,
                                                           DSOIerrorHandler::ReplyErrMsg);
			    if (rc !=0)
			      { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:13:CRITICAL:putMsg() FAILED to send the ERROR REPLYQ=",
                                                         (char *) "QL_DSOI_ERROR_REPLY",
							 returnCode, (char *) "main", (char *) __FILE__, __LINE__);
			        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
			        exit(1);
			      }	//end of if returnCode !-0 from requestErrorReply->putMsg
			    else // need to send to REPORTQ 
                              {  memset(DSOImessageRequest::_msgTrunc,0,450);
                                 DSOImessageRequest::_msgTrunc[0] = '\0';
                                 strcat(DSOImessageRequest::_msgTrunc,"OWN=");
                                 strcat(DSOImessageRequest::_msgTrunc,ownerCode);
                                 strcat(DSOImessageRequest::_msgTrunc,"\xff");
                                 strcat(DSOImessageRequest::_msgTrunc,"FunctionCode=");
                                 strcat(DSOImessageRequest::_msgTrunc,functionCode);
                                 strcat(DSOImessageRequest::_msgTrunc,"\xff");
                                 strcat(DSOImessageRequest::_msgTrunc,"SopId=");
                                 strcat(DSOImessageRequest::_msgTrunc,sopId);
                                 strcat(DSOImessageRequest::_msgTrunc,"\xff");
                                 strncat(DSOImessageRequest::_msgTrunc,receiveQueue->message,200);
			         if (sendReport(reportQ, errorSendQ, DSOImessageRequest::_msgTrunc) != 0)
				  { exit(1);
				  }
			      } //end of else send to REPORTQ 

			    mainErrObj.formatErrMsg((char *) "DSOIREQUEST:DSOImessageRequest:messageMassage failed---",(char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
			    mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);

			  } //end of if(returnCode != 0) //messageMassage()
		        else //messageMassage() successful
                          { memset(_putTime,0,_sizeOfTime);
                            memset(hldMsg,0,1048576);
                            hldMsg[0] = '\0';
                            strcpy(hldMsg,"SopId=");
                            strncat(hldMsg,sopId,3);
                            strcat(hldMsg,"\xff");
                            strcat(hldMsg,"OWN=");
                            strncat(hldMsg,ownerCode,1);
                            strcat(hldMsg,"\xff");
                            strcat(hldMsg,receiveQueue->message);
#ifdef DEBUG
cout << "   Holdmessage=" <<  hldMsg        << endl;
#endif
			    int rc = 0;
			    rc = sendHoldQ->putMsg(_putTime,_msgId,_correlId, hldMsg);

			    if (rc !=0)
			      { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:16:CRITICAL:DSOIREQUEST is unable to send to the HOLDQ error in MQPUT(): DSOIREQUEST exiting",(char *) "",rc, (char *) "main", (char *) __FILE__, __LINE__);
			        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
			        exit(1);
			      } // end of if(rc !=0) from HOLDQ
                            
			    setPriority = atoi(setPriortyA);
                            MQBYTE24 TmpMqmiNone;
                            memcpy(TmpMqmiNone,MQMI_NONE,sizeof(TmpMqmiNone));
			    strcpy(DSOIconnectMQ::replyToQMgr,argv[y]);
			    strcpy(DSOIconnectMQ::replyToQ,replyFromSop);
                            if (strcmp(sopId,"RSO")==0)
	                      { if (strstr(sopToSend, "IQ_RE") != NULL)
                                  { if (openRSOiq == 0)
                                      { sendRSOiq = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openRSOiq =  1;
                                      }
                                    returnCode = sendRSOiq->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "CL_RE") != NULL)
                                  { if (openRSOcl == 0)
                                      { sendRSOcl = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openRSOcl =  1;
                                      }
                                    returnCode = sendRSOcl->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "QUERY") != NULL)
                                  { if (openRSOquery == 0)
                                      { sendRSOquery = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openRSOquery =  1;
                                      }
                                    returnCode = sendRSOquery->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "DX_RE") != NULL)
                                  { if (openRSOdx == 0)
                                      { sendRSOdx = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openRSOdx =  1;
                                      }
                                    returnCode = sendRSOdx->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                                else if (strstr(sopToSend, "DATA_AT") != NULL)
                                  { if (openRSOATdata == 0)
                                      { sendRSOATdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                      openRSOATdata =  1;
                                      }
                                   returnCode = sendRSOATdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  } 
                                else
                                  { if (openRSOdata == 0)
                                      { sendRSOdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openRSOdata =  1;
                                      }
                                    returnCode = sendRSOdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                              }
                            else if (strcmp(sopId,"ESS")==0)
	                      { if (strstr(sopToSend, "IQ_RE") != NULL)
                                  { if (openESSiq == 0)
                                      { sendESSiq = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESSiq =  1;
                                      }
                                    returnCode = sendESSiq->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "CL_RE") != NULL)
                                  { if (openESScl == 0)
                                      { sendESScl = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESScl =  1;
                                      }
                                    returnCode = sendESScl->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "QUERY") != NULL)
                                  { if (openESSquery == 0)
                                      { sendESSquery = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESSquery =  1;
                                      }
                                    returnCode = sendESSquery->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "DX_RE") != NULL)
                                  { if (openESSdx == 0)
                                      { sendESSdx = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESSdx =  1;
                                      }
                                    returnCode = sendESSdx->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                                else if (strstr(sopToSend, "DATA_AT") != NULL)
                                  { if (openESSATdata == 0)
                                      { sendESSATdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                      openESSATdata =  1;
                                      }
                                   returnCode = sendESSATdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                                else
                                  { if (openESSdata == 0)
                                      { sendESSdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESSdata =  1;
                                      }
                                    returnCode = sendESSdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                              }
                            else if (strcmp(sopId,"ESN")==0)
	                      { if (strstr(sopToSend, "IQ_RE") != NULL)
                                  { if (openESNiq == 0)
                                      { sendESNiq = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESNiq =  1;
                                      }
                                    returnCode = sendESNiq->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "CL_RE") != NULL)
                                  { if (openESNcl == 0)
                                      { sendESNcl = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESNcl =  1;
                                      }
                                    returnCode = sendESNcl->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "QUERY") != NULL)
                                  { if (openESNquery == 0)
                                      { sendESNquery = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESNquery =  1;
                                      }
                                    returnCode = sendESNquery->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "DX_RE") != NULL)
                                  { if (openESNdx == 0)
                                      { sendESNdx = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESNdx =  1;
                                      }
                                    returnCode = sendESNdx->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                                else if (strstr(sopToSend, "DATA_AT") != NULL)
                                  { if (openESNATdata == 0)
                                      { sendESNATdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                      openESNATdata =  1;
                                      }
                                   returnCode = sendESNATdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                                else
                                  { if (openESNdata == 0)
                                      { sendESNdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openESNdata =  1;
                                      }
                                    returnCode = sendESNdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                              }
                            else if (strcmp(sopId,"SOC")==0)
	                      { if (strstr(sopToSend, "IQ_RE") != NULL)
                                  { if (openSOCiq == 0)
                                      { sendSOCiq = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOCiq =  1;
                                      }
                                    returnCode = sendSOCiq->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "CL_RE") != NULL)
                                  { if (openSOCcl == 0)
                                      { sendSOCcl = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOCcl =  1;
                                      }
                                    returnCode = sendSOCcl->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "QUERY") != NULL)
                                  { if (openSOCquery == 0)
                                      { sendSOCquery = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOCquery =  1;
                                      }
                                    returnCode = sendSOCquery->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "DX_RE") != NULL)
                                  { if (openSOCdx == 0)
                                      { sendSOCdx = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOCdx =  1;
                                      }
                                    returnCode = sendSOCdx->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                               else if (strstr(sopToSend, "DATA_AT") != NULL)
                                 { if (openSOCATdata == 0)
                                      { sendSOCATdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                      openSOCATdata =  1;
                                      }
                                   returnCode = sendSOCATdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                 }
				else
				  { if (openSOCdata == 0)
                                      { sendSOCdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOCdata =  1;
                                      }
                                    returnCode = sendSOCdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                              }
                            else if (strcmp(sopId,"SOU")==0)
	                      { if (strstr(sopToSend, "IQ_RE") != NULL)
                                  { if (openSOUiq == 0)
                                      { sendSOUiq = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOUiq =  1;
                                      }
                                    returnCode = sendSOUiq->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "CL_RE") != NULL)
                                  { if (openSOUcl == 0)
                                      { sendSOUcl = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOUcl =  1;
                                      }
                                    returnCode = sendSOUcl->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "QUERY") != NULL)
                                  { if (openSOUquery == 0)
                                      { sendSOUquery = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOUquery =  1;
                                      }
                                    returnCode = sendSOUquery->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
	                        else if (strstr(sopToSend, "DX_RE") != NULL)
                                  { if (openSOUdx == 0)
                                      { sendSOUdx = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOUdx =  1;
                                      }
                                    returnCode = sendSOUdx->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                               else if (strstr(sopToSend, "DATA_AT") != NULL)
                                 { if (openSOUATdata == 0)
                                      { sendSOUATdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                      openSOUATdata =  1;
                                      }
                                   returnCode = sendSOUATdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                 }
                                else
                                  { if (openSOUdata == 0)
                                      { sendSOUdata = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                        openSOUdata =  1;
                                      }
                                    returnCode = sendSOUdata->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                                  }
                              }
                            else if (strcmp(sopId,"ONS")==0)
                              { if (openONSR == 0)
                                  { sendONSR = new DSOIsendMQ(setExpiry, setPriority, sopToSend,argv[y]);
                                    openONSR =  1;
                                  }
                                returnCode = sendONSR->putMsg(_putTime,_msgId,TmpMqmiNone, DSOImessageRequest::newMessage);
                              }
                            else
                              { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:16:CRITICAL:DSOIREQUEST invalid requestType: DSOIREQUEST exiting",(char *) "",returnCode, (char *) "main", (char *) __FILE__, __LINE__);
                                mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
                                exit(1);
                              }

#ifdef DEBUG
cout << "newMessage="   << DSOImessageRequest::newMessage << endl;
cout << "sopToSend="    << sopToSend    << endl;
cout << "replyFromSop=" << replyFromSop << endl;
#endif
			    if (returnCode != 0) //error sending to SOP
			      { if (requestErrorReply==0)
			          { setExpiry = 0;
                                    setPriority = 0;
			            requestErrorReply = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_ERROR_REPLY", argv[y]);
				  }

				mainErrObj.formatErrMsg((char *) "DSOIREQUEST:putMsg() FAILED to send to the Service Order Processor! DSOIRequest will exit",
							sopToSend,returnCode,(char *) "main",
							(char *) __FILE__, __LINE__);
				mainErrObj.createReplyErrMsg(DSOIerrorHandler::_formatErrMsg);
				mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
                                memset(_putTime,0,_sizeOfTime);
				if (returnCode = requestErrorReply->putMsg(_putTime,_msgId,
                                       _correlId, DSOIerrorHandler::ReplyErrMsg) !=0)
				  { mainErrObj.formatErrMsg((char *) "DSOIREQUEST:15:CRITICAL:putMsg() FAILED to send an error to queue=",
                                                         (char *) "QL_DSOI_ERROR_REPLY",
							 returnCode,(char *) "main", (char *) __FILE__, __LINE__);
				    mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
							//mainErrObj.sendErrorMessage(DSOIerrorHandler::ReplyErrMsg,errorSendQ);
				    exit(1);
				  } // end if (returnCode = requestErrorReply->putMsg(_putTime,
			        else //send to REPORTQ
			          { if (sendReport(reportQ, errorSendQ, DSOImessageRequest::_msgTrunc) != 0)
				      { exit(1);
				      }
				  } //end of else send to REPORTQ
				mainErrObj.formatErrMsg((char *) "DSOIREQUEST:09:CRITICAL:putMsg() FAILED because MQPUT() failed to send to Service Order Processor:DSOIREQUEST is exiting",
							(char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);

			        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
			        exit(1);
			      } // end if (returnCode != 0) //error sending to SOP
			    else // no error sending to SOP
			      { if (sendReport(reportQ,errorSendQ,DSOImessageRequest::_msgTrunc) != 0)
			          { exit(1);
				  }
			      } // end of  else //no error sending to SOP
#ifdef DEBUG
cout << "returnCode from send " << returnCode << endl;
#endif
		          } // end  else //messageMassage() successful

		      } // else // message received and no errors
	      
                  } // end while(returnCode != MQRC_NO_MSG_AVAILABLE)

		if (requestErrorReply) //queue of REQUESTOR
		 	delete requestErrorReply;
		delete errorSendQ;
		delete receiveQueue;
		delete reportQ;
		delete sendHoldQ;
                if (openRSOdata  == 1)    delete sendRSOdata;
                if (openRSOquery == 1)    delete sendRSOquery;
                if (openRSOiq    == 1)    delete sendRSOiq;
                if (openRSOcl    == 1)    delete sendRSOcl;
                if (openRSOdx    == 1)    delete sendRSOdx;
		if (openRSOATdata  == 1)    delete sendRSOATdata;
                if (openESSdata  == 1)    delete sendESSdata;
                if (openESSquery == 1)    delete sendESSquery;
                if (openESSiq    == 1)    delete sendESSiq;
                if (openESScl    == 1)    delete sendESScl;
                if (openESSdx    == 1)    delete sendESSdx;
		if (openESSATdata  == 1)    delete sendESSATdata;
                if (openESNdata  == 1)    delete sendESNdata;
                if (openESNquery == 1)    delete sendESNquery;
                if (openESNiq    == 1)    delete sendESNiq;
                if (openESNcl    == 1)    delete sendESNcl;
                if (openESNdx    == 1)    delete sendESNdx;
		if (openESNATdata  == 1)    delete sendESNATdata;
                if (openSOCdata  == 1)    delete sendSOCdata;
                if (openSOCATdata  == 1)    delete sendSOCATdata;
		if (openSOCquery == 1)    delete sendSOCquery;
                if (openSOCiq    == 1)    delete sendSOCiq;
                if (openSOCcl    == 1)    delete sendSOCcl;
                if (openSOCdx    == 1)    delete sendSOCdx;
                if (openSOUdata  == 1)    delete sendSOUdata;
                if (openSOUATdata  == 1)    delete sendSOUATdata;
		if (openSOUquery == 1)    delete sendSOUquery;
                if (openSOUiq    == 1)    delete sendSOUiq;
                if (openSOUcl    == 1)    delete sendSOUcl;
                if (openSOUdx    == 1)    delete sendSOUdx;
                if (openONSR     == 1)    delete sendONSR;
                delete []functionCode;
                delete []sopId;
                delete []sopToSend;
                delete []replyFromSop;
                delete []ownerName;
                delete []ownerCode;
                delete []setPriortyA;
                delete []hldMsg;

	   } // end of try

	catch( ThrowError& err )
	  {
#ifdef DEBUG
		cout << "ERROR= " << endl;
		cout << DSOIerrorHandler::_formatErrMsg << endl;
		//err.printMessage();
		cout << endl;
#endif
#ifdef DEBUG
	cout << "In catch(ThrowError) " << endl;
#endif
		try
		{
			mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
			if (requestErrorReply)          //queue of REQUESTOR
				delete requestErrorReply;
			if (errorSendQ)
			        delete errorSendQ;
			mainErrObj.createReplyErrMsg(DSOIerrorHandler::_formatErrMsg);
			int rc=0;
                        setExpiry=0;
                        setPriority=0;
			requestErrorReply = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_ERROR_REPLY", argv[y]);
                        memset(_putTime,0,_sizeOfTime);
			rc = requestErrorReply->putMsg(_putTime,_msgId,_correlId,
                                                       DSOIerrorHandler::ReplyErrMsg);
			delete requestErrorReply;
			if (rc !=0)
			  { cout << "Error sending requestor reply reporting critical error!" << endl;
			    cout << "Error Message = " << DSOIerrorHandler::ReplyErrMsg << endl;
			  }
			exit(1);
		}
		catch(ThrowSendError& err)
		  {
			DSOIlogMQ* logQ = new DSOIlogMQ();
			try{
				logQ->logCriticalError(DSOIerrorHandler::_formatErrMsg);
			   }
			catch( ThrowError& err )
			  { cout << "Error sending to log critical error!" << endl;
			    cout << "Error Message = " << DSOIerrorHandler::_formatErrMsg << endl;
			  }
			delete logQ;
			if(errorSendQ !='\0')
				delete errorSendQ;
			mainErrObj.createReplyErrMsg(DSOIerrorHandler::_formatErrMsg);
			int rc=0;
                        setExpiry=0;
                        setPriority=0;
			requestErrorReply = new DSOIsendMQ(setExpiry, setPriority, (char *) "QL_DSOI_ERROR_REPLY", argv[y]);
                        memset(_putTime,0,_sizeOfTime);
			rc = requestErrorReply->putMsg(_putTime,_msgId,_correlId,
                                                       DSOIerrorHandler::ReplyErrMsg);
			delete requestErrorReply;
			if(rc !=0)
			  { cout << "Error sending requestor reply reporting critical error!" << endl;
			    cout << "Error Message = " << DSOIerrorHandler::ReplyErrMsg << endl;
			  }
			exit(1);

		  }
	}
	catch(ThrowSendError& err)
	  {
#ifdef DEBUG
	cout << "In catch(ThrowSendError) " << endl;
#endif
	// cerr << endl << endl;
	// cerr << "ERROR SENDING TO ERRORQ, error= " ;
	// cerr << DSOIerrorHandler::_formatErrMsg << endl << endl;
	// cerr << "ReplyErrorMessage= " ;
	// cerr << DSOIerrorHandler::ReplyErrMsg << endl;
		DSOIlogMQ* logQ = new DSOIlogMQ();
		try{
			logQ->logCriticalError(DSOIerrorHandler::_formatErrMsg);
		   }
		catch( ThrowError& err )
		  { cout << "Error sending to log critical error!" << endl;
		    cout << "Error Message = " << DSOIerrorHandler::_formatErrMsg << endl;
		  }
		delete logQ;
		exit(1);

	  }
	catch(...)
	  {
		exit(1);
	  }
	exit(0);
}

////////////////////////////////////////////////
//  FUNCTION: int sendReport
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS:  
//  
//  OTHER INPUTS: 
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int sendReport(DSOIsendMQ* reportQueue,  DSOIsendMQ* errorSendQ, char * reportMsg)
{ char* rptMsg=       new char[250];

        memset(rptMsg,0,250);
        rptMsg[0] = '\0';
        if (strlen(reportMsg) > 250)
          { strncpy(rptMsg,reportMsg,250);
            memset(reportMsg,0,250);
            reportMsg[0] = '\0';
            strcpy(reportMsg, rptMsg);
          }

        delete  []rptMsg;
	formatReport(_reportMsg,
                                _Date,
				_putTime,
				_getTime,
				"DSOIRequest",
				_putApplName,
				_putApplTime,
				_putApplDate,
				_msgId,
				reportMsg);
	int returnCode = 0;
        memset(_putTime,0,_sizeOfTime);
	returnCode = reportQueue->putMsg(_putTime,_msgId,_correlId, _reportMsg);
	if(returnCode !=0)
	{
		mainErrObj.formatErrMsg((char *) "DSOIREQUEST:07:CRITICAL:errored because message can't be put on the QL_DSOI_REPORTQ: DSOIREQUEST exiting", (char *) "", returnCode, (char *) "main", (char *) __FILE__, __LINE__);
		mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
		return -1;
	}
	return 0;
}


///////////////////////////////////////////////////////
//  FILE:  DSOImainRouteReply.C
//
//  PURPOSE:  To process replies from SOL and send requests to DSOIRequest.
//  
//  REVISION HISTORY:
//-----------------------------------------------------------
//  version	date		author		description
//-----------------------------------------------------------
//  7.00.00     01/07/00        rfuruta		CR 3126, initial development
//  7.00.00	01/20/00	bxcox		CR 3126, further development
//  7.01.00     07/03/00        rfuruta 	CR 4275, 4P connection
//
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
//
///////////////////////////////////////////////////////

#include <iostream.h>
#include <new>
#include "DSOIThrowError.H"
#include "DSOIThrowSendError.H"
#include "DSOIsendMQ.H"
#include "DSOIreceiveMQ.H"
#include "DSOIrouteReply.H"
#include "DSOItoolbox.H"
#include "DSOIlogMQ.H"

char* newMessage = new(nothrow) char[1048576];
char* msgTrunc   = new(nothrow) char[150];

////////////////////////////////////////////////
//  FUNCTION: int main (int argc, char * argv[])
//
//  PURPOSE: To process all reply messages from the SOL, and send requests to DSOIRequest.
//
//  METHOD: Is triggered to start when there is one message on the SOL reply queues.
//          Processes all messages on the queue until there are no more messages.
//
//  PARAMETERS: int argc - the number of command line parameters
//		char * argv[] - the command line parameters
//  
//  OTHER OUTPUTS: Sends an error message to the QL_DSOI_ERRORQ (DSOILog)
//	when a message could not be processed.  Also sends an error
//	message to the QL_DSOI_ERROR_REPLY (DSOIReply) when the message
//	could not be processed so that a 'Requestor' application
//	gets a reply from DSOI.
//  
//  RETURNS: exits ONLY if there is an exception thrown.
// 
//  OTHER INPUTS: Requires a command line parameter as to
//	where the messages are received (the name of the 
//	MQSeries message queue). 
//	Optional: a parameter to specify the queue manager.
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

int main (int argc, char * argv[])
{
	if(argc < 3)
	{
		// not enough command line parameters

		cout << "Usage:  " << endl
		     << argv[0] << "  receiveQ" << "  queueManager" << endl;

		exit(1);
	}

	int x = 0;
	int y = 0;

        DSOIsendMQ* errorReplyQueue;
        DSOIsendMQ* sendQueue4RS;
        DSOIsendMQ* sendQueueRSO;
        DSOIsendMQ* sendQueueESS;
        DSOIsendMQ* sendQueueESN;
        DSOIsendMQ* sendQueueSOC;
        DSOIsendMQ* sendQueueSOU;
        DSOIsendMQ* sendQueueRPY;

        int open4RS = 0;
        int openRSO = 0;
        int openESS = 0;
        int openESN = 0;
        int openSOC = 0;
        int openSOU = 0;
        int openRPY = 0;

	if(strstr(argv[1], "TMC") == NULL)
	{
		// manual execution of process from command line
		x = 1;		// command line position of request queue parameter
		y = 2;		// command line position of queue manager parameter
	}

	else
	{
		// process executed by MQSeries trigger
		x = 2;
		y = 3;
	}

	DSOIerrorHandler mainErrObj;
	int returnCode = 0;

	// connect to queue manager and open the send error queue;

	DSOIsendMQ * sendErrorQ = new(nothrow) DSOIsendMQ("QL_DSOI_ERRORQ", argv[y]);
	if(sendErrorQ == 0)
	{
                returnCode = -1;
                mainErrObj.formatErrMsg("DSOIROUTEREPLY:unable to allocate new memory", "",
                                        returnCode, "main", __FILE__, __LINE__);
                mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
		exit(1);
	}

	try
	{
		// initialize DSOIrouteReply object with receive queue and queue manager

		DSOIrouteReply * routeReply = new(nothrow) DSOIrouteReply(argv[x], argv[y]);
		if(routeReply == 0)
		{
                        returnCode = -1;
                        mainErrObj.formatErrMsg("DSOIROUTEREPLY:unable to allocate new memory", "",
                                                returnCode, "main", __FILE__, __LINE__);
                        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
		        exit(1);
		}

		DSOIreceiveMQ * receiveQueue = new(nothrow) DSOIreceiveMQ(routeReply->getReceiveQueue(),
                                                   routeReply->getQueueManager() );
		if(receiveQueue == 0)
		{
                        returnCode = -1;
                        mainErrObj.formatErrMsg("DSOIROUTEREPLY:unable to allocate new memory", "",
                                                returnCode, "main", __FILE__, __LINE__);
                        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
		        exit(1);
		}

		// test  receiveQueue->setWaitInterval(60000);

		// open the hold queue

		DSOIreceiveMQ * receiveHoldQ = new(nothrow) DSOIreceiveMQ(routeReply->getHoldQueue(), 
                                                   routeReply->getQueueManager(), 1 );
		if(receiveHoldQ == 0)
		{
                        returnCode = -1;
                        mainErrObj.formatErrMsg("DSOIROUTEREPLY:unable to allocate new memory", "",
                                                returnCode, "main", __FILE__, __LINE__);
                        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
		        exit(1);
		}

		memset(_Date, 0, 11);
		calculateDate(_Date);	// DSOItoolbox

		// open the report queue

		DSOIsendMQ * reportQ = new(nothrow) DSOIsendMQ(routeReply->getReportQueue(), routeReply->getQueueManager() );
		if(reportQ == 0)
		{
                        returnCode = -1;
                        mainErrObj.formatErrMsg("DSOIROUTEREPLY:unable to allocate new memory", "",
                                                returnCode, "main", __FILE__, __LINE__);
                        mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
		        exit(1);
		}

		// process all messages on the receiveQueue

		while(returnCode != MQRC_NO_MSG_AVAILABLE)
		{
			unsigned char _msgId[DSOI_STRING_25];
			MQBYTE24 _correlId;

			memset(_getTime, 0, _sizeOfTime);
			memset(_putApplName, 0, 28);
			memset(_putApplDate, 0, 8);
			memset(_putApplTime, 0, 6);
			memset(_msgId, 0, 25);
			memset(_correlId, 0, 24);
		
			// set convert option on because DSOIRouteReply reads messages sent from MVS
			receiveQueue->setConvert(1);
			
			// do not match msgId when getting a message
			receiveQueue->_getCorrel = 0;

			// get messages from the receiving queue

	 		returnCode = receiveQueue->getMsg(_putApplName, _putApplDate, _putApplTime, 
                                                          _getTime, _msgId, _correlId, sendErrorQ);
        
			if(returnCode > 0)
			{
				// could not get message

				if(returnCode != MQRC_NO_MSG_AVAILABLE)
				{
					// no messages on the queue

					mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, sendErrorQ);

					if(returnCode != MQRC_TRUNCATED_MSG_FAILED)
					{
						// problem reading queue
						// 1005
						mainErrObj.formatErrMsg("DSOIROUTEREPLY:6000:CRITICAL:unable to get message from receive queue because getMsg() failed on RECEIVEQ=", 
                                                                         routeReply->getReceiveQueue(), returnCode,
                                                                         "main", __FILE__, __LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, sendErrorQ);
						exit(1);
					}
				}
			}

			else
			{
				// received a message
#ifdef DEBUG
char messageId[49];
char ptr[3];
memset(messageId,0,49);
for(int i=0; i < 24; i++)
   { memset(ptr,0,3);
     int rc = sprintf(ptr,"%02d",_msgId[i]);
     strcat(messageId, ptr);
   }
cout << "msgId=" << messageId << endl
     << "  Time=" << _getTime << endl
     << "  Date=" << _Date << endl
     << "  _putApplName=" << _putApplName << endl
     << "  request message=" <<  receiveQueue->message << endl;
#endif
				char _tmpTime[DSOI_STRING_15];
				char _tmpApplDate[DSOI_STRING_9];
				char _tmpApplName[DSOI_STRING_29];
				char _tmpApplTime[DSOI_STRING_9];

                                memset(_tmpTime, 0, 11);
                                memset(_tmpApplName, 0, 28);
                                memset(_tmpApplDate, 0, 8);
                                memset(_tmpApplTime, 0, 6);

				receiveHoldQ->_getCorrel = 1;
	 		        returnCode = receiveHoldQ->getMsg(_tmpApplName,_tmpApplDate, _tmpApplTime,
                                                                  _tmpTime, _msgId, _correlId, sendErrorQ);

  	                        if (returnCode > 0)      // if rc=2033,  reallocate receiveHoldQ
	                          { delete receiveHoldQ;
		                    DSOIreceiveMQ * receiveHoldQ = new(nothrow) DSOIreceiveMQ(routeReply->getHoldQueue(), 
                                                                   routeReply->getQueueManager(), 1 );
				    receiveHoldQ->_getCorrel = 1;
	 		            returnCode = receiveHoldQ->getMsg(_tmpApplName,_tmpApplDate, _tmpApplTime,
                                                                  _tmpTime, _msgId, _correlId, sendErrorQ);
                                  }
				if (returnCode > 0)
				{
					// message could not be correlated from hold queue
                                        char id[49];
                                        char ptr[3];
                                        memset(id,0,49);
                                        for(int i=0; i < 24; i++)
                                           { memset(ptr,0,3);
                                             int rc = sprintf(ptr,"%02d",_msgId[i]);
                                             strcat(id, ptr);
                                           }
					mainErrObj.formatErrMsg("DSOIROUTEREPLY:6001:CRITICAL:message could not be correlated from hold queue, reply message will not be sent! msgId=",
                                                                id, returnCode, "main", __FILE__, __LINE__);
					mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, sendErrorQ);
					returnCode = 0;
					continue;
				}

				else
				{
					// found correlated message

#ifdef DEBUG
char messageId[49];
char ptr[3];
memset(messageId,0,49);
for(int i=0; i < 24; i++)
   { memset(ptr,0,3);
     int rc = sprintf(ptr,"%02d",_msgId[i]);
     strcat(messageId, ptr);
   }
cout << "msgId=" << messageId << endl
     << "  message from HOLDQ=" << endl << receiveHoldQ->message << endl;
#endif
                                        memset(newMessage,0,strlen(receiveQueue->message));

					// process request message

					returnCode = routeReply->messageMassage(receiveQueue->message, 
                                                                 receiveHoldQ->message, newMessage,
                                                                 sendErrorQ);
					if(returnCode != 0)
					{
						// messageMassage failed
						// put reply error message to error reply queue for DSOIReply
						if (errorReplyQueue == 0)
						  { errorReplyQueue = new(nothrow) DSOIsendMQ(routeReply->getErrorReplyQueue(), routeReply->getQueueManager() );
                                                  }
						memset(_putTime, 0, _sizeOfTime);
			  			returnCode = errorReplyQueue->putMsg(_putTime, _msgId, _correlId,
                                                                     DSOIerrorHandler::ReplyErrMsg);
						if (returnCode != 0)
						{
							mainErrObj.formatErrMsg("DSOIROUTEREPLY:6003:CRITICAL:putMsg() FAILED to send the ERROR ReplyQ=",
                                                                        routeReply->getErrorReplyQueue(),
                                                                        returnCode,"main",__FILE__,__LINE__);
							mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, sendErrorQ);
							exit(1);
						}
                                                mainErrObj.sendErrorMessage(DSOIerrorHandler::ReplyErrMsg,sendErrorQ);
                                                mainErrObj.formatErrMsg("DSOIROUTEREPLY:DSOIrouteReply:messageMassage failed---",
                                                        "", returnCode, "main",__FILE__, __LINE__);
                                                mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,sendErrorQ);
					}	// end messageMassage failed
                                        else
                                        {

#ifdef DEBUG
cout << "    routeReply->getSendQueue()=" << routeReply->getSendQueue() << endl;
cout << " routeReply->getQueueManager()=" << routeReply->getQueueManager() << endl;
cout << " receiveQueue->message=" << receiveQueue->message << endl;
cout << "            newMessage=" << newMessage << endl;
#endif

				        returnCode = 9; 
                                        if (strstr(routeReply->getSendQueue(), "4RS") != NULL)
                                          { if (open4RS == 0)
                                              { sendQueue4RS = new(nothrow) DSOIsendMQ(routeReply->getSendQueue(),
                                                                              routeReply->getQueueManager() );
                                                open4RS =  1;
                                              }
				            returnCode = sendQueue4RS->putMsg(_putTime, _msgId, _correlId, 
                                                                              newMessage );
                                          }
                                        else if (strstr(routeReply->getSendQueue(), "RSO") != NULL)
                                          { if (openRSO == 0)
                                              { sendQueueRSO = new(nothrow) DSOIsendMQ(routeReply->getSendQueue(),
                                                                              routeReply->getQueueManager() );
                                                openRSO =  1;
                                              }
				            returnCode = sendQueueRSO->putMsg(_putTime, _msgId, _correlId, 
                                                                              newMessage );
                                          }
                                        else if (strstr(routeReply->getSendQueue(), "ESS") != NULL)
                                          { if (openESS == 0)
                                              { sendQueueESS = new(nothrow) DSOIsendMQ(routeReply->getSendQueue(),
                                                                              routeReply->getQueueManager() );
                                                openESS =  1;
                                              }
				            returnCode = sendQueueESS->putMsg(_putTime, _msgId, _correlId, 
                                                                              newMessage );
                                          }
                                        else if (strstr(routeReply->getSendQueue(), "ESN") != NULL)
                                          { if (openESN == 0)
                                              { sendQueueESN = new(nothrow) DSOIsendMQ(routeReply->getSendQueue(),
                                                                              routeReply->getQueueManager() );
                                                openESN =  1;
                                              }
				            returnCode = sendQueueESN->putMsg(_putTime, _msgId, _correlId, 
                                                                              newMessage );
                                          }
                                        else if (strstr(routeReply->getSendQueue(), "SOC") != NULL)
                                          { if (openSOC == 0)
                                              { sendQueueSOC = new(nothrow) DSOIsendMQ(routeReply->getSendQueue(),
                                                                              routeReply->getQueueManager() );
                                                openSOC =  1;
                                              }
				            returnCode = sendQueueSOC->putMsg(_putTime, _msgId, _correlId, 
                                                                              newMessage );
                                          }
                                        else if (strstr(routeReply->getSendQueue(), "SOU") != NULL)
                                          { if (openSOU == 0)
                                              { sendQueueSOU = new(nothrow) DSOIsendMQ(routeReply->getSendQueue(),
                                                                              routeReply->getQueueManager() );
                                                openSOU =  1;
                                              }
				            returnCode = sendQueueSOU->putMsg(_putTime, _msgId, _correlId, 
                                                                              newMessage );
                                          }
                                        else if (strstr(routeReply->getSendQueue(), "DSOIREPLY_REQUEST") != NULL)
                                          { if (openRPY == 0)
                                              { sendQueueRPY = new(nothrow) DSOIsendMQ(routeReply->getSendQueue(),
                                                                              routeReply->getQueueManager() );
                                                openRPY =  1;
                                              }
				            returnCode = sendQueueRPY->putMsg(_putTime, _msgId, _correlId, 
                                                                              newMessage );
                                          }

					if(returnCode)
					{
						mainErrObj.formatErrMsg("DSOIROUTEREPLY:6005:CRITICAL:msg not sent to DSOIRequest because putMsg() failed in ",
                                                    routeReply->getSendQueue(),returnCode,"main",__FILE__,__LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, sendErrorQ);
						exit(1);
					}

                                        int msgLength;
                                        char* location;
                                        location = strchr(newMessage, '\n');
                                        if (location != NULL)
                                          { msgLength = location - newMessage;
                                          }
                                        else
                                          { msgLength = 150;
                                          }
					memset(msgTrunc, 0, 150);
                                        if (strlen(newMessage) > 150)
					  { strncpy(msgTrunc, newMessage,msgLength);
					    msgTrunc[149] = '\0';
                                          }
                                        else
					  { strcpy(msgTrunc, newMessage);
                                          }
					char * hexffPtr = msgTrunc;
					while ( hexffPtr = strchr(hexffPtr, '\x9f') )
						hexffPtr[0] = '\xff';
                                        memset(_reportMsg, 0, 1024);
					formatReport(_reportMsg,
			                                _Date,
							_putTime,
							_getTime,
							"DSOIRouteReply",
							_putApplName,
							_putApplTime,
							_putApplDate,
							_msgId,
							msgTrunc);
#ifdef DEBUG
char messageId[49];
char ptr[3];
memset(messageId,0,49);
for(int i=0; i < 24; i++)
   { memset(ptr,0,3);
     int rc = sprintf(ptr,"%02d",_msgId[i]);
     strcat(messageId, ptr);
   }
cout << "msgId=" << messageId << endl;
cout << "reportMsg=" << _reportMsg << endl;
#endif

					// send report message
                                        memset(_putTime, 0, _sizeOfTime);
					returnCode = reportQ->putMsg(_putTime, _msgId, _correlId, _reportMsg);

					if(returnCode)
					{
						mainErrObj.formatErrMsg("DSOIROUTEREPLY:6006:CRITICAL:message not sent to DSOIReport because putMsg() failed on ",
                                                                     routeReply->getReportQueue(),
                                                                     returnCode,"main",__FILE__,__LINE__);
						mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, sendErrorQ);
						exit(1);
					}
#ifdef DEBUG
sprintf(messageId, "%024d", _msgId);

cout << "msgId=" << messageId << endl
     << "  _putApplName=" << _putApplName << endl
     << "message put to REPORTQ=" << endl << _reportMsg << endl;
#endif
					}	// end else messageMassage success

				}	// end else found correlated message

			}	// end else received a message

		}	// end while

	        if (errorReplyQueue)   delete errorReplyQueue;
		delete routeReply;
		delete sendErrorQ;
		delete receiveQueue;
		delete receiveHoldQ;
		delete reportQ;

                if (open4RS == 1)    delete sendQueue4RS;
                if (openRSO == 1)    delete sendQueueRSO;
                if (openESS == 1)    delete sendQueueESS;
                if (openESN == 1)    delete sendQueueESN;
                if (openSOC == 1)    delete sendQueueSOC;
                if (openSOU == 1)    delete sendQueueSOU;
                if (openRPY == 1)    delete sendQueueRPY;

	}	// end try

	catch (ThrowError & err)
	{
#ifdef DEBUG
cout << "ERROR="  << DSOIerrorHandler::_formatErrMsg << endl;
#endif
		exit(1);
	}

	catch (ThrowSendError & err)
	{
		cerr << endl << endl
		     << "ERROR SENDING TO ERRORQ, error=  "
		     << DSOIerrorHandler::_formatErrMsg << endl << endl << endl
		     << "ReplyErrorMessage=  "
		     << DSOIerrorHandler::ReplyErrMsg << endl;

		// open queue for logging critical errors
		DSOIlogMQ * logQ = new(nothrow) DSOIlogMQ();
		if(logQ == 0)
		{
			returnCode = -1;
			mainErrObj.formatErrMsg("DSOIROUTEREPLY:unable to allocate new memory", 
                                                "", returnCode, "main", __FILE__, __LINE__);
			mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, sendErrorQ);
			exit(1);
		}

		try
		{
			// log critical errors
		        logQ->logCriticalError(DSOIerrorHandler::_formatErrMsg);
		}

		catch(ThrowError & err)
		{
		        cout << "Error sending to log critical error!" << endl
		             << "Error Message = " << DSOIerrorHandler::_formatErrMsg << endl;
		}

		delete logQ;
		exit(1);
	}

	catch(...)
	{

#ifdef DEBUG
cout << "in catch(...) " << endl;
#endif
		cerr << "Unknown exception caught!" << endl;
		exit(1);
	}
	return 0;
}


//////////////////////////////////////////////////////////
//  FILE: DSOImainRouteRequest.C
//
//  PURPOSE:
//
//  REVISION HISTORY:
//--------------------------------------------------------
//  version     date            author  description
//  7.00.00     01/07/00        rfuruta CR3123, initial development
//  7.01.00     07/03/00        rfuruta CR 4275, 4P connection
//  7.01.00     07/14/00        rfuruta CR 4329, 4P override
//
//              CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////////

#include <iostream.h>
#include <cstring>
#include <cerrno>
#include <new>
//#include <libc.h>
#include "DSOIThrowError.H"
#include "DSOIThrowSendError.H"
#include "DSOIsendMQ.H"
#include "DSOIreceiveMQ.H"
#include "DSOIrouteRequest.H"
#include "DSOIroute.H"
#include "DSOItoolbox.H"
#include "DSOIlogMQ.H"

int sendReport( DSOIsendMQ* , DSOIsendMQ* );
DSOIerrorHandler mainErrObj;
unsigned char _msgId[DSOI_STRING_25];
static MQBYTE24 _correlId;
char* GivenSopId =   new(nothrow) char[20];
char* DefaultSopId = new(nothrow) char[20];
char* RequestSopId = new(nothrow) char[20];
char* ReplySopId =   new(nothrow) char[20];
char* Is4P =         new(nothrow) char[4];
char* functionCode = new(nothrow) char[3];
char* sopToSend =    new(nothrow) char[48];
char* replyFromSop = new(nothrow) char[48];
char* hldMsg =       new(nothrow) char[1048576];
char* hdr4Pmsg =     new(nothrow) char[500];

// 4P Platform Message Header Layout  
const char* archHdrLen = "00097";
const char* archHdrVer = "0001";
const char* applDtaVer = "0000";
char* servName = new(nothrow) char[9];
const char* srcApplCode =  "DSOI    ";
char* resolvHostCd = new(nothrow) char[5];
const char* replyInd =     "Y";
const char* archRespInd =  "Y";
const char* susTxnErrInd = "N";
const char* routeInd =     "Y";
char* replyCode = new(nothrow) char[3];
char* userId    = new(nothrow) char[9];
char* sessionId = new(nothrow) char[33];
char* hostName  = new(nothrow) char[17];
char* perfEvalCode = new(nothrow) char[3];
char* dtl4Pmsg = new(nothrow) char[200];

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
//	command line parameters.  The _queuesOpen array is an array of flags to
//	identify which SOP to send the received messages.
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
	if(argc < 3)
	{
		cout << "Usage:  " << endl;
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
	DSOIsendMQ* sendQueueN4P;
	DSOIsendMQ* sendQueueSOL;

        DSOIsendMQ* sendQueue4RS;
        DSOIsendMQ* sendQueueRSO;
        DSOIsendMQ* sendQueueESS;
        DSOIsendMQ* sendQueueESN;
        DSOIsendMQ* sendQueueSOC;
        DSOIsendMQ* sendQueueSOU;

        int openN4P = 0;
        int openSOL = 0;

        int open4RS = 0;
        int openRSO = 0;
        int openESS = 0;
        int openESN = 0;
        int openSOC = 0;
        int openSOU = 0;

        int requestType = 0;
        const char* filler =  "                                             ";

        strncpy(servName,filler,9);
        strncpy(resolvHostCd,filler,5);
        strncpy(replyCode,filler,3);
        strncpy(sessionId,filler,33);
        strncpy(hostName,filler,17);
        strncpy(perfEvalCode,filler,3);

	if(strstr(argv[1], "TMC") != NULL)
	  { x = 2;
	    y = 3;
	  }
	else
	  { x = 1;
	    y = 2;
	  }
	try{
		try
		{
			errorSendQ = new(nothrow) DSOIsendMQ("QL_DSOI_ERRORQ",argv[y]);
		}
		catch( ThrowError& err )
		{
#ifdef DEBUG
cout << "In catch ThrowError " << endl;
cout << "lineNo= " << __LINE__ << endl;
cout << "ERRORQ not available!" << endl;
cout << "ERROR= "  ;
cout << DSOIerrorHandler::_formatErrMsg << endl;
#endif
			DSOIlogMQ* logQ = new(nothrow) DSOIlogMQ();
			try{
				logQ->logCriticalError(DSOIerrorHandler::_formatErrMsg);
			}
			catch( ThrowError& err )
			{
				cout << "Error sending to log critical error!" << endl;
				cout << "Error Message = " << DSOIerrorHandler::_formatErrMsg << endl;
			}
			delete logQ;
			if(errorSendQ !='\0')
				delete errorSendQ;
			exit(1);
		}
		DSOIsendMQ *sendHoldQ = new(nothrow) DSOIsendMQ("QL_DSOI_HOLD",argv[y]);
		// queue for receiving messages for routing with the last parameter
		// set to 1 so that the queue is opened in browse mode
		DSOIreceiveMQ  *dataRefQueue = new(nothrow) DSOIreceiveMQ("QL_DSOI_DATAREFQ",argv[y],1);
                memset(_getTime,0,_sizeOfTime);
                memset(_putApplName,0,28);
                memset(_putApplDate,0,8);
                memset(_putApplTime,0,6);
                memset(_msgId,0,25);
                memset(_correlId,0,24);

                returnCode = dataRefQueue->getMsg(_putApplName,_putApplDate,_putApplTime,
                                                  _getTime,_msgId,_correlId,errorSendQ);

		if(returnCode !=0)
		  { mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5017:CRITICAL:getMsg() FAILED because MQGET() failed for the DATAREFQ:DSOIROUTEREQUEST is exiting",
						"QL_DSOI_DATAREFQ", returnCode,"main",__FILE__,__LINE__);
		    mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
		    exit(1);
		  }
		// need to reset the browse option so that messages are now taken
		// from the queue from now on
		dataRefQueue->setBrowse(0);
#ifdef DEBUG
cout << "main DATAREF+ " << dataRefQueue->message << endl;
cout << " requestQueue=" << argv[x] << endl;
#endif

		returnCode = DSOIrouteRequest::readDataRef(dataRefQueue->message);
		if(returnCode !=0)
		  { mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5018:CRITICAL:readDataRef() FAILED for the DATAREFQ:DSOIROUTEREQUEST is exiting",
						"QL_DSOI_DATAREFQ", returnCode,"main",__FILE__,__LINE__);
		    mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
		    exit(1);
		  }
		// queue for receiving messages from a requesting application
		DSOIreceiveMQ  *receiveQueue = new(nothrow) DSOIreceiveMQ(argv[x],argv[y]);
		DSOIroute *routeQueue = new(nothrow) DSOIroute(argv[y]);

		//Queue to send report messages to
		reportQ = new(nothrow) DSOIsendMQ("QL_DSOI_REPORTQ",argv[y]);
                memset(_Date,0,11);
                calculateDate(_Date);

		while(returnCode != MQRC_NO_MSG_AVAILABLE)
                  {     memset(_getTime,0,_sizeOfTime);
                        memset(_putApplName,0,28);
                        memset(_putApplDate,0,8);
                        memset(_putApplTime,0,6);
                        memset(_msgId,0,25);
                        memset(_correlId,0,24);

                        returnCode = receiveQueue->getMsg(_putApplName,_putApplDate,_putApplTime,
                                                          _getTime,_msgId,_correlId,errorSendQ);
       
			if (returnCode != 0)
			  { if (returnCode != MQRC_NO_MSG_AVAILABLE)
		              {	mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
				if (returnCode != MQRC_TRUNCATED_MSG_FAILED)
			          { mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5008:CRITICAL:getMsg() FAILED because MQGET() failed:DSOIROUTEREQUEST is exiting",
								"", returnCode, "main", __FILE__,__LINE__);
				    mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
				    exit(1);
				  } // end of if (returnCode != MQRC_TRUNCATED_MSG_FAILED)
			      } // end of if (returnCode != MQRC_NO_MSG_AVAILABLE)
			  } // if (returnCode != 0) from getMsg()
			else // message received and no errors
		         { returnCode = 0;
                           memset(GivenSopId,0,20);
                           GivenSopId[0] = '\0';
                           memset(DefaultSopId,0,20);
                           DefaultSopId[0] = '\0';
                           memset(Is4P,0,4);
                           memset(functionCode,0,3);
                           memset(userId,0,9);
                           memset(dtl4Pmsg,0,200);
#ifdef DEBUG
cout << "  requestMsg="   <<  receiveQueue->message  << endl;
#endif
			   int returnCode = DSOIrouteRequest::messageMassage(receiveQueue->message, errorSendQ,
                                      functionCode, userId, GivenSopId, DefaultSopId, Is4P, dtl4Pmsg);
			   if(returnCode != 0) //messageMassage()
			     { int rc =0;
                               memset(_putTime,0,_sizeOfTime);
			       rc = sendHoldQ->putMsg(_putTime, _msgId,_correlId, DSOIrouteRequest::_msgTrunc);
			       if(rc !=0) //HOLDQ error
				 { mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5012:CRITICAL:FAILED because can't put correlated error message(messageMassage() failed) on HOLDQ so no error will be sent back to the REQUESTOR","", rc, "main",__FILE__, __LINE__);
				   mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   exit(1);
				 }
			       if(requestErrorReply==0)
				 { requestErrorReply = new(nothrow) DSOIsendMQ("QL_DSOI_ERROR_REPLY", argv[y]);
				 }
			       rc=0;
                               memset(_putTime,0,_sizeOfTime);
			       rc = requestErrorReply->putMsg(_putTime,_msgId,_correlId, DSOIerrorHandler::ReplyErrMsg);
			       if(rc !=0)
				 { mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5013:CRITICAL:putMsg() FAILED to send the ERROR REPLYQ=",
                                                           "QL_DSOI_ERROR_REPLY",returnCode,"main",__FILE__,__LINE__);
				   mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   exit(1);
				 }		 //end of if returnCode !-0 from requestErrorReply->putMsg
		               else // need to send to REPORTQ 
				 { if(sendReport( reportQ, errorSendQ) !=0)
				     { exit(1);
				     }
				 } //end of else send to REPORTQ 

			       mainErrObj.sendErrorMessage(DSOIerrorHandler::ReplyErrMsg,errorSendQ);
			       mainErrObj.formatErrMsg("DSOIROUTEREQUEST:DSOIrouteRequest:messageMassage failed---","", returnCode, "main",__FILE__, __LINE__);
			       mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
			     }//end of if(returnCode != 0) //messageMassage()

			   else //messageMassage() successful
			     { int rc = 0;
                               memset(hldMsg,0,1048576);
                               if (GivenSopId[0] == '\0')
                                 { if (strcmp(Is4P,"SOL")==0)
                                     { strncpy(DefaultSopId, "4RS",3);
                                     }
                                   strcpy(hldMsg,"SopId=");
                                   strncat(hldMsg,DefaultSopId,3);
                                   strcat(hldMsg,"\xff");
                                   strcat(hldMsg,receiveQueue->message);
                                   strncpy(GivenSopId,DefaultSopId,3);
                                 }
                               else
                                 { strcpy(hldMsg,receiveQueue->message);
                                   if ((strcmp(GivenSopId,"YYY")==0) || 
                                       (strcmp(GivenSopId,"NNN")==0) )  // 4P exceptions,  CR4329
                                     { char* location;
                                       int startPosition;
                                       location = strstr(hldMsg, "SopId=");
                                       startPosition = location - hldMsg;
                                       startPosition = startPosition + 6;
                                       if (strcmp(GivenSopId,"YYY")==0)
                                         { hldMsg[startPosition]   = '4';
                                           hldMsg[startPosition+1] = 'R';
                                           hldMsg[startPosition+2] = 'S';
                                           strncpy(DefaultSopId, "4RS",3);
                                         }
                                       else
                                         { hldMsg[startPosition]   = DefaultSopId[0];
                                           hldMsg[startPosition+1] = DefaultSopId[1];
                                           hldMsg[startPosition+2] = DefaultSopId[2];
                                           strncpy(GivenSopId,DefaultSopId,3);
                                         }
                                     }
                                   else
                                     { strncpy(DefaultSopId,GivenSopId,3);
                                     }
                                 }
#ifdef DEBUG
cout << "functionCode="   <<  functionCode  << endl;
cout << "      userId="   <<  userId        << endl;
cout << "    GivenSopId=" <<  GivenSopId    << endl;
cout << "  DefaultSopId=" <<  DefaultSopId  << endl;
cout << "  4P indicator=" <<  Is4P          << endl;
cout << "  4P  dtl4Pmsg=" <<  dtl4Pmsg      << endl;
cout << "   Holdmessage=" <<  hldMsg        << endl;
#endif
                               memset(_putTime,0,_sizeOfTime);
			       rc = sendHoldQ->putMsg(_putTime,_msgId,_correlId, hldMsg);
			       if(rc !=0)
				 { mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5016:CRITICAL:DSOIROUTEREQUEST is unable to send to the HOLDQ error in MQPUT(): DSOIROUTEREQUEST exiting","",rc, "main", __FILE__, __LINE__);
				   mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   exit(1);
				 } // end of if(rc !=0) from HOLDQ
			       strcpy(DSOIconnectMQ::replyToQMgr,argv[y]);
		      
                               memset(RequestSopId,0,20);
                               memset(ReplySopId,0,20);
                               strcpy(RequestSopId,"N4P ");
                               strcpy(ReplySopId,"N4P ");
                               strncpy(servName,filler,9);
                               requestType = 0;
                               if (strcmp(Is4P,"SOL")==0)
                                 { strcpy(RequestSopId,"SOL ");
                                   strcpy(ReplySopId,"SOL ");
                                   strncpy(servName,"GETSOL   ",8);
                                   requestType = 1;
                                 }
                               else
                                 { if (strcmp(GivenSopId,"4RS")==0)
                                       requestType = 2;
                                   else if (strcmp(GivenSopId,"RSO")==0)
                                           requestType = 3;
                                   else if (strcmp(GivenSopId,"ESS")==0)
                                           requestType = 4;
                                   else if (strcmp(GivenSopId,"ESN")==0)
                                           requestType = 5;
                                   else if (strcmp(GivenSopId,"SOU")==0)
                                           requestType = 6;
                                   else if (strcmp(GivenSopId,"SOC")==0)
                                           requestType = 7;
                                   else { requestType = 0;
                                          strcpy(GivenSopId,"N4P ");
                                        }
                                   strcpy(RequestSopId, GivenSopId);
                                 }
                               memset(hdr4Pmsg,0,500);
                               strncpy(hdr4Pmsg,archHdrLen, 5);
                               strncat(hdr4Pmsg,archHdrVer, 4);
                               strncat(hdr4Pmsg,applDtaVer, 4);
                               strncat(hdr4Pmsg,servName, 8);
                               strncat(hdr4Pmsg,srcApplCode, 8);
                               strncat(hdr4Pmsg,resolvHostCd, 4);
                               strncat(hdr4Pmsg,replyInd, 1);
                               strncat(hdr4Pmsg,archRespInd, 1);
                               strncat(hdr4Pmsg,susTxnErrInd, 1);
                               strncat(hdr4Pmsg,routeInd, 1);
                               strncat(hdr4Pmsg,replyCode, 2);
                               strncat(hdr4Pmsg,userId, 8);
                               strncat(hdr4Pmsg,sessionId, 32);
                               strncat(hdr4Pmsg,hostName, 16);
                               strncat(hdr4Pmsg,perfEvalCode, 2);
                               strncat(hdr4Pmsg,dtl4Pmsg, strlen(dtl4Pmsg));
                               hdr4Pmsg[strlen(hdr4Pmsg)+1] = '\0';
                               if (strcmp(RequestSopId,"SOL ") == 0)
                                 { memset(hldMsg,0,1048576);
                                   strncpy(hldMsg, hdr4Pmsg, strlen(hdr4Pmsg));
                                   hldMsg[strlen(hldMsg)+1] = '\0';
                                 }

                               memset(sopToSend,0,48);
                               returnCode = requestToDSOI(RequestSopId, sopToSend);
			       if (returnCode !=0)
				 { mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5011:CRITICAL:DSOIROUTEREQUEST does not have a valid requestQ: DSOIROUTEREQUEST exiting","",rc, "main", __FILE__, __LINE__);
				   mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   exit(1);
				 }

                               memset(replyFromSop,0,48);
                               returnCode = replyToSop(ReplySopId, replyFromSop);
			       if(returnCode !=0)
				 { mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5019:CRITICAL:DSOIROUTEREQUEST does not have a valid replyQ: DSOIROUTEREQUEST exiting","",rc, "main", __FILE__, __LINE__);
				   mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				   exit(1);
				 }
                               strcpy(DSOIconnectMQ::replyToQ, replyFromSop);

#ifdef DEBUG
cout << "4Pcandidate="    <<  Is4P          << endl;
cout << "sopToSend="      <<  sopToSend     << endl;
cout << "  replyFromSop=" <<  replyFromSop  << endl;
cout << "  RequestSopId=" <<  RequestSopId  << endl;
cout << "      dtl4Pmsg=" <<  dtl4Pmsg      << endl;
cout << "      hdr4Pmsg=" <<  hdr4Pmsg      << endl;
cout << "        hldMsg=" <<  hldMsg        << endl;
#endif

                               memset(_putTime,0,_sizeOfTime);
                               switch (requestType)
                                  { case  0:
                                      if (openN4P == 0)
			                { sendQueueN4P = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          openN4P =  1;
                                        }
			              returnCode = sendQueueN4P->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    case  1:
                                      if (openSOL == 0)
			                { sendQueueSOL = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          openSOL =  1;
                                        }
			              returnCode = sendQueueSOL->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    case  2:
                                      if (open4RS == 0)
                                        { sendQueue4RS = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          open4RS =  1;
                                        }
                                      returnCode = sendQueue4RS->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    case  3:
                                      if (openRSO == 0)
                                        { sendQueueRSO = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          openRSO =  1;
                                        }
                                      returnCode = sendQueueRSO->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    case  4:
                                      if (openESS == 0)
                                        { sendQueueESS = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          openESS =  1;
                                        }
                                      returnCode = sendQueueESS->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    case  5:
                                      if (openESN == 0)
                                        { sendQueueESN = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          openESN =  1;
                                        }
                                      returnCode = sendQueueESN->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    case  6:
                                      if (openSOU == 0)
                                        { sendQueueSOU = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          openSOU =  1;
                                        }
                                      returnCode = sendQueueSOU->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    case  7:
                                      if (openSOC == 0)
                                        { sendQueueSOC = new(nothrow) DSOIsendMQ(sopToSend,argv[y]);
                                          openSOC =  1;
                                        }
                                      returnCode = sendQueueSOC->putMsg(_putTime,_msgId,_correlId, hldMsg);
                                      break;
                                    default:
				      mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				      mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5010:CRITICAL:DSOIROUTEREQUEST invalid requestType: DSOIROUTEREQUEST exiting","",rc, "main", __FILE__, __LINE__);
				      mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
                                      exit(1);
                                  }
          
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
#endif

                               if (returnCode != 0) //error sending to SOP
				 { int rc =0;
                                   memset(_putTime,0,_sizeOfTime);
				   if(rc = sendHoldQ->putMsg(_putTime,_msgId,_correlId,DSOIrouteRequest::_msgTrunc)!=0)
				     { mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5014:CRITICAL:FAILED because can't put correlated error message on HOLDQ so no error will be sent back to the REQUESTOR","", rc, "main",__FILE__, __LINE__);
				       mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				       exit(1);
				     } //end of if (rc = sendHoldQ->putMsg(DSOIrouteRequest::_msgTrunc)!=0)

				   if(requestErrorReply==0)
				     { requestErrorReply = new(nothrow) DSOIsendMQ("QL_DSOI_ERROR_REPLY", argv[y]);
				     }

				   mainErrObj.formatErrMsg("DSOIROUTEREQUEST:putMsg() FAILED to send to the Service Order Processor! DSOIRouteRequest will exit",
				   sopToSend,returnCode,"main",__FILE__,__LINE__);
				   mainErrObj.createReplyErrMsg(DSOIerrorHandler::_formatErrMsg);
                                   memset(_putTime,0,_sizeOfTime);
				   if(returnCode = requestErrorReply->putMsg(_putTime,_msgId,_correlId,
                                                                             DSOIerrorHandler::ReplyErrMsg) !=0)
				     { mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				       mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5015:CRITICAL:putMsg() FAILED to send an error to queue=",
                                                  "QL_DSOI_ERROR_REPLY",returnCode,"main",__FILE__,__LINE__);
				       mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				       exit(1);
				     } //end of if returnCode !-0 from replyReqeust->putMsg

				   else //send to REPORTQ
				     { if(sendReport( reportQ, errorSendQ) != 0)
					 { exit(1);
					 }
			             } //end of else send to REPORTQ
				   mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5009:CRITICAL:putMsg() FAILED because MQPUT() failed to send to Service Order Processor:DSOIROUTEREQUEST is exiting",
								"", returnCode,"main",__FILE__,__LINE__);

				   mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg ,errorSendQ);
				   exit(1);
				 }  //end of if(returnCode != 0) //error sending to SOP
                               else // if (returnCode != 0) //error sending to SOP
				 { if(sendReport(reportQ, errorSendQ) != 0)
				     { exit(1);
				     }
				 } // end of  else //no error sending to SOP

			     } // end of else messageMassage() successful

			 } // end of else message received (no errors)

		  } // end of while

		if(requestErrorReply) //queue of REQUESTOR
			delete requestErrorReply;
		delete errorSendQ;
		delete sendHoldQ;
		delete receiveQueue;
		delete routeQueue;
		delete reportQ;
		delete dataRefQueue;
		if (openN4P == 1)    delete sendQueueN4P;
		if (openSOL == 1)    delete sendQueueSOL;

                if (open4RS == 1)    delete sendQueue4RS;
                if (openRSO == 1)    delete sendQueueRSO;
                if (openESS == 1)    delete sendQueueESS;
                if (openESN == 1)    delete sendQueueESN;
                if (openSOC == 1)    delete sendQueueSOC;
                if (openSOU == 1)    delete sendQueueSOU;

	   } // end of try
	
        catch( ThrowError& err )
	{
#ifdef DEBUG
		cout << "ERROR= "  ;
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
			if(requestErrorReply) //queue of REQUESTOR
				delete requestErrorReply;
			if(errorSendQ)
			delete errorSendQ;
			mainErrObj.createReplyErrMsg(DSOIerrorHandler::_formatErrMsg);
			int rc=0;
			requestErrorReply = new(nothrow) DSOIsendMQ("QL_DSOI_ERROR_REPLY", argv[y]);
                        memset(_putTime,0,_sizeOfTime);
			rc = requestErrorReply->putMsg(_putTime,_msgId,_correlId,
                                                       DSOIerrorHandler::ReplyErrMsg);
			delete requestErrorReply;
			if(rc !=0)
			{
				cout << "Error sending requestor reply reporting critical error!" << endl;
				cout << "Error Message = " << DSOIerrorHandler::ReplyErrMsg << endl;
			}
			exit(1);
		}
		catch(ThrowSendError& err)
		{
			DSOIlogMQ* logQ = new(nothrow) DSOIlogMQ();
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
			if(errorSendQ !='\0')
				delete errorSendQ;
			mainErrObj.createReplyErrMsg(DSOIerrorHandler::_formatErrMsg);
			int rc=0;
			requestErrorReply = new(nothrow) DSOIsendMQ("QL_DSOI_ERROR_REPLY", argv[y]);
                        memset(_putTime,0,_sizeOfTime);
			rc = requestErrorReply->putMsg(_putTime,_msgId,_correlId,
                                                       DSOIerrorHandler::ReplyErrMsg);
			delete requestErrorReply;
			if(rc !=0)
			{
				cout << "Error sending requestor reply reporting critical error!" << endl;
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
		DSOIlogMQ* logQ = new(nothrow) DSOIlogMQ();
		try{
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
int sendReport(DSOIsendMQ* reportQueue, DSOIsendMQ* errorSendQ)
{
	formatReport(_reportMsg,
                                _Date,
				_putTime,
				_getTime,
				"DSOIRouteRequest",
				_putApplName,
				_putApplTime,
				_putApplDate,
				_msgId,
				DSOIrouteRequest::_msgTrunc);
	int returnCode = 0;
        memset(_putTime,0,_sizeOfTime);
	returnCode = reportQueue->putMsg(_putTime,_msgId,_correlId, _reportMsg);
	if(returnCode !=0)
	 { mainErrObj.formatErrMsg("DSOIROUTEREQUEST:5007:CRITICAL:errored because message can't be put on the QL_DSOI_REPORTQ: DSOIROUTEREQUEST exiting", "", returnCode, "main", __FILE__, __LINE__);
		mainErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
		return -1;
	 }
	return 0;
}

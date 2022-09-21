//////////////////////////////////////////////////////
//
//  FILE: DSOIrouteReply.C
//
//  PURPOSE:
//
//  REVISION HISTORY:
//-----------------------------------------------------------
//  version     date            author          description
//-----------------------------------------------------------
//  7.00.00     01/07/00        rfuruta         CR 3126, initial development
//  7.00.00	01/20/00	bxcox		CR 3126, further development
//  7.01.00     07/03/00        rfuruta 	CR 4275, 4P connection
//
//              CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of
//  U S West and Its Affiliates Having A Need To Know
//
///////////////////////////////////////////////////////

#include <new>
#include "DSOIsendMQ.H"
#include "DSOIrouteReply.H"
#include "DSOItoolbox.H"


////////////////////////////////////////////////
//
//  FUNCTION: DSOIrouteReply::DSOIrouteReply(char *, char *)
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
////////////////////////////////////////////////

DSOIrouteReply::DSOIrouteReply(char * receiveQ, char * qMgr)
{
	strcpy(receiveQueue, receiveQ);
	strcpy(queueManager, qMgr);

	sendQueue[0] = '\0';
	strcpy(holdQueue, "QL_DSOI_HOLD");
	strcpy(reportQueue, "QL_DSOI_REPORTQ");
	strcpy(errorReplyQueue, "QL_DSOI_ERROR_REPLY");
	strcpy(SOLReplyQueue, "QL_SOL_4P_DSOI_QUERY_REPLY");
	strcpy(DSOIReplyQueue, "QL_DSOIROUTEREPLY_DSOIREPLY_REQUEST");

	strcpy(sopIdTag, "SopId=");
	strcpy(sopIdValue, "   ");

	architectureHeaderLength = 0;
}


////////////////////////////////////////////////
//
//  FUNCTION: DSOIrouteReply::~DSOIrouteReply()
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
///////////////////////////////////////////////

DSOIrouteReply::~DSOIrouteReply()
{
}


//////////////////////////////////////////////////////
//
//  FUNCTION: char * get_______ ()
//
//  PURPOSE: return the respective private member
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

char * DSOIrouteReply::getQueueManager()
{ return queueManager; }

char * DSOIrouteReply::getReceiveQueue()
{ return receiveQueue; }

char * DSOIrouteReply::getSendQueue()
{ return sendQueue; }

char * DSOIrouteReply::getHoldQueue()
{ return holdQueue; }

char * DSOIrouteReply::getReportQueue()
{ return reportQueue; }

char * DSOIrouteReply::getErrorReplyQueue()
{ return errorReplyQueue; }

char * DSOIrouteReply::getSOLReplyQueue()
{ return SOLReplyQueue; }

char * DSOIrouteReply::getDSOIReplyQueue()
{ return DSOIReplyQueue; }


//////////////////////////////////////////////////////
//
//  FUNCTION: int DSOIrouteReply::messageMassage(char * msg, char * holdMsg, DSOIsendMQ * sendErrorQ)
//
//  PURPOSE: Formats a message for DSOIRequest, including the SOP Id which may or may not be provided by SOL
//
//  METHOD: 
//
//  PARAMETERS: char * msg - the SOL message
//		char * holdMsg - the client request message (gets passed on to DSOIRequest)
//  
//  OTHER OUTPUTS: Errored messages when parseMessage fails 
//  
//  OTHER INPUTS: _sendErrQ - the queue to send error messages only
//
//  RETURNS: One if it does not format the message correctly or
//	parseMessage fails, zero otherwise.
// 
//  FUNCTIONS USED: DSOIrouteReply::parseMessage(),
//	DSOIrouteReply::endOfMessage(), 
//	DSOIerrorHandler::sendErrorMesage(), strcpy(),
//	memset()
//
//////////////////////////////////////////////////////

int DSOIrouteReply::messageMassage(char * msg, char * holdMsg, char * newMessage, DSOIsendMQ * sendErrorQ)
{
	// 4P services response architecture header fixed record fields

	const int architectureHeaderLengthPosition = 0;
	char architectureHeaderLengthString [6];

	const int messageTypePosition = 17;
	char messageType [3];

	// find architecture header length

	_sendErrQ = sendErrorQ;
	int returnCode = 0;

        memset(architectureHeaderLengthString, 0, 6);
	strncpy(architectureHeaderLengthString, msg + architectureHeaderLengthPosition, 5);
	architectureHeaderLengthString[5] = '\0';
	architectureHeaderLength = atoi(architectureHeaderLengthString);

#ifdef DEBUG
cout << "architectureHeaderLength = " << architectureHeaderLength << endl;
#endif
        memset(messageType, 0, 3);
	strncpy(messageType, msg + messageTypePosition, 2);
	messageType[2] = '\0';

#ifdef DEBUG
cout << "messageType=" << messageType << endl;
#endif

	// determine if message is application response or error response

	if ( strcmp(messageType, "AR") == 0 )
	{
		// process application-defined response

		// determine if response is from SOL
	
		if( strcmp(receiveQueue, SOLReplyQueue) == 0 )
		{
			// process SOL message
	
			returnCode = processSOLMessage(msg, holdMsg, newMessage);
	
			if( returnCode )
			{
				// error processing SOL message
				returnCode = -1;

			        sprintf(_errorMessage,"%s%s","DSOIROUTEREPLY:unable to process SOL message=",
                                        sopIdValue);
			        messageErrObj.createReplyErrMsg(_errorMessage);
                                messageErrObj.formatErrMsg("DSOIROUTEREPLY:unable to process SOL message=", 
                                                sopIdValue, returnCode, "messageMassage",__FILE__, __LINE__);
                                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
				return returnCode;
			}
		}
	
		else
		{
			// message did not come from a known SOL local reply queue
			returnCode = -1;
			sprintf(_errorMessage,"%s%s","DSOIROUTEREPLY:message came from unknown SOL replyQ=",
                                receiveQueue);
			messageErrObj.createReplyErrMsg(_errorMessage);
                        messageErrObj.formatErrMsg("DSOIROUTEREPLY:message came from unknown SOL replyQ=",
                                      receiveQueue, returnCode, "messageMassage",__FILE__, __LINE__);
                        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return returnCode;
		}
	
	}	// end if messageType == AR

	else if ( strcmp(messageType, "ER") == 0 )
	{
		// process standard error message response

		returnCode = processStandardErrorResponse(msg, newMessage);

		if ( returnCode )
		{
			// error processing error response 
	
			returnCode = -1;
			messageErrObj.createReplyErrMsg("DSOIROUTEREPLY:unable to process SOL error response");
                        messageErrObj.formatErrMsg("DSOIROUTEREPLY:unable to process SOL error response",
                                      "", returnCode, "messageMassage",__FILE__, __LINE__);
                        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return returnCode;
		}

		// send flag back to calling function to indicate that we processed an error message

		returnCode = -1;
		return returnCode;
	}

	else
	{
		// unknown message type

		returnCode = -1;
		sprintf(_errorMessage, "%s%s", "DSOIROUTEREPLY:SOL returned unhandled messageType=",
                                           messageType);
		messageErrObj.createReplyErrMsg(_errorMessage);
                messageErrObj.formatErrMsg("DSOIROUTEREPLY:SOL returned unhandled messageType=",
                                 messageType, returnCode, "messageMassage",__FILE__, __LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return returnCode;
	}

	return returnCode;
}


//////////////////////////////////////////////////////
//
//  FUNCTION: int DSOIrouteReply::processSOLMessage (char *, char *)
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

int DSOIrouteReply::processSOLMessage(char * msg, char * holdMsg, char * newMessage)
{
	// SOL response fixed record fields
	int returnCode;
        const char * SopIdTag = "SopId=";
        char  oldSopId[4];
        char* tagValue=  new(nothrow) char[10];

	const int solCompCodePosition = 0;
	//const int solCompCodeLength   = 1;

	const int solReasonCodePosition = 1;
	char reasonCode[6];

	char* solSopIdPosition = msg + architectureHeaderLength + 106;
	const int solSopIdLength   =   3;	

	// find COMP_CODE in fixed record message

	char c = msg[architectureHeaderLength + solCompCodePosition];
	int retCd = atoi( &c );

#ifdef DEBUG
cout << "returnCode=" << retCd << endl;
#endif

	// if ( retCd )
	if ((retCd != 0) && (retCd != 1))   // returnCode=1  not found
	{
		// SOL query errored

		// find REASON_CODE in fixed record message

                memset(reasonCode, 0, 6);
		strncpy(reasonCode, msg + architectureHeaderLength + solReasonCodePosition, 5);
	        reasonCode[5] = '\0';

#ifdef DEBUG
cout << "reasonCode=" << reasonCode << endl;
#endif

		returnCode = -1;
		sprintf(_errorMessage, "%s%s", "DSOIROUTEREPLY:SOL query errored with reasonCode=", reasonCode);
		messageErrObj.createReplyErrMsg(_errorMessage);
                messageErrObj.formatErrMsg("DSOIROUTEREPLY:SOL query errored with reasonCode=",
                                           reasonCode, returnCode, "processSOLMessage",__FILE__, __LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);

		return returnCode;
	}

	// find FunctionCode tag/value in hold message

	char functionCode[3];
	returnCode = findTagValue(holdMsg, "FunctionCode=", functionCode);

	if ( (returnCode == 0)  &&  (strcmp(functionCode, "CL") == 0) )
	{
		// for FunctionCode=CL, SOL will provide the data we need so we don't need to go to the SOP; 
                // we will construct a reply message in RSOLAR format and send it directly to DSOIReply

		// construct CL reply message

		returnCode = createCLReply(retCd, msg, holdMsg, newMessage);

		if ( returnCode )
		{
			// unable to create CL reply message

			returnCode = -1;
			messageErrObj.createReplyErrMsg("DSOIROUTEREPLY:unable to create FunctionCode=CL reply message for DSOIReply");
			messageErrObj.formatErrMsg("DSOIROUTEREPLY:unable to create FunctionCode=CL reply message for DSOIReply",
                                                   "", returnCode, "processSOLMessage", __FILE__, __LINE__);
                        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);

			return returnCode;
		}

		// set reply queue for messages to DSOIReply

		strcpy(sendQueue, DSOIReplyQueue);

#ifdef DEBUG
cout << "sendQueue = " << sendQueue << endl;
#endif

		return returnCode;
	}

	// find RSLVD-HST-CD in fixed record message
        memset(sopIdValue,' ',10);
	strncpy(sopIdValue, solSopIdPosition, solSopIdLength);
	sopIdValue[solSopIdLength] = '\0';
#ifdef DEBUG
cout << "SOL sopId=" << sopIdValue << endl;
#endif

        memset(tagValue,0,10);
        if (findTagValue(holdMsg, (char *)SopIdTag, tagValue) == 0)
          { strcpy(oldSopId, tagValue);
	    oldSopId[3] = '\0';
	    if ( strlen(sopIdValue) == 0 )
	      { // SOL could not determine the SopId
		// use the SopId from holdMsg
                strcpy(sopIdValue, oldSopId);
              }
          }
#ifdef DEBUG
cout << "Old sopId=" << oldSopId << endl;
#endif

	if ( strcmp(sopIdValue, "RSO") == 0	||
	     strcmp(sopIdValue, "4RS") == 0	||
	     strcmp(sopIdValue, "ESS") == 0	||
	     strcmp(sopIdValue, "ESN") == 0	||
	     strcmp(sopIdValue, "SOU") == 0	||
	     strcmp(sopIdValue, "SOC") == 0		)
	{
		// SOL provided a known SOP Id

		returnCode = insertNewSopId(holdMsg, newMessage);

		if ( returnCode )
		{
			// failed to insert new SopId in hold message

			returnCode = -1;
			sprintf(_errorMessage, "%s%s", "DSOIROUTEREPLY:unable to insert new SopId in message to DSOIRequest, SopId=", sopIdValue);
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg("DSOIROUTEREPLY:unable to insert new SopId in message to DSOIRequest, SopId=",
                                           sopIdValue, returnCode, "processSOLMessage", __FILE__, __LINE__);
                        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);

			return returnCode;
		}
	}

	else
	{
		// Unrecognized SopID
		returnCode = -1;
		sprintf(_errorMessage, "%s%s", "DSOIROUTEREPLY:SOL returned unrecognized SopId=", sopIdValue);
		messageErrObj.createReplyErrMsg(_errorMessage);
		messageErrObj.formatErrMsg("DSOIROUTEREPLY:SOL returned unrecognized SopId=",
                                          sopIdValue, returnCode, "processSOLMessage", __FILE__, __LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return returnCode;
	}

	// determine which queue to use for messages to DSOIRequest

	returnCode = requestToDSOI(sopIdValue, sendQueue);

	if ( returnCode )
	{
		// could not determine reply queue
	
		returnCode = -1;
		sprintf(_errorMessage, "%s%s", "DSOIRouteReply:unable to determine reply queue, SopId=",
                        sopIdValue);
		messageErrObj.createReplyErrMsg(_errorMessage);
		messageErrObj.formatErrMsg("DSOIROUTEREPLY:unable to determine reply queue, SopId=",
                                           sopIdValue, returnCode, "processSOLMessage", __FILE__, __LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return returnCode;
	}

#ifdef DEBUG
cout << "sendQueue = " << sendQueue << endl;
#endif

	return returnCode;
}


//////////////////////////////////////////////////////
//
//  FUNCTION: int DSOIrouteReply::processStandardErrorResponse(char *)
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

int DSOIrouteReply::processStandardErrorResponse(char * msg, char * newMessage)
{
	const char * returnCodeTag = "ReturnCode=";
	const char * errorMessageCountTag = "ErrorMessageCount=";
	const char * errorMessageTag = "ErrorMessage=";

	// standard error response fixed record fields

	char* errorCountPosition = msg + architectureHeaderLength + 9;
	char errorCount[4];

	// total length of all the error fields for each error (1 - 25 errors)

	const int errorDataLength = 1319;

	// the following positions are variable for each error

	char* errorReturnCodePosition   = msg + architectureHeaderLength +  40;
	const int errorReturnCodeLength =  2;
	char errorReturnCode[3];

	char* errorTextPosition      = msg + architectureHeaderLength + 216;
	const int errorTextLength    =  50;

	char* errorCodePosition      = msg + architectureHeaderLength + 516;
        const int errorCodeLength    =   6;
        int returnCode = 0;

	// find error count
        memset(errorCount, 0, 4);
	strncpy(errorCount, errorCountPosition, 3);
	errorCount[3] = '\0';
	const int errorCountNumber = atoi(errorCount);

#ifdef DEBUG
cout << "errorCountNumber = " << errorCountNumber << endl;
#endif

	// find error return code
        memset(errorReturnCode, 0, 3);
	strncpy(errorReturnCode, errorReturnCodePosition, errorReturnCodeLength); 
	errorReturnCode[2] = '\0';

#ifdef DEBUG
cout << "errorReturnCode = " << errorReturnCode << endl;
#endif

	// ReturnCode=02ÿErrorMessageCount=003ÿErrorMessage=

	sprintf(newMessage, "%s%s%c%s%s%c%s", returnCodeTag, errorReturnCode, 0xff,
                errorMessageCountTag, errorCount, 0xff, errorMessageTag);

	// collect all six text lines for each error; also include return code and error code for each error
	// For the Messaging Architecture, the first error in the response will always be the last one reported
        // by the application.

	if ( errorCountNumber > 0 )
	  { strncat(newMessage, errorReturnCodePosition, errorReturnCodeLength);
	    strncat(newMessage, errorCodePosition, errorCodeLength);
	    for ( int n = 0; n < errorCountNumber; ++n )
	       {
	         // jump forward in message to next error

		 strncat(newMessage, errorTextPosition, errorTextLength);
	// 	 strcat(newMessage, "\n");
		 errorTextPosition      += errorDataLength;

	       }
	  }

	sprintf(newMessage, "%s%c", newMessage, 0xff);

#ifdef DEBUG
cout << "newMessage = " << newMessage << endl;
#endif

	return returnCode;
}


//////////////////////////////////////////////////////
//
//  FUNCTION: int DSOIrouteReply::insertNewSopId(char *)
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

int DSOIrouteReply::insertNewSopId(char * holdMsg, char * newMessage)
{
        const char * SopIdTag = "SopId=";
        char oldSopIdVal[4] = "   ";

	// find SOP Id tag/value in hold message
	int returnCode = findTagValue(holdMsg, (char *)SopIdTag, oldSopIdVal);

	if( returnCode )
	{
		// SOP Id tag/value not found in hold message;
		// concatenate new SOP Id tag/value with hold message

		returnCode = 0;

		// SopId= + 4RS + ÿ + holdMsg + \0
		sprintf(newMessage, "%s%s%c%s", SopIdTag, sopIdValue, 0xff, holdMsg);
	}
	else
	{
		// SOP Id tag/value found in hold message;
		// overlay existing SOP Id tag/value in the hold message

		returnCode = 0;
                char* location;
                int   startPosition;
		strncpy(newMessage, holdMsg, strlen(holdMsg));
                location=strstr(newMessage, "SopId=");
                startPosition = location - newMessage;
                startPosition = startPosition + 6;
                newMessage[startPosition]   = sopIdValue[0];
                newMessage[startPosition+1] = sopIdValue[1];
                newMessage[startPosition+2] = sopIdValue[2];
	}

#ifdef DEBUG
cout << "hold   Msg=" << holdMsg    << endl;
cout << "newMessage=" << newMessage << endl;
#endif

	return returnCode;
}


//////////////////////////////////////////////////////
//
//  FUNCTION: int DSOIrouteReply::createCLReply(char *, char *)
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

int DSOIrouteReply::createCLReply(int retCd, char * msg, char * holdMsg, char * newMessage)
{
        char* candidateCountPosition    = msg + architectureHeaderLength + 6;
	const int candidateListLength   = 99;
	char candidateCountString[5];

	// the following positions are variable for each candidate list

	char* orderNumberPosition        = msg + architectureHeaderLength + 10;
	const int orderNumberLength 	 =  9;
	
	char* orderStatusPosition        = msg + architectureHeaderLength + 19;
	const int orderStatusLength 	 =  2;

	char* telephoneNumberPosition    = msg + architectureHeaderLength + 21;
	const int telephoneNumberLength  = 10;
	
	char* customerCodePosition       = msg + architectureHeaderLength + 31;
	const int customerCodeLength  	 =  3;
        
	char* listedNamePosition         = msg + architectureHeaderLength + 34;
	const int listedNameLength 	 = 28;
        
	char* listedAddressPosition      = msg + architectureHeaderLength + 62;
	const int listedAddressLength    = 28;
        
	char* clsServPosition            = msg + architectureHeaderLength + 90;
	const int clsServLength 	 =  5;
        
	char* dueDatePosition            = msg + architectureHeaderLength + 95;
	const int dueDateLength 	 = 10;
        
	char* resolvedHostCodePosition   = msg + architectureHeaderLength + 105;
	const int resolvedHostCodeLength =  4;

	int returnCode = 0;

	// find candidate list count
        memset(candidateCountString, 0, 5);
        strncpy(candidateCountString, candidateCountPosition, 4);
        candidateCountString[4] = '\0';
	const int candidateCount = atoi(candidateCountString);

        if ( holdMsg[strlen(holdMsg) - 1] == '\n' )
	  { holdMsg[strlen(holdMsg) - 1] = '\0';
          }

	if ((candidateCount == 0) || (retCd == 1))
	{
		// construct ReturnCode=03 reply

                char* location;
                int   startPosition;
                location = strstr(holdMsg,"FunctionCode=");
                startPosition= location - holdMsg;
                startPosition= startPosition + 13;
                holdMsg[startPosition]   = 'O';
                holdMsg[startPosition+1] = 'E';
		
		const char * ReturnCodeTag = "ReturnCode=03";
		const char * ErrMsgCntTag  = "ErrorMessageCount=000";
                sprintf(newMessage, "%s%s%c%s%c", holdMsg, ReturnCodeTag, 0xff,
                        ErrMsgCntTag, 0xff);

	}  // end if ( candidateCount == 0 )
	else
	{
		const char * candidateCountTag = "CandidateCount=";
		const char * candidateListTag  = "CandidateList=";
                sprintf(newMessage, "%s%s%s%c%s", holdMsg, candidateCountTag, 
                        candidateCountString, 0xff, candidateListTag);
                for ( int n = 0; n < candidateCount; ++n )
		{
			// collect data for each candidate list
			strncat(newMessage, orderNumberPosition, 1);
			strncat(newMessage, " ", 1);
			strncat(newMessage, orderNumberPosition+1, 8);
			strncat(newMessage, listedNamePosition, 28);
			strncat(newMessage, listedAddressPosition, 28);
			strncat(newMessage, telephoneNumberPosition, 3);
			strncat(newMessage, " ", 1);
			strncat(newMessage, telephoneNumberPosition+3, 3);
			strncat(newMessage, "-", 1);
			strncat(newMessage, telephoneNumberPosition+6, 4);
			strncat(newMessage, " ", 1);
			strncat(newMessage, customerCodePosition, 3);
			strncat(newMessage, orderStatusPosition,  2);
			strncat(newMessage, clsServPosition,    5);
			strncat(newMessage, dueDatePosition+5,    5);
			strncat(newMessage, "-", 1);
			strncat(newMessage, dueDatePosition, 4);
			strncat(newMessage, resolvedHostCodePosition,  4);
     //        		strcat(newMessage, "\n");
			orderNumberPosition     += candidateListLength;
			orderStatusPosition     += candidateListLength;
			telephoneNumberPosition += candidateListLength;
			customerCodePosition    += candidateListLength;
			listedNamePosition      += candidateListLength;
			listedAddressPosition   += candidateListLength;
	 		dueDatePosition         += candidateListLength;
                }   // end for ( int n = 0; n < candidateCount; ++n )

                sprintf(newMessage, "%s%c", newMessage, 0xff );

	}	// end else candidateCount > 0
	
        char * hexffPtr = newMessage;
        while ( hexffPtr = strchr(hexffPtr, '\xff') )
                hexffPtr[0] = '\x9f';

#ifdef DEBUG
cout << "hold   Msg=" << holdMsg    << endl;
cout << "newMessage=" << newMessage << endl;
#endif
	return returnCode;
}


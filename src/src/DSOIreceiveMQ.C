///////////////////////////////////////////////////////
//  FILE: DSOIreceiveMQ.C
//
//  PURPOSE:  Create a connection to a receive message queue
//	using the MQOPEN call and also to receive messages
//	from the queue using the MQSeries MQGET() call.
//  
//  REVISION HISTORY:
//-------------============-----------------------------------------
//  version	date		author	description
//------------------------------------------------------------------
//  1.00.00	06/30/97	lparks	initial release IMA
//  2.00.01	09/24/97	lparks	initial release CENTRAL
//  2.00.02	10/09/97	lparks  increased _getTime array	
//  7.00.00	12/23/99	rfuruta CR____, changed DSOIrouteMQ to DSOIconnectMQ	
//  8.03.00     06/07/01        rfuruta CR5391, translate the msgId and correlId to character
//  8.08.00     01/02/03        rfuruta CR8000, upgrade to HP-UX 11.0
//  8.09.00     03/22/03        rfuruta CR 8180, fix memory leaks
//  8.10.00     06/11/03        rfuruta CR 8316, change message size from 4M to 1M
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOIreceiveMQ.H"
#include "DSOIerrorHandler.H"
#include "DSOIThrowError.H"
#include "DSOIdistribute.H"
#include "DSOItoolbox.H"
#include <cstring>
#include <new>

int DSOIreceiveMQ::_getCorrel=0;

////////////////////////////////////////////////
//  FUNCTION: DSOIreceiveMQ::DSOIreceiveMQ
//	(char* objName, char* mgrName) :DSOIconnectMQ(mgrName)
//
//  PURPOSE: The constructor for the DSOIreceiveMQ class.
//
//  METHOD: Initializes the MQ Series structures that are needed as input
//	to the MQOPEN() call.  Opens a connection the the MQ Series message queue
//	to receive messages and creates an object of
//	DSOIreceiveMQ class.
//
//  PARAMETERS: char* objName - the name of the message queue
//		char* mgrName - the name of the queue manager
//  
//  OTHER OUTPUTS: If the MQOPEN() fails then it updates the
//	reasonCode and returnCode of the DSOIErrorHandler class
//	for error messaging.
//  
//  OTHER INPUTS: The connection handle (HConn) to the queue manager
//	and the initial values for the MQSeries structures.
//
//  RETURNS: None
// 
//  FUNCTIONS USED: MQSeries MQOPEN(), memcpy(), strcat(), throw() 
//
//////////////////////////////////////////////////////
DSOIreceiveMQ::DSOIreceiveMQ(char* objName, char* mgrName, MQLONG browse,MQLONG input) :DSOIconnectMQ(mgrName)
{
#ifdef DEBUG
cout << " DSOIreceiveMQ constructor " << endl;
cout << "objName in receive constructor " << objName << endl;
#endif
	// set the Options
	getOptions = MQOO_FAIL_IF_QUIESCING;
	if(browse > 0)
	{
		getOptions += MQOO_BROWSE;
		_browseOpt = 1;
	}
	else 
	{
		getOptions += input ;
		_browseOpt  =0;
	}

	// initialize all members
	closeOptions =0;
	_convertOpt = 0L;
	message = '\0'; 
	messageLength = 0;
	msgLengthReceived = 0;
	_waitInterval = 0L;

	// initialize the Object Descriptor (MQOD)
	strcpy(objDescriptor.StrucId, MQOD_STRUC_ID);
	objDescriptor.Version = MQOD_VERSION_1;
	objDescriptor.ObjectType = MQOT_Q;
	strcpy(objDescriptor.ObjectName,objName);
	strcpy(objDescriptor.ObjectQMgrName,QueueManager);

	// initialize the messageDescriptor (MQMD)
	strcpy(messageDescriptor.StrucId, MQMD_STRUC_ID);
	messageDescriptor.Version = MQMD_VERSION_1;
	messageDescriptor.Report = MQRO_NONE;
	messageDescriptor.MsgType = MQMT_DATAGRAM;
	messageDescriptor.Encoding = MQENC_NATIVE;
	messageDescriptor.Expiry = MQEI_UNLIMITED;
	messageDescriptor.Feedback = MQFB_NONE;
	messageDescriptor.Encoding = MQENC_NATIVE;
	messageDescriptor.CodedCharSetId = MQCCSI_Q_MGR;
	strcpy(messageDescriptor.Format, MQFMT_NONE);
	messageDescriptor.Priority = MQPRI_PRIORITY_AS_Q_DEF;
	messageDescriptor.Persistence = MQPER_PERSISTENCE_AS_Q_DEF;
	memcpy(messageDescriptor.MsgId, MQMI_NONE, sizeof(messageDescriptor.MsgId));
	memcpy(messageDescriptor.CorrelId, MQCI_NONE, sizeof(messageDescriptor.CorrelId));
	memcpy(messageDescriptor.AccountingToken, MQCI_NONE, sizeof(MQBYTE32));
	messageDescriptor.PutApplType = MQAT_NO_CONTEXT;
	strncpy(messageDescriptor.ReplyToQ, "", (unsigned)MQ_Q_NAME_LENGTH);
	strncpy(messageDescriptor.ReplyToQMgr, "", (unsigned)MQ_Q_MGR_NAME_LENGTH);

	// initialize the getObj (MQGMO)
	memcpy(getObj.StrucId, MQGMO_STRUC_ID, sizeof(MQGMO_STRUC_ID));
	getObj.Version = MQGMO_VERSION_1;
	getObj.Options = MQGMO_NO_WAIT;

	MQOPEN (Hconn,
		&objDescriptor,
		getOptions,
		&objHandle,
		&errorObj.completionCode,
		&errorObj.reasonCode);

	if(errorObj.completionCode == MQCC_FAILED)
	{
		errorObj.formatErrMsg((char *) "DSOIMQ:4002:CRITICAL:Constructor  for DSOIreceiveMQ class MQOPEN failed for object = ", objName, errorObj.reasonCode, (char *) "DSOIreceiveMQ", (char *) __FILE__, __LINE__ );
		throw ThrowError(DSOIerrorHandler::_formatErrMsg,errorObj.reasonCode);
	}
}

////////////////////////////////////////////////
//  FUNCTION: int DSOIreceiveMQ::getMsg( DSOIsendMQ* errorSendQ)
//
//  PURPOSE: To receive messages from the MQ Series message queue.
//
//  METHOD:  Sets the appropriate MQ Series structures to valid values
//	(these are input parameters to MQSeries) and calls MQGET()
//	to receive messages from the message queue.
//	Then holds the message descriptor msgId and CorrelId for later
//	correlation.
//
//  PARAMETERS: DSOIsendMQ* errorSendQ - the message queue to send error
//	messages to when the MQGET() call fails.
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS: The connection to the queue manager(HConn) and the
//	handle (objHandle) to the message queue returned from the 
//	MQOPEN() call.
//
//  RETURNS: Returns the resaon code from the MQGET() call or the
//	return code from the DSOIroute::DSOImessage::messageMassage()
//	or zero otherwise.
// 
//  FUNCTIONS USED: MQGET(), memset(), memcpy(),strcpy(), 
//	DSOIerrorHandler::sendErrorMessage()
//
//////////////////////////////////////////////////////

int DSOIreceiveMQ::getMsg( char* putApplName,    char* putApplDate, 
                           char* putApplTime,    char* getTime,
                           const unsigned char* msgId, MQBYTE24* correlId,
                           DSOIsendMQ* errorSendQ)
{  MQLONG msg_len =0;

	if(_getCorrel==1) //for the MQGET call want to get a
					// message matching the MsgId
	{
		memcpy(messageDescriptor.MsgId, msgId, sizeof(messageDescriptor.MsgId));
		//only use this next memcpy when both MsgId
		//and CorrelId is matched for MQGET
		//memcpy(messageDescriptor.CorrelId, correlId, sizeof(messageDescriptor.CorrelId));
		// Allow any correlation ID to be matched this must
		// be used to correlate ONLY on MsgId !
		memcpy(messageDescriptor.CorrelId, MQCI_NONE, sizeof(messageDescriptor.CorrelId));


 #ifdef DEBUG
 cout << "message id in Correlate " << messageDescriptor.MsgId << endl;
 #endif
 	} // end of if correlation
 	else
	{
	// Make sure that the CorrelId and MsgId are cleared before
	// the next call to MQGET or the queue manager will try to
	// get a message off of the queue with the same CorrelId and
	// MsgId as the last message received from the MQGET call.
		memcpy(messageDescriptor.MsgId, MQMI_NONE, sizeof(messageDescriptor.MsgId));
		memcpy(messageDescriptor.CorrelId, MQCI_NONE, sizeof(messageDescriptor.CorrelId));
	} //end of else no correlation

	if(message == '\0')
	{
		message = new char[1048576];
		messageLength = 1048576;
	}

	//clear all buffers
	memset(message,0,(unsigned)messageLength);
	memset(replyToQ,0,sizeof(replyToQ));
	memset(replyToQMgr,0,sizeof(replyToQMgr));

	// initialize the getObj (MQGMO)

	if(_waitInterval > 0)
	{
		getObj.WaitInterval = _waitInterval;
		getObj.Options = MQGMO_WAIT;
	}
	else
	{
		getObj.Options = MQGMO_NO_WAIT;
	}
	if(_browseOpt > 0)
	{
		getObj.Options += MQGMO_BROWSE_NEXT;
	}
	if(_convertOpt > 0)
	{
		getObj.Options += MQGMO_CONVERT;
	}


	MQGET(Hconn,
		objHandle,
		&messageDescriptor,
		&getObj,
		messageLength,
		message,
		&msgLengthReceived,
		&errorObj.completionCode,
		&errorObj.reasonCode);
#ifdef DEBUG
cout << "messageLengthReceived= " << msgLengthReceived << endl;
cout << "        messageLength= " << messageLength     << endl;
#endif
        msg_len = messageLength;

 	if(errorObj.completionCode != MQCC_OK)
	{
		if(errorObj.reasonCode != MQRC_NO_MSG_AVAILABLE) //no message
		{
			if(errorObj.reasonCode == MQRC_TRUNCATED_MSG_FAILED) //means that the messageBuffer too small
			{
				delete [] message; //create larger message buffer
				message = new char[msgLengthReceived+100]; //create larger message buffer
				messageLength = msgLengthReceived+100;
				char lengthMsg[20];
	//			sprintf(lengthMsg,"%d",msgLengthReceived+100);
				sprintf(lengthMsg,"%d",msg_len);
				errorObj.formatErrMsg((char *) "DSOIreceiveMQ: Msg Buffer too small and msg would be truncated, msg buffer is created as size=",
					lengthMsg, 
					errorObj.reasonCode, 
					(char *) "getMsg",
					(char *) __FILE__, 
					__LINE__);
				errorObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
			} // message buffer too small for message to be received
			else
			{
				errorObj.formatErrMsg((char *) "DSOIMQ:4003:CRITICAL:getMsg() FAILED because MQGET failed: message was not received and is still on queue ",
					(char *) "",
					errorObj.reasonCode, 
					(char *) "getMsg",
					(char *) __FILE__, 
					__LINE__);
				errorObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,errorSendQ);
				sprintf(_errorMessage,"%s%04d%s","DSOIMQ:4003:CRITICAL:getMsg() FAILED because MQGET failed: message was not received and is still on queue <", errorObj.reasonCode,">"); 
				errorObj.createReplyErrMsg(_errorMessage);
			}
		}
		return errorObj.reasonCode;
	} 
	else //no error from MQGET()
	{
#ifdef DEBUG
cout << "replyToQ= " << messageDescriptor.ReplyToQ << endl;
cout << "replyToQMgr= " << messageDescriptor.ReplyToQMgr << endl;
#endif
		if(_getCorrel==1) //message received is correlated
		{
			char* qPtr = messageDescriptor.ReplyToQ;
			char* blankPtr = strchr(messageDescriptor.ReplyToQ,' ');
			strncpy(replyToQ, messageDescriptor.ReplyToQ,
						blankPtr-qPtr);
			replyToQ[blankPtr-qPtr] = '\0';
			qPtr = messageDescriptor.ReplyToQMgr;
			blankPtr = strchr(messageDescriptor.ReplyToQMgr,' ');
			strncpy(replyToQMgr, messageDescriptor.ReplyToQMgr,
					blankPtr-qPtr);
			replyToQMgr[blankPtr-qPtr] = '\0';

			memcpy(correlId,messageDescriptor.CorrelId,
					sizeof(messageDescriptor.CorrelId));

#ifdef DEBUG
cout << "replyToQ= " << replyToQ << endl;
cout << "replyToQMgr= " << replyToQMgr << endl;
#endif
		} //end of if (_getCorrel==1)
		else //no correlation
		{
			calculateTime(getTime);
			strncpy(replyToQ, messageDescriptor.ReplyToQ,
						sizeof(messageDescriptor.ReplyToQ));
			strncpy(replyToQMgr, messageDescriptor.ReplyToQMgr,
						sizeof(messageDescriptor.ReplyToQMgr));
			// copy the msgId, correlId, 
			//		and ReplyToQ for message correlation
			memcpy(correlId,messageDescriptor.CorrelId,
					sizeof(messageDescriptor.CorrelId));
			memcpy((char*)msgId,messageDescriptor.MsgId,
					sizeof(messageDescriptor.MsgId));
#ifdef DEBUG
cout << "message ID copied  " << msgId << endl;
cout << "message id from MQGET" << messageDescriptor.MsgId << endl;
#endif
			strncpy(putApplName,messageDescriptor.PutApplName,DSOI_STRING_28);
			putApplName[DSOI_STRING_28]='\0';
			strncpy(putApplTime,messageDescriptor.PutTime,
							DSOI_STRING_8);
			putApplTime[DSOI_STRING_8] = '\0';
			convertTime(putApplTime);
			strncpy(putApplDate,messageDescriptor.PutDate,DSOI_STRING_8);
			putApplDate[DSOI_STRING_8] = '\0';
		} //end of else no correlation
		memcpy(errorObj.errCorrelId,messageDescriptor.CorrelId,sizeof(messageDescriptor.CorrelId));
		memcpy(errorObj.errMsgId,messageDescriptor.MsgId,sizeof(messageDescriptor.MsgId));
		return (errorObj.reasonCode);
	} //end of else no error from MQGET()
	return 0;

}

////////////////////////////////////////////////
//  FUNCTION: DSOIreceiveMQ::~DSOIreceiveMQ()
//
//  PURPOSE: Destructor for the DSOIreceiveMQ class.
//
//  METHOD: Removes the DSOIreceiveMQ class object from scope and 
//	closes the connection from the message queue using MQCLOSE().
//
//  PARAMETERS: None
//  
//  OTHER OUTPUTS: Throws an exception when the MQCLOSE() fails.
//  
//  OTHER INPUTS: The connection handle (Hconn) returned from the
//	connection to the queue manager, the object handle(objHandle)
//	returned from the MQOPEN().
//
//  RETURNS: None
// 
//  FUNCTIONS USED: MQCLOSE(), strcpy(), sprintf(), strcpy()
//
//////////////////////////////////////////////////////

DSOIreceiveMQ::~DSOIreceiveMQ()
{
	delete [] message;
	MQCLOSE(Hconn,
		&objHandle,
		closeOptions,
		&errorObj.completionCode,
		&errorObj.reasonCode);
	if(errorObj.completionCode != MQCC_OK)
	{
		errorObj.formatErrMsg((char *) "DSOIMQ:4004:CRITICAL:Destructor  for DSOIreceiveMQ class MQCLOSE failed ",
			(char *) "",
			errorObj.reasonCode,
			(char *) "~DSOIreceive",
			(char *) __FILE__,
			__LINE__);
		throw ThrowError(DSOIerrorHandler::_formatErrMsg, errorObj.reasonCode);
	}
}

////////////////////////////////////////////////
//  FUNCTION: int DSOIreceiveMQ::setWaitInterval(MQLONG)
//
//  PURPOSE: To set the time to wait for a message for MQGET()
//
//  METHOD: sets an interval and updates the -waitInterval variable
//
//  PARAMETERS: interval - the interval in milliseconds
//  
//  OTHER OUTPUTS:  -waitInterval - updated with interval value
//  
//  RETURNS: None
// 
//
//////////////////////////////////////////////////////
void DSOIreceiveMQ::setWaitInterval( MQLONG interval)
{
	_waitInterval = interval;
	
}

////////////////////////////////////////////////
//  FUNCTION: int DSOIreceiveMQ::setBrowse(MQLONG)
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS:  
//  
//  RETURNS: None
// 
//
//////////////////////////////////////////////////////
void DSOIreceiveMQ::setBrowse( MQLONG opt)
{
	_browseOpt = opt;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOIreceiveMQ::setInput(MQLONG)
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS:  
//  
//  RETURNS: None
// 
//
//////////////////////////////////////////////////////
void DSOIreceiveMQ::setInput( MQLONG input)
{
	_InputOpt = input;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOIreceiveMQ::setConvert(MQLONG)
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS:  
//  
//  RETURNS: None
// 
//
//////////////////////////////////////////////////////
void DSOIreceiveMQ::setConvert( MQLONG convert)
{
	_convertOpt = convert;
}

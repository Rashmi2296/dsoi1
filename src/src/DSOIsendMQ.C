///////////////////////////////////////////////////////
//  FILE:  DSOIsendMQ.C
//
//  PURPOSE: Functions for the DSOIsendMQ class
//  
//  REVISION HISTORY:
//---------------------------------------------------------
//  version	date		author		description
//---------------------------------------------------------
//  1.00.00	07/01/97	lparks		initial release IMA
//  2.00.01	09/24/97	lparks		initial release CENTRAL
//  2.00.02	10/06/97	lparks	        increased _putTime array
//  6.00.00     07/31/99        rfuruta         CR1118, ReturnCode=05 is not always true
//  6.00.00     11/12/99        rfuruta         CR2847, report record is missing the 'Date=' field
//  7.00.00     12/23/99        rfuruta         CR____, changed DSOIrouteMQ to DSOIconnectMQ
//  8.00.00     11/25/00        rfuruta         CR4767, remove 4P stuff
//  8.08.00     01/02/03        rfuruta         CR8000, upgrade to HP-UX 11.0
//  8.09.00     03/22/03        rfuruta         CR8180, fix memory leaks
//  9.04.00     02/11/04        rfuruta         CR8911, setExpiry time
//  9.08.00     12/14/04        rfuruta         CR9745, setPriority
//  9.08.00     02/15/05        rfuruta         CR9873, set CorrelId
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////
#include "DSOIsendMQ.H"
#include "DSOIerrorHandler.H"
#include "DSOIThrowError.H"
#include "DSOIdistribute.H"
#include "DSOItoolbox.H"
#include <cstring>
#include <new>

////////////////////////////////////////////////
//  FUNCTION: DSOIsendMQ::DSOIsendMQ
//
//  PURPOSE: To connect to the queue manager and to the
//	send message queue and create a DSOIsendMQ class object.
//
//  METHOD: Initialize all MQ Series structures that are required
//	input data members to the MQOPEN call.
//
//  PARAMETERS: char* objName - 
//	char* mgrName  - 
//	: DSOIconnectMQ(mgrName)
//  
//  OTHER OUTPUTS: objHandle - connection handle to the message queue
//	Creates an error message when the MQOPEN is unsuccessful and
//	throws an exception.
//  
//  OTHER INPUTS: Hconn - connection to the MQSeries queue manager
//
//  RETURNS: a connection the the message queue and an object of
//	DSOIsendMQ class.
// 
//  FUNCTIONS USED: MQOPEN() , throw(), 
//	DSOIerrorHandler::createReplyErrMsg();
//
//////////////////////////////////////////////////////
DSOIsendMQ::DSOIsendMQ(int setExpiry, int setPriority, char* objName, char* mgrName) : DSOIconnectMQ(mgrName)
{
	// initialize messageDescriptor (MQMD)
	strncpy(messageDescriptor.StrucId,MQMD_STRUC_ID,sizeof(MQCHAR4));
	messageDescriptor.Version = MQMD_VERSION_1;
	messageDescriptor.Report  = MQRO_NONE;
	messageDescriptor.MsgType = MQMT_DATAGRAM;
        if (setExpiry > 0)
	    messageDescriptor.Expiry = 1200;      // 2 minutes
        else
	    messageDescriptor.Expiry = MQEI_UNLIMITED;

	messageDescriptor.Feedback = MQFB_NONE;
	messageDescriptor.Encoding = MQENC_NATIVE;
	messageDescriptor.CodedCharSetId = MQCCSI_Q_MGR;
	memcpy(messageDescriptor.Format, MQFMT_NONE,(unsigned)MQ_FORMAT_LENGTH);
        if (setPriority > 0)
	    messageDescriptor.Priority = setPriority;
        else
	    messageDescriptor.Priority = MQPRI_PRIORITY_AS_Q_DEF;

	messageDescriptor.Persistence = MQPER_PERSISTENCE_AS_Q_DEF;
	memcpy(messageDescriptor.MsgId, MQMI_NONE, (unsigned)MQ_MSG_ID_LENGTH);
	memcpy(messageDescriptor.CorrelId, MQMI_NONE, (unsigned)MQ_CORREL_ID_LENGTH);
	memcpy(messageDescriptor.ReplyToQ, "", (unsigned)MQ_Q_NAME_LENGTH);
	memcpy(messageDescriptor.UserIdentifier, "", (unsigned)MQ_USER_ID_LENGTH);
	memcpy(messageDescriptor.AccountingToken, MQACT_NONE, sizeof(MQBYTE32));
	messageDescriptor.PutApplType = MQAT_NO_CONTEXT;

	// set outputOptions
	outputOptions = MQOO_OUTPUT+MQOO_FAIL_IF_QUIESCING;

	closeOptions = 0;

	//set the object descriptor inforamtion (MQOD)
	strncpy(objDescriptor.StrucId , MQOD_STRUC_ID,sizeof(MQOD_STRUC_ID));
	objDescriptor.Version = MQOD_VERSION_1;
	objDescriptor.ObjectType = MQOT_Q;
	memcpy(objDescriptor.ObjectQMgrName, QueueManager,(unsigned) MQ_Q_NAME_LENGTH);
	//memcpy(objDescriptor.ObjectQMgrName, QueueManager,sizeof(objDescriptor.ObjectQMgrName));
	//memcpy(objDescriptor.ObjectName ,objName,(unsigned) MQ_Q_NAME_LENGTH); // name of object opened

	strcpy(objDescriptor.ObjectName ,objName); // name of object opened

#ifdef DEBUG
cout << "QueueManger  is " << QueueManager << endl;
cout << "objName is " << objName << endl;
#endif

	//initialize MQPMO structure
	memcpy(putOptions.StrucId , MQPMO_STRUC_ID,sizeof(MQPMO_STRUC_ID));
	putOptions.Version = MQOD_VERSION_1;
	putOptions.Options = MQPMO_NONE;
	putOptions.Context = 0L;

	// initialize members
	message = 0;
	messageLength = 0;
	_sizeOfMessage = 0;

	MQOPEN(Hconn,
		&objDescriptor,
		outputOptions,
		&objHandle,
		&errorObj.completionCode,
		&errorObj.reasonCode);
	if (errorObj.completionCode == MQCC_FAILED)
	{
#ifdef DEBUG
cout << "errorObj.reasonCode= " << errorObj.reasonCode << endl;
#endif
		errorObj.formatErrMsg((char *) "DSOIMQ:4007:CRITICAL:Constructor  for DSOIsendMQ class MQOPEN failed for object of ", objName,errorObj.reasonCode, (char *) "DSOIsendMQ", (char *) __FILE__, __LINE__);
		throw ThrowError(DSOIerrorHandler::_formatErrMsg,errorObj.reasonCode);
	}
}

////////////////////////////////////////////////
//  FUNCTION: int DSOIsendMQ::putMsg
//
//  PURPOSE: To put a message on an MQ Series message queue
//
//  METHOD:  Determine the length of the message, set the input MQ Series
//	structure members to values defined  through the MQGET() and
//
//  PARAMETERS: char* msg -  the message to put on the message queue
//	DSOIsendMQ* errorSendQ - 
//  
//  OTHER OUTPUTS: DSOIerrorHandler::reasonCode - MQSeries ReasonCode from an
//	unsuccessful MQPUT() call
//	DSOIerrorHandler::returnCode - MQ Series ReturnCode from an 
//	unsuccessful MQPUT() call
//	
//  
//  OTHER INPUTS: DSOIsendMQ::message - the defined length message copied
//	from the msg parameter and sent to message queue
//
//  RETURNS: The ResaonCode from an unsuccessfull MQPUT()
//	call, 0 otherwise.
// 
//  FUNCTIONS USED: MQPUT(), strcpy(), memcpy(), memset(),
//	DSOIerrorHandler::createReplyErrMsg();
//
//////////////////////////////////////////////////////
int DSOIsendMQ::putMsg(char* putTime, const unsigned char* msgId, MQBYTE24 correlId, char* msg)
{
	messageLength = strlen(msg);
#ifdef DEBUG
cout << "messageLength= " << messageLength << endl;
#endif
	if(message == 0)
	{
		message = new char[messageLength+1];
		_sizeOfMessage = messageLength+1;
		message[0] = '\0';
	} // end of if
	else
	{
		if(messageLength > _sizeOfMessage)
		{
			delete [] message;
			message = new char[messageLength+1];
		        message[0] = '\0';
			_sizeOfMessage = messageLength+1;
		}
	} // end of else

	// only uncomment this when the CorrelId will be used
	// for the MQGET call
	memcpy(messageDescriptor.CorrelId,correlId,sizeof(messageDescriptor.CorrelId));
	memcpy(messageDescriptor.MsgId,msgId,sizeof(messageDescriptor.MsgId));
	//strcpy(messageDescriptor.ReplyToQ,replyToQ);
	memcpy(messageDescriptor.ReplyToQ,replyToQ,sizeof(messageDescriptor.ReplyToQ));
	//strcpy(messageDescriptor.ReplyToQMgr,replyToQMgr);
	memcpy(messageDescriptor.ReplyToQMgr,replyToQMgr,sizeof(replyToQMgr));

        if( (strstr(msg, "ErrorMessage=DSOIREQUEST") != NULL)		||
            (strstr(msg, "ErrorMessage=DSOIREPLY") != NULL)	)
        {
            char* location;
            int   startPosition;
            delete [] message;
	    message = new char[messageLength+1];
	    _sizeOfMessage = messageLength+1;
	    // memset(message, 0, (unsigned)messageLength);
	    memset(message, 0, (unsigned)_sizeOfMessage);
	    strncpy(message,msg,(unsigned)_sizeOfMessage);
            location=strstr(message,"ReturnCode=");
            startPosition = location - message;
            startPosition = startPosition + 11;

            if (strstr(msg, "ErrorMessage=DSOIREQUEST") != NULL)
            {
                message[startPosition]   = '0';
                message[startPosition+1] = '4';
            }
            else
            {
                message[startPosition]   = '0';
                message[startPosition+1] = '5';
            }
        }

        else
	{
            memset(message, 0, (unsigned)messageLength);
	    strncpy(message,msg,(unsigned)messageLength);
        } 

replyToQ[47] = '\0';
replyToQMgr[47] = '\0';

//to allow the cout remove when cout is removed
#ifdef DEBUG
cout << "message=" << message << endl;
cout << "objName=" << objDescriptor.ObjectName << endl;
cout << "replyTQ= " << DSOIconnectMQ::replyToQ << endl;
cout << "replyTQMgr= " << DSOIconnectMQ::replyToQMgr << endl;
cout << "correlId in putMsg()= " << correlId << endl;
cout << "msgId in putMsg()= " << msgId << endl;
#endif

	MQPUT(	Hconn, 
		objHandle,
		&messageDescriptor,
		&putOptions,
		messageLength,
		message,
		&errorObj.completionCode,
		&errorObj.reasonCode);
	if (errorObj.completionCode == MQCC_FAILED)
	{
		errorObj.formatErrMsg((char *) "DSOIMQ:4008:CRITICAL:putMsg() MQPUT FAILED for message=",message, errorObj.reasonCode, (char *) "putMsg", (char *) __FILE__, __LINE__ );
		sprintf(_errorMessage,"%s%s%s%d%s","DSOIMQ:4008:CRITICAL:putMsg() MQPUT() FAILED for message=",message, "<",errorObj.reasonCode,">");
		errorObj.createReplyErrMsg(_errorMessage);
		return(errorObj.reasonCode);
	}
	calculateTime(putTime);
	return (0);				
}

////////////////////////////////////////////////
//  FUNCTION: DSOIsendMQ::~DSOIsendMQ()
//
//  PURPOSE: To close the connection to the 
//	message queue and put the DSOIsendMQ object
//	out of scope.
//
//  METHOD: Call MQCLOSE() and if there is an error
//	throw an exception.
//
//  PARAMETERS: none
//  
//  OTHER OUTPUTS: Creates an error message and throws an exception
//	Updates the DSOIerrorHandler::reasonCode and 
//	DSOIerrorHandler::returnCode from an unsuccessful MQCLOSE()
//  
//  OTHER INPUTS: The connection handle(HConn) to the queue manager, 
//	the connection handle (objHandle) to the message queue.
//
//  RETURNS: none
// 
//  FUNCTIONS USED: MQ Series MQCLOSE(), throw(), strcat(),
//	strcpy(), sprintf()
//
//////////////////////////////////////////////////////
DSOIsendMQ::~DSOIsendMQ()
{
#ifdef DEBUG
cout << "in destructor for DSOIsendMQ " << endl;
#endif
	delete [] message;
	MQCLOSE(Hconn, 
		&objHandle,
		closeOptions,
		&errorObj.completionCode,
		&errorObj.reasonCode);	
	if(errorObj.completionCode != MQCC_OK)
	{
		errorObj.formatErrMsg((char *) "DSOIMQ:Destructor  for DSOIsendMQ class MQCLOSE() failed ",(char *) "",errorObj.reasonCode, (char *) "~DSOIsendMQ", (char *) __FILE__,__LINE__);
		throw ThrowError(DSOIerrorHandler::_formatErrMsg, errorObj.reasonCode);
	}
}

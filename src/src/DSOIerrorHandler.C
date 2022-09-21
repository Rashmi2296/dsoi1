///////////////////////////////////////////////////////
//  FILE: DSOIerrorHandler.C
//
//  PURPOSE:
//  
//  RELATED HEADER FILES:
//
//  REVISION HISTORY:
//  version	date		author		description
//  6.00.00     08/04/99        rfuruta         CR582, critical err msg written to stdout
//  6.00.00     08/04/99        rfuruta         CR590, critical err msg written to stdout
//  8.03.00     06/07/01        rfuruta         CR5391, translate the msgId and correlId to character
//  8.08.00     01/02/03        rfuruta         CR8000, upgrade to HP-UX 11.0
//  8.09.00     03/22/03        rfuruta         CR8180, fix memory leaks
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////
#include <iostream.h>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <new>
#include "DSOIerrorHandler.H"
#include "DSOIsendMQ.H"
#include "DSOIThrowSendError.H"

char* DSOIerrorHandler::ReplyErrMsg = '\0';
int DSOIerrorHandler::_formatSize = 3000;
int DSOIerrorHandler::_lengthMsgSend = 0;
int DSOIerrorHandler::_lengthMsgReply = 0;
char* DSOIerrorHandler::_formatErrMsg = new char[_formatSize];
static char _Date[11];
static char _putTime[12];
unsigned char DSOIerrorHandler::errMsgId[] = {0};
MQBYTE24 DSOIerrorHandler::errCorrelId = "";

////////////////////////////////////////////////
//  FUNCTION: DSOIerrorHandler::DSOIerrorHandler()
//
//  PURPOSE: Constructor for the DSOIerrorHandler class
//
//  METHOD: Creates the array to hold the error message.
//	Initializes all members of the DSOIerrorHandler class.
//
//  PARAMETERS: None
//  
//  FUNCTIONS USED: memset(), new() 
//
//////////////////////////////////////////////////////

DSOIerrorHandler::DSOIerrorHandler()
{
#ifdef DEBUG
cout << "in constructor for DSOIerrorhandler" << endl;
#endif
	completionCode = 0;
	reasonCode = 0;
	_lengthMsgSend = 0;
	_lengthMsgReply = 0;
	errorMessage = '\0';
}

////////////////////////////////////////////////
//  FUNCTION: void DSOIerrorHandler::sendErrorMessage
//	(char* message, DSOIsendMQ* sendQueue, MQLONG returnCode)
//
//  PURPOSE: To create a logged message and send it to the
//	error queue.
//
//  METHOD: Adds the date and time the message is created 
//	before it logs the message. Then sends it to the
//	error queue.
//
//  PARAMETERS:char* message - error message
//	DSOIsendMQ* sendQueue - queue to send error
//	MQLONG returnCode - any related return code to error
//  
//  OTHER OUTPUTS: The time and date the error message was sent,
//	the correlId and msgId received from the MQGET call.
//  
//  OTHER INPUTS: The CorrelId and Msgid from the message
//	descriptor structure (MQMD) from the MQGET() call.
//
//  RETURNS: void but pprints out the message to stdout if
//	unable to send the message.
// 
//  FUNCTIONS USED: strftime(), time(), strcat(), strcpy(),
//	sprintf(), DSOIsendMQ::putMsg()
//
//////////////////////////////////////////////////////
void DSOIerrorHandler::sendErrorMessage(char* message, DSOIsendMQ* sendQueue)
{
#ifdef DEBUG
	cout << "in sendErrorMessage and message= " << message << endl;
#endif
	int length = strlen(message);
	
	_lengthMsgSend = length+176;
	char * errMsg = new char[_lengthMsgSend];
	memset(errMsg,0,_lengthMsgSend);
	time_t timeNow;
	static int dateTimeSize = 9;
	time(&timeNow);

	static char curTime[9];
	strftime(curTime,dateTimeSize,"%H%M%S",localtime(&timeNow) );

	static char curDate[11];
	strftime(curDate,dateTimeSize,"%m%d%Y",localtime(&timeNow) );

        // cout << "test DSOIErrHndlr    ErrObj.errMsgId=" <<  errMsgId    << endl;
        // cout << "test DSOIErrHndlr ErrObj.errCorrelId=" <<  errCorrelId << endl;

        char Mid[49];
        char Mptr[4];
        memset(Mid,0,49);
        Mid[0] = '\0';
        for(int i=0; i < 24; i++)
        {
                memset(Mptr,0,4);
                int returnCode = sprintf(Mptr,"%d",errMsgId[i]);
                Mptr[strlen(Mptr)] = '\0';
                if (returnCode > 0)
                	strcat(Mid, Mptr);
                else	break;
        }

	sprintf(errMsg, "%s%s%s%s%s%s%s",
		message, ":Date=", curDate,
		":Time=", curTime,
		":msgId=", Mid);
//		":Time=", curTime, ":correlId=", errCorrelId,
//		":msgId=", Mid);
#ifdef DEBUG
	cout << " errMsg  = " << errMsg << endl;
#endif
        memset(_putTime,0,12);

        int returnSendCode = sendQueue->putMsg(_putTime, errMsgId, errCorrelId, errMsg);
	if(returnSendCode != 0)
	{
		char errorCritical[2048];
		memset (errorCritical,0,2048);
		sprintf(errorCritical, "%s%s%s",
			errMsg, 
			"DSOIMQ:4001:CRITICAL:Can't send messages to the ERRORQ  return code=",
                        returnSendCode); 
#ifdef DEBUG
	cout << " errorCritical  = " << errorCritical << endl;
#endif
		formatErrMsg(errorCritical,(char *) "",
			returnSendCode,
			(char *) "sendErrorMsg()",
			(char *) __FILE__,
			__LINE__);
		throw ThrowSendError(_formatErrMsg,returnSendCode);
	}
        delete []errMsg;
}

////////////////////////////////////////////////
//  FUNCTION:int DSOIerrorHandler::createReplyErrMsg
//		(char* message)
//
//  PURPOSE: When DSOI receives a message from ICADS and there
//	the message can't be sent to the SOP for some reason then
//	an error message must be sent back to ICADS for correlation
//	purposes.
//
//  METHOD: Formats the message in a SOP format so that it can be
//	put on the reply to queue for DSOI.  This allows the reply
//	process to format the error message in a ICAD format and
//	then send it to the reply to queue.
//
//  PARAMETERS: char* message - the errored message defining the problem
//  
//  OTHER OUTPUTS: Sets the ErrorCode to 1 
// 	and sets the errorCount to 1 whcih are ICAD parameters and ends
//	the message with hexFF which is also expected by the reply process
//	of DSOI. Sets the global member icadErrMsg to the errored
//	message so that the message can be reached from any portion of
//	the DSOIRequest code.
//  
//  RETURNS: One if the array size for icadErrMsg can't be created,
//	zero otherwise.
// 
//  FUNCTIONS USED: memset(), stcpy(), strcat(), new(), delete()
//
//////////////////////////////////////////////////////
int DSOIerrorHandler::createReplyErrMsg(char* message)
{
	int length = strlen(message);
	int mod = length%74; //test for message chunks greater than 74
	int size = length/74;
#ifdef DEBUG
cout << "size =    " << size << "mod =    " << mod << endl;
#endif
	if(mod != 0)
	{
		size++;
		mod = 74 - mod;
#ifdef DEBUG
cout << "new mod = " << mod << endl;
#endif
	}
	if(_lengthMsgReply < length)
	{
		// test if(_lengthMsgReply != 0)
			delete [] ReplyErrMsg;
		// 81 bytes = 74 filler+2 "EE" + 3 "001" + 2 "01" + size +EOL
		int sizeOfErrorMessage = length + size + 81 + mod;
		ReplyErrMsg = new char[sizeOfErrorMessage];
		_lengthMsgReply = sizeOfErrorMessage;
	} //end of if _lengthMsgRepyl < length
	memset(ReplyErrMsg,0, _lengthMsgReply);
	strcpy(ReplyErrMsg,"F");
	for(int i = 0; i < 28; i++) //add filler before Function Code
		strcat(ReplyErrMsg,"F");
	strcat(ReplyErrMsg,"E"); //DSOI Function Code
	strcat(ReplyErrMsg,"E"); //DSOI Function Code
	for(int i = 0; i < 44; i++) //add filler after Function Code
		strcat(ReplyErrMsg,"F");
	strcat(ReplyErrMsg,"01");
	char ErrorCount[3];
	sprintf(ErrorCount,"%03d",size);
	strcat(ReplyErrMsg,ErrorCount);

	char* messagePtr = message;
	while(length > 74)
	{
		strncat(ReplyErrMsg,messagePtr,74);
		strcat(ReplyErrMsg,"\xff");
		messagePtr +=74;
		length -= 74;
	}
	if(length >0)
	{
		strncat(ReplyErrMsg,messagePtr,length);
		ReplyErrMsg[strlen(ReplyErrMsg)] = '\0';
	}
	for(int i =0; i< mod; i++)
		strcat(ReplyErrMsg," ");
	ReplyErrMsg[_lengthMsgReply-2] = '\xFF';
	ReplyErrMsg[_lengthMsgReply-1] = '\0';
#ifdef DEBUG
cout << "ReplyErrorMessage = " << ReplyErrMsg << endl;
#endif

	return 0;
}


////////////////////////////////////////////////
//  FUNCTION:int DSOIerrorHandler::formatErrMsg
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS: 
//  
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOIerrorHandler::formatErrMsg(char* message, 
			char* value, MQLONG returnCode, 
			char* function, char* file, int line)
{
	memset(_formatErrMsg, 0, _formatSize);
	sprintf(_formatErrMsg, "%s%s%s%04d%s%s%s%s%s%s%07d",
		message, value, ":rc=",
		returnCode, ":function=", function,
		"()", ":file=", file,
		":line=", line);
	return 0;

}
////////////////////////////////////////////////////////
//  FUNCTION: DSOIerrorHandler::~DSOIerrorHandler()
//
//  PURPOSE: Destructor for the DSOIerrorhandler class.
//
//  METHOD: 
//
//////////////////////////////////////////////////////
DSOIerrorHandler::~DSOIerrorHandler()
{
#ifdef DEBUG
cout << "in errorHandler destructor " << endl;
#endif
	if(errorMessage != '\0')
		delete [] errorMessage;
}

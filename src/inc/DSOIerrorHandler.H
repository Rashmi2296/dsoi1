#ifndef DSOIerrorHandler_H
#define DSOIerrorHandler_H

#include <cmqc.h>

class DSOIsendMQ;

class DSOIerrorHandler
{
public:
	MQLONG completionCode ;
	MQLONG	reasonCode ;
	static char* _formatErrMsg;
	DSOIerrorHandler();
	void sendErrorMessage(char*, DSOIsendMQ *);
	int formatErrMsg(char*, char*, MQLONG, char*, char*,  int);
	int createReplyErrMsg(char*);
	static char* ReplyErrMsg;
        static unsigned char  errMsgId[25];
        static MQBYTE24 errCorrelId;

	~DSOIerrorHandler();
protected:
private:
	static int _lengthMsgReply;
	static int _lengthMsgSend;
	static int _formatSize;
	char * errorMessage;
};
#endif

#ifndef  DSOIrouteReply_H
#define  DSOIrouteReply_H

class DSOIerrorHandler;
class DSOIsendMQ;

// maximum length of MQ object name
const int maxLength = 50;

class DSOIrouteReply
{
	public:

		DSOIrouteReply(char *, char *);
		~DSOIrouteReply();

		int messageMassage(char *, char *, char *, DSOIsendMQ *);

		char * getQueueManager();

		char * getReceiveQueue();
		char * getSendQueue();
		char * getHoldQueue();
		char * getReportQueue();
		char * getErrorReplyQueue();
		char * getSOLReplyQueue();
		char * getDSOIReplyQueue();

	private:

		int processSOLMessage(char *, char *, char *);
		int processStandardErrorResponse(char *, char *);
		int insertNewSopId(char *, char *);
		int createCLReply(int, char *, char *, char *);


		DSOIerrorHandler messageErrObj;
		DSOIsendMQ * _sendErrQ;

		char queueManager[maxLength];
		char receiveQueue[maxLength];
		char sendQueue[maxLength];
		char holdQueue[maxLength];
		char reportQueue[maxLength];
		char errorReplyQueue[maxLength];
		char SOLReplyQueue[maxLength];
		char DSOIReplyQueue[maxLength];

		char sopIdTag[7];
		char sopIdValue[4];

		int architectureHeaderLength;
};

#endif

#ifndef  DSOImessageReply_H
#define  DSOImessageReply_H

#include "DSOIerrorHandler.H"

class DSOImessageReply
{
	public:
		static int messageMassage(char*, char*, MQLONG, char*, DSOIsendMQ*,char*);
		static int returnFunctionType() { return _FunctionType; }
		static int findNPANXX(char*);
		static int _ONSflag;
		static char * _requestQueue;

 	private:
		static int _sizeOfServiceOrder;
		static int _sizeOfData;
		static int _sizeOfNewMessage;
		static int _sizeNeeded;
		static char* _serviceOrder;
		static char* _data;
		static int _FunctionType;
		static char _elementType[2];
		static char _Lock[2];
		static char _actionCode[2];
		static char _OrderType[3];
		static char _returnCode[3];
		static char _OriginalFunctionCode[3];
		static char _FunctionCode[3];
		static char _SectionId[3];
		static char _UserId[9];
		static char _UserId2[9];
		static char errorMessageCount[4];
		static char _candidateCount[4];
		static char _statusCount[4];
		static char _SbnQty[3];
		static char _CiQty[3];
		static char _tagName[6];
		static char _ParentSeq[7];
		static char _SeqNo[7];
		static char _npaNxx[7];		//to be updated from the HOLDQ message
		static char _OrderNumber[9];
		static char _LsrId[11];
		static char _ownCode[2];
		static char _errorMessage[1024];

		static DSOIerrorHandler messageErrObj;
		static DSOIsendMQ* _sendErrQ;

		static int parseMessage(char*, int, char*, int, int, char*);
		static void mergeMessage(char*);
		static void mergeServiceOrder();
		static int sizeNewMessage(const int);
		static int endOfMessage(char*);
		static int setParameters(char*);
		static int queryListSetup(char*, char*);
		static int OrderStatusSetup(char*, char*);
		static int BillingNumberSetup(char*, char*);
		static int CircuitIdSetup(char*, char*);
		static int serviceOrderSetup(char*, MQLONG, char*);
		static int checkIfRsolar(char*);
		static int endOfLine(char*);
		static int findElement(char*);
		static int checkActionCode(char*);
		static int setSectionId(char*);
		static int getFunction(char*);
		static int parseErrorMessage(char*, char*);
		static int parseCandidateList(char*, char*);
		static int parseOrderStatusList(char*, char*);
		static int parseBillingNumberList(char*, char*);
		static int parseCircuitIdList(char*, char*);
		static int setONS(char*, char*);
		static int parseServiceOrder(char*, MQLONG);
		static void fillSpace(char*, int);	// fill spaces in message
		static int find9F(char*);
		static int getClientSopId(char*, char*);
		static int parseMsgRQ(char*);
		static int findTagVal9f(char*, const char*, char *);
 };

 #endif

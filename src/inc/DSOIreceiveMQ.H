#ifndef DSOIreceiveMQ_H
#define DSOIreceiveMQ_H
///////////////////////////////////////////////////////
//  FILE: 
//
//  PURPOSE:
//  
//  RELATED HEADER FILES:
//
//  REVISION HISTORY:
//  version	date	author	description
//  1.00.00     06/24/97  lparks  initial release IMA
//  2.00.01     09/24/97  lparks  initial release Eastern
//  7.00.00     12/23/99  rfuruta  CR____, changed DSOIrouteMQ to DSOIconnectMQ
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <cmqc.h>
#include "DSOIconnectMQ.H"
#include "DSOIerrorHandler.H"
#include "DSOIdistribute.H"

class DSOIsendMQ;

class DSOIreceiveMQ : public DSOIconnectMQ
{
public:
	DSOIreceiveMQ(char*, char*, MQLONG=0,MQLONG=MQOO_INPUT_SHARED);
	~DSOIreceiveMQ();
	MQLONG getMsgRecv(){return msgLengthReceived;}
	int getMsg(char*,char*,char*,char*,const unsigned char*,MQBYTE24*, DSOIsendMQ* = NULL);

	//class members
	static int _getCorrel;
	char *message;

	void setWaitInterval(MQLONG);
	void setBrowse(MQLONG);
	void setInput(MQLONG);
	void setConvert(MQLONG);
	MQMD getMessageDescriptor(){return messageDescriptor;}
protected:
private:
	MQLONG _browseOpt;
	MQLONG closeOptions;
	MQLONG getOptions;
	MQLONG _InputOpt;
	MQLONG messageLength;
	MQLONG msgLengthReceived;
	MQLONG _waitInterval;
	MQLONG _convertOpt;
	MQGMO  getObj;
	MQHOBJ objHandle;
	MQMD messageDescriptor;
	MQOD objDescriptor;
};
#endif //DSOIreceiveMQ_H

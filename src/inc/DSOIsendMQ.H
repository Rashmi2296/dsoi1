#ifndef DSOIsendMQ_H
#define DSOIsendMQ_H

///////////////////////////////////////////////////////
//  FILE: 
//
//  PURPOSE:
//  
//  RELATED HEADER FILES:
//
//  REVISION HISTORY:
//  version	date	author	description
//  1.00.00     06/24/97 lparks  initial release IMA
//  2.00.01     09/24/97 lparks  initial release DSOI eastern
//  2.00.02     10/09/97 lparks  remove overloaded putMsg
//  7.00.00     12/23/99 rfuruta CR____, changed DSOIrouteMQ to DSOIconnectMQ
//  9.04.00     02/11/04 rfuruta CR 8911, setExpiry time
//  9.08.00     12/14/04 rfuruta CR 9745, setPriority
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <cmqc.h>
#include "DSOIconnectMQ.H"
#include "DSOIerrorHandler.H"
#include "DSOIdistribute.H"

class DSOIsendMQ  : public DSOIconnectMQ
{
public:
	DSOIsendMQ(int, int, char*, char*);
	virtual ~DSOIsendMQ();
	int putMsg(char*, const unsigned char*, MQBYTE24, char*);   // putting a message on a queue
protected:
private:
	MQLONG	closeOptions;	// MQCLOSE options		
	MQLONG 	messageLength;
	MQLONG	outputOptions;	// MQOPEN options
	MQLONG  _sizeOfMessage;
	char * 	message;
	MQHOBJ 	objHandle;
	MQMD 	messageDescriptor;
	MQOD  	objDescriptor;
	MQPMO	putOptions;	//  options that control the action of MQPUT
};
#endif // DSOIsendMQ_H

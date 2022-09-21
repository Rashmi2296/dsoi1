#ifndef DSOIconnectMQ_H
#define DSOIconnectMQ_H

///////////////////////////////////////////////////////
//  FILE: 
//
//  PURPOSE:
//  
//  RELATED HEADER FILES:
//
//  REVISION HISTORY:
//  version	date	 author	 description
//  7.00.00     12/23/99 rfuruta initial development
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////
// DSOIconnectMQ.H
// This class is used for MQ Series routing

#include <cmqc.h>
#include <iostream.h>
#include <cstdlib>
#include "DSOIerrorHandler.H"
#include "DSOIconstants.H"

class DSOIconnectMQ
{
	friend ostream& operator<<(ostream &, const DSOIconnectMQ & );
public:
//	DSOIconnectMQ(const PMQCHAR="" ); 	//queue manager name defaulted 
	DSOIconnectMQ(char * );  		//queue manager name defaulted 
	virtual ~DSOIconnectMQ();			//destructor

	//class members
	static MQCHAR48 replyToQ; 	
	static MQCHAR48 replyToQMgr;
	DSOIerrorHandler errorObj; 	//composition
protected:
	char* getEnvVariable(char*);
	static MQHCONN Hconn;		// connection handle to queue manager long value
	MQCHAR48 QueueManager;
private:
	static int connectFlag;
	static int connections;
};

#endif //DSOIconnectMQ_H

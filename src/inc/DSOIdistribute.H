#ifndef DSOIdistribute_H
#define DSOIdistribute_H

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
// DSOIdistribute.H
// This class is used for MQ Series routing

#include <cmqc.h>
#include <iostream.h>
#include <cstdlib>
#include "DSOIerrorHandler.H"
#include "DSOIconstants.H"

class DSOIerrorHandler;

const int _NumberOfSops=11;

class DSOIdistribute
{
	friend ostream& operator<<(ostream &, const DSOIdistribute & );
public:
//	DSOIdistribute(const PMQCHAR="" ); 	//queue manager name defaulted 
	DSOIdistribute(char * );	 	//queue manager name defaulted 
	virtual ~DSOIdistribute();			//destructor

	//class members
	static char* sopToSend;
	static char* replyFromSop;	
	DSOIerrorHandler errorObj; 	//composition
protected:
	char* getEnvVariable(char*);
	static MQHCONN Hconn;		// connection handle to queue manager long value
	MQCHAR48 QueueManager;
private:
};

#endif //DSOIdistribute_H

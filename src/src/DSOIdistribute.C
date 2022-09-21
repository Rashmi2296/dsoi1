//////////////////////////////////////////////////////
//  FILE:  DSOIdistribute.C
//
//  PURPOSE: Contains all the functions for the DSOIdistribute.
//  
//  REVISION HISTORY:
//-----------------------------------------------------------------
//  version	date		author		description
//-----------------------------------------------------------------
//  7.00.00	12/23/99	rfuruta		initial development
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOIdistribute.H"
#include "DSOIThrowError.H"
#include "DSOItoolbox.H"
#include "DSOIconstants.H"
#include <cstdio>   
#include <cstddef>
#include <ctime>
#include <cctype>
#include <new>
//#include <unistd.h>
//#include <sys/stat.h>

MQHCONN DSOIdistribute::Hconn = 0;
char * DSOIdistribute::sopToSend = 0;
char * DSOIdistribute::replyFromSop = 0;

////////////////////////////////////////////////
//  FUNCTION: DSOIdistribute
//
//  PURPOSE: The constructor for DSOIreoutMQ class where the queue
//	     manager is defined as the default queue manager. If 
//	     the queue manager is defined as the QMgrName.
//
//  METHOD: An instance of the DSOIdistribute class is created and the 
//          QueueManager member is initialized to the QMgrName.
//
//  PARAMETERS: const PMQCHAR QMgrName - the name of the queue manager and
//              the PMQCHAR is a pointer to a char (char*)
//
//  OTHER OUTPUTS: The handle to the queue manager (Hconn), the 
//	reason code and completion coede returned from  MQCONN().
//
//  FUNCTIONS USED: MQCONN() from the MQSeries function interface 
//                  that connects to the queue manager.
//
//////////////////////////////////////////////////////

DSOIdistribute::DSOIdistribute (const PMQCHAR QMgrName)
{
	//memcpy(QueueManager,QMgrName,(unsigned)MQ_Q_MGR_NAME_LENGTH );
	strncpy(QueueManager,QMgrName,(unsigned)MQ_Q_MGR_NAME_LENGTH );
	QueueManager[47] = '\0';
        sopToSend = new char[48];
        replyFromSop = new char[48];
        memset(_errorMessage,0,DSOI_STRING_1024);

}

////////////////////////////////////////////////
//  FUNCTION:  ~DSOIdistribute
//
//  PURPOSE: The destructor for DSOIdistribute
//
//  METHOD: This destructor is called when the DSOIdistribute class 
//          object goes out of scope.  It calls the MQ Series 
//          MQDISC function to disconnect the queue from the 
//          queue manager.
//
//  PARAMETERS: None
//  
//  OTHER OUTPUTS: Throws an exception with an error message when
//	the MQDISC() fails
//
//  FUNCTIONS USED: MQDISC - the MQ Series disconnect function
//	from the queue manager.
//
//////////////////////////////////////////////////////

DSOIdistribute::~DSOIdistribute()
{
#ifdef DEBUG
cout << " in destructor of DSOIdistribute "<< endl;
#endif
	//delete [] sopToSend;
	//delete replyFromSop;

}

////////////////////////////////////////////////
//  FUNCTION:
//
//  PURPOSE:
//
//  METHOD: outputs the contents of the DSOIdistribute object
//
//  PARAMETERS:
//  
//  OTHER OUTPUTS:
//  
//  RETURNS:
// 
//  OTHER OUTPUTS:
//
//  KNOWN USERS:
//
//  FUNCTIONS USED:
//
//  KNOWN DEFECTS:
//
//  REVISION HISTORY:
//
//////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const DSOIdistribute& route)
{
	os << "Manager connection handle " << route.Hconn ;

	return os;
}


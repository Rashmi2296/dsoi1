//////////////////////////////////////////////////////
//  FILE:  DSOIroute.C
//
//  PURPOSE: Contains all the functions for the DSOIroute.
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

#include "DSOIroute.H"
#include "DSOIThrowError.H"
#include "DSOItoolbox.H"
#include <cctype>
#include <cstdio>   
#include <cstddef>
#include <ctime>
#include <new>
//#include <sys/stat.h>
//#include <unistd.h>

char * DSOIroute::replyFromSop = 0;
MQHCONN DSOIroute::Hconn = 0;

////////////////////////////////////////////////
//  FUNCTION: DSOIroute
//
//  PURPOSE: The constructor for DSOIreoutMQ class where the queue
//	     manager is defined as the default queue manager. If 
//	     the queue manager is defined as the QMgrName.
//
//  METHOD: An instance of the DSOIroute class is created and the 
//          QueueManager member is initialized to the QMgrName.
//
//  PARAMETERS: const PMQCHAR QMgrName - the name of the queue manager and
//              the PMQCHAR is a pointer to a char (char*)
//
//  OTHER INPUTS: When the connectFlag is set then it has already been
//                connected to the queue manager and does not need to
//                connect to it again.
//  
//  OTHER OUTPUTS: The handle to the queue manager (Hconn), the 
//	reason code and completion coede returned from  MQCONN().
//
//  FUNCTIONS USED: MQCONN() from the MQSeries function interface 
//                  that connects to the queue manager.
//
//////////////////////////////////////////////////////

DSOIroute::DSOIroute (const PMQCHAR QMgrName)
{
	//memcpy(QueueManager,QMgrName,(unsigned)MQ_Q_MGR_NAME_LENGTH );
	strncpy(QueueManager,QMgrName,(unsigned)MQ_Q_MGR_NAME_LENGTH );
	QueueManager[47] = '\0';
        replyFromSop = new char[48];
        memset(_errorMessage,0,DSOI_STRING_1024);

}

////////////////////////////////////////////////
//  FUNCTION:  ~DSOIroute
//
//  PURPOSE: The destructor for DSOIroute
//
//  METHOD: This destructor is called when the DSOIroute class 
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

DSOIroute::~DSOIroute()
{
#ifdef DEBUG
cout << " in destructor of DSOIroute "<< endl;
#endif

}

////////////////////////////////////////////////
//  FUNCTION:
//
//  PURPOSE:
//
//  METHOD: outputs the contents of the DSOIroute object
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
ostream& operator<<(ostream& os, const DSOIroute& route)
{
	os << "Manager connection handle " << route.Hconn ;

	return os;
}


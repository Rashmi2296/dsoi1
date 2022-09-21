//////////////////////////////////////////////////////
//  FILE:  DSOIconnectMQ.C
//
//  PURPOSE: Contains all the functions for the DSOIconnectMQ.
//  
//  REVISION HISTORY:
//-----------------------------------------------------------------
//  version	date		author		description
//-----------------------------------------------------------------
//  7.00.00	12/23/99	rfuruta		initial development
//  8.08.00     01/02/03        rfuruta         CR8000, upgrade to HP-UX 11.0
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOIconnectMQ.H"
#include "DSOIThrowError.H"
#include "DSOItoolbox.H"
#include <cstdio>   
#include <ctime>
#include <cctype>
#include <cstddef>
//#include <unistd.h>
//#include <sys/stat.h>


int DSOIconnectMQ::connectFlag = 0;
int DSOIconnectMQ::connections = 0;
//MQCHAR48 replyToQ[] = {'n','o','n','e'};
//MQCHAR48 replyToQMgr[] = {'n','o','n','e'};
MQCHAR48 DSOIconnectMQ::replyToQ = "none";
MQCHAR48 DSOIconnectMQ::replyToQMgr = "none";
MQHCONN DSOIconnectMQ::Hconn = 0;


////////////////////////////////////////////////
//  FUNCTION: DSOIconnectMQ
//
//  PURPOSE: The constructor for DSOIreoutMQ class where the queue
//	     manager is defined as the default queue manager. If 
//	     the queue manager is defined as the QMgrName.
//
//  METHOD: An instance of the DSOIconnectMQ class is created and the 
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

DSOIconnectMQ::DSOIconnectMQ (const PMQCHAR QMgrName)
{
	//memcpy(QueueManager,QMgrName,(unsigned)MQ_Q_MGR_NAME_LENGTH );
	strncpy(QueueManager,QMgrName,(unsigned)MQ_Q_MGR_NAME_LENGTH );
	QueueManager[47] = '\0';

#ifdef DEBUG
cout << " in constructor for DSOIconnectMQ " << endl;
cout << " connectFlag= " << connectFlag << endl;
#endif
	if(!connectFlag) // connection to queue manager not complete
	{
#ifdef DEBUG
cout << "NO connectFlag" << endl;
#endif
		memset(_errorMessage,0,DSOI_STRING_1024);
		MQCONN(QMgrName, 
			&Hconn, 
			&errorObj.completionCode, 
			&errorObj.reasonCode);
		if(errorObj.completionCode == MQCC_FAILED)
		{
                        // 09/29/99  error message will be caught by ThrowError and
                        //           will be appended to the DSOI_alarm.log file

			errorObj.formatErrMsg((char *) "DSOIMQ:4005:CRITICAL:Constructor for DSOIconnectMQ MQCONN() failed for QueueManager",
					QMgrName, errorObj.reasonCode,
					(char *) "DSOIconnectMQ", (char *) __FILE__,__LINE__) ;
			sprintf(_errorMessage,"%s%s%s","DSOIMQ:4005:CRITICAL:Constructor for DSOIconnectMQ MQCONN() failed for QueueManager <",QMgrName,">"); 
			// errorObj.createReplyErrMsg(_errorMessage);
			connectFlag=0;

			throw ThrowError(DSOIerrorHandler::_formatErrMsg,errorObj.reasonCode);
		}
		connectFlag=1;
	}
	connections++;
#ifdef DEBUG
cout << "connections = " << connections << endl;
#endif

}

////////////////////////////////////////////////
//  FUNCTION:  ~DSOIconnectMQ
//
//  PURPOSE: The destructor for DSOIconnectMQ
//
//  METHOD: This destructor is called when the DSOIconnectMQ class 
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

DSOIconnectMQ::~DSOIconnectMQ()
{
#ifdef DEBUG
cout << " in destructor of DSOIconnectMQ "<< endl;
#endif
	connections--;
#ifdef DEBUG
cout << " in destructor of DSOIconnectMQ connections = "<<  connections << endl;
#endif
	// don't disconnect from the queue manager until all queues
	// connected to the queue manager have closed
	if(connections==0)
	{
#ifdef DEBUG
cout << "connections equal zero " << endl;
#endif
		MQDISC(&Hconn,
			&errorObj.completionCode,
			&errorObj.reasonCode);
		if(errorObj.completionCode == MQCC_FAILED)
		{
			errorObj.formatErrMsg((char *) "DSOIMQ:Destructor  for DSOIconnectMQ class MQCONN failed ",(char *) "",errorObj.reasonCode,(char *) "~DSOIconnectMQ", (char *) __FILE__, __LINE__ );
			sprintf(_errorMessage,"%s%04d%s","DSOIMQ:Destructor  for DSOIconnectMQ class MQCONN failed <",errorObj.reasonCode,">");
			errorObj.createReplyErrMsg(_errorMessage);
			connectFlag=0;
			throw ThrowError(DSOIerrorHandler::ReplyErrMsg,errorObj.reasonCode);
		}
		connectFlag=0;
	}

}

////////////////////////////////////////////////
//  FUNCTION:
//
//  PURPOSE:
//
//  METHOD: outputs the contents of the DSOIconnectMQ object
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
ostream& operator<<(ostream& os, const DSOIconnectMQ& route)
{
	os << "Manager connection handle " << route.Hconn ;

	return os;
}


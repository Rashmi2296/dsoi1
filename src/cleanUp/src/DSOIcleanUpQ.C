//////////////////////////////////////////////////////////////////////
//  FILE: DSOIcleanUpQ.C
//
//  PURPOSE: The main function to delete old messages
//           from sourceQ.
//
//  REVISION HISTORY:
//--------------------------------------------------------
//  version     date            author  description
//  1.00.00     08/20/00        rfuruta CR3452, initial development
//  7.01.01     09/14/00        rfuruta Upgrade to aCC compiler
//  8.08.00     01/02/03        rfuruta CR8000, upgrade to HP-UX 11.0
//  9.04.00     02/11/04        rfuruta CR 8911, set Expiry time
//  9.08.00     01/03/05        rfuruta CR 9745, set Priority
// 10.04.00     02/01/07        rfuruta CR10999, migrate DSOI to Solaris
//
//              CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of
//  U S West and Its Affiliates Having A Need To Know
//////////////////////////////////////////////////////////////////////
#include <iostream.h>
#include <cerrno>
#include <new>
#include "DSOIThrowError.H"
#include "DSOIThrowSendError.H"
#include "DSOIsendMQ.H"
#include "DSOIreceiveMQ.H"
#include "DSOIlogMQ.H"
#include "DSOItoolbox.H"

// static char _putTime[DSOI_STRING_15];
// static char _getTime[DSOI_STRING_15];
// static char _putApplDate[DSOI_STRING_9];
// static char _putApplName[DSOI_STRING_29];
// static char _putApplTime[DSOI_STRING_9];
// static char _Date[DSOI_STRING_9];
// static char _Time[DSOI_STRING_9];

 static char _tmpTime[DSOI_STRING_15];
 static char _tmpApplDate[DSOI_STRING_9];
 static char _tmpApplName[DSOI_STRING_29];
 static char _tmpApplTime[DSOI_STRING_9];

 unsigned char _msgId[DSOI_STRING_25];
 static MQBYTE24 _correlId;

 int purgeDate;
 int julianDate;
 int newJulianDate;
 int changeDays;
 int setExpiry=0;
 int setPriority=0;
 char newGregDate[DSOI_STRING_9];
 char julianYear[5];
 char newJulianYear[5];
 static char errMsg[100];

//////////////////////////////////////////////////////////////////////
//  FUNCTION: int main ( int argc, char* argv[])
//
//  PURPOSE: To process all messages on the queue
//	sent from a 'Requestor' application.
//
//  METHOD: Reads messages off of the queue using the
//	DSOI archive library getMSg()
//
//  PARAMETERS: int argc - the number of  command line parameters
//		char* argv[] - the command line parameters
//  
//  RETURNS: exits ONLY if there is an exception thrown.
// 
//  OTHER INPUTS: Requires a command line parameter as to
//	where the messages are received, the name of the 
//	MQSeries message queue.
//
//  FUNCTIONS USED: DSOIreceiveMQ::getMsg(),
//
//////////////////////////////////////////////////////////////////////
int main ( int argc, char* argv[])
{
	if(argc < 4 )	// executing with name of queue manager
	{
		cout << "Usage:  " << endl;
		cout << argv[0] ;
		cout << "  sourceQueue  " << " queueManager " <<  " purgeDate(YYYYMMDD)"  <<  endl; 
		exit(1);
	}
// cout << "TargetQ="   << argv[1] << endl;
// cout << "QmgrName="  << argv[2] << endl;
// cout << "CurrentDate=" << argv[3] << endl;
	register int returnCode =0;
	try{
                setExpiry=0;
                setPriority=0;
                DSOIsendMQ* sendErrorQ = new DSOIsendMQ(setExpiry,setPriority, (char *) "QL_DSOI_ERRORQ",argv[2]);
                
                //   0  opened for read   mode  
                //   1  opened for browse mode
		
                // Target Queue,  if date is less than purgeDate then remove msg
                DSOIreceiveMQ* browseQueue = new DSOIreceiveMQ(argv[1], argv[2], 1);
                
                // Use MsgId to remove msg from source queue
                DSOIreceiveMQ* removeQueue = new DSOIreceiveMQ(argv[1], argv[2], 0);
	
		removeQueue->setConvert(1);
		browseQueue->setConvert(1);
        	returnCode=0;

                memset(errMsg,0,100);
                returnCode=computeJulianDate(argv[3], &julianDate, julianYear, errMsg);
                if (returnCode != 0)
                  { cout <<  errMsg   << endl;
                    exit(1);
                  }
                // if (atoi(argv[4]) == 0)
	        if(argc == 4 )	
                    changeDays = -2;
                else
                    changeDays = atoi(argv[4]);

// cout << "OldJulianDate=" << julianDate      << endl;
// cout << "OldJulianYear=" << julianYear      << endl;
// cout << "   changeDays=" << changeDays      << endl;
                if ( changeDays < 0 || changeDays > 0)
                   { returnCode=computeChangeDate(&julianDate, julianYear, &changeDays,
                                             &newJulianDate,newJulianYear, errMsg);
                   }
                else
                   { newJulianDate = julianDate;
                     strcpy(newJulianYear,julianYear);
                   }

                if (returnCode != 0)
                  { cout <<  errMsg   << endl;
                    exit(1);
                  }
// cout << "NewJulianDate=" << newJulianDate   << endl;
// cout << "NewJulianYear=" << newJulianYear   << endl;

                memset(errMsg,0,100);
                returnCode=computeGregDate(&newJulianDate, newJulianYear, newGregDate, errMsg);
                if (returnCode == 0)
                  { purgeDate = atoi(newGregDate);
                  }
                else
                  { cout <<  errMsg   << endl;
                    exit(1);
                  }
cout << "TargetQ="   << argv[1] << endl;
cout << "QmgrName="  << argv[2] << endl;
cout << "CurrentDate=" << argv[3]   << endl;
cout << "PurgeDate  =" << purgeDate << endl;
cout << "*************************************************** " <<  endl;

		while(returnCode != MQRC_NO_MSG_AVAILABLE)
		{
                        DSOIreceiveMQ::_getCorrel = 0;
                        memset(_getTime,0,14);
                        memset(_putApplName,0,28);
                        memset(_putApplDate,0,8);
                        memset(_putApplTime,0,6);
                        memset(_msgId,0,25);
                        memset(_correlId,0,24);

                        returnCode = browseQueue->getMsg(_putApplName,_putApplDate,_putApplTime, 
                                                         _getTime,_msgId,(MQBYTE24*)_correlId, sendErrorQ);

			if(returnCode > 0 )
			{
			      if (returnCode != MQRC_NO_MSG_AVAILABLE)
				{
				   cout << DSOIerrorHandler::_formatErrMsg << endl;
				   if (returnCode != MQRC_TRUNCATED_MSG_FAILED)
				     {
					  cout << "getMsg() failed because MQGET() failed: exiting for "<< returnCode << endl;
					  exit(1);
				     }//end of if(returnCode != MQRC_TRUNCATED_MSG_FAILED)
				} // end of if(returnCode != MQRC_NO_MSG_AVAILABLE)
			} //end of returnCode > 0 for getMsg()
			else //received a message
			{
                              if (atoi(_putApplDate) < purgeDate)
                                {  DSOIreceiveMQ::_getCorrel = 1;
                                   memset(_tmpTime,0,11);
                                   memset(_tmpApplName,0,28);
                                   memset(_tmpApplDate,0,8);
                                   memset(_tmpApplTime,0,6);

                                   returnCode = removeQueue->getMsg(_tmpApplName,_tmpApplDate,
                                                                    _tmpApplTime,_tmpTime,_msgId,
                                                                    (MQBYTE24*)_correlId, sendErrorQ);
cout << "ReplyToQmgr="  << browseQueue->replyToQMgr << endl;
cout << "ReplyToQueue=" << browseQueue->replyToQ    << endl;
cout << "putApplDate="  << _putApplDate             << endl;
cout << "putApplTime="  << _putApplTime             << endl;
cout << "Message="      << browseQueue->message     << endl;
cout << "*************************************************** " <<  endl;
                                }

			} //end of else there was no error
		} // end of while

		delete browseQueue;
		delete removeQueue;
                delete sendErrorQ;

	} // end of try

	catch ( ThrowError& err )
	{
		exit(1);
	}
	catch(...)
	{
		cerr << "EXCEPTION CAUGHT!" << endl;
		exit(1);
	}
	exit(0);
}


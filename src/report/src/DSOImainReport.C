///////////////////////////////////////////////////////
//  FILE:  DSOImainReport.C
//
// Description: This file contains the main() which drives the DSOIReport process.
//
//  REVISION HISTORY:
//---------------------------------------------------------
//  version     date            author          description
//---------------------------------------------------------
//  1.00.00     10/03/97        jalee3          initial release
//  1.00.01     10/09/97        jalee3          CR2034:M:set wait interval to zero
//  1.00.03     10/27/97        jalee3          CR2099:M:changed cerr to cout in catch block
//  1.00.04     02/26/98        jalee3          CR2328:M:Report Util not working correctly
//  1.00.05     03/16/98        lparks          CR2360:M:Added DSOIconstants.C and descriptive error message: DSOIReport
//  6.00.00     08/04/99        rfuruta         CR582, critical err msg written to stdout
//  6.00.00     08/04/99        rfuruta         CR590, critical err msg written to stdout
//  8.00.00     08/23/00        rfuruta         CR 5095, added blank line between entries
//  8.08.00     01/02/03        rfuruta         CR8000, upgrade to HP-UX 11.0
//  8.09.00     03/12/03        rfuruta         CR8010, set waitInterval time
//  8.09.00     03/22/03        rfuruta         CR8180, fix memory leaks
// 10.04.00     02/01/07        rfuruta 	CR10999, migrate DSOI to Solaris
//
//              CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <iostream.h>
#include <cstdlib>
#include <new>

#include "DSOIThrowError.H"
#include "DSOIconstants.H"
#include "DSOIerrorHandler.H"
#include "DSOIreportMQ.H"
#include "DSOIlogMQ.H"

int readParmFile(char*, int* , int*);

static const long WAIT_INTERVAL   = 60000; // millisecs = 1 minute

void checkArgs(int argc, const char* argv[])
{
	if (argc < 2)
	{
		cout << "\nUsage: " << argv[0]
		     << " <queue_manager>" << endl << endl;
		exit(1);
	}
}

int getQueueManagerIndex(int argc)
{
	// The command-line location of the queue manager name depends
	// on whether the executable was trigger by MQSeries or was run
	// manually. If triggered, a MQSeries string is in argv[1],
	// and the queue manager name is in argv[2]. If manual, the
	// queue manager is in argv[1].

	return (argc > 2) ? 2 : 1;
}

int main(int argc, const char* argv[])
{
        int waitIntervalTime = 0;
        int maxMsgsReadin = 0;
        int msgCnt = 0;

	const char* functionName = "main";

	// Check input arguments
	checkArgs(argc, argv);

	DSOIreportMQ *report;
	DSOIerrorHandler errHandler;

	try
	{
	        int queueManagerIndex = 0;
		// Get queue manager name depending on manual vs. triggered execution
		queueManagerIndex = getQueueManagerIndex(argc);

		if (report = new DSOIreportMQ(DSOIstrings::REPORT_Q,argv[queueManagerIndex]))
		{
                        int retnCd = readParmFile((char*) "/opt/DSOI/etc/ParmFile", 
                                                  &waitIntervalTime, &maxMsgsReadin);
                        if (retnCd == 0)
                          { report->setWaitInterval(waitIntervalTime);
                          }
                        else
                          { report->setWaitInterval(0);
                            maxMsgsReadin = 100;
                          }
                        while ((report->processMessages(waitIntervalTime)) &&
                               (msgCnt < maxMsgsReadin))
                             { msgCnt++;}

		}
		else // error allocating DSOIreportMQ object
		{
			char errorMessage[DSOI_STRING_256];
			strcpy(errorMessage,"Unable to allocate memory for DSOIreportMQ report");
			errHandler.formatErrMsg(errorMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
			throw ThrowError(errHandler._formatErrMsg,0);

		}

		if (report) delete report;
	}
	catch (ThrowError err)
	{
                // 09/29/99 append error message to DSOI_alarm.log file

                DSOIlogMQ* logQ = new DSOIlogMQ();
                try{
                     logQ->logCriticalError( errHandler._formatErrMsg );
                   }
                catch( ThrowError& err )
                   {
                     cout << "Error sending to log critical error!" << endl;
                     cout << "DSOImainReport:Error Message = " << errHandler._formatErrMsg << endl;
                   }
                delete logQ;

                exit(1);

	}

//	catch (bad_alloc) {
//		cout << "Operator new failed to allocate memory." << endl;
//		exit(1);
//	}

	catch (...)
	{
                // 09/29/99 append error message to DSOI_alarm.log file
                
                DSOIlogMQ* logQ = new DSOIlogMQ();
                try{
                     logQ->logCriticalError( errHandler._formatErrMsg );
                   }
                catch( ThrowError& err )
                   {
                     cout << "Error sending to log critical error!" << endl;
                     cout << "DSOImainReport:Error Message = " << errHandler._formatErrMsg << endl;
                   }
                delete logQ;
		
                cerr << "in catch(DSOImainReport...) " << endl;

	}

	return 0;
}

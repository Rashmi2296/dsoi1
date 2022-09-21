///////////////////////////////////////////////////////
//  FILE:  DSOImainLog.C
//
// Description: This file contains the main() which drives the DSOILog process.
//
//  REVISION HISTORY:
//---------------------------------------------------------
//  version     date            author          description
//---------------------------------------------------------
//  1.00.00     10/03/97        jalee3          initial release
//  1.00.01     10/10/97        jalee3          cleanup code
//  6.00.00     08/04/99        rfuruta         CR582, critical err msg written to stdout
//  6.00.00     08/04/99        rfuruta         CR590, critical err msg written to stdout
//  8.00.00     08/23/00        rfuruta         CR4767, remove 4P stuff
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
#include "DSOIlogMQ.H"

int readParmFile(char*, int* , int*);

static const long WAIT_INTERVAL = 60000; // millisecs = 1 minute

void checkArgs(int argc, const char* argv[])
{
	if (argc < 2)
	{
		cout << "\nUsage: " << argv[0]
		     << " <queue_manager>" << endl << endl;
		exit(1);
	}
}

int archiveLogFiles(int argc, const char* argv[])
{
	char* inArg;
	char  c;
	int   archiveFlag = FALSE;

	// Cycle thru all the arguments
	for (int i=0; i<argc; i++)
	{
		// Process "-" command-line options
		if ((inArg = strstr((char*)argv[i],"-")) != NULL)
		{
			// Check each command-line option
			while (c = (char) *++inArg)
			{
				switch(c)
				{
					case('a'):
						archiveFlag = TRUE;
						break;

					default:
						break;
				}
				if (archiveFlag == TRUE)
					break;
			}
		}
	}

	return archiveFlag;
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

	DSOIlogMQ *logger;
	DSOIerrorHandler errHandler;
	int queueManagerIndex = 0;
	int returnCode = 0;

	try
	{
		// Check if archiving is requested
		if (archiveLogFiles(argc, argv) == TRUE)
		{
			if (logger = new DSOIlogMQ())
				logger->archiveLogFiles();
			else // error allocating DSOIlogMQ object
			{
				errHandler.formatErrMsg(
					(char *) "Unable to allocate memory for DSOIlogMQ logger",(char *) "", 0, (char *) functionName, (char *) __FILE__, __LINE__);
				throw ThrowError(errHandler._formatErrMsg,0);
			}
		}
		else
		{
			// Get queue manager name depending on manual vs. triggered execution
			queueManagerIndex = getQueueManagerIndex(argc);
	
			if (logger = new DSOIlogMQ(DSOIstrings::ERROR_Q,argv[queueManagerIndex]))
			{
                                int retnCd = readParmFile((char*) "/opt/DSOI/etc/ParmFile", 
                                                               &waitIntervalTime, &maxMsgsReadin);
                                if (retnCd == 0)
				  { logger->setWaitInterval(waitIntervalTime);
                                  }
                                else
				  { logger->setWaitInterval(0);
                                    maxMsgsReadin = 100;
                                  }
				while ((returnCode = logger->processMessages(waitIntervalTime)) &&
                                       (msgCnt < maxMsgsReadin))
                                     { msgCnt++;}
			}
			else // error allocating DSOIlogMQ object
			{
				errHandler.formatErrMsg(
					(char *) "Unable to allocate memory for DSOIlogMQ logger", (char *) "", 0, (char *) functionName, (char *) __FILE__, __LINE__);
				throw ThrowError(errHandler._formatErrMsg,0);
			}
		}
		if (logger)
			delete logger;
	}
	catch (ThrowError err)
	{
                // 09/29/99 append error message to DSOI_alarm.log file

                DSOIlogMQ* logQ = new DSOIlogMQ();
                try{
                     // logQ->logCriticalError(strcat("DSOILOG:", errHandler._formatErrMsg));
                     logQ->logCriticalError( errHandler._formatErrMsg );
                   }
                catch( ThrowError& err )
                   {
                     cout << "Error sending to critical error log!" << endl;
                     cout << "DSOImainLog:Error Message = " << errHandler._formatErrMsg << endl;
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
                DSOIlogMQ* logQ = new DSOIlogMQ();
                try{
                     // logQ->logCriticalError(strcat("DSOILOG:", errHandler._formatErrMsg));
                     logQ->logCriticalError( errHandler._formatErrMsg );
                   }
                catch( ThrowError& err )
                   {
                     cout << "Error sending to log critical error!" << endl;
                     cout << "DSOImainLog:Error Message = " << errHandler._formatErrMsg << endl;
                   }
                delete logQ;

                cerr << "in catch(DSOImainLog...) " << endl;
	}

	return 0;
}

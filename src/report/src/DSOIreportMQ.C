///////////////////////////////////////////////////////
//  FILE:  DSOIreportMQ.C
//
// Description: See header file for description.
//
//  REVISION HISTORY:
//---------------------------------------------------------
//  version     date            author          description
//---------------------------------------------------------
//  1.00.00     10/03/97        jalee3          initial release
//  1.00.02     10/03/97        jalee3          CR1998 cast MQLONG parameter passed to write()
//                                              to unsigned int 
//  1.00.03     10/09/97        jalee3          CR2034 mod logMessage(): replace "xff" with ":"
//                                              in report messages
//  1.00.04     10/27/97        jalee3          CR2099 changed logMessage() - using buffered
//                                              open/write/close;
//                                              directory "report/" created with mode 777
//  1.00.04     02/26/98        lparks          CR2328 Report Util not working  correctly
//  6.00.00     08/04/99        rfuruta         CR582, critical err msg written to stdout
//  6.00.00     08/04/99        rfuruta         CR590, critical err msg written to stdout
//  7.00.00     12/23/99        rfuruta         CR____, 
//  8.00.00     08/23/00        rfuruta         CR 5095, added blank line between entries
//  8.08.00     01/02/03        rfuruta         CR8000, upgrade to HP-UX 11.0
//  8.09.00     03/12/03        rfuruta         CR8010, set waitInterval time
//  8.09.00     03/22/03        rfuruta         CR8180, fix memory leaks
//  9.04.00     02/11/04        rfuruta         CR8911, set Expiry time
//  9.08.00     01/03/05        rfuruta         CR9745, set Priority
// 10.04.00     02/01/07        rfuruta 	CR10999, migrate DSOI to Solaris
//
//              CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOIlogMQ.H"
#include "DSOIreportMQ.H"
#include <cstdlib>
#include <new>
#include <sys/stat.h>	// stat()
			// not sure where this file is
//#include <fcntl.h>
//#include <dirent.h>
//#include <sys/types.h>
//#include <unistd.h>

static const char* XFF_DELIMITER="\xff";
static char _getTime[DSOI_STRING_15];
static char _putApplName[DSOI_STRING_29];
static char _putApplDate[DSOI_STRING_9];
static char _putApplTime[DSOI_STRING_9];
static const int _sizeOfTime = 14;
static unsigned char _msgId[DSOI_STRING_25];
static MQBYTE24 _correlId;
int setExpryR = 0;
int setPriortyR = 0;

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOIreportMQ (const char* queueName, const char* queueManagerName)
//
//  PURPOSE: This constructor for DSOIreport is used when report messages are to
//           be retrieved from the MQSeries report queue and are to be processed.
//
//  METHOD: The DSOIreceiveMQ and DSOIsendMQ objects are allocated memory and
//          data members are initialized. The report queue name and the queue
//          manager name are passed down to the DSOIreceiveMQ and DSOIsendMQ
//          constructors.
//
//  PARAMETERS: const char* queueName - name of the report queue
//              const char* queueManagerName - name of the queue manager
//                                             overseeing the report queue
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, delete, sprintf, strcat, throw, initClassMembers,
//     readConfigFile
//
////////////////////////////////////////////////////////////////////////////////
DSOIreportMQ::DSOIreportMQ (const char* queueName, const char* queueManagerName) :
	LOG_DIRECTORY_SIZE_128  (DSOI_STRING_128),
	LOG_FILE_SIZE_64        (DSOI_STRING_64)
{
	const char* functionName = "DSOIreportMQ";
	int errorFlag = FALSE;
	char alarmText[DSOI_STRING_256];
	memset(alarmText,0,sizeof(alarmText));

	// Instantiate DSOIreceiveMQ object
	if (!(_receive = new DSOIreceiveMQ((char*) queueName,
	                                   (char*) queueManagerName)))
	{
		sprintf(alarmText,"%s%s%s",
			"DSOIReport:Memory allocation of class member _receive failed",
			" because new() or DSOIreceiveMQ construction failed",
			": DSOIReport process may terminate");
		errorFlag = TRUE;
	}

        setExpryR = 0;
        setPriortyR = 0;
	if (!(_errorSendQ = new DSOIsendMQ(setExpryR, setPriortyR, (char*) DSOIstrings::ERROR_Q,
	                                   (char*) queueManagerName)) &&
	    (errorFlag==FALSE))
	{
		sprintf(alarmText,"%s%s%s",
			"DSOIReport: Memory allocation of class member _errorSendQ failed",
			" because new() or DSOIsendMQ construction failed",
			": DSOIReport process may terminate");
		errorFlag = TRUE;
	}

	if (errorFlag)
	{
		// Write this error to standard error & throw
		char errorMessage[DSOI_STRING_512];
		strcpy(errorMessage,alarmText);
		strcat(errorMessage,":logDate=");
		strcat(errorMessage,getCurrentDate());
		strcat(errorMessage,":logTime=");
		strcat(errorMessage,getCurrentTime());

		_errorHandler.formatErrMsg(errorMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
	}

	// Initialize class members to process messages on the error queue
	initClassMembers();

	// Read configuration parameters
	readConfigFile();

	// Set umask : set file access to "-rw-rw-r--"
	umask(S_IWOTH);

	// Create directories
	createDirectories();
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOIreportMQ (const DSOIreportMQ &reportInput)
//
//  PURPOSE: This copy constructor for DSOIreportMQ explicitly copies the
//     individual class members.
//
//  METHOD:  Use the overloaded assignment operator to copy the DSOIreportMQ
//     input object.
//
//  PARAMETERS: const DSOIreportMQ& reportInput - input DSOIreportMQ object
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: operator=
//
////////////////////////////////////////////////////////////////////////////////
DSOIreportMQ::DSOIreportMQ(const DSOIreportMQ &reportInput) :
	LOG_DIRECTORY_SIZE_128  (DSOI_STRING_128),
	LOG_FILE_SIZE_64        (DSOI_STRING_64)
{
	*this = reportInput;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: ~DSOIreportMQ ()
//
//  PURPOSE: This destructor removes allocated memory.
//
//  METHOD:  Delete memory if non-NULL.
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: delete
//
////////////////////////////////////////////////////////////////////////////////
DSOIreportMQ::~DSOIreportMQ ()
{
	delete _receive;
	delete _errorSendQ;

	delete [] _logDirectory;
	delete [] _logFile;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: buildFullFileName () const
//
//  PURPOSE: This method
//
//  METHOD:
//
//  PARAMETERS: None
//
//  RETURNS: const char* - filename
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED:
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIreportMQ::buildFullFileName () const
{
	static char fileName[DSOI_STRING_512];
	memset(fileName,0,DSOI_STRING_512);

	strncpy(fileName,_logDirectory,LOG_DIRECTORY_SIZE_128);
	strcat(fileName,"/");
	strncat(fileName,_logFile,LOG_FILE_SIZE_64);
	strcat(fileName,".");
	strcat(fileName,getCurrentDate("%Y%m%d"));

	return (const char*) fileName;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: createDirectories ()
//
//  PURPOSE: This method creates the directories in the relative path to the
//     report files.
//
//  METHOD: Creates directories if nonexistant.
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: stat, makeDirectory
//
////////////////////////////////////////////////////////////////////////////////
void DSOIreportMQ::createDirectories ()
{
	struct stat buf;

	// Create report directory
	stat(_logDirectory,&buf);
	if (!S_ISDIR(buf.st_mode))
		makeDirectory(_logDirectory);
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getCurrentDate (const char* format) const
//
//  PURPOSE: This method determines the current date.
//
//  METHOD: Determines the local date with the format provided or using a
//     default format.
//
//  PARAMETERS: const char* - requested format, if none given defaults to "%m%d%Y"
//
//  RETURNS: const char* - date string in requested or default format ("%m%d%Y")
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: time, strlen, strftime, localtime
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIreportMQ::getCurrentDate(const char* format) const
{
	// Get date/time stamp
	const int dateSize = 11;
	static char currentDate[DSOI_STRING_16];
	memset(currentDate, 0, sizeof(currentDate));
	time_t timeNow;
	time (&timeNow);
	if (strlen(format)>0)
		strftime (currentDate,dateSize,format,localtime(&timeNow));
	else
		strftime (currentDate,dateSize,"%m%d%Y",localtime(&timeNow));
	return (const char*) currentDate;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getCurrentTime (const char* format) const
//
//  PURPOSE: This method determines the current time.
//
//  METHOD: Determines local time.
//
//  PARAMETERS: const char* - requested format, defaults to ""
//
//  RETURNS: const char* - time string in requested or default format ("%H%M%S")
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: time, strftime, strlen, localtime
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIreportMQ::getCurrentTime(const char* format) const
{
	// Get date/time stamp
	const int timeSize = 7;
	static char currentTime[DSOI_STRING_16];
	memset(currentTime, 0, sizeof(currentTime));
	time_t timeNow;
	time (&timeNow);
	if (strlen(format)>0)
		strftime (currentTime,timeSize,format,localtime(&timeNow));
	else
		strftime (currentTime,timeSize,"%H%M%S",localtime(&timeNow));
	return (const char*) currentTime;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getMessage ( int )
//
//  PURPOSE: This method retrieves a report message from the report queue.
//
//  METHOD: Calls the DSOIreceiveMQ object's getMsg() function which retrieves
//     the message from the report queue. The message is stored as a member in
//     the DSOIreceiveMQ object. If getMsg() is unable to retrieve a message
//     from the error queue, this method writes a critical message directly
//     to the alarm file. If unable to write to the alarm file, the catch throws
//     again with the critical message stored in the error handler object.
//
//  PARAMETERS: None
//
//  RETURNS: char* - pointer to the message buffer
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: DSOIreceiveMQ::getMsg, sprintf, strcat, throw
//
////////////////////////////////////////////////////////////////////////////////
char* DSOIreportMQ::getMessage ( int waitIntervalTime )
{
	const char* functionName = "getMessage";
	static const char* alarmText = "DSOIReport:3001:CRITICAL:Unable to get error messages from the REPORT_Q because DSOIreceiveMQ::getMsg() did not returned MQCC_OK or MQRC_NO_MSG_AVAILABLE:DSOIReport process terminating";
	char  criticalMessage[DSOI_STRING_512], addnInfo[DSOI_STRING_256];
	char* message = NULL;
	int   returnCode=0;

	try
	{
		// Set wait interval on the report queue
		// _receive->setWaitInterval(_waitInterval);
		_receive->setWaitInterval(waitIntervalTime);
                memset(_getTime,0,_sizeOfTime);
                memset(_putApplName,0,28);
                memset(_putApplDate,0,8);
                memset(_putApplTime,0,6);
                memset(_msgId,0,25);
                memset(_correlId,0,24);
	
		if (((returnCode=_receive->getMsg(_putApplName,_putApplDate,
                     _putApplTime,_getTime,_msgId,(MQBYTE24*)_correlId, _errorSendQ)) != MQCC_OK) &&
		    (returnCode != MQRC_NO_MSG_AVAILABLE))
		{
			// Create critical message for alarm file
			sprintf(addnInfo,":%s%s:%s%s:%s%s",
				"function=",functionName,"file=",__FILE__,"line=",__LINE__);
			strcat(addnInfo,":logDate=");
			strcat(addnInfo,getCurrentDate());
			strcat(addnInfo,":logTime=");
			strcat(addnInfo,getCurrentTime());
			sprintf(criticalMessage,"%s%s",alarmText,addnInfo);

			// Instantiate DSOIlogMQ object and then write directly to alarm file
			DSOIlogMQ logger;
			logger.logCriticalError(criticalMessage);
		}
		else if (returnCode == MQRC_NO_MSG_AVAILABLE)
			message = NULL;
		else
			message = _receive->message;
	}
	catch (ThrowError err)
	{
		err.printMessage();
		strcpy(addnInfo,"The following critical message could not be written to the alarm file:");
		sprintf(criticalMessage,"%s \"%s\"",addnInfo,alarmText);
		_errorHandler.formatErrMsg(criticalMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
	}

//	catch (bad_alloc) {
//		cout << "Operator new failed to allocate memory." << endl;
//		exit(1);
//	}

	catch (...)
	{
                DSOIlogMQ* logQ = new DSOIlogMQ();
                try{
                     logQ->logCriticalError(criticalMessage);
                   }
                catch( ThrowError& err )
                   {
                     cout << "Error sending to log critical error!" << endl;
                     cout << "Error Message = " << criticalMessage << endl;
                   }
                delete logQ;

		cout << "in catch(...) of DSOIreportMQ::getMessage()" << endl;
	}

	return message;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: initClassMembers ()
//
//  PURPOSE: Allocates and initializes class members used in the processing
//    of messages from the report queue.
//
//  METHOD:  Sets class members to zero.
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, memset
//
////////////////////////////////////////////////////////////////////////////////
void DSOIreportMQ::initClassMembers ()
{
	// Allocate memory
	_logDirectory    = new char[LOG_DIRECTORY_SIZE_128];
	_logFile         = new char[LOG_FILE_SIZE_64];

	// Initialize memory
	memset(_logDirectory,0,LOG_DIRECTORY_SIZE_128);
	memset(_logFile,0,LOG_FILE_SIZE_64);

	_waitInterval = 0;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: logMessage (const char* message)
//
//  PURPOSE: This method logs report message to report flat file.
//
//  METHOD: 
//
//  PARAMETERS: const char* - report message
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: 
//
////////////////////////////////////////////////////////////////////////////////
void DSOIreportMQ::logMessage (const char* message)
{
	const char* functionName = "logMessage";
	const int margin = 64;
	const int reportFileSize = LOG_DIRECTORY_SIZE_128 + LOG_FILE_SIZE_64 + margin;
	FILE* fp;
	int   errorFlag,messageLength=0;
	char* reportFile = new char[reportFileSize];
	char errorMessage[DSOI_STRING_512+DSOI_STRING_512];
	errorFlag=FALSE;

	// Build report file string
	memset(errorMessage,0,DSOI_STRING_512+DSOI_STRING_512);
	memset(reportFile,0,sizeof(reportFile));
	strncpy(reportFile,buildFullFileName(),reportFileSize);

	// Log to report file
	if ((fp=fopen(reportFile,"a"))!=NULL)
	{
		// Replace "xff" with "~" delimiter - easier for UNIX sort cmd to handle in DSOIReportUtil
		char* token = '\0';
		messageLength = strlen(message);
		char* messageBuffer = new char[messageLength+1];
		char* messageConverted = new char[messageLength+1];

		// Store message in temporary buffer
		strcpy(messageBuffer,message);

		// Tokenize message & replace "xff" with "~"
		token = strtok(messageBuffer,XFF_DELIMITER);
		strcpy(messageConverted,token);
		strcat(messageConverted,"~");
		while (token = strtok(NULL,XFF_DELIMITER))
		{
			strcat(messageConverted,token);
			strcat(messageConverted,"~");
		}

		// Write message to flat file
		fprintf(fp,messageConverted);
		fprintf(fp,"\n");
		fprintf(fp,"\n");

		// Clean-up
		delete [] messageBuffer;
		delete [] messageConverted;
	}
	else
	{
		// Write error to standard error, then throw
		sprintf(errorMessage,"%s \"%s\" %s:%s \"",
			"DSOIReport:Unable to open report file",_logFile,"for append",
			"The error message is");
		strncat(errorMessage,message,strlen(message));
		strcat(errorMessage,"\".");

		errorFlag=TRUE;
	}

	if (errorFlag == TRUE)
	{
		strcat(errorMessage,":logDate=");
		strcat(errorMessage,getCurrentDate());
		strcat(errorMessage,":logTime=");
		strcat(errorMessage,getCurrentTime());

		_errorHandler.formatErrMsg(errorMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
	}

	// Clean-up memory
	delete [] reportFile;

	fclose(fp);
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: makeDirectory (const char* inputDirectory)
//
//  PURPOSE: This method creates each level of the input directory path.
//
//  METHOD: Tokenizes the input directory path and creates each level with
//     permissions "drwxr-xr-x". Then returns TRUE or FALSE depending on
//     existance of full input directory path.
//
//  PARAMETERS: const char* - directory path
//
//  RETURNS: int - TRUE or FALSE
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, delete, memset, strncpy, strcat, strtok, mkdir
//
////////////////////////////////////////////////////////////////////////////////
int DSOIreportMQ::makeDirectory (const char* inputDirectory)
{
	struct stat buf;
	const char* LOCAL_DELIMITERS = "/\0\n";
	const char* token = "\0";
	int stringLength = strlen(inputDirectory);
	char newDirectory[DSOI_STRING_256];
	char subDirectory[DSOI_STRING_256];
	memset(newDirectory,0,DSOI_STRING_256);
	memset(subDirectory,0,DSOI_STRING_256);

	strncpy(newDirectory,inputDirectory,stringLength);
	token = strtok(newDirectory,LOCAL_DELIMITERS);

	do {
		strncat(subDirectory,token,strlen(token));
		mkdir(subDirectory,S_IRWXU | S_IRWXG | S_IRWXO);
		strcat(subDirectory,"/");
	} while (token = strtok(NULL,LOCAL_DELIMITERS));

	stat(inputDirectory,&buf);
	return (S_ISDIR(buf.st_mode)) ? TRUE : FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: processMessages ( int )
//
//  PURPOSE: To retrieve messages from the report queue, to parse the messages,
//           and to write the messages to a log file.
//
//  METHOD: This method first gets the message, parses it, and then logs it
//     into an application error file.
//
//  PARAMETERS: None
//
//  RETURNS: int - TRUE (success) or FALSE (failure)
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: getMessage, parseMessage, logMessage
//
////////////////////////////////////////////////////////////////////////////////
int DSOIreportMQ::processMessages (int waitIntervalTime)
{
	char* msg;
	int   returnCode;

	if ((msg = getMessage(waitIntervalTime)) != NULL)
	{
		// Log report message using parsed members
		logMessage((const char*) msg);

		returnCode = TRUE;
	}
	else
	{
		// either no messages or error
		returnCode = FALSE;
	}

	return returnCode;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: readConfigFile ()
//
//  PURPOSE: This method reads parameter values from a configuration file.
//
//  METHOD: After opening the configuration file in read-only mode, two
//     parameters are read:
//        - report file directory
//        - report filename
//     If the file cannot be opened or if any of the parameters could not
//     be read correctly, default values will be used and an error message
//     written to standard error.
//          
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: default string constants from DSOIconstants
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: fopen, fclose, fscanf, strcmp, strncpy, sprintf
//
////////////////////////////////////////////////////////////////////////////////
void DSOIreportMQ::readConfigFile()
{
	FILE* fp;
	char  input[DSOI_STRING_128];
	char  tmp[DSOI_STRING_128];
	char  warningMessage[DSOI_STRING_256];
	int   warningFlag = FALSE;

        if ((fp = fopen(DSOIstrings::REPORT_CONFIG_FILE,"r")) != NULL)
	{
		// Read relative path to report file
		fscanf(fp,"%s %s %s %s",tmp,tmp,tmp,input);
		if (strcmp(input,"")!=NULL)
			strncpy(_logDirectory,input,LOG_DIRECTORY_SIZE_128);
		else
		{
			// Use default value
			strncpy(_logDirectory,DSOIstrings::DEFAULT_REPORT_DIRECTORY,
				LOG_DIRECTORY_SIZE_128);
			warningFlag = TRUE;
		}

		// Read alarm filename
		fscanf(fp,"%s %s %s",tmp,tmp,input);
		if (strcmp(input,"")!=NULL)
			strncpy(_logFile,input,LOG_FILE_SIZE_64);
		else
		{
			// Use default value
			strncpy(_logFile,DSOIstrings::DEFAULT_REPORT_FILE,
				LOG_FILE_SIZE_64);
			warningFlag = TRUE;
		}

		// Close config file before exiting
		fclose(fp);

		if (warningFlag)
		{
			// Write warning message to standard error
			sprintf(warningMessage,"%s \"%s\" %s:%s%s:%s%s:%s",
				"Some parameters were not read correctly from the configuration file=",
				DSOIstrings::REPORT_CONFIG_FILE,"and so default values were used; values used",
				"log directory=",_logDirectory,
				"base filename=",_logFile,
				"Please check these values to ensure DSOIReport is processing properly");
			strcat(warningMessage,":logDate=");
			strcat(warningMessage,getCurrentDate());
			strcat(warningMessage,":logTime=");
			strcat(warningMessage,getCurrentTime());

			cerr << warningMessage << endl;
		}
	}
	else // uable to open DSOIReport configuration file
	{
		// Use default values
		strncpy(_logDirectory,DSOIstrings::DEFAULT_REPORT_DIRECTORY,
			LOG_DIRECTORY_SIZE_128);
		strncpy(_logFile,DSOIstrings::DEFAULT_REPORT_FILE,LOG_FILE_SIZE_64);

		// Make note of this to standard error
                // Commenting the below sprintf as it causing the CORE.
		// This is a temporary solution ONLY.
		/*sprintf(warningMessage,"%s \"%s\":%s:%s%s:%s%s:%s%s:%s%s",
			"DSOIReport:Unable to access file=",DSOIstrings::REPORT_CONFIG_FILE,
			"DSOIReport using default values",
			"base filename=",DSOIstrings::DEFAULT_REPORT_FILE,
			"log directory=",DSOIstrings::DEFAULT_REPORT_DIRECTORY);*/
		strcpy(warningMessage,":logDate=");
		strcat(warningMessage,getCurrentDate());
		strcat(warningMessage,":logTime=");
		strcat(warningMessage,getCurrentTime());

		cerr << warningMessage << endl;
	}
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: operator= (const DSOIreportMQ& reportInput)
//
//  PURPOSE: This method assigns the class members of the DSOIreportMQ
//     input object to this object.
//
//  METHOD:
//
//  PARAMETERS: const DSOIreportMQ& reportInput - input DSOIreportMQ object
//
//  RETURNS: DSOIreportMQ& - reference to this object
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, strcmp, strcpy, initClassMembersFull,
//     DSOIreceiveMQ::operator=, DSOIsendMQ::operator=,
//     getReceive, getReceive, getErrorSendQ,
//     getCriticalDirectory, getCriticalLogFile,
//     getNoncriticalDirectory, getNoncriticalLogFile,
//     getProcess, getAlarmNumber, getSeverity, getAlarmText, getAddnInfo
//
////////////////////////////////////////////////////////////////////////////////
DSOIreportMQ& DSOIreportMQ::operator=(const DSOIreportMQ& reportInput)
// jal add copy constructors to class objects
//  : _receive(reportInput.getReceive()), _errorSendQ(reportInput.getErrorSendQ()),
//    _errorHandler = reportInput.getErrorHandler();
{
	// Initialize class members to process messages on the error queue
	initClassMembers();

	// Copy class member strings
	strncpy(_logDirectory,reportInput.getLogDirectory(),LOG_DIRECTORY_SIZE_128);
	strncpy(_logFile,reportInput.getLogFile(),LOG_FILE_SIZE_64);

    return *this;
}

// end

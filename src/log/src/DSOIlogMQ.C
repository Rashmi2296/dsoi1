///////////////////////////////////////////////////////
//  FILE:  DSOIlogMQ.C
//
// Description: See header file for description.
//
//  REVISION HISTORY:
//---------------------------------------------------------
//  version     date            author          description
//---------------------------------------------------------
//  1.00.00     10/03/97        jalee3          initial release
//  1.00.01     10/10/97        jalee3          cleanup code
//  6.00.00     08/04/99        rfuruta         CR582, critical err msg written to stdout
//  6.00.00     08/04/99        rfuruta         CR590, critical err msg written to stdout
//  6.00.05     10/14/99        rfuruta         CR1782,each alarm msg should be discrete
//  7.00.00     12/23/99        rfuruta         CR3123, changed DSOIrouteMQ to DSOIdistribute
//  8.00.00     10/03/00        rfuruta         CR4767, remove 4P stuff
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
#include <cstdlib>
#include <new>
#include <sys/stat.h>	// stat()
			// don't know where this file is
//#include <sys/types.h>
//#include <dirent.h>
//#include <unistd.h>

static const char* DELIMITER=":";
static char _getTime[DSOI_STRING_15];
static char _putApplName[DSOI_STRING_29];
static char _putApplDate[DSOI_STRING_9];
static char _putApplTime[DSOI_STRING_9];
static const int _sizeOfTime = 14;
static unsigned char _msgId[DSOI_STRING_25];
static MQBYTE24 _correlId;
int setExpryL = 0;
int setPriortyL = 0;

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOIlogMQ (const char* queueName,
//                       const char* queueManagerName)
//
//  PURPOSE: This constructor for DSOIlogMQ is used when error messages are to
//           be retrieved from the MQSeries error queue and are to be processed.
//
//  METHOD: The DSOIreceiveMQ and DSOIsendMQ objects are allocated memory and
//          data members are initialized. The error queue name and the queue
//          manager name are passed down to the DSOIreceiveMQ and DSOIsendMQ
//          constructors.
//
//  PARAMETERS: const char* queueName - name of the error queue
//              const char* queueManagerName - name of the queue manager
//                                             overseeing the error queue
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, delete, sprintf, strcat, throw, initClassMembersFull,
//     readConfigFile
//
////////////////////////////////////////////////////////////////////////////////
DSOIlogMQ::DSOIlogMQ (const char* queueName, const char* queueManagerName) :
	ADDN_INFO_SIZE_256   (DSOI_STRING_256),
	ALARM_NUMBER_SIZE_16 (DSOI_STRING_16),
	ALARM_TEXT_SIZE_256  (DSOI_STRING_256),
	DIRECTORY_SIZE_256   (DSOI_STRING_256),
	LOG_FILE_SIZE_64     (DSOI_STRING_64),
	PROCESS_SIZE_16      (DSOI_STRING_16),
	SEVERITY_SIZE_32     (DSOI_STRING_32)
{
	const char* functionName = "DSOIlogMQ";
	int errorFlag = FALSE;
	char alarmText[DSOI_STRING_256];
	memset(alarmText,0,sizeof(alarmText));

	// Instantiate DSOIreceiveMQ object
	if (!(_receive = new DSOIreceiveMQ((char*) queueName,
	                                   (char*) queueManagerName)))
	{
		sprintf(alarmText, "%s", "DSOILog:Memory allocation of class member _receive failed because new() or DSOIreceiveMQ construction failed: DSOILog process may terminate");
		errorFlag = TRUE;
	}

        setExpryL = 0;
        setPriortyL = 0;
	if (!(_errorSendQ = new DSOIsendMQ(setExpryL, setPriortyL, (char*) DSOIstrings::ERROR_Q,
	                                   (char*) queueManagerName)) &&
	    (errorFlag==FALSE))
	{
		sprintf(alarmText, "%s", "DSOILog: Memory allocation of class member _errorSendQ failed because new() or DSOIsendMQ construction failed: DSOILog process may terminate");
		errorFlag = TRUE;
	}

	if (errorFlag)
	{
		// Write error to standard error & throw
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
	initClassMembersFull();

	// Read configuration parameters
	readConfigFile();

	// Set umask : set file access to "-rw-rw-r--"
	umask(S_IWOTH);

	// Create directories
	createDirectories();
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOIlogMQ ()
//
//  PURPOSE: This constructor for DSOIlogMQ is used only when a critical error
//           message is to be logged directly to the alarm file or when log files
//           are to be archived. This version of this class is meant to be as
//           lean as possible in its usage of memory and cpu cycles.
//
//  METHOD:  The DSOIreceiveMQ DSOIsendMQ class members are not allocated memory
//           since the error queue will not accessed.
//
//  PARAMETERS: const char* queueName - name of the error queue
//              const char* queueManagerName - name of the queue manager of
//                                             overseeing the error queue
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: initClassMembersLean, readConfigFile
//
////////////////////////////////////////////////////////////////////////////////
DSOIlogMQ::DSOIlogMQ () :
	ADDN_INFO_SIZE_256   (DSOI_STRING_256),
	ALARM_NUMBER_SIZE_16 (DSOI_STRING_16),
	ALARM_TEXT_SIZE_256  (DSOI_STRING_256),
	DIRECTORY_SIZE_256   (DSOI_STRING_256),
	LOG_FILE_SIZE_64     (DSOI_STRING_64),
	PROCESS_SIZE_16      (DSOI_STRING_16),
	SEVERITY_SIZE_32     (DSOI_STRING_32)
{
	// Set DSOIreceiveMQ & DSOIsendMQ objects to NULL
	_receive = '\0';
	_errorSendQ = '\0';

	// Initialize class members to log error messages to alarm file
	initClassMembersLean();

	// Read configuration parameters
	readConfigFile();

	// Set umask : set file access to "-rw-rw-r--"
	umask(S_IWOTH);

	// Create directories
	createDirectories();
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOIlogMQ (const DSOIlogMQ &logInput)
//
//  PURPOSE: This copy constructor for DSOIlogMQ explicitly copies the
//     individual class members.
//
//  METHOD:  Use the overloaded assignment operator to copy the DSOIlogMQ
//     input object.
//
//  PARAMETERS: const DSOIlogMQ& logInput - DSOIlogMQ input object
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
DSOIlogMQ::DSOIlogMQ(const DSOIlogMQ &logInput) :
	ADDN_INFO_SIZE_256   (DSOI_STRING_256),
	ALARM_NUMBER_SIZE_16 (DSOI_STRING_16),
	ALARM_TEXT_SIZE_256  (DSOI_STRING_256),
	DIRECTORY_SIZE_256   (DSOI_STRING_256),
	LOG_FILE_SIZE_64     (DSOI_STRING_64),
	PROCESS_SIZE_16      (DSOI_STRING_16),
	SEVERITY_SIZE_32     (DSOI_STRING_32)
{
	*this = logInput;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: ~DSOIlogMQ ()
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
DSOIlogMQ::~DSOIlogMQ ()
{
	delete _receive;
	delete _errorSendQ;

	delete [] _addnInfo;
	delete [] _alarmNumber;
	delete [] _alarmText;
	delete [] _criticalDirectory;
	delete [] _criticalLogFile;
	delete [] _noncriticalDirectory;
	delete [] _noncriticalLogFile;
	delete [] _process;
	delete [] _severity;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: archiveLogFiles () const
//
//  PURPOSE: This method archives both the alarm and application-specific
//     error files.
//
//  METHOD:  Calls the methods to archive each type of log file.
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: archiveCriticalFiles, archiveNonCriticalFiles
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::archiveLogFiles()
{
	archiveCriticalFiles();

	archiveNonCriticalFiles();
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: archiveCriticalFiles () const
//
//  PURPOSE: This method archives the alarm file.
//
//  METHOD: The alarm file contains critical error messages where were taken
//    from the error queue. This file is not self-archiving since a date stamp
//    is not appended to the filename; the filename is static as required by the
//    log encapsulator. The archival task is to move the file into an appropriate
//    archival subdirectory and to create an empty alarm file using the static
//    filename.
//
//    The archival strategy is to create necessary subdirectories below the
//    current level to maintain a history of the alarm file:
//          ../logs - current level
//          ../logs/alarm
//          ../logs/alarm/9709
//          ../logs/alarm/9710
//             :
//    The subdirectories are named using the format "year month", i.e. "yymm".
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: memset, strncpy, strncat, strcpy, strcat, fopen, fclose,
//     sprintf, throw, stat, makeDirectory, rename, rewind, getc, putc, sizeof,
//     getArchiveDirectory, getArchiveFilename, new, delete, remove
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::archiveCriticalFiles()
{
	const char* functionName = "archiveCriticalFiles";
	const int margin = 64;
	const int criticalFileSize = DIRECTORY_SIZE_256 + LOG_FILE_SIZE_64 + margin;
	char* criticalFile = new char[criticalFileSize];
	struct stat buf;

	// Build alarm file string
	strncpy(criticalFile,buildFullFileName(DSOIstrings::CRITICAL),
		criticalFileSize);

	// Check that alarm file exists
	if (stat(criticalFile,&buf) < 0)
	{
		// Create empty file since nonexistent
		createCriticalFile(criticalFile);

		// Warn that file didn't exist
		char warningMessage[DSOI_STRING_256];
		sprintf(warningMessage,"%s%s%s%s:%s%s:%s%d",
			"Expected alarm file=\"",criticalFile,
			"\" could not be found and so could not be archived",
			";however, an empty alarm file was created.",
			"file=", __FILE__,"line=",__LINE__);
		strcat(warningMessage,":logDate=");
		strcat(warningMessage,getCurrentDate());
		strcat(warningMessage,":logTime=");
		strcat(warningMessage,getCurrentTime());

		cerr << warningMessage << endl;
	}
	else // file exists
	{
		// Archive error file
		if (S_ISREG(buf.st_mode))
		{
			// Ensure archival subdirectory exists, create if nonexistent
			// format is "year month" as in "logs/alarm/199709"
			char archiveDirectory[DSOI_STRING_128];
			strcpy(archiveDirectory, getArchiveDirectory(DSOIstrings::CRITICAL));
			makeDirectory(archiveDirectory);

			// Check whether archive file exists
			char archiveFile[DSOI_STRING_256];
			strncpy(archiveFile,buildFullArchiveFileName(DSOIstrings::CRITICAL),
				sizeof(archiveFile));

			if (stat(archiveFile,&buf) < 0)
			{
				// Archive file does not exist, so archive
				rename(criticalFile,archiveFile);

				// Create empty alarm file
				createCriticalFile(criticalFile);
			}
			else // Archive file exists so concatenate current alarm file into archive file
			{
				concatActiveToArchiveFile(criticalFile, archiveFile);

				// Clean out the alarm file & create empty one
				remove(criticalFile);
				createCriticalFile(criticalFile);
			}

		}
	}

	delete [] criticalFile;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: archiveNonCriticalFiles ()
//
//  PURPOSE: This method archives application error log files.
//
//  METHOD: The application error file contains non-critical error messages
//    which were taken from the error queue. The error file is somewhat self-
//    archiving since a date stamp is appended to the filename. The archival
//    task is to move the file into an appropriate subdirectory.
//
//    The archival strategy is to create necessary subdirectories below the
//    current level to maintain a history of the application error files:
//          ../logs - current level
//          ../logs/noncritical
//          ../logs/noncritical/9707
//          ../logs/noncritical/9708
//             :
//    A new application error file is created no more than once a day and moved
//    into an appropriate subdirectory once a day as well. The subdirectories
//    are named using the format "year month", i.e. "yymm".
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, delete, memset, sprintf, strcpy, sizeof, stat, throw,
//     makeDirectory, rename, getArchiveDirectory, getArchiveFilename
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::archiveNonCriticalFiles()
{
	const char* functionName = "archiveNonCriticalFiles";
	const int margin = 64;
	int noncriticalFileSize = DIRECTORY_SIZE_256 + LOG_FILE_SIZE_64 + margin;
	char* noncriticalFile = new char[noncriticalFileSize];
	struct stat buf;

	// Build error file string
	memset(noncriticalFile,0,noncriticalFileSize);
	strcpy(noncriticalFile,_noncriticalDirectory);
	strcat(noncriticalFile,"/");
	strcat(noncriticalFile,getArchiveFilename(DSOIstrings::NONCRITICAL));

	// Check that error file exists
	if (stat(noncriticalFile,&buf) < 0)
	{
		// Warn that file didn't exist
		char warningMessage[DSOI_STRING_256];
		sprintf(warningMessage,"%s%s%s:%s%s:%s%d",
			"Expected application error file=\"",noncriticalFile,
			"\" could not be found and so could not be archived.",
			"file=",__FILE__,"line=",__LINE__);
		strcat(warningMessage,":logDate=");
		strcat(warningMessage,getCurrentDate());
		strcat(warningMessage,":logTime=");
		strcat(warningMessage,getCurrentTime());

		cerr << warningMessage << endl;
	}
	else // file exists
	{
		// Archive error file
		if (S_ISREG(buf.st_mode))
		{
			// Ensure archival subdirectory exists, create if nonexistent
			// format is "year month" as in "logs/noncritical/199709"
			char archiveDirectory[DSOI_STRING_128];
			strcpy(archiveDirectory, getArchiveDirectory(DSOIstrings::NONCRITICAL));
			makeDirectory(archiveDirectory);

			// Check whether archive file exists
			char archiveFile[DSOI_STRING_256];
			memset(archiveFile,0,sizeof(archiveFile));
			strcpy(archiveFile,archiveDirectory);
			strcat(archiveFile,"/");
			strcat(archiveFile,getArchiveFilename(DSOIstrings::NONCRITICAL));

			if (stat(archiveFile,&buf) < 0)
			{
				// Archive file does not exist, so archive
				rename(noncriticalFile,archiveFile);
			}
			else // Archive file exists so concatenate current file into archive file
			{
				concatActiveToArchiveFile(noncriticalFile, archiveFile);

				// Remove the current log file
				remove(noncriticalFile);
			}
		}
	}

	// Cleanup memory
	delete [] noncriticalFile;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: buildFullArchiveFileName (const char* fileType) const
//
//  PURPOSE: This method 
//
//  METHOD: 
//
//  PARAMETERS: const char* - file type, i.e. "CRITICAL" or other
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
const char* DSOIlogMQ::buildFullArchiveFileName (const char* fileType) const
{
	static char archiveFilename[DSOI_STRING_512];
	memset(archiveFilename,0,DSOI_STRING_512);

	if (strcmp(fileType,DSOIstrings::CRITICAL)==NULL)
	{
		// Build alarm file string
		strcpy(archiveFilename,getArchiveDirectory((const char*)DSOIstrings::CRITICAL));
		strcat(archiveFilename,"/");
		strcat(archiveFilename,getArchiveFilename(DSOIstrings::CRITICAL));
	}
	else
	{
		// Build error file string
		strncpy(archiveFilename,_noncriticalDirectory,DIRECTORY_SIZE_256);
		strcat(archiveFilename,"/");
		strcat(archiveFilename,getArchiveFilename(DSOIstrings::NONCRITICAL));
	}

	return (const char*) archiveFilename;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: buildFullFileName (const char* fileType) const
//
//  PURPOSE: This method 
//
//  METHOD: 
//
//  PARAMETERS: const char* - file type, i.e. "CRITICAL" or other
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
const char* DSOIlogMQ::buildFullFileName (const char* fileType) const
{
	static char fileName[DSOI_STRING_512];
	memset(fileName,0,DSOI_STRING_512);

	if (strcmp(fileType,DSOIstrings::CRITICAL)==NULL)
	{
		// Build full path to alarm file
		strncpy(fileName,_criticalDirectory,DIRECTORY_SIZE_256);
		strcat(fileName,"/");
		strncat(fileName,_criticalLogFile,LOG_FILE_SIZE_64);
	}
	else
	{
		// Build full path to DSOI error file
		strncpy(fileName,_noncriticalDirectory,DIRECTORY_SIZE_256);
		strcat(fileName,"/");
		strncat(fileName,_noncriticalLogFile,LOG_FILE_SIZE_64);
		strcat(fileName,".");
		strcat(fileName,getCurrentDate("%Y%m%d"));
	}

	return (const char*) fileName;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: checkIfLeapYear () const
//
//  PURPOSE: This method checks if this year is a leap year.
//
//  METHOD: Checks the year against an array of leap years. This method
//     is valid from 2000 to 2100.
//
//  PARAMETERS: None
//
//  RETURNS: int - TRUE or FALSE
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcpy, atoi, sizeof
//
////////////////////////////////////////////////////////////////////////////////
int DSOIlogMQ::checkIfLeapYear() const
{
	int arraySize, year;
	int leapYear = FALSE;
	int leapYears[] = {2000,2004,2008,2012,2016,
	                   2020,2024,2028,2032,2036,
	                   2040,2044,2048,2052,2056,
	                   2060,2064,2068,2072,2076,
	                   2080,2084,2088,2092,2096,2100};
	char thisYear[DSOI_STRING_16];

	strcpy(thisYear,getCurrentDate("%Y"));
	year = atoi(thisYear);
	arraySize = (sizeof(leapYears)/sizeof(int));

	for (int i=0; i<arraySize; i++)
	{
		if (year == leapYears[i])
		{
			leapYear = TRUE;
			break;
		}
	}

	return leapYear;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: checkIfNewYear () const
//
//  PURPOSE: This method checks if today is the start of a new year.
//
//  METHOD: If today is the 1st day of the 1st month, then it's a new year.
//
//  PARAMETERS: None
//
//  RETURNS: int - TRUE or FALSE
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcpy, atoi
//
////////////////////////////////////////////////////////////////////////////////
int DSOIlogMQ::checkIfNewYear() const
{
	char thisDay[DSOI_STRING_16],thisMonth[DSOI_STRING_16];
	strcpy(thisDay,getCurrentDate("%d"));
	strcpy(thisMonth,getCurrentDate("%m"));

	return ((atoi(thisDay) == 1) && (atoi(thisMonth) == 1)) ?
		TRUE : FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: checkIfNewMonth () const
//
//  PURPOSE: This method checks if today is the start of a new month.
//
//  METHOD: If today is the 1st day, then it's a new month.
//
//  PARAMETERS: None
//
//  RETURNS: int - TRUE or FALSE
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcpy, atoi
//
////////////////////////////////////////////////////////////////////////////////
int DSOIlogMQ::checkIfNewMonth() const
{
	char thisDay[DSOI_STRING_16];
	strcpy(thisDay,getCurrentDate("%d"));

	return (atoi(thisDay) == 1) ? TRUE : FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: concatActiveToArchiveFile ()
//
//  PURPOSE: This method concatenates the contents of the active log file
//     into the archive file.
//
//  METHOD: Opens both files and writes characters from one file to
//     the other. Cleans out the active file.
//
//  PARAMETERS: const char* activeFile - active log filename
//              const char* archiveFile  - active file's archive name
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: fopen, fclose, rewind, getc, putc, throw
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::concatActiveToArchiveFile (const char* activeFile,
                                           const char* archiveFile)
{
	const char* functionName="concatActiveToArchiveFile";

	// Open files
	FILE *fpIn, *fpOut;
	if (!(fpIn = fopen(activeFile,"r")))
	{
		char errorMessage[DSOI_STRING_64];
		sprintf(errorMessage,"%s%s%s",
			"Could not open active log file= \"",activeFile,
			"\" for read access. DSOILog may terminate.");
		_errorHandler.formatErrMsg(errorMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
	}
	if (!(fpOut = fopen(archiveFile,"a")))
	{
		char errorMessage[DSOI_STRING_64];
		sprintf(errorMessage,"%s%s%s",
			"Could not open archival log file= \"",archiveFile,
			"\" for write. DSOILog may terminate.");
		_errorHandler.formatErrMsg(errorMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
	}
	rewind(fpIn);

	// Copy fpIn contents into fpOut 
	int c;
	while ((c=getc(fpIn))!=EOF)
		putc(c,fpOut);

	fclose (fpIn);
	fclose (fpOut);
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: createCriticalFile ()
//
//  PURPOSE: This method creates and empty alarm file using values specified
//     in the configuration file or from default values.
//
//  METHOD: Simply opens file for write and then closes.
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: fopen, fclose, sprintf, throw
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::createCriticalFile (const char* criticalFile)
{
	const char* functionName = "createCriticalFile";
	FILE* fp;
	if ((fp = fopen(criticalFile,"w"))==NULL)
	{
		char errorMessage[DSOI_STRING_256];
		sprintf(errorMessage,"%s%s%s%s",
			"Could not create alarm file= \"",criticalFile,
			"\". DSOILog may terminate.");
		_errorHandler.formatErrMsg(errorMessage,(char *) "",0,(char *)functionName,
			(char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
	}
	else
		fclose(fp);
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: createDirectories ()
//
//  PURPOSE: This method 
//
//  METHOD: 
//
//  PARAMETERS: None
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
void DSOIlogMQ::createDirectories ()
{
	struct stat buf;

	// Create alarm file directory
	stat(_criticalDirectory,&buf);
	if (!S_ISDIR(buf.st_mode))
		makeDirectory(_criticalDirectory);

	// Create noncritical file directory
	stat(_noncriticalDirectory,&buf);
	if (!S_ISDIR(buf.st_mode))
		makeDirectory(_noncriticalDirectory);
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getArchiveDate () const
//
//  PURPOSE: This method determines the archival date in the form "year month",
//     i.e. "yymm", used in building the archival subdirectory and timestamp
//     to the application error filename.
//
//  METHOD: Computes the archival date and maintains the string as a static
//     since this value is unchanged during a run.
//
//  PARAMETERS: None
//
//  RETURNS: const char* - string of the dated, archive subdirectory
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcmp, sprintf, strcpy, checkIfNewMonth, checkIfNewYear
//     getPreviousMonth, getPreviousYear, getCurrentDate
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIlogMQ::getArchiveDate() const
{
	char year[DSOI_STRING_16],month[DSOI_STRING_16];
	static char archiveDate[DSOI_STRING_16];

	if (strcmp(archiveDate,"")==NULL)
	{
		// Check if new month
		if (checkIfNewMonth())
			strcpy(month,getPreviousMonth());
		else
			strcpy(month,getCurrentDate("%m"));
	
		// Check if new year
		if (checkIfNewYear())
			strcpy(year,getPreviousYear());
		else
			strcpy(year,getCurrentDate("%Y"));
	
		sprintf(archiveDate,"%s%s",year,month);
	}

	return (const char*) archiveDate;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getArchiveDirectory (const char* fileType) const
//
//  PURPOSE: This method builds the string for the archival subdirectory.
//
//  METHOD: Builds the string depending on whether the subdirectory
//     is for the alarm or noncritical log file.
//
//  PARAMETERS: const char* fileType - either DSOIstrings::CRITICAL or ::NONCRITICAL
//
//  RETURNS: const char* - archive subdirectory string (including the date)
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcmp, sprintf, getArchiveDate
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIlogMQ::getArchiveDirectory(const char* fileType) const
{
	static char archiveDirectory[DSOI_STRING_256];

	if (strcmp(fileType,DSOIstrings::CRITICAL)==NULL)
	{
		sprintf(archiveDirectory,"%s/",_criticalDirectory);
		strcat(archiveDirectory,getArchiveDate());
	}
	else // non-critical
	{
		sprintf(archiveDirectory,"%s/",_noncriticalDirectory);
		strcat(archiveDirectory,getArchiveDate());
	}

	return archiveDirectory;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getArchiveFilename (const char* fileType) const
//
//  PURPOSE: This method builds the archive filename.
//
//  METHOD: Builds the base filename for the alarm or non-critical file and
//     a date stamp to the base name.
//
//  PARAMETERS: const char* fileType - either DSOIstrings::CRITICAL or ::NONCRITICAL
//
//  RETURNS: const char* - archive filename string (including the date stamp)
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcmp, sprintf, getArchiveDate, getPreviousDay
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIlogMQ::getArchiveFilename(const char* fileType) const
{
	static char archiveFilename[DSOI_STRING_128];

	if (strcmp(fileType,DSOIstrings::CRITICAL)==NULL)
	{
		sprintf(archiveFilename,"%s.",_criticalLogFile);
		strcat(archiveFilename,getArchiveDate());
		strcat(archiveFilename,getPreviousDay());
	}
	else // non-critical
	{
		sprintf(archiveFilename,"%s.",_noncriticalLogFile);
		strcat(archiveFilename,getArchiveDate());
		strcat(archiveFilename,getPreviousDay());
	}

	return (const char*) archiveFilename;
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
const char* DSOIlogMQ::getCurrentDate(const char* format) const
{
	// Get date/time stamp
	const int dateSize = 11;
	static char currentDate[DSOI_STRING_16];
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
const char* DSOIlogMQ::getCurrentTime(const char* format) const
{
	// Get date/time stamp
	const int timeSize = 7;
	static char currentTime[DSOI_STRING_16];
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
//  PURPOSE: This method retrieves an error message from the error queue.
//
//  METHOD: Calls the DSOIreceiveMQ object's getMsg() function which retrieves
//     the message from the error queue. The message is stored as a member in
//     the DSOIreceiveMQ object.
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
char* DSOIlogMQ::getMessage ( int waitIntervalTime )
{
	const char* functionName = "getMessage";
	static const char* alarmText = "DSOILog:2001:CRITICAL:Unable to get error messages from the ERROR_Q because DSOIreceiveMQ::getMsg() did not returned MQCC_OK or MQRC_NO_MSG_AVAILABLE:DSOILog process terminating";
	char  criticalMessage[DSOI_STRING_512], addnInfo[DSOI_STRING_256];
	char* message = NULL;
	int   returnCode=0;

	try
	{
		// Set wait interval on the error queue
		// _receive->setWaitInterval(_waitInterval);
		_receive->setWaitInterval(waitIntervalTime);
	        memset(_getTime,0,_sizeOfTime);
                memset(_putApplName,0,28);
                memset(_putApplDate,0,8);
                memset(_putApplTime,0,6);
                memset(_msgId,0,25);
                memset(_correlId,0,24);

		if (((returnCode=_receive->getMsg(_putApplName, _putApplDate,
                     _putApplTime, _getTime, _msgId,(MQBYTE24*)_correlId, _errorSendQ)) != MQCC_OK) &&
		    (returnCode != MQRC_NO_MSG_AVAILABLE))
		{
			// Write to alarm file
			sprintf(addnInfo,":%s%s:%s%s:%s%s",
				"function=",functionName,"file=",__FILE__,"line=",__LINE__);
			strcat(addnInfo,":logDate=");
			strcat(addnInfo,getCurrentDate());
			strcat(addnInfo,":logTime=");
			strcat(addnInfo,getCurrentTime());
			sprintf(criticalMessage,"%s%s",alarmText,addnInfo);
			logCriticalError(criticalMessage);
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
	       sprintf(addnInfo,":%s%s:%s%s:%s%s",
		"function=",functionName,"file=",__FILE__,"line=",__LINE__);
	       strcat(addnInfo,":logDate=");
	       strcat(addnInfo,getCurrentDate());
	       strcat(addnInfo,":logTime=");
	       strcat(addnInfo,getCurrentTime());
	       sprintf(criticalMessage,"%s%s",alarmText,addnInfo);
               DSOIlogMQ* logQ = new DSOIlogMQ();
               try{
                    logQ->logCriticalError(criticalMessage);
                  }
               catch( ThrowError& err )
                  {
                    cout << "Error sending to log critical error!" << endl;
                    cout << "Error Message = " << criticalMessage  << endl;
                  }
               delete logQ;
 
               cout << "in catch(DSOILogMQ...) " << endl;
	}

	return message;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getPreviousDay () const
//
//  PURPOSE: This method gets the previous day in "dd" format.
//
//  METHOD: Checks if the previous day was the end of the previous month,
//     and if so, determines what that day was. Checks for 31, 30, 28 or
//     29 (leap year) month.
//
//  PARAMETERS: None
//
//  RETURNS: const char* - day in "dd" format
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcpy, atoi, getPreviousMonth, checkIfLeapYear,
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIlogMQ::getPreviousDay() const
{
	int previousDay;
	static char previousDayStr[DSOI_STRING_8];
	char today[DSOI_STRING_16];
	strcpy(today,getCurrentDate("%d"));
	previousDay = atoi(today) - 1;

	// Determine last day of previous month
	if (previousDay == 0)
	{
		char monthStr[DSOI_STRING_8];
		strcpy(monthStr,getPreviousMonth());
		switch (atoi(monthStr))
		{
			case(1):
			case(3):
			case(5):
			case(7):
			case(8):
			case(10):
			case(12):
				previousDay = 31;
				break;
			case(4):
			case(6):
			case(9):
			case(11):
				previousDay = 30;
				break;
			case(2):
				if (checkIfLeapYear())
					previousDay = 29;
				else
					previousDay = 28;
				break;
			default:
				// Error, assign to a value to flag the error
				previousDay = 99;
				break;
		}
	}
	sprintf(previousDayStr,"%.2d",previousDay);

	return previousDayStr;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getPreviousMonth () const
//
//  PURPOSE: This method gets the previous month in "mm" format.
//
//  METHOD: If the previous month is "0", then returns 12 (Dec).
//
//  PARAMETERS: None
//
//  RETURNS: const char* - month in "mm" format
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcpy, atoi
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIlogMQ::getPreviousMonth() const
{
	static int previousMonth = -1;
	static char previousMonthStr[DSOI_STRING_8];

	if (previousMonth == -1)
	{
		char thisMonth[DSOI_STRING_16];
		strcpy(thisMonth,getCurrentDate("%m"));
		previousMonth = atoi(thisMonth) - 1;
		previousMonth = (previousMonth == 0) ? 12 : previousMonth;
		sprintf(previousMonthStr,"%.2d",previousMonth);
	}

	return previousMonthStr;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: getPreviousYear () const
//
//  PURPOSE: This method gets the previous year in "yy" format, without
//     "19" or "20" prefix.
//
//  METHOD: Subtracts 1900 or 2000 from the previous year to return a value
//     in the "yy" format.
//
//  PARAMETERS: None
//
//  RETURNS: const char* - year in "yy" format
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strcpy, atoi
//
////////////////////////////////////////////////////////////////////////////////
const char* DSOIlogMQ::getPreviousYear() const
{
	static int previousYear = -1;
	static char previousYearStr[DSOI_STRING_8];

	if (previousYear == -1)
	{
		char thisYear[DSOI_STRING_16];
		strcpy(thisYear,getCurrentDate("%Y"));
		previousYear = atoi(thisYear) - 1;
		previousYear = (previousYear < 2000) ? (previousYear-1900) :
		                                       (previousYear-2000);
		sprintf(previousYearStr,"%.2d",previousYear);
	}

	return previousYearStr;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: initClassMembersFull ()
//
//  PURPOSE: Allocates and initializes class members used in the processing
//    of messages from the error queue. Should only be called by the
//    parameterized constructor.
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
void DSOIlogMQ::initClassMembersFull ()
{

	_addnInfo             = new char[ADDN_INFO_SIZE_256];
	_alarmNumber          = new char[ALARM_NUMBER_SIZE_16];
	_alarmText            = new char[ALARM_TEXT_SIZE_256];
	_criticalDirectory    = new char[DIRECTORY_SIZE_256];
	_criticalLogFile      = new char[LOG_FILE_SIZE_64];
	_noncriticalDirectory = new char[DIRECTORY_SIZE_256];
	_noncriticalLogFile   = new char[LOG_FILE_SIZE_64];
	_process              = new char[PROCESS_SIZE_16];
	_severity             = new char[SEVERITY_SIZE_32];

	memset(_addnInfo,0,ADDN_INFO_SIZE_256);
	memset(_alarmNumber,0,ALARM_NUMBER_SIZE_16);
	memset(_alarmText,0,ALARM_TEXT_SIZE_256);
	memset(_criticalDirectory,0,DIRECTORY_SIZE_256);
	memset(_criticalLogFile,0,LOG_FILE_SIZE_64);
	memset(_noncriticalDirectory,0,DIRECTORY_SIZE_256);
	memset(_noncriticalLogFile,0,LOG_FILE_SIZE_64);
	memset(_process,0,PROCESS_SIZE_16);
	memset(_severity,0,SEVERITY_SIZE_32);

	_waitInterval = 0;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: initClassMembersLean ()
//
//  PURPOSE: Allocates and initializes class members used to log error messages
//    directly into the alarm file. Should be used by parameterless constructor.
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
void DSOIlogMQ::initClassMembersLean ()
{
	// Initialize class member pointers to NULL
	_addnInfo = _alarmNumber = _alarmText = _process = _severity = '\0';

	// Initialize directory and log file class member pointers
	_criticalDirectory    = new char[DIRECTORY_SIZE_256];
	_criticalLogFile      = new char[LOG_FILE_SIZE_64];
	_noncriticalDirectory = new char[DIRECTORY_SIZE_256];
	_noncriticalLogFile   = new char[LOG_FILE_SIZE_64];

	memset(_criticalDirectory,0,DIRECTORY_SIZE_256);
	memset(_criticalLogFile,0,LOG_FILE_SIZE_64);
	memset(_noncriticalDirectory,0,DIRECTORY_SIZE_256);
	memset(_noncriticalLogFile,0,LOG_FILE_SIZE_64);

	_waitInterval = 0;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: logCriticalError(const char* inputText)
//
//  PURPOSE: This method logs a critical message directly to the alarm file.
// 
//  METHOD: Adds the current date and time to the message text. Passes
//     the message to logCriticalMessage() which actually writes to the
//     log file.
//
//  PARAMETERS: const char* inputText - error text containing necessary data
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, delete, sprintf, strcat, time, getCurrentDate,
//     getCurrentTime, logCriticalMessage
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::logCriticalError (const char* inputText)
{       char* alarmMessage = new char[strlen(inputText) + 64];
        int size = strlen(inputText);

	strcpy(alarmMessage,inputText);
	strcat(alarmMessage,":logDate=");
	strcat(alarmMessage,getCurrentDate());
	strcat(alarmMessage,":logTime=");
	strcat(alarmMessage,getCurrentTime());
        alarmMessage[strlen(alarmMessage)+1] = '\0';
	if (strstr(alarmMessage,"CRITICAL")!=NULL)
	     logCriticalMessage(alarmMessage);
        else
	     logNonCriticalMessage(alarmMessage);

	delete [] alarmMessage;

}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: logCriticalMessage (char* criticalMessage)
//
//  PURPOSE: This method writes critical error message to the alarm file.
//
//  METHOD: Opens the alarm file for appending, and then writes to it.
//
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, delete, memset, strncpy, strncat, strcat, fopen, fclose,
//     sprintf, fprintf, throw, getCurrentDate, getCurrentTime
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::logCriticalMessage(const char* criticalMessage)
{
	const char* functionName = "logCriticalMessage";
	FILE* fp;
	const int margin = 64;
	int   criticalFileSize = DIRECTORY_SIZE_256 + LOG_FILE_SIZE_64 + margin;
	char* criticalFile = new char[criticalFileSize];

	// Build alarm file string
	memset(criticalFile,0,sizeof(criticalFile));
	strncpy(criticalFile,buildFullFileName(DSOIstrings::CRITICAL),
		criticalFileSize);

	// Log to alarm file
	if ((fp = fopen(criticalFile,"a")) != NULL)
	{
		fprintf(fp,criticalMessage);
		fprintf(fp,"\n\n");
	}
	else
        {   // makeDirectory(_criticalDirectory);
            mkdir("../logfiles/alarm",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	    if ((fp = fopen("../logfiles/alarm/DSOI_alarm.log","a")) != NULL)
	      {
		fprintf(fp,criticalMessage);
		fprintf(fp,"\n\n");
              }
            else
              {
		// Write this error to standard error & throw
		char errorMessage[DSOI_STRING_512+DSOI_STRING_512];
		sprintf(errorMessage,"%s \"%s\" %s:%s \"",
			"DSOILog:Unable to open alarm file",criticalFile,"for append",
                        "The error message is");
		strncat(errorMessage,criticalMessage,strlen(criticalMessage));
		strcat(errorMessage,"\".");
		strcat(errorMessage,":logDate=");
		strcat(errorMessage,getCurrentDate());
		strcat(errorMessage,":logTime=");
		strcat(errorMessage,getCurrentTime());

		_errorHandler.formatErrMsg(errorMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
              }
	}

	// Cleanup memory
	delete [] criticalFile;

	fclose(fp);
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: logNonCriticalMessage()
//
//  PURPOSE: This method writes non-critical error messages to the application
//     error file.
//
//  METHOD: 
//
//  PARAMETERS: None
//
//  RETURNS:
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: void
//
//  FUNCTIONS USED: new, delete, memset, strncpy, strncat, strcpy, strcat,
//     sprintf, fopen, fclose, fprintf, getCurrentDate, getCurrentTime
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::logNonCriticalMessage (const char* message)
{
	const char* functionName = "logNonCriticalMessage";
	FILE* fp;
	const int margin = 64;
	int   logFileSize = DIRECTORY_SIZE_256 + LOG_FILE_SIZE_64 + margin;
	char* logFile = new char[logFileSize];

	// Build log file string
	memset(logFile,0,sizeof(logFile));
	strncpy(logFile,buildFullFileName(DSOIstrings::NONCRITICAL),
		logFileSize);
	if ((fp = fopen(logFile,"a")) != NULL)
	  { fprintf(fp,message);
	    fprintf(fp,"\n\n");
          }
        else
          { // makeDirectory(_noncriticalDirectory);
            // if ((fp=fopen(logFile,"a")) != NULL)
            mkdir("../logfiles/noncritical",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	    if ((fp = fopen("../logfiles/noncritical/DSOI_error.log","a")) != NULL)
	      { fprintf(fp,message);
	        fprintf(fp,"\n\n");
              }
	    else
	      {
		// Write this error to standard error
		char errorMessage[DSOI_STRING_512+DSOI_STRING_512];
		sprintf(errorMessage,"%s \"%s\" %s:%s \"",
			"DSOILog:Unable to open application error file",_noncriticalLogFile,"for append",
			"The error message is");
		strncat(errorMessage,message,strlen(message));
		strcat(errorMessage,"\".");
		strcat(errorMessage,":logDate=");
		strcat(errorMessage,getCurrentDate());
		strcat(errorMessage,":logTime=");
		strcat(errorMessage,getCurrentTime());

		_errorHandler.formatErrMsg(errorMessage, (char *) "", 0, (char *)functionName, (char *) __FILE__, __LINE__);
		throw ThrowError(_errorHandler._formatErrMsg,0);
	      }
          }

	// Cleanup memory
	delete [] logFile;

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
int DSOIlogMQ::makeDirectory (const char* inputDirectory)
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
		mkdir(subDirectory,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		strcat(subDirectory,"/");
	} while (token = strtok(NULL,LOCAL_DELIMITERS));

	stat(inputDirectory,&buf);
	return (S_ISDIR(buf.st_mode)) ? TRUE : FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: parseMessage()
//
//  PURPOSE: To parse the error message retrieved from the error queue into
//           its component parts.
//
//  METHOD: Uses strtok() to tokenize the message using DELIMITER (colon ":"),
//          and then copies the tokens to their respective strings. The message
//          format is fixed. 
//
//             "process : alarmNumber : severity : alarmText : function :
//              line : reasonCode : date : time : correlId : msgId"
//
//           The alarmText field actually includes everything from severity to
//           the end of the line. An example line could be,
//
//             "DSOI:1001:CRITICAL:Not connected because MQCONN() failed: 
//              function=DSOIdistribute:line=64:rc=2059:date=080897:
//              time=132531:correlId=4004e8d0d:msgId=4004e8e8d"
//
//  PARAMETERS: char* msg - message from the error queue
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: strtok, strncpy, strncat, strstr
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::parseMessage (char* msg)
{
	const char* FUNCTION_TAG="function=";
	char *token;

	if ((token = strtok(msg,DELIMITER)) != NULL)
		strncpy(_process,token,PROCESS_SIZE_16);
	if ((token = strtok(NULL,DELIMITER)) != NULL)
		strncpy(_alarmNumber,token,ALARM_NUMBER_SIZE_16);
	if ((token = strtok(NULL,DELIMITER)) != NULL)
		strncpy(_severity,token,SEVERITY_SIZE_32);
	if ((token = strtok(NULL,DELIMITER)) != NULL)
		strncpy(_alarmText,token,ALARM_TEXT_SIZE_256);

	token = strtok(NULL,DELIMITER);
	if (strstr(token,FUNCTION_TAG) != NULL)
	{
		strncpy(_addnInfo,token,ADDN_INFO_SIZE_256);
		if ((token = strtok(NULL,"\0")) != NULL)
		{
			int remainder = ADDN_INFO_SIZE_256-strlen(_addnInfo);
			remainder = (remainder>0) ? remainder : 0;
			strncat(_addnInfo,token,remainder);
		}
	}
	else
	{
		strcat(_alarmText,DELIMITER);
		int remainder = ALARM_TEXT_SIZE_256-strlen(_alarmText);
		remainder = (remainder>0) ? remainder : 0;
		strncat(_alarmText,token,remainder);
		if ((token = strtok(NULL,"\0")) != NULL)
			strncpy(_addnInfo,token,ADDN_INFO_SIZE_256);
	}
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: processMessages ( int )
//
//  PURPOSE: To retrieve messages from the error queue and to log the messages
//     messages either to an alarm file for critical errors or to an application
//     log file for non-critical errors.
//
//  METHOD: This method first gets the message, and then logs the message to the
//     appropriate file depending on the severity level.
//
//  PARAMETERS: None
//
//  RETURNS: int - TRUE (success) or FALSE (failure)
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: getMessage, parseMessage, logCriticalMessage,
//     logNonCriticalMessage
//
////////////////////////////////////////////////////////////////////////////////
int DSOIlogMQ::processMessages (int waitIntervalTime)
{
	char* msg;
	int   returnCode;

	if ((msg = getMessage( waitIntervalTime )) != NULL)
	{
		// Process error message according to its severity
		// Check if "CRITICAL" severity string is in message
		// test if (strstr(msg,DSOIstrings::CRITICAL)!=NULL)
	        //		logCriticalMessage((const char*) msg);
		//      else
	        //		logNonCriticalMessage((const char*) msg);
		logCriticalError((const char*) msg);

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
//  METHOD: After opening the configuration file in read-only mode, four
//     parameters are read:
//        - alarm file directory
//        - alarm filename
//        - noncritical error file directory
//        - noncritical error filename
//     If the file cannot be opened or if any of the parameters could not
//     be read correctly, default values will be used and an error message
//     written to standard error.
//          
//  PARAMETERS: None
//
//  RETURNS: None
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: fopen, fclose, fscanf, strcmp, strncpy, sprintf
//
////////////////////////////////////////////////////////////////////////////////
void DSOIlogMQ::readConfigFile()
{
	FILE* fp;
	char  input[DSOI_STRING_128];
	char  tmp[DSOI_STRING_128];
	char  warningMessage[DSOI_STRING_512];
	int   warningFlag = FALSE;
        if ((fp = fopen(DSOIstrings::LOG_CONFIG_FILE,"r")) != NULL)
	{
		// Read relative path to alarm file
		fscanf(fp,"%s %s %s %s",tmp,tmp,tmp,input);
		if (strcmp(input,"")!=NULL)
			strncpy(_criticalDirectory,input,DIRECTORY_SIZE_256);
		else
		{
			// Use default value
			strncpy(_criticalDirectory,DSOIstrings::DEFAULT_ALARM_DIRECTORY,
				DIRECTORY_SIZE_256);
			warningFlag = TRUE;
		}

		// Read alarm filename 
		fscanf(fp,"%s %s %s",tmp,tmp,input);
		if (strcmp(input,"")!=NULL)
			strncpy(_criticalLogFile,input,LOG_FILE_SIZE_64);
		else
		{
			// Use default value
			strncpy(_criticalLogFile,DSOIstrings::DEFAULT_ALARM_FILE,
				LOG_FILE_SIZE_64);
			warningFlag = TRUE;
		}

		// Read relative path to noncritical error file
		fscanf(fp,"%s %s %s %s %s",tmp,tmp,tmp,tmp,input);
		if (strcmp(input,"")!=NULL)
			strncpy(_noncriticalDirectory,input,DIRECTORY_SIZE_256);
		else
		{
			// Use default value
			strncpy(_noncriticalDirectory,DSOIstrings::DEFAULT_NONCRITICAL_DIRECTORY,
				DIRECTORY_SIZE_256);
			warningFlag = TRUE;
		}

		// Read noncritical filename 
		fscanf(fp,"%s %s %s %s",tmp,tmp,tmp,input);
		if (strcmp(input,"")!=NULL)
			strncpy(_noncriticalLogFile,input,LOG_FILE_SIZE_64);
		else
		{
			// Use default value
			strncpy(_noncriticalLogFile,DSOIstrings::DEFAULT_NONCRITICAL_FILE,
				LOG_FILE_SIZE_64);
			warningFlag = TRUE;
		}

		// Close config file
		fclose(fp);

		if (warningFlag)
		{
			// Write warning message to standard error
			sprintf(warningMessage,"%s \"%s\" %s:%s%s:%s%s:%s%s:%s%s:%s",
				"Some parameters were not read correctly from the configuration file=",
				DSOIstrings::LOG_CONFIG_FILE,"and so default values were used; values used",
				"alarm directory=",_criticalDirectory,
				"alarm filename=",_criticalLogFile,
				"noncritical directory=",_noncriticalDirectory,
				"noncritical base filename=",_noncriticalLogFile,
				"Please check these values to ensure DSOILog is processing properly");
			strcat(warningMessage,":logDate=");
			strcat(warningMessage,getCurrentDate());
			strcat(warningMessage,":logTime=");
			strcat(warningMessage,getCurrentTime());

			// cerr << "DSOIlogMQ::readConfigFile:" << warningMessage << endl;
		}
	}
	else // unable to open DSOILog configuration file
	{
		// Use default values
		strncpy(_criticalDirectory,DSOIstrings::DEFAULT_ALARM_DIRECTORY,DIRECTORY_SIZE_256);
		strncpy(_criticalLogFile,DSOIstrings::DEFAULT_ALARM_FILE,LOG_FILE_SIZE_64);
		strncpy(_noncriticalDirectory,DSOIstrings::DEFAULT_NONCRITICAL_DIRECTORY,DIRECTORY_SIZE_256);
		strncpy(_noncriticalLogFile,DSOIstrings::DEFAULT_NONCRITICAL_FILE,LOG_FILE_SIZE_64);

		// Make note of this to standard error
		sprintf(warningMessage,"%s \"%s\":%s:%s%s:%s%s:%s%s:%s%s",
			"DSOILog:Unable to access file=",DSOIstrings::LOG_CONFIG_FILE,
			"DSOILog using default values",
			"alarm file=",DSOIstrings::DEFAULT_ALARM_FILE,
			"alarm directory=",DSOIstrings::DEFAULT_ALARM_DIRECTORY,
			"noncritical file=",DSOIstrings::DEFAULT_NONCRITICAL_FILE,
			"noncritical directory=",DSOIstrings::DEFAULT_NONCRITICAL_DIRECTORY);
		strcat(warningMessage,":logDate=");
		strcat(warningMessage,getCurrentDate());
		strcat(warningMessage,":logTime=");
		strcat(warningMessage,getCurrentTime());

		// cerr << "DSOIlogMQ::readConfigFile:" << warningMessage << endl;
	}
}

////////////////////////////////////////////////////////////////////////////////
//  FUNCTION: operator= (const DSOIlogMQ& logInput)
//
//  PURPOSE: This method assigns the class members of the DSOIlogMQ
//     input object to this object.
//
//  METHOD: 
//        
//  PARAMETERS: const DSOIlogMQ& logInput - DSOIlogMQ input object
//
//  RETURNS: DSOIlogMQ& - reference to this object
//
//  OTHER INPUTS: None
//
//  OTHER OUTPUTS: None
//
//  FUNCTIONS USED: new, strcmp, strcpy, initClassMembersFull,
//     getReceive, getReceive, getErrorSendQ,
//     getCriticalDirectory, getCriticalLogFile,
//     getNoncriticalDirectory, getNoncriticalLogFile,
//     getProcess, getAlarmNumber, getSeverity, getAlarmText, getAddnInfo
//
////////////////////////////////////////////////////////////////////////////////
DSOIlogMQ& DSOIlogMQ::operator=(const DSOIlogMQ& logInput)
// jal add copy constructors to class objects
//	: _receive(logInput.getReceive()), _errorSendQ(logInput.getErrorSendQ()),
//    _errorHandler = logInput.get_errorHandler();
{
	if (strcmp(logInput.getCriticalDirectory(),"")!=0)
		strcpy(_criticalDirectory,logInput.getCriticalDirectory());
	if (strcmp(logInput.getCriticalLogFile(),"")!=0)
		strcpy(_criticalLogFile,logInput.getCriticalLogFile());
	if (strcmp(logInput.getNoncriticalDirectory(),"")!=0)
		strcpy(_noncriticalDirectory,logInput.getNoncriticalDirectory());
	if (strcmp(logInput.getNoncriticalLogFile(),"")!=0)
		strcpy(_noncriticalLogFile, logInput.getNoncriticalLogFile());

	if (strcmp(logInput.getProcess(),"")!=0)
	{
		// Initialize class members to process messages on the error queue
		initClassMembersFull();

		// Copy class member strings
		strcpy(_process,logInput.getProcess());
		strcpy(_alarmNumber,logInput.getAlarmNumber());
		strcpy(_severity,logInput.getSeverity());
		strcpy(_alarmText,logInput.getAlarmText());
		strcpy(_addnInfo,logInput.getAddnInfo());
	}
	else
	{
		// Initialize class members log error messages to the alarm file
		initClassMembersLean();
	}

	return *this;
}

// end

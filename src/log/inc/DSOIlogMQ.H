#ifndef DSOIlogMQ_H
#define DSOIlogMQ_H

//------------------------------------------------------------------------------
//                    Confidential and Proprietary
//
//      Copyright 1992 U S WEST Information Technologies.
//      All Rights Reserved.
//
//      Disclose and distribute solely to employees of U S WEST and its
//      affiliates having a need to know.
//
//
//------------------------------------------------------------------------------
// File: DSOIlogMQ.H
//
// Author(s): Joo Ahn Lee, PDS
//
// Description: See bottom of file for description.
//
//------------------------------------------------------------------------------
// Check-In History:
/*
 * $Log:   V:/dsoi/src/log/inc/DSOIlogMQ.H-arc  $

   Rev 1.5   Mar 13 2003 09:37:06   rfuruta
Set Wait Interval Time

   Rev 1.4   Aug 04 2000 09:29:52   bxcox
Project: Swift - Change Requests
Record(s): 3567
aCC compiler upgrade

   Rev 1.3   Apr 07 1999 17:36:32   jwschae
Stripped Line Feeds
 * 
 *    Rev 1.2   07 Apr 1999 16:21:00   pvcs
 * CR#2034:M:removed extraneous revision comments
 * Revision 1.2  1997/10/09 18:48:08  jalee3
 * CR#2034:M:removed extraneous revision comments
 *
 * Revision 1.1  1997/10/03 09:42:46  jalee3
 * Initial revision
 *
 *
 */
//
//------------------------------------------------------------------------------

#include "DSOIreceiveMQ.H"
#include "DSOIsendMQ.H"
#include "DSOIThrowError.H"
//#include "DSOIerrorHandler.H"
//#include "DSOIconstants.H"

class DSOIlogMQ {
public:
	DSOIlogMQ();
	DSOIlogMQ(const char*, const char*);
	DSOIlogMQ(const DSOIlogMQ&);
	~DSOIlogMQ();

	void archiveLogFiles();
	void logCriticalError(const char*);
	int  processMessages(int);

	DSOIlogMQ& operator=(const DSOIlogMQ&);

	// Accessors
	const DSOIerrorHandler getErrorHandler() const;  // inline
	const DSOIreceiveMQ* getReceive() const;         // inline
	const DSOIsendMQ* getErrorSendQ() const;         // inline

	const char* getCriticalDirectory() const;        // inline
	const char* getCriticalLogFile() const;          // inline
	const char* getNoncriticalDirectory() const;     // inline
	const char* getNoncriticalLogFile() const;       // inline

	const char* getAddnInfo() const;                 // inline
	const char* getAlarmNumber() const;              // inline
	const char* getAlarmText() const;                // inline
	const char* getProcess() const;                  // inline
	const char* getSeverity() const;                 // inline
	long        getWaitInterval() const;             // inline

	// Mutators
	void setWaitInterval(long);                      // inline

protected:
private:
	// internal processing
	void  archiveCriticalFiles();
	void  archiveNonCriticalFiles();
	const char* buildFullFileName (const char*) const;
	const char* buildFullArchiveFileName (const char*) const;
	int   checkIfLeapYear() const;
	int   checkIfNewMonth() const;
	int   checkIfNewYear() const;
	void  concatActiveToArchiveFile(const char*,const char*);
	void  createCriticalFile(const char*);
	void  createDirectories();
	const char* getArchiveDate() const;
	const char* getArchiveDirectory(const char*) const;
	const char* getArchiveFilename(const char*) const;
	const char* getCurrentDate(const char* ="") const;
	const char* getCurrentTime(const char* ="") const;
	char* getMessage(int);
	const char* getPreviousDay() const;
	const char* getPreviousMonth() const;
	const char* getPreviousYear() const;
	void  initClassMembersFull();
	void  initClassMembersLean();
	void  logCriticalMessage(const char*);
	void  logNonCriticalMessage (const char*);
	int   makeDirectory(const char*);
	void  parseMessage(char*);
	void  readConfigFile();

	// Utility
	const int DIRECTORY_SIZE_256;
	const int LOG_FILE_SIZE_64;
	const int ADDN_INFO_SIZE_256;
	const int ALARM_NUMBER_SIZE_16;
	const int ALARM_TEXT_SIZE_256;
	const int PROCESS_SIZE_16;
	const int SEVERITY_SIZE_32;

	// Member data
	DSOIerrorHandler _errorHandler;
	DSOIreceiveMQ*   _receive;
	DSOIsendMQ*      _errorSendQ;

	char* _criticalDirectory;
	char* _criticalLogFile;
	char* _noncriticalDirectory;
	char* _noncriticalLogFile;

	char* _addnInfo;
	char* _alarmText;
	char* _alarmNumber;
	char* _process;
	char* _severity;

	long  _waitInterval;
};

inline const char* DSOIlogMQ::getAddnInfo() const
{
	return (const char*) _addnInfo;
}
inline const char* DSOIlogMQ::getAlarmNumber() const
{
	return (const char*) _alarmNumber;
}
inline const char* DSOIlogMQ::getAlarmText() const
{
	return (const char*) _alarmText;
}
inline const char* DSOIlogMQ::getCriticalDirectory() const
{
	return (const char*) _criticalDirectory;
}
inline const char* DSOIlogMQ::getCriticalLogFile() const
{
	return (const char*) _criticalLogFile;
}
inline const DSOIerrorHandler DSOIlogMQ::getErrorHandler() const
{
	return _errorHandler;
}
inline const char* DSOIlogMQ::getNoncriticalDirectory() const
{
	return (const char*) _noncriticalDirectory;
}
inline const char* DSOIlogMQ::getNoncriticalLogFile() const
{
	return (const char*) _noncriticalLogFile;
}
inline const DSOIsendMQ* DSOIlogMQ::getErrorSendQ() const
{
	return _errorSendQ;
}
inline const char* DSOIlogMQ::getProcess() const
{
	return (const char*) _process;
}
inline const DSOIreceiveMQ* DSOIlogMQ::getReceive() const
{
	return (const DSOIreceiveMQ*) _receive;
}
inline const char* DSOIlogMQ::getSeverity() const
{
	return (const char*) _severity;
}
inline long DSOIlogMQ::getWaitInterval() const
{
	return _waitInterval;
}
inline void DSOIlogMQ::setWaitInterval(long waitInterval)
{
	_waitInterval = waitInterval;
}

////////////////////////////////////////////////////////////////////////////////
//
// Class: DSOIlogMQ
//
// Purpose: To retrieve error messages from the MQSeries error queue, to log
//          critical error messages to an alarm file, to log noncritical error
//          messages to an application error log file, and to archive these
//          log files.
//
// Usage: The DSOIlogMQ class can be used in one of three ways:
//
//          1. The primary use is to retrieve and process error messages from
//             the error queue. Only the DSOILog process should need this class.
//
//             To use,
//                - instantiate using the parameterized constructor:
//
//                  const char* errorQueueName = <..>;
//                  const char* queueManagerName = <..>;
//                    :
//                  DSOIlogMQ logger(errorQueueName, queueManagerName);
//
//                - then call processErrorMessages() in a loop:
//
//                  while (logger.processErrorMessages())
//                  {
//                    :
//                  }
//
//          2. The other use is to log critical errors to the alarm file whenever
//             a DSOI process encounters a problem and is unable to write to the
//             error queue, such as being unable to connect to the queue manager
//             or being unable to open a queue. The instantiated object is lean
//             in its use of memory and cpu cycles.
//
//             To use,
//                - instantiate a const object using the parameterless constructor:
//
//                  const DSOIlogMQ logger;
//
//                - then call logCriticalError():
//
//                  logger.logCriticalError(alarmNumber, alarmText, addnInfo);
//
//          3. The third use is to archive application error files. This process
//             searchs for yesterday's log file (checks date stamps), and moves
//             the file to a subdirectory of the log directory.
//
//             To use,
//                - instantiate a const object using the parameterless constructor:
//
//                  const DSOIlogMQ logger;
//
//                - then call archiveLogFile():
//
//                  logger.archiveLogFile();
//
// Methods:
//
// PUBLIC
//
//   DSOIlogMQ()
//      This constructor should be used to log critical error messages directly
//      to an alarm file. This object will not be able to process error messages
//      from the error queue. This object uses a minimal amount of resources.
//
//   DSOIlogMQ(const char*, const char*)
//      This constructor should be used to instantiate an object which will be
//      used to process error messages from the error queue.
//
//   DSOIlogMQ(const DSOIlogMQ&)
//      This is the copy constructor.
//
//   ~DSOIlogMQ()
//      This destructor deletes any dynamic memory allocated in either constructor.
//
//   void archiveLogFiles()
//      Archives both the alarm and noncritical log files.
//
//   void logCriticalError(const char*)
//      Logs error messages directly into the alarm file. This method should be used
//      in conjunction with the parameterless constructor.
//
//   int processErrorMessages()
//      Entry point for processing error messages on the error queue. Each call
//      processes one message so this method can be called from a loop.
//
//   DSOIlogMQ& operator=(const DSOIlogMQ&);
//      This is the assignment operator.
//
//   const DSOIerrorHandler getErrorHandler() const
//   const DSOIreceiveMQ* getReceive() const
//   const DSOIsendMQ* getErrorSendQ() const
//   const char* getCriticalDirectory() const
//   const char* getCriticalLogFile() const
//   const char* getNoncriticalDirectory() const
//   const char* getNoncriticalLogFile() const
//   long        getWaitInterval() const
//      Inline functions to retrieve the values of various class member data.
//
//   const char* getAddnInfo() const
//   const char* getAlarmNumber() const
//   const char* getAlarmText() const
//   const char* getProcess() const
//   const char* getSeverity() const
//      Inline functions which should be obsoleted if parseMesage() is not called.
//
//   void setWaitInterval()
//      Sets the time DSOILog remains alive waiting for more messages to be put
//      into the error queue.
//
// PRIVATE
//   void  archiveCriticalFiles();
//      Archive the alarm file.
//
//   void  archiveNonCriticalFiles();
//      Archives the DSOI-specific error file.
//
//   const char* buildFullFileName (const char*) const;
//      Builds full relative path to the critical or noncritical log file.
//
//   const char* buildFullArchiveFileName (const char*) const;
//      Builds full relative path to the archival critical or noncritical log file.
//
//   int   checkIfLeapYear() const;
//   int   checkIfNewMonth() const;
//   int   checkIfNewYear() const;
//      Checks for the stated condition.
//
//   void  concatActiveToArchiveFile(const char*,const char*);
//      Concatenates the contents of the active file into the archive file.
//
//   void  createCriticalFile(const char*);
//      Creates an empty alarm file.
//
//   void  createDirectories();
//      Creates the relative paths to the alarm and noncritical log files.
//
//   const char* getArchiveDate() const;
//      Determines the archive date in the format "YYYYMM".
//
//   const char* getArchiveDirectory(const char*) const;
//      Determines the archival directory for the critical or noncritical archive
//      file. Adds "YYYYMM" to the end of the relative path.
//
//   const char* getArchiveFilename(const char*) const;
//      Determines the filename for the critical or noncritical archive file by
//      appending a date stamp, i.e. "YYYYMMDD", to the base filename.
//
//   const char* getCurrentDate(const char* ="") const;
//      Gets the current date formatted according to the input string. See strftime()
//      for allowable formats.
//
//   const char* getCurrentTime(const char* ="") const;
//      Gets the current time formatted according to the input string. See strftime()
//      for allowable formats.
//
//   char* getMessage(int);
//      Retrieves a message from the error queue.
//
//   const char* getPreviousDay() const;
//   const char* getPreviousMonth() const;
//   const char* getPreviousYear() const;
//      Determines the state value; takes into account different month lengths and
//      leap years from 2000 - 2100.
//
//   void  initClassMembersFull();
//      Allocates memory for variables used in processing messages from the error
//      queue. This method is called by the parameterized constructor.
//
//   void  initClassMembersLean();
//      Allocates memory for variables used in logging critical errors directly
//      into the alarm file. This method is called by the parameterless constructor.
//
//   void  logCriticalMessage(const char*);
//      Logs error message to the alarm file.
//
//   void  logNonCriticalMessage (const char*);
//      Logs error message to the application specific error files.
//
//   int   makeDirectory(const char*);
//   void  parseMessage(char*);
//      Extracts various elements from the error message. Used in conjunction with
//      logCriticalMessage(void). Is not used currently; should be obsoleted if not
//      continued to be used.
//
//   void  readConfigFile();
//
// Members:
//
//   Note that all the class members are pointers except for the DSOIerrorHandler
//   object. When the parameterless constructor is used, memory is not allocated for
//   the points and so the resulting DSOIlogMQ object is lean.
//
//   DSOIerrorHandler _errorHandler
//      Used to print errors encountered in DSOIlogMQ to standard error.
//
//   DSOIreceiveMQ* _receive
//      Used to get messages from the error queue.
//
//   DSOIsendMQ* _errorSendQ
//      Used as a parameter to DSOIreceiveMQ calls.
//
//   char* _criticalDirectory;
//      Holds relative path to the alarm file.
//
//   char* _criticalLogFile;
//      Holds alarm filename.
//
//   char* _noncriticalDirectory;
//      Holds relative path to the noncritical log file.
//
//   char* _noncriticalLogFile;
//      Holds noncritical filename.
//
//   char* _process
//   char* _alarmNumber
//   char* _severity
//   char* _alarmText
//   char* _addnInfo
//      These members are no longer used. They were used by parseMessage(), but
//      but can be obsoleted.
//
//   long  _waitInterval
//      Time DSOILog remains alive waiting for more messages to be put into
//      the error queue.
//
////////////////////////////////////////////////////////////////////////////////

#endif

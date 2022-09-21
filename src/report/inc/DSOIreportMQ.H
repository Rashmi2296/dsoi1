#ifndef DSOIreportMQ_H
#define DSOIreportMQ_H

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
// File: DSOIreportMQ.H
//
// Author(s): Joo Ahn Lee
//
// Description: See bottom of file for description.
//
//------------------------------------------------------------------------------
// Check-In History:
/*
 * $Log:   V:/dsoi/src/report/inc/DSOIreportMQ.H-arc  $

   Rev 1.5   Mar 13 2003 09:33:42   rfuruta
Set Wait Interval Time

   Rev 1.4   Aug 04 2000 09:25:54   bxcox
Project: Swift - Change Requests
Record(s): 3567
aCC compiler upgrade

   Rev 1.3   Apr 07 1999 17:38:56   jwschae
Stripped Line Feeds
 * 
 *    Rev 1.2   07 Apr 1999 16:25:38   pvcs
 * CR#2239:M:removed parse() & added comments; no change in functionality
 * Revision 1.2  1997/12/31 18:07:53  jalee3
 * CR#2239:M:removed parse() & added comments; no change in functionality
 *
 * Revision 1.1  1997/10/03 09:44:24  jalee3
 * Initial revision
 *
 *
 */
//
//------------------------------------------------------------------------------

#include "DSOIThrowError.H"
#include "DSOIconstants.H"
#include "DSOIerrorHandler.H"
#include "DSOIreceiveMQ.H"
#include "DSOIsendMQ.H"

class DSOIreportMQ {
public:
	DSOIreportMQ(const char*, const char*);
	DSOIreportMQ(const DSOIreportMQ &);
	~DSOIreportMQ();

	int processMessages(int);

	DSOIreportMQ& operator=(const DSOIreportMQ&);

	// Accessors
	const char* getLogDirectory() const;  // inline
	const char* getLogFile() const;       // inline
	long        getWaitInterval() const;  // inline

	// Mutators
	void setWaitInterval(long);           // inline

protected:
private:

	// Internal processing
	const char* buildFullFileName () const;
	void  createDirectories();
	const char* getCurrentDate(const char* ="") const;
	const char* getCurrentTime(const char* ="") const;
	char* getMessage(int);
	void  initClassMembers();
	int   makeDirectory (const char*);
	void  logMessage(const char*);
	void  readConfigFile();

	// Utility
	const int LOG_DIRECTORY_SIZE_128;
	const int LOG_FILE_SIZE_64;

	// Member data
	DSOIerrorHandler _errorHandler;
	DSOIreceiveMQ*   _receive;
	DSOIsendMQ*      _errorSendQ;

	char* _logDirectory;
	char* _logFile;

	long  _waitInterval;
};

inline const char* DSOIreportMQ::getLogDirectory() const
{
	return (const char*) _logDirectory;
}
inline const char* DSOIreportMQ::getLogFile() const
{
	return (const char*) _logFile;
}
inline long DSOIreportMQ::getWaitInterval() const
{
	return _waitInterval;
}
inline void DSOIreportMQ::setWaitInterval(long waitInterval)
{
	_waitInterval = waitInterval;
}

////////////////////////////////////////////////////////////////////////////////
//
// Class: DSOIreportMQ
//
// Purpose: To retrieve report messages from the MQSeries report queue and to log
//    that report data to archival files.
//
// Usage: The DSOIreportMQ class should be used to retrieve and process report
//    messages from the report queue.
//
//    To use,
//       - instantiate using the parameterized constructor:
//
//         const char* errorQueueName = <..>;
//         const char* queueManagerName = <..>;
//         // .. code ..
//         DSOIreportMQ logger(reportQueueName, queueManagerName);
//
//       - then call processMessages() in a loop:
//
//         while (logger.processMessages())
//         {
//            // .. code ..
//         }
//
// Methods:
//
//   PUBLIC
//     DSOIreportMQ(const char*, const char*)
//       This constructor should be used to instantiate an object which will be
//       used to process the report messages in the report queue. See the source
//       file for the expected parameters.
//
//     DSOIreportMQ(const DSOIreportMQ&)
//       This is the copy constructor.
//
//     ~DSOIreportMQ()
//       This destructor deletes any dynamic memory allocated in either constructor.
//
//     DSOIreportMQ& operator=(const DSOIreportMQ&)
//       This is the assignment operator.
//
//     int processMessages(int)
//       Entry point for processing report messages on the report queue. Each call
//       processes one message so this method can be called from a loop.
//
//     const char* getLogDirectory() const
//     const char* getLogFile() const
//     long        getWaitInterval() const
//       Inline functions to retrieve the values of various class member data.
//
//     void setWaitInterval(long)
//       Sets the time DSOIReport remains alive waiting for more messages to be put
//       into the report queue.
//
//   PROTECTED
//     None
//
//   PRIVATE
//     const char* buildFullFileName () const
//       Builds full relative path to the report file.
//
//     void createDirectories()
//       Creates the relative path to the report file.
//
//     const char* getCurrentDate(const char* ="") const
//       Gets the current date formatted according to the input string. See strftime()
//       for allowable formats.
//
//     const char* getCurrentTime(const char* ="") const
//       Gets the current time formatted according to the input string. See strftime()
//       for allowable formats.
//
//     char* getMessage(int)
//       Retrieves a message from the report queue.
//
//     void initClassMembers()
//       Allocates memory for variables used in processing messages from the report
//       queue. This method is called by the constructor.
//
//     int makeDirectory (const char*)
//       Creates directories specified by input. Builds intermediate directories
//       if nonexistent. Uses system() call.
//
//     void logMessage(const char*)
//       Logs report message to the report file.
//
//     void readConfigFile()
//       Reads the DSOIReport configuration file for the relative path to the report
//       directory and for the report filename.
//
// Members:
//
//   PUBLIC
//   PROTECTED
//     None
//
//   PRIVATE
//     Note that all the class members are pointers except for the DSOIerrorHandler object.
//     When the parameterless constructor is used, memory is not allocated for the points
//     and so the resulting DSOIreportMQ object is lean.
//
//     DSOIerrorHandler _errorHandler
//       Used to print errors encountered in DSOIreportMQ to standard error.
//
//     DSOIreceiveMQ* _receive
//       Used to get messages from the report queue.
//
//     DSOIsendMQ* _errorSendQ
//       Used as a parameter to DSOIreceiveMQ calls.
//
//     char* _logDirectory
//       Holds relative path to the report file.
//
//     char* _logFile
//       Holds report filename.
//
//     long _waitInterval
//       Time DSOIReport remains alive waiting for more messages to be put into
//       the report queue.
//
////////////////////////////////////////////////////////////////////////////////

#endif // DSOIreportMQ_H

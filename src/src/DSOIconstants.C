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
// File: DSOIconstants.C
//
// Author(s): Joo Ahn Lee
//
// Description: See header file for description.
//
//------------------------------------------------------------------------------
// Check-In History:
/*
 * $Log: DSOIconstants.C,v $
 * Revision 1.1  2008-01-31 13:47:23-07  cmsadmin
 * CR#333:A:Initial checkin for dev_11_00 from Julie and Revathi tar file, JLE
 *

   Rev 1.4   Aug 03 2000 16:38:38   bxcox
Project: Swift - Change Requests
Record(s): 3567
aCC compiler upgrade
#include <cstdlib>   // rather than <stdlib.h>
#include <new>
new(nothrow)
const char * foo = "string";
(char *)foo   // downcast for a fn() that takes a char *

   Rev 1.3   Apr 07 1999 17:42:02   jwschae
Stripped Line Feeds
 * 
 *    Rev 1.2   07 Apr 1999 16:34:50   pvcs
 * CR#2327:M:create libDSOIMQ.a
 * Revision 1.2  1998/02/25 22:52:30  lparks
 * CR#2327:M:create libDSOIMQ.a
 *
Revision 1.1  97/10/03  10:21:00  10:21:00  jalee3 (Joo Lee)
Initial revision

Revision 1.1  97/09/25  14:42:53  14:42:53  jalee3 (Joo Lee)
Initial revision

 *
 *
 */
//
//------------------------------------------------------------------------------

#include "DSOIconstants.H"

// Used for error message processing
const char DSOIstrings::CRITICAL                     []="CRITICAL";
const char DSOIstrings::DEFAULT_ALARM_DIRECTORY      []="../logfiles/alarm";
const char DSOIstrings::DEFAULT_ALARM_FILE           []="DSOI_alarm.log";
const char DSOIstrings::DEFAULT_NONCRITICAL_DIRECTORY[]="../logfiles/noncritical";
const char DSOIstrings::DEFAULT_NONCRITICAL_FILE     []="DSOI_error.log";
const char DSOIstrings::ERROR_Q                      []="QL_DSOI_ERRORQ";
const char DSOIstrings::INFORMATIONAL                []="INFORMATIONAL";
const char DSOIstrings::LOG_CONFIG_FILE              []="../etc/DSOILog.config";
const char DSOIstrings::MAJOR                        []="MAJOR";
const char DSOIstrings::MINOR                        []="MINOR";
const char DSOIstrings::NONCRITICAL                  []="NONCRITICAL";

// Used for report message processing
const char DSOIstrings::DEFAULT_REPORT_DIRECTORY[]="../logfiles/report";
const char DSOIstrings::DEFAULT_REPORT_FILE     []="DSOI_report.log";
const char DSOIstrings::REPORT_Q                []="QL_DSOI_REPORTQ";
const char DSOIstrings::REPORT_CONFIG_FILE      []="../etc/DSOIReport.config";

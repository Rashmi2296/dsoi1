#ifndef DSOIconstants_H
#define DSOIconstants_H

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
// File: DSOIconstants.H
//
// Author(s): Joo Ahn Lee
//
// Description:
//
//------------------------------------------------------------------------------
// Check-In History:
/*
 * $Log:   V:/dsoi/src/inc/DSOIconstants.H-arc  $

   Rev 1.6   Aug 03 2000 16:24:32   bxcox
Project: Swift - Change Requests
Record(s): 3567
aCC compiler upgrade
aCC compiler upgrade

   Rev 1.5   Jul 27 2000 08:23:34   rfuruta
Redesigned DSOI for 4P

   Rev 1.4   Apr 06 2000 14:38:30   rfuruta
Project: Swift - Change Requests
Record(s): 3123
Update DSOI Request Routing for the 4P
DSOIrouteMQ.H is obsolete...  MQConnect was removed
from DSOIrouteMQ.H and put into DSOIconnectMQ.H..
The other functions from DSOIrouteMQ.H were moved
to DSOIdistribute.H, DSOIroute.H, and DSOItoolbox.H.

   Rev 1.3   Apr 07 1999 17:35:48   jwschae
Stripped Line Feeds
 * 
 *    Rev 1.2   07 Apr 1999 16:19:52   pvcs
 * CR#2327:M:create libDSOIMQ.a
 * Revision 1.2  1998/02/25 22:56:53  lparks
 * CR#2327:M:create libDSOIMQ.a
 *
Revision 1.1  97/10/03  10:17:57  10:17:57  jalee3 (Joo Lee)
Initial revision

Revision 1.1  97/09/25  14:41:55  14:41:55  jalee3 (Joo Lee)
Initial revision

 *
 *
 */
//
//------------------------------------------------------------------------------

// General constants
const int DSOI_STRING_7   =   7;
const int DSOI_STRING_8   =   8;
const int DSOI_STRING_9   =   9;
const int DSOI_STRING_15  =  15;
const int DSOI_STRING_16  =  16;
const int DSOI_STRING_25  =  25;
const int DSOI_STRING_28  =  28;
const int DSOI_STRING_29  =  29;
const int DSOI_STRING_32  =  32;
const int DSOI_STRING_64  =  64;
const int DSOI_STRING_128 = 128;
const int DSOI_STRING_256 = 256;
const int DSOI_STRING_512 = 512;
const int DSOI_STRING_1024 = 1024;

const int DSOI_ARCHIVE_START=020000; // 2 am, hhmmss format
const int DSOI_ARCHIVE_END=040000;   // 4 am

const int FALSE = 0;
const int TRUE  = 1;

const int DEFAULT_LOG_WAIT_INTERVAL = 60000; // millisec = 1 minute
const int DEFAULT_REPORT_WAIT_INTERVAL = 60000; // millisec = 1 minute

// Global string constants
class DSOIstrings {
public:
	// Used for error message processing
	static const char CRITICAL          [DSOI_STRING_16];
	static const char DEFAULT_ALARM_FILE           [DSOI_STRING_16];
	static const char DEFAULT_ALARM_DIRECTORY      [DSOI_STRING_64];
	static const char DEFAULT_NONCRITICAL_FILE     [DSOI_STRING_16];
	static const char DEFAULT_NONCRITICAL_DIRECTORY[DSOI_STRING_64];
	static const char ERROR_Q           [DSOI_STRING_16];
	static const char INFORMATIONAL     [DSOI_STRING_16];
	static const char LOG_CONFIG_FILE   [DSOI_STRING_32];
	static const char MAJOR             [DSOI_STRING_8];
	static const char MINOR             [DSOI_STRING_8];
	static const char NONCRITICAL       [DSOI_STRING_16];

	// Used for report message processing
	static const char DEFAULT_REPORT_FILE     [DSOI_STRING_16];
	static const char DEFAULT_REPORT_DIRECTORY[DSOI_STRING_64];
	static const char REPORT_CONFIG_FILE[DSOI_STRING_32];
	static const char REPORT_Q          [DSOI_STRING_16];
};

#endif // DSOIconstants_H

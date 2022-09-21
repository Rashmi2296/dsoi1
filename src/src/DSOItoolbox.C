//////////////////////////////////////////////////////
//  FILE:  DSOItoolbox.C
//
//  PURPOSE: Contains all the functions for the DSOItoolbox.
//  
//  REVISION HISTORY:
//-----------------------------------------------------------------
//  version	date        author     description
//-----------------------------------------------------------------
//  7.00.00	12/23/99    rfuruta    initial development
//  7.01.00	07/03/00    rfuruta    CR 4275
//  7.01.00     08/20/00    rfuruta    CR 3452, date rtns for computing purge date
//  8.00.00     08/23/00    rfuruta    CR  339, added original request to report line
//  8.00.00     08/23/00    rfuruta    CR 2848, expanded report detail for both DSOIRequest and DSOIReply
//  8.00.00     08/23/00    rfuruta    CR 4767, remove 4P stuff
//  8.03.00     06/07/01    rfuruta    CR 5391, add LSRID to report
//  8.03.00     07/25/01    rfuruta    CR 6781, Select Section from service order
//  8.09.00     03/22/03    rfuruta    CR 8180, fix memory leaks
//  9.02.00     09/22/03    rfuruta    CR 8582, return SopId to requesting application
//  9.03.00     02/11/04    rfuruta    CR 8911, setExpiry time
//  9.04.00     02/08/04    rfuruta    CR 8948, write 'IQ' txns to separate queue
//  9.04.00     03/10/04    rfuruta    CR 9039, insert 'OWN' info to logfile rpt
//  9.10.00     05/16/05    rfuruta    CR 9898, new function 'RQ' for RSOLAR
//  9.11.00     07/26/05    rfuruta    CR10186, write 'CL' txns to separate queue
// 10.02.00     11/29/05    rfuruta    CR10269, write 'DX' txns to separate queue
// 10.04.02     08/16/07    rfuruta    CR11357, fix data conversion problem with the RQ function
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOItoolbox.H"
#include "DSOIThrowError.H"
#include <cstdio>   
#include <cstddef>
#include <ctime>
#include <cctype>
#include <new>
//#include <sys/stat.h>
//#include <unistd.h>

////////////////////////////////////////////////
//  FUNCTION:
//            calculateTime()
// 
//  PURPOSE: Calculates the time of transaction for the router
//
//  METHOD:  Uses the UNIX gettimeofday() function to calculate the time
//	     in seconds and microseconds.
//
//  PARAMETERS:
//
//  OTHER OUTPUTS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////
void calculateTime (char* Time)
{
	static int timeSize = _sizeOfTime;
	struct timeval millTime;
	struct timezone timeZone;

	// get the time in seconds and microseconds since the Epoch
	if (gettimeofday(&millTime,&timeZone) != 0)
	{
#ifdef DEBUG
cout << "the gettimeofday() errored" << errno << endl;
#endif
	}

	// reset Time and convert time in seconds from the Epoch
	// to mountain standard time (MST) in HHMMSS format
	memset(Time,0,_sizeOfTime);
	strftime( Time, timeSize, "%H%M%S", localtime(&millTime.tv_sec) );

	// attach the microseconds after seconds in Time
	static char milliSec[7];
	sprintf(milliSec,"%06d",millTime.tv_usec);
	strncat(Time,milliSec,6);

#ifdef DEBUG
cout << "Time = " << Time << endl;
#endif
}

////////////////////////////////////////////////
//  FUNCTION:
//            convertTime()
// 
//  PURPOSE: MQ uses Greenwich Mean Time. There is a 6 hour difference between
//           GMT and mountain time.  Also have to account for daylight savings.
//  METHOD: 
//
//  PARAMETERS:
//
//  OTHER OUTPUTS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////
void convertTime (char* ApplTime)
{
	time_t timeNow = time(&timeNow);
	struct tm* timeNowStruc = localtime(&timeNow);
#ifdef DEBUG
cout << "the tm structure for tm_hour= " << timeNowStruc->tm_hour << endl;
cout << "the tm structure for tm_isdst= " << timeNowStruc->tm_isdst << endl;
#endif
	int timeInt;
	char timeHold[3];
	timeHold[0] = ApplTime[0]; //get the first two digits of the ApplTime
	timeHold[1] = ApplTime[1]; // to convert to Mountain time ( no need to
	timeHold[2] = '\0';  		// convert the seconds.
	timeInt = atoi(timeHold);
#ifdef DEBUG
cout << "timeInt before = " << timeInt << endl;
#endif
	if(timeNowStruc->tm_isdst >0) //daylight savings time is in effect
	{
		if(timeInt > 0  && timeInt < 7)
		{
			timeInt +=18;
		}
	 	else
			timeInt -=6;
	}
	else
	{
		if(timeInt > 0 && timeInt < 8)
		{
			timeInt +=17;
		}
	 	else
			timeInt -=7;
	}
#ifdef DEBUG
cout << "timeInt after = " << timeInt << endl;
#endif
	sprintf(timeHold,"%02d", timeInt);
	strncpy(ApplTime,timeHold,2);
#ifdef DEBUG
cout << "converted time= " << ApplTime << endl;
#endif

}

////////////////////////////////////////////////
//  FUNCTION:
//            calculateDate()
//
//  PURPOSE: Calculates the date of transaction for the router
//
//  METHOD:  Uses the UNIX time function to calculate the date
//
//  PARAMETERS:
//
//  OTHER OUTPUTS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////
void calculateDate (char* Date)
{
	time_t timeNow;
	static int dateSize = _sizeOfDate;
	time(&timeNow);
	strftime(Date, dateSize, "%m%d%Y", localtime(&timeNow) );
}
////////////////////////////////////////////////
//  FUNCTION:
//            formatReport()
//
//  PURPOSE: Calculates the date of transaction for the router
//
//
//  METHOD:  Uses the UNIX time function to calculate the date
//
//  PARAMETERS:
//
//  OTHER OUTPUTS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////
void formatReport(char* rptMsg,
                const char* Date,
		const char* sendTime,
		const char* getTime,
		const char* dsoiAppl,
		const char* putAppl,
		const char* putApplTime,
		const char* putApplDate,
		const unsigned char* msgId,
		const char* msg)
{
	sprintf( rptMsg,
	"%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c%s",
		"Date=",Date,0xff,
		"sendTime=",sendTime,0xff,
		"getTime=",getTime,0xff,
		"DsoiAppl=", dsoiAppl,0xff,
		"PutAppl=", putAppl, 0xff,
		"PutApplTime=", putApplTime, 0xff,
		"PutApplDate=", putApplDate, 0xff,
		"MsgId=");
// the msgId must be converted to a string of decimal values
// since they are a string of bits!
	char buffer[49];
	char ptr[4];
	memset(buffer,0,49);
	for(int i=0; i < 24; i++)
	{
		memset(ptr,0,4);
		ptr[0] = '\0';
		int returnCode = sprintf(ptr,"%d",msgId[i]);
		ptr[strlen(ptr)] = '\0';
                if (returnCode > 0)
			strcat(buffer, ptr);
                else
			break;
	}
	const char * hexName = "\xff";
	strcat(rptMsg,buffer);
	strcat(rptMsg,hexName);
	strcat(rptMsg,msg);
	strcat(rptMsg,hexName);
#ifdef DEBUG
cout << "in formatReport, ReportLine= " << rptMsg  << endl;
#endif
}

////////////////////////////////////////////////
//  FUNCTION:int findTagValue
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char* msg - message to parse 
//              char* tagType 
//              char* tagValue
//  
//  OTHER OUTPUTS: If tagType is found in msg,  sets the tagValue.
//      If there is an error in the message sent from the
//	Reply application.  
//  
//  OTHER INPUTS:
//
//  RETURNS: -1 if the tagType is not found, zero otherwise.
// 
//  FUNCTIONS USED: 
//		strcpy(), strcat(), sprintf(), strstr(), strncpy()
//
//////////////////////////////////////////////////////
int findTagValue(char* msg, const char* tagType, char *tagValue)
{
#ifdef DEBUG
cout << "in findTagValue, SearchItem= " << tagType  << endl;
#endif
	char* msgPtr = msg;
	char* equalPtr;
	char* hexPtr;
	int size =0;
        msgPtr = msg;
        // test  tagValue[0] = '\0';
	msgPtr = strstr(msgPtr,tagType); //locate tagType
	if(msgPtr == NULL) 
	{
	  return -1;
	}
	equalPtr = strchr(msgPtr,'=');  //locate equal sign
	if(equalPtr == NULL)
	{
	  return -1;
	}
	equalPtr++;
	hexPtr = strchr(msgPtr,'\xff');  //locate delimiter xff
	if(hexPtr == NULL) 
	{
	  return -1;
	}
 	size = (hexPtr-equalPtr);
        
	if(size <= 0 )
	{
	  return -1;
	}
	strncpy(tagValue,equalPtr,size);
	tagValue[size] = '\0';

#ifdef DEBUG
cout << "tagType=" << tagType << "  size=" << size << "  tagValue=" << tagValue << endl;
#endif
        return 0;
}

////////////////////////////////////////////////
//  FUNCTION: const char* DSOIrouteRequest::binarySearch
//                                      (const char**  msgToSearch,
//                                      const char* item, const int size,
//                                      const int compareSize)
//
//  PURPOSE: To find the item in the msgToSearch.  This uses
//      the binary search method.
//
//  METHOD: Binary search.
//
//  PARAMETERS: const char** msgToSearch - a two dimensional array
//                              that holds the string to search.
//              const char* item - the item to look for
//              const int size - the size of the array to search
//              const int compareSize - the size of the item to search
//
//  RETURNS: returns the item found or NULL if the item is not found.
//
//  FUNCTIONS USED: strncmp()
//
//////////////////////////////////////////////////////
char* binarySearch(char** msgToSearch, char* item,
                   int size, int compareSize)
{
#ifdef DEBUG
cout << "in binarySearch, SearchItem= " << item  << endl;
#endif
       int midIndex = 0;
       int begIndex = 0;
       int endIndex = size-1;

       int returnCode = 0;
       int found = 0;
       while (endIndex >= begIndex)
         {
           midIndex = (begIndex + endIndex) / 2;
           returnCode = strncmp(msgToSearch[midIndex],item,compareSize);
           if (returnCode > 0 )
             {
               endIndex = midIndex-1;
             }
           else if (returnCode == 0)
                  {
                    found = 1;
                    begIndex = 0;
                    endIndex = -1;
                  }
                else
                  {
                    begIndex = midIndex + 1;
                  }
         } //end of while

#ifdef DEBUG
cout << "SearchValue=" <<  msgToSearch[midIndex]   << endl;
#endif
       if (found)    return (msgToSearch[midIndex]);
       return NULL;
}

////////////////////////////////////////////////
//  FUNCTION:int getFidValue
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char* msg - message to parse 
//              char* tagType 
//              char* tagValue
//  
//  OTHER OUTPUTS: If tagType is found in msg,  sets the tagValue.
//      If there is an error in the message sent from the
//	Reply application.  
//  
//  OTHER INPUTS:
//
//  RETURNS: -1 if the tagType is not found, zero otherwise.
// 
//  FUNCTIONS USED: 
//		strcpy(), strcat(), sprintf(), strstr(), strncpy()
//
//////////////////////////////////////////////////////
int getFidValue(char* msg, char* tagType, char *tagValue)
{
#ifdef DEBUG
cout << "in getFidValue, FidName= " << tagType  << endl;
#endif
	char* msgPtr = msg;
	char* startPtr;
	char* endPtr;
	int size =0;
        msgPtr = msg;
        tagValue[0] = '\0';
	msgPtr = strstr(msgPtr,tagType); //locate tagType
	if(msgPtr == NULL) 
	{
	  return -1;
	}
	startPtr = msgPtr + 9;
	endPtr = strchr(msgPtr,'\n');  //locate delimiter xff
	if(endPtr == NULL) 
	{
	  return -1;
	}
 	size = (endPtr-startPtr);
        
	if(size <= 0 )
	{
	  return -1;
	}
	strncpy(tagValue,startPtr,size);
	tagValue[size] = '\0';

#ifdef DEBUG
cout << "tagType=" << tagType << "  size=" << size << "  tagValue=" << tagValue << endl;
#endif
        return 0;
}

////////////////////////////////////////////////
//  FUNCTION:int getSection
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMERERS: char* sectionName
//              char* srvOrdr - message to parse 
//  
//  OTHER OUTPUTS: If sectionName is found in srvOrdr,  
//                 parse out the sectionData.
//  
//  OTHER INPUTS:
//
//  RETURNS: -1 if the section is not found, zero otherwise.
// 
//  FUNCTIONS USED: 
//		strcpy(), strcat(), sprintf(), strstr(), strncpy()
//
//////////////////////////////////////////////////////
int getSection(char* sectionName, char* srvOrdr)
{
   const char * tblPtr;
   const char * tblPtr1;
   int  tblIdx=99;
   const char* sectionTable[]= {"04S IDEX","03S ID"  ,"05S LIST" ,"06S CTL", 
                                "07S DIR" ,"08S TFC" ,"09S BILL" ,"11S RMKS",
                                "10S S&E" ,"12S STAT","14S ZASGM","13S ASGM",
                                "15S ERR" ,"02S HDR"};
   static char* msg;
   static char* msg2;
   static char* sectionData = new char[strlen(srvOrdr)];
   static int   ptrMsg;
   static int   tokenLen;
   static int   msgLen;
   static char token[200];
   static char sectionNo[4];
   static char hex0A[2];
   char* str_ptr;

#ifdef DEBUG
cout << "in getSection, querySectionName= " << sectionName  << endl;
#endif
         hex0A[0] = '\x0a';
         hex0A[1] = '\0';
         sectionNo[0]    = '\0';
         for(int i=0; i<=14; i++)
            { tblPtr = sectionTable[i] + 4;
              tblPtr1= sectionTable[i];
              if (strstr(sectionName, tblPtr) != 0)
                { tblIdx = i;
                  sectionNo[0] = tblPtr1[0];
                  sectionNo[1] = tblPtr1[1];
                  i = 99;
                }
            }
         memset(sectionData,0,strlen(srvOrdr));
         sectionData[0]='\0';
         if ((str_ptr = strstr(srvOrdr,"ServiceOrder=")) != NULL)
           {
             strncpy(sectionData, srvOrdr,  str_ptr - srvOrdr);
             strncat(sectionData, "ServiceOrder=",13);
           }

         if ((msg=strstr(srvOrdr,sectionTable[tblIdx])) != NULL)
           { ptrMsg=(msg - srvOrdr) - 13;
             msg=srvOrdr + ptrMsg;
             msgLen = 0;
             while (0 < 1)
               {
                 if (msg != NULL)
                   { if ((msg[13] == sectionNo[0]) && (msg[14] == sectionNo[1]))
                       { msg = msg + 1;
                         msg2=strstr(msg, hex0A);
                         for (int k=0; k<100; k++)
                           { if (msg[k] != '\x0a')
			       { tokenLen = k;
                               }
                             else 
                               { k = 999;
			         tokenLen = tokenLen + 1;
                               }
                           }
                         strncat(sectionData,msg,tokenLen);
                         msgLen = msgLen + tokenLen + 1;
                      // strcat(sectionData,"\n");
                         strcat(sectionData,"\x0a");
                         msg = msg2;
                       }
                     else
                       { strcat(sectionData,"\xff");
                         memset(srvOrdr,0,1048576);
                         srvOrdr[0]='\0';
                         strcpy(srvOrdr,sectionData);
                         delete sectionData;
                         return 0;
                       }
                   }
                 else { delete sectionData;  return 1; }
               }
           }
         else { delete sectionData;  return 1; }

         delete sectionData;
         return 1;
}

////////////////////////////////////////////////
//  FUNCTION: int routeToSop
//
//  PURPOSE: gets the remote queue for a specific SopID
//
//  METHOD:
//
//  PARAMETERS: (char* SopId, char* QueueName)
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
int routeToSop(char* SopId, char* ReqName)
{
   const char * msgPtr;
   int  length;
   const char * RequestQ[]  = { "ONSR QR_DSOI_ONS_REQUEST",
				"RSOD QR_DSOI_RSOLAR_GI_DATA_REQUEST",
				"RSOA QR_DSOI_RSOLAR_GI_DATA_AT_REQUEST",
				"RSOQ QR_DSOI_RSOLAR_GI_QUERY_REQUEST",
				"RSOI QR_DSOI_RSOLAR_GI_IQ_REQUEST",
				"RSOC QR_DSOI_RSOLAR_GI_CL_REQUEST",
				"RSOX QR_DSOI_RSOLAR_GI_DX_REQUEST",
				"ESSD QR_DSOI_SOLAR_XJ_DATA_REQUEST",
				"ESSA QR_DSOI_SOLAR_XJ_DATA_AT_REQUEST",
				"ESSQ QR_DSOI_SOLAR_XJ_QUERY_REQUEST",
				"ESSI QR_DSOI_SOLAR_XJ_IQ_REQUEST",
				"ESSC QR_DSOI_SOLAR_XJ_CL_REQUEST",
				"ESSX QR_DSOI_SOLAR_XJ_DX_REQUEST",
				"ESND QR_DSOI_SOLAR_XK_DATA_REQUEST",
				"ESNA QR_DSOI_SOLAR_XK_DATA_AT_REQUEST",
				"ESNQ QR_DSOI_SOLAR_XK_QUERY_REQUEST",
				"ESNI QR_DSOI_SOLAR_XK_IQ_REQUEST",
				"ESNC QR_DSOI_SOLAR_XK_CL_REQUEST",
				"ESNX QR_DSOI_SOLAR_XK_DX_REQUEST",
				"SOUD QR_DSOI_SOPAD_QS_DATA_REQUEST",
				"SOUA QR_DSOI_SOPAD_QS_DATA_AT_REQUEST",
				"SOUQ QR_DSOI_SOPAD_QS_QUERY_REQUEST",
				"SOUI QR_DSOI_SOPAD_QS_IQ_REQUEST",
				"SOUC QR_DSOI_SOPAD_QS_CL_REQUEST",
				"SOUX QR_DSOI_SOPAD_QS_DX_REQUEST",
				"SOCD QR_DSOI_SOPAD_RN_DATA_REQUEST",
				"SOCA QR_DSOI_SOPAD_RN_DATA_AT_REQUEST",
				"SOCQ QR_DSOI_SOPAD_RN_QUERY_REQUEST",
				"SOCI QR_DSOI_SOPAD_RN_IQ_REQUEST",
				"SOCC QR_DSOI_SOPAD_RN_CL_REQUEST",
				"SOCX QR_DSOI_SOPAD_RN_DX_REQUEST" };
#ifdef DEBUG
cout << "in routeToSop, SopId= " << SopId  << endl;
#endif

         for(int i=0; i<=31; i++)
            { length = strlen(RequestQ[i]) - 5;
              msgPtr = RequestQ[i];
              if (strncmp(msgPtr, SopId,4) == 0)
                { msgPtr = msgPtr + 5;
                  strncpy(ReqName, msgPtr, length);
                  ReqName[length] = '\0';
                  return 0;
                }
            }

#ifdef DEBUG
cout << "RequestQueueName=" <<  ReqName << endl;
#endif
         return 1;

} //end of routeToSop()


////////////////////////////////////////////////
//  FUNCTION: int replyToSop
//
//  PURPOSE: gets the reply queue for a specific SopID
//
//  METHOD:
//
//  PARAMETERS: (char* SopId, char* QueueName)
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
int replyToSop(char* SopId, char* ReplyName)
{
   const char * msgPtr;
   int  length;
   const char * ReplyQ[] = {	"ONSR QL_ONS_DSOI_REPLY",
				"RSOD QL_RSOLAR_GI_DSOI_DATA_REPLY",
				"RSOA QL_RSOLAR_GI_AT_DSOI_DATA_REPLY",
				"RSOQ QL_RSOLAR_GI_DSOI_QUERY_REPLY",
				"RSOI QL_RSOLAR_GI_DSOI_QUERY_REPLY",
				"RSOC QL_RSOLAR_GI_DSOI_QUERY_REPLY",
				"RSOX QL_RSOLAR_GI_DSOI_QUERY_REPLY",
				"ESSD QL_SOLAR_XJ_DSOI_DATA_REPLY",
				"ESSA QL_SOLAR_XJ_AT_DSOI_DATA_REPLY",
				"ESSQ QL_SOLAR_XJ_DSOI_QUERY_REPLY",
				"ESSI QL_SOLAR_XJ_DSOI_QUERY_REPLY",
				"ESSC QL_SOLAR_XJ_DSOI_QUERY_REPLY",
				"ESSX QL_SOLAR_XJ_DSOI_QUERY_REPLY",
				"ESND QL_SOLAR_XK_DSOI_DATA_REPLY",
				"ESNA QL_SOLAR_XK_AT_DSOI_DATA_REPLY",
				"ESNQ QL_SOLAR_XK_DSOI_QUERY_REPLY",
				"ESNI QL_SOLAR_XK_DSOI_QUERY_REPLY",
				"ESNC QL_SOLAR_XK_DSOI_QUERY_REPLY",
				"ESNX QL_SOLAR_XK_DSOI_QUERY_REPLY",
				"SOUD QL_SOPAD_QS_DSOI_DATA_REPLY",
				"SOUA QL_SOPAD_QS_AT_DSOI_DATA_REPLY",
				"SOUQ QL_SOPAD_QS_DSOI_QUERY_REPLY",
				"SOUI QL_SOPAD_QS_DSOI_QUERY_REPLY",
				"SOUC QL_SOPAD_QS_DSOI_QUERY_REPLY",
				"SOUX QL_SOPAD_QS_DSOI_QUERY_REPLY",
				"SOCD QL_SOPAD_RN_DSOI_DATA_REPLY",
				"SOCA QL_SOPAD_RN_AT_DSOI_DATA_REPLY",
				"SOCQ QL_SOPAD_RN_DSOI_QUERY_REPLY",
				"SOCI QL_SOPAD_RN_DSOI_QUERY_REPLY",
				"SOCC QL_SOPAD_RN_DSOI_QUERY_REPLY",
				"SOCX QL_SOPAD_RN_DSOI_QUERY_REPLY" };
#ifdef DEBUG
cout << "in replyToSop, SopId= " << SopId  << endl;
#endif

         for(int i=0; i<=31; i++)
            { length = strlen(ReplyQ[i]) - 5;
              msgPtr = ReplyQ[i];
              if (strncmp(msgPtr, SopId,4) == 0)
                { msgPtr = msgPtr + 5;
                  strncpy(ReplyName, msgPtr, length);
                  ReplyName[length] = '\0';
                  return 0;
                }
            }

#ifdef DEBUG
cout << "ReplyQueueName=" <<  ReplyName << endl;
#endif
         return 1;

} //end of replyToSop()


////////////////////////////////////////////////
//  FUNCTION: int requestToDSOI
//
//  PURPOSE: gets the request queue for a specific SopID
//
//  METHOD:
//
//  PARAMETERS: (char* SopId, char* QueueName)
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
int requestToDSOI(char* SopId, char* RequestName)
{
   const char * msgPtr;
   int  length;
   const char * RequestQ[] = {  "SOL  QR_DSOI_SOL_4P_QUERY_REQUEST",
				"N4P  QL_DSOIROUTE_DSOI_4RS_REQUEST",
				"4RS  QL_DSOIROUTE_DSOI_4RS_REQUEST",
				"RSO  QL_DSOIROUTE_DSOI_RSO_REQUEST",
				"ESS  QL_DSOIROUTE_DSOI_ESS_REQUEST",
				"ESN  QL_DSOIROUTE_DSOI_ESN_REQUEST",
				"SOU  QL_DSOIROUTE_DSOI_SOU_REQUEST",
				"SOC  QL_DSOIROUTE_DSOI_SOC_REQUEST" };
#ifdef DEBUG
cout << "in requestToDSOI, SopId= " << SopId  << endl;
#endif

         for(int i=0; i<=8; i++)
            { length = strlen(RequestQ[i]) - 5;
              msgPtr = RequestQ[i];
              if (strstr(msgPtr, SopId) != NULL)
                { msgPtr = msgPtr + 5;
                  strncpy(RequestName, msgPtr, length);
                  RequestName[length] = '\0';
                  return 0;
                }
            }

#ifdef DEBUG
cout << "requestToDSOIqueueName=" <<  RequestName << endl;
#endif
         return 1;

} //end of requestToDSOI()

////////////////////////////////////////////////
//  FUNCTION: int buildMsgTrunc
//
//  PURPOSE: builds msgTrunc for DSOI report log
//
//  METHOD:
//
//  PARAMETERS: (char* sopId, char* holdMessage, char* newMessage, char* msgTrunc)
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
void buildMsgTrunc (char* sopId, char* holdMessage, char* newMessage, char* msgTrunc)
{
	const char * OWNTag = "OWN=";
	char  OWNVal[2];
	const char * SopIdTag = "SopId=";
	char  SopIdVal[4];
	const char * FunctionCodeTag = "FunctionCode=";
	char  FunctionCodeVal[3];
	const char * UserIDTag = "UserID=";
	char  UserIDVal[9];
	const char * LSRIDTag = "LSRID=";
	char  LSRIDVal[11];
	const char * NPANXXTag = "NPANXX=";
	char  NPANXXVal[7];
	const char * OrderTypeTag = "OrderType=";
	char  OrderTypeVal[3];
	const char * LockTag = "Lock=";
	char  LockVal[2];
	const char * OrderNumberTag = "OrderNumber=";
	char  OrderNumberVal[9];
	const char * QueryListTypeTag = "QueryListType=";
	char  QueryListTypeVal[3];
	const char * QueryDataTag = "QueryData=";
	char  QueryDataVal[46];
	const char * SbnQtyTag = "SbnQty=";
	char  SbnQtyVal[3];
	const char * SbnTypeTag = "SbnType=";
	char  SbnTypeVal[2];
	const char * ExchangeTag = "Exchange=";
	char  ExchangeVal[5];
	const char * CustNameTag = "CustName=";
	char  CustNameVal[6];
	const char * ClassSvcTag = "ClassSvc=";
	char  ClassSvcVal[6];
	const char * PrivateLineTag = "PrivateLine=";
	char  PrivateLineVal[2];
	const char * ServiceCodeTag = "ServiceCode=";
	char  ServiceCodeVal[5];
	const char * CiPrefixTag = "CiPrefix=";
	char  CiPrefixVal[3];
	const char * CiQtyTag = "CiQty=";
	char  CiQtyVal[3];
	const char * QuerySectionTag = "QuerySection=";
	char  QuerySectionVal[6];
	
        const char * ReturnCodeTag = "ReturnCode=";
	char  ReturnCodeVal[3];
	const char * ErrorMessageCountTag = "ErrorMessageCount=";
	char  ErrorMessageCountVal[4];
	const char * ErrorMessageTag = "ErrorMessage=";
	char  ErrorMessageVal[80];
	const char * CandidateCountTag = "CandidateCount=";
	char  CandidateCountVal[5];
	const char * CandidateListTag = "CandidateList=";
	char  CandidateListVal[100];
	const char * ONSMessageTag = "ONSMessage=";
	char  ONSMessageVal[86];

	const char * RQMSGTag  = "RQMSG=";
	const char * RQDATATag = "RQDATA=";
        
	char * tagValue = new char[90];

#ifdef DEBUG
cout << "   sopId=" <<  sopId << endl;
cout << "   holdmsg=" <<  holdMessage << endl;
cout << "    newmsg=" <<  newMessage  << endl;
#endif

        memset(FunctionCodeVal,0,3);
        memset(OWNVal,0,2);
        memset(SopIdVal,0,4);
        memset(UserIDVal,0,9);
        memset(LSRIDVal,0,11);
        memset(NPANXXVal,0,7);
        memset(OrderTypeVal,0,3);
        memset(OrderNumberVal,0,9);
        memset(LockVal,0,2);
        memset(QueryListTypeVal,0,3);
        memset(QueryDataVal,0,46);
	memset(SbnQtyVal,0,3);
	memset(SbnTypeVal,0,2);
	memset(ExchangeVal,0,5);
	memset(CustNameVal,0,6);
	memset(ClassSvcVal,0,6);
	memset(PrivateLineVal,0,2);
	memset(ServiceCodeVal,0,5);
	memset(CiPrefixVal,0,3);
	memset(CiQtyVal,0,3);
	memset(QuerySectionVal,0,6);
        memset(ReturnCodeVal,0,3);
        memset(ErrorMessageCountVal,0,4);
        memset(ErrorMessageVal,0,80);
        memset(CandidateCountVal,0,5);
        memset(ONSMessageVal,0,86);

        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)OWNTag, tagValue) == 0)
          { strcpy(OWNVal, tagValue);
	    strncat(msgTrunc,"OWN=",4);
	    strncat(msgTrunc,OWNVal,1);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)SopIdTag, tagValue) == 0)
          { strcpy(SopIdVal, tagValue);
	    strncat(msgTrunc,"SopId=",6);
	    strncat(msgTrunc,SopIdVal,3);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)NPANXXTag, tagValue) == 0)
          { strcpy(NPANXXVal, tagValue);
            strncat(msgTrunc,"NPANXX=",7);
            strncat(msgTrunc,NPANXXVal,6);
            strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)FunctionCodeTag, tagValue) == 0)
          { strcpy(FunctionCodeVal, tagValue);
            strncat(msgTrunc,"FunctionCode=",13);
            strncat(msgTrunc,FunctionCodeVal,2);
            strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)UserIDTag, tagValue) == 0)
          { strcpy(UserIDVal, tagValue);
	    strncat(msgTrunc,"UserID=",7);
	    strncat(msgTrunc,UserIDVal,8);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)LSRIDTag, tagValue) == 0)
          { strcpy(LSRIDVal, tagValue);
	    strncat(msgTrunc,"LSRID=",6);
	    strncat(msgTrunc,LSRIDVal,10);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)OrderTypeTag, tagValue) == 0)
          { strcpy(OrderTypeVal, tagValue);
            strncat(msgTrunc,"OrderType=",10);
            strcat(msgTrunc,OrderTypeVal);
            strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)OrderNumberTag, tagValue) == 0)
          { strcpy(OrderNumberVal, tagValue);
            strncat(msgTrunc,"OrderNumber=",12);
            strncat(msgTrunc,OrderNumberVal,8);
            strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)QueryListTypeTag, tagValue) == 0)
          { strcpy(QueryListTypeVal, tagValue);
            strncat(msgTrunc,"QueryListType=",14);
	    strncat(msgTrunc,QueryListTypeVal,2);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)QueryDataTag, tagValue) == 0)
          { strcpy(QueryDataVal, tagValue);
	    strncat(msgTrunc,"QueryData=",10);
	    strncat(msgTrunc,QueryDataVal,45);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)SbnQtyTag, tagValue) == 0)
          { strcpy(SbnQtyVal, tagValue);
	    strncat(msgTrunc,"SbnQty=",7);
	    strncat(msgTrunc,SbnQtyVal,2);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)SbnTypeTag, tagValue) == 0)
          { strcpy(SbnTypeVal, tagValue);
	    strncat(msgTrunc,"SbnType=",8);
	    strncat(msgTrunc,SbnTypeVal,1);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)ExchangeTag, tagValue) == 0)
          { strcpy(ExchangeVal, tagValue);
	    strncat(msgTrunc,"Exchange=",9);
	    strncat(msgTrunc,ExchangeVal,4);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)CustNameTag, tagValue) == 0)
          { strcpy(CustNameVal, tagValue);
	    strncat(msgTrunc,"CustName=",9);
	    strncat(msgTrunc,CustNameVal,5);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)ClassSvcTag, tagValue) == 0)
          { strcpy(ClassSvcVal, tagValue);
	    strncat(msgTrunc,"ClassSvc=",9);
	    strncat(msgTrunc,ClassSvcVal,5);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)PrivateLineTag, tagValue) == 0)
          { strcpy(PrivateLineVal, tagValue);
	    strncat(msgTrunc,"PrivateLine=",12);
	    strncat(msgTrunc,PrivateLineVal,1);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)ServiceCodeTag, tagValue) == 0)
          { strcpy(ServiceCodeVal, tagValue);
	    strncat(msgTrunc,"ServiceCode=",12);
	    strncat(msgTrunc,ServiceCodeVal,4);
	    strcat(msgTrunc,"\xff");
          }
	memset(CiPrefixVal,0,3);
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)CiPrefixTag, tagValue) == 0)
          { strcpy(CiPrefixVal, tagValue);
	    strncat(msgTrunc,"CiPrefix=",9);
	    strncat(msgTrunc,CiPrefixVal,2);
	    strcat(msgTrunc,"\xff");
          }
	memset(CiQtyVal,0,3);
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)CiQtyTag, tagValue) == 0)
          { strcpy(CiQtyVal, tagValue);
	    strncat(msgTrunc,"CiQty=",6);
	    strncat(msgTrunc,CiQtyVal,2);
	    strcat(msgTrunc,"\xff");
          }
	memset(QuerySectionVal,0,6);
        memset(tagValue,0,90);
        if (findTagValue(holdMessage, (char *)QuerySectionTag, tagValue) == 0)
          { strcpy(QuerySectionVal, tagValue);
	    strncat(msgTrunc,"QuerySection=",13);
	    strncat(msgTrunc,QuerySectionVal,5);
	    strcat(msgTrunc,"\xff");
          }

        memset(tagValue,0,90);
        if (findTagValue(newMessage, (char *)ReturnCodeTag, tagValue) == 0)
          { strcpy(ReturnCodeVal, tagValue);
	    strncat(msgTrunc,"ReturnCode=",11);
	    strncat(msgTrunc,ReturnCodeVal,2);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(newMessage, (char *)CandidateCountTag, tagValue) == 0)
          { strcpy(CandidateCountVal, tagValue);
	    strncat(msgTrunc,"CandidateCount=",15);
	    strncat(msgTrunc,CandidateCountVal,4);
	    strcat(msgTrunc,"\xff");
          }
        memset(tagValue,0,90);
        if (findTagValue(newMessage, (char *)ErrorMessageCountTag, tagValue) == 0)
          { strcpy(ErrorMessageCountVal, tagValue);
	    strncat(msgTrunc,"ErrorMessageCount=",18);
	    strncat(msgTrunc,ErrorMessageCountVal,3);
	    strcat(msgTrunc,"\xff");
            int ix = atoi(ErrorMessageCountVal);
            if (ix < 2)
              { memset(tagValue,0,90);
                if (findTagValue(newMessage, (char *)ErrorMessageTag, tagValue) == 0)
                  { strcpy(ErrorMessageVal, tagValue);
	            strncat(msgTrunc,"ErrorMessage=",13);
	            strncat(msgTrunc,ErrorMessageVal,80);
	            strcat(msgTrunc,"\xff");
                  }
              }
          }
        memset(tagValue,0,90);
        if (findTagValue(newMessage, (char *)ONSMessageTag, tagValue) == 0)
          { strcpy(ONSMessageVal, tagValue);
	    strncat(msgTrunc,"ONSMessage=",13);
	    strncat(msgTrunc,ONSMessageVal,85);
	    strcat(msgTrunc,"\xff");
          }


	if (strcmp(FunctionCodeVal,"RQ")==0)
          {  memset(tagValue,0,90);
             if (findTagValue(newMessage, (char *)RQMSGTag, tagValue) == 0)
  	       { strncat(msgTrunc,"RQMSG=",6);
  	         strncat(msgTrunc,tagValue,50);
  	         strcat(msgTrunc,"\xff");
               }
//           memset(tagValue,0,90);
//           if (findTagValue(newMessage, (char *)RQDATATag, tagValue) == 0)
//	       { strncat(msgTrunc,"RQDATA=",7);
//	         strncat(msgTrunc,tagValue,14);
//	         strcat(msgTrunc,"\xff");
//             }
	     strncat(msgTrunc,"RQDATA=",7);
             strcat(msgTrunc,"\xff");
          }  

	delete []tagValue;

#ifdef DEBUG
cout << "msgTrunc=" <<  msgTrunc << endl;
#endif

} //end of buildMsgTrunc()


//////////////////////////////////////////////////////////////////////
//  FUNCTION: int computeJulianDate
//
//  PURPOSE:  Computes Julian Date
//
//  METHOD:
//
//  PARAMETERS:     passed  gregorian Date (YYYYMMDD)  char*
//                  returns julian Date (DDD)          int*
//                  returns julian Year (YYYY)         char*
//                  returns error msg                  char*
//
//  OTHER OUTPUTS:
//
//  OTHER INPUTS:
//
//  RETURNS:  returns  0  for success
//                    -1  for failed
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////////////////////
int computeJulianDate(char* sourceDate, int* julianDate, char* julianYr, char* errMsg)
{
 static char sourceYY[3];
 static char sourceMM[3];
 static char sourceDD[3];
 int jDate;
 int sourceYear;
 int jan=31;   int feb=28;   int mar=31;   int apr=30;
 int may=31;   int jun=30;   int jul=31;   int aug=31;
 int sep=30;   int oct=31;   int nov=30;   int dec=31;

     julianYr[0] = sourceDate[0];
     julianYr[1] = sourceDate[1];
     julianYr[2] = sourceDate[2];
     julianYr[3] = sourceDate[3];
     julianYr[4] = '\0';
     sourceYY[0] = sourceDate[2];
     sourceYY[1] = sourceDate[3];
     sourceYY[2] = '\0';
     sourceMM[0] = sourceDate[4];
     sourceMM[1] = sourceDate[5];
     sourceMM[2] = '\0';
     sourceDD[0] = sourceDate[6];
     sourceDD[1] = sourceDate[7];
     sourceDD[2] = '\0';

     if ((atoi(sourceMM) < 1) || (atoi(sourceMM) > 13))
       { sprintf(errMsg,"%s%s","ERROR invalid GregorianDate must be in YYYYMMDD format, SourceDate=", sourceDate);
         return -1;
       }
     if ((atoi(sourceDD) < 1) || (atoi(sourceDD) > 31))
       { sprintf(errMsg,"%s%s","ERROR invalid GregorianDate must be in YYYYMMDD format, SourceDate=", sourceDate);
         return -1;
       }
     sourceYear = atoi(sourceYY);
     if ((sourceYear ==  0) ||  (sourceYear ==  4) ||
         (sourceYear ==  8) ||  (sourceYear == 12) ||
         (sourceYear == 16) ||  (sourceYear == 20) ||
         (sourceYear == 24) ||  (sourceYear == 28) ||
         (sourceYear == 32) ||  (sourceYear == 36) ||
         (sourceYear == 40) ||  (sourceYear == 44) ||
         (sourceYear == 48) ||  (sourceYear == 52) ||
         (sourceYear == 56) ||  (sourceYear == 60) ||
         (sourceYear == 64) ||  (sourceYear == 68) ||
         (sourceYear == 72) ||  (sourceYear == 76) ||
         (sourceYear == 80) ||  (sourceYear == 84) ||
         (sourceYear == 88) ||  (sourceYear == 92) ||
         (sourceYear == 96) )
           feb = 29;

     jDate = 0;
     switch (atoi(sourceMM))
       { case  1:   // January
                 jDate = atoi(sourceDD);
                 break;
         case  2:   // February
                 jDate = jan + atoi(sourceDD);
                 break;
         case  3:   // March
                 jDate = jan + feb + atoi(sourceDD);
                 break;
         case  4:   // April
                 jDate = jan + feb + mar + atoi(sourceDD);
                 break;
         case  5:   // May
                 jDate = jan + feb + mar + apr + atoi(sourceDD);
                 break;
         case  6:   // June
                 jDate = jan + feb + mar + apr + may + 
                              atoi(sourceDD);
                 break;
         case  7:   // July
                 jDate = jan + feb + mar + apr + may + jun +
                              atoi(sourceDD);
                 break;
         case  8:   // August
                 jDate = jan + feb + mar + apr + may + jun +
                              jul + atoi(sourceDD);
                 break;
         case  9:   // September
                 jDate = jan + feb + mar + apr + may + jun +
                              jul + aug + atoi(sourceDD);
                 break;
         case 10:   // October
                 jDate = jan + feb + mar + apr + may + jun +
                              jul + aug + sep + atoi(sourceDD);
                 break;
         case 11:   // November
                 jDate = jan + feb + mar + apr + may + jun +
                              jul + aug + sep + oct + atoi(sourceDD);
                 break;
         case 12:   // December
                 jDate = jan + feb + mar + apr + may + jun +
                              jul + aug + sep + oct + nov + 
                              atoi(sourceDD);
                 break;
         default:
             sprintf(errMsg,"%s%s","ERROR invalid GregorianDate must be in YYYYMMDD format, SourceDate=", sourceDate);
             return -1;
       }
     *julianDate = jDate;

     return 0;
}

//////////////////////////////////////////////////////////////////////
//  FUNCTION: int computeGregDate
//
//  PURPOSE:  Computes Gregorian Date
//
//  METHOD:
//
//  PARAMETERS:     passed  julian Date (DDD)          int*
//                  passed  julian Year (YYYY)         char*
//                  returns gregorian Date (YYYYMMDD)  char*
//                  returns error msg                  char*
//
//  OTHER OUTPUTS:
//
//  OTHER INPUTS:
//
//  RETURNS:  returns  0  for success
//                    -1  for failed
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////////////////////
int computeGregDate(int* julianDate, char* julianYear, char* targetDate, char* errMsg)
{
  int idx  = 0;
  int hold = 99;
  static char sourceYr[5];
  static char sourceYY[3];
  int sourceYear;
  int days [13] = { 0, 31, 59, 90,120,151,181,212,243,273,304,334,365};
  const char* month[] = {"00","01","02","03","04","05","06","07","08","09","10","11","12"};

     sourceYr[0] = julianYear[0];
     sourceYr[1] = julianYear[1];
     sourceYr[2] = julianYear[2];
     sourceYr[3] = julianYear[3];
     sourceYr[4] = '\0';
     sourceYY[0] = julianYear[2];
     sourceYY[1] = julianYear[3];
     sourceYY[2] = '\0';

     if ((*julianDate < 1) || (*julianDate > 366))
       { sprintf(errMsg,"%s%i","ERROR invalid JulianDate must be within 001 and 366, SourceDate=", *julianDate);
         return -1;
       }
     sourceYear = atoi(sourceYY);
     if ((sourceYear ==  0) ||  (sourceYear ==  4) ||
         (sourceYear ==  8) ||  (sourceYear == 12) ||
         (sourceYear == 16) ||  (sourceYear == 20) ||
         (sourceYear == 24) ||  (sourceYear == 28) ||
         (sourceYear == 32) ||  (sourceYear == 36) ||
         (sourceYear == 40) ||  (sourceYear == 44) ||
         (sourceYear == 48) ||  (sourceYear == 52) ||
         (sourceYear == 56) ||  (sourceYear == 60) ||
         (sourceYear == 64) ||  (sourceYear == 68) ||
         (sourceYear == 72) ||  (sourceYear == 76) ||
         (sourceYear == 80) ||  (sourceYear == 84) ||
         (sourceYear == 88) ||  (sourceYear == 92) ||
         (sourceYear == 96) )
         { days[1] =  31;  days[2]  =  60;  days[3]  =  91;  days[4]  = 121;
           days[5] = 152;  days[6]  = 182;  days[7]  = 213;  days[8]  = 244;
           days[9] = 274;  days[10] = 305;  days[11] = 335;  days[12] = 366;
         }
     int x = 0;
     while (x < 1)
       { if (*julianDate <= days[idx])
           { hold = idx;
             x = 9;
           }
         idx++;
       }
     if (hold == 99)
       { sprintf(errMsg,"%s%i","ERROR invalid JulianDate must be DDD format, SourceDate=", *julianDate);
         return -1;
       }

     targetDate[0] = julianYear[0];
     targetDate[1] = julianYear[1];
     targetDate[2] = julianYear[2];
     targetDate[3] = julianYear[3];
     targetDate[4] = month[hold][0];
     targetDate[5] = month[hold][1];
     int diffx = *julianDate - days[hold - 1];
     switch (diffx)
        { case  1:   targetDate[6] = '0';   targetDate[7] = '1';   break;
          case  2:   targetDate[6] = '0';   targetDate[7] = '2';   break;
          case  3:   targetDate[6] = '0';   targetDate[7] = '3';   break;
          case  4:   targetDate[6] = '0';   targetDate[7] = '4';   break;
          case  5:   targetDate[6] = '0';   targetDate[7] = '5';   break;
          case  6:   targetDate[6] = '0';   targetDate[7] = '6';   break;
          case  7:   targetDate[6] = '0';   targetDate[7] = '7';   break;
          case  8:   targetDate[6] = '0';   targetDate[7] = '8';   break;
          case  9:   targetDate[6] = '0';   targetDate[7] = '9';   break;
          case 10:   targetDate[6] = '1';   targetDate[7] = '0';   break;
          case 11:   targetDate[6] = '1';   targetDate[7] = '1';   break;
          case 12:   targetDate[6] = '1';   targetDate[7] = '2';   break;
          case 13:   targetDate[6] = '1';   targetDate[7] = '3';   break;
          case 14:   targetDate[6] = '1';   targetDate[7] = '4';   break;
          case 15:   targetDate[6] = '1';   targetDate[7] = '5';   break;
          case 16:   targetDate[6] = '1';   targetDate[7] = '6';   break;
          case 17:   targetDate[6] = '1';   targetDate[7] = '7';   break;
          case 18:   targetDate[6] = '1';   targetDate[7] = '8';   break;
          case 19:   targetDate[6] = '1';   targetDate[7] = '9';   break;
          case 20:   targetDate[6] = '2';   targetDate[7] = '0';   break;
          case 21:   targetDate[6] = '2';   targetDate[7] = '1';   break;
          case 22:   targetDate[6] = '2';   targetDate[7] = '2';   break;
          case 23:   targetDate[6] = '2';   targetDate[7] = '3';   break;
          case 24:   targetDate[6] = '2';   targetDate[7] = '4';   break;
          case 25:   targetDate[6] = '2';   targetDate[7] = '5';   break;
          case 26:   targetDate[6] = '2';   targetDate[7] = '6';   break;
          case 27:   targetDate[6] = '2';   targetDate[7] = '7';   break;
          case 28:   targetDate[6] = '2';   targetDate[7] = '8';   break;
          case 29:   targetDate[6] = '2';   targetDate[7] = '9';   break;
          case 30:   targetDate[6] = '3';   targetDate[7] = '0';   break;
          case 31:   targetDate[6] = '3';   targetDate[7] = '1';   break;
          default: 
               sprintf(errMsg,"%s%i","ERROR invalid JulianDate must be DDD format, SourceDate=", *julianDate);
               return -1;
        }
     targetDate[8] = '\0';

     return 0;
}


//////////////////////////////////////////////////////////////////////
//  FUNCTION: int computeChangeDate
//
//  PURPOSE:  Computes new Julian Date
//
//  METHOD:
//
//  PARAMETERS:     passed  julian Date (DDD)          int*
//                  passed  julian Year (YYYY)         char*
//                  passed  add/minus days             int*
//                  return  new julian Date (DDD)      int*
//                  return  new julian Year (YYYY)     char*
//                  returns error msg                  char*
//
//  OTHER OUTPUTS:
//
//  OTHER INPUTS:
//
//  RETURNS:  returns  0  for success
//                    -1  for failed
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////////////////////
int computeChangeDate(int* oldJulianDate, char* oldJulianYear, int* chgDays,
                      int* newJulianDate, char* newJulianYear, char* errMsg)
{
      div_t ans;

      static char sourceYY[3];
      int sourceYear;
      int newJdate;
      int newJyear;
  
         sourceYY[0] = oldJulianYear[2];
         sourceYY[1] = oldJulianYear[3];
         sourceYY[2] = '\0';
  
         if (*chgDays == 0)
           { newJdate = *chgDays;
             ans.quot = 0;
             ans.rem  = *oldJulianDate;
           }
         else
           { sourceYear = atoi(sourceYY);
             newJdate   = *oldJulianDate;
             newJyear = atoi(oldJulianYear);
             if (*chgDays  > 0)
               { for (int incr=1; incr<(*chgDays + 1); incr++)
                    { if ((sourceYear ==  0) ||  (sourceYear ==  4) ||
                          (sourceYear ==  8) ||  (sourceYear == 12) ||
                          (sourceYear == 16) ||  (sourceYear == 20) ||
                          (sourceYear == 24) ||  (sourceYear == 28) ||
                          (sourceYear == 32) ||  (sourceYear == 36) ||
                          (sourceYear == 40) ||  (sourceYear == 44) ||
                          (sourceYear == 48) ||  (sourceYear == 52) ||
                          (sourceYear == 56) ||  (sourceYear == 60) ||
                          (sourceYear == 64) ||  (sourceYear == 68) ||
                          (sourceYear == 72) ||  (sourceYear == 76) ||
                          (sourceYear == 80) ||  (sourceYear == 84) ||
                          (sourceYear == 88) ||  (sourceYear == 92) ||
                          (sourceYear == 96) )
                           { newJdate = newJdate + 1;
                             if (newJdate > 366)
                               { sourceYear = sourceYear + 1;
                                 newJdate = 1;
                                 newJyear++;
                               }
                           }
                      else
                           { newJdate = newJdate + 1;
                             if (newJdate > 365)
                               { sourceYear = sourceYear + 1;
                                 newJdate = 1;
                                 newJyear++;
                               }
                           }
                    }
               }
             else
               { for (int incr=1; incr<(abs(*chgDays) + 1); incr++)
                    { newJdate = newJdate - 1;
                      if (newJdate < 1)
                        { sourceYear = sourceYear - 1;
                          newJyear--;
                          if ((sourceYear ==  0) ||  (sourceYear ==  4) ||
                              (sourceYear ==  8) ||  (sourceYear == 12) ||
                              (sourceYear == 16) ||  (sourceYear == 20) ||
                              (sourceYear == 24) ||  (sourceYear == 28) ||
                              (sourceYear == 32) ||  (sourceYear == 36) ||
                              (sourceYear == 40) ||  (sourceYear == 44) ||
                              (sourceYear == 48) ||  (sourceYear == 52) ||
                              (sourceYear == 56) ||  (sourceYear == 60) ||
                              (sourceYear == 64) ||  (sourceYear == 68) ||
                              (sourceYear == 72) ||  (sourceYear == 76) ||
                              (sourceYear == 80) ||  (sourceYear == 84) ||
                              (sourceYear == 88) ||  (sourceYear == 92) ||
                              (sourceYear == 96) )
                               { newJdate = 366;
                               }
                          else
                               { newJdate = 365;
                               }
                        }
                    }
               }
           }
         
// cout << "   newJyear=" << newJyear    <<  endl;
// cout << "   newJDate=" << newJdate    <<  endl;
    
         *newJulianDate = newJdate;
    // Convert newJyear to character, save value to newJulianYear
         int x = newJyear;
         int i = 3;
         while (i > -1) 
            { ans = div(x, 10);
              switch (ans.rem)
                { case  0:   newJulianYear[i] = '0';   break;
                  case  1:   newJulianYear[i] = '1';   break;
                  case  2:   newJulianYear[i] = '2';   break;
                  case  3:   newJulianYear[i] = '3';   break;
                  case  4:   newJulianYear[i] = '4';   break;
                  case  5:   newJulianYear[i] = '5';   break;
                  case  6:   newJulianYear[i] = '6';   break;
                  case  7:   newJulianYear[i] = '7';   break;
                  case  8:   newJulianYear[i] = '8';   break;
                  case  9:   newJulianYear[i] = '9';   break;
                }
              x = ans.quot;
              i = i - 1;
            }

         return 0;
}

////////////////////////////////////////////////
//  FUNCTION:  computeElapseTime
//
//  PURPOSE: Computes Elapse Time = sendTime - putTime
//           (time is in hhmmssdddddd)
//  METHOD:
//
//  PARAMETERS: char* getTime
//              char* sendTime
//
//  RETURNS:  Returns the computed elapse time,
//            otherwise returns -1
////////////////////////////////////////////////
double computeElapseTime(char* getTime, char* sendTime)
{
#ifdef DEBUG
 cout << "in cmpElapseTm, getTime=" << getTime << "   sendTime=" << sendTime  << endl;
#endif

   double  elapse_time;
   double  get_time1  = 0.00000;
   double  snd_time1  = 0.00000;
   double total_getTime = 0.00000;
   double total_sndTime = 0.00000;
   float get_time2  = 0.00000;
   float snd_time2  = 0.00000;
   char hour_time[3];
   char mint_time[3];
   char secd_time[10];

   hour_time[0] = getTime[0];
   hour_time[1] = getTime[1];
   hour_time[3] = '\0';
   mint_time[0] = getTime[2];
   mint_time[1] = getTime[3];
   mint_time[3] = '\0';
   secd_time[0] = getTime[4];
   secd_time[1] = getTime[5];
   secd_time[2] = '.';
   secd_time[3] = getTime[6];
   secd_time[4] = getTime[7];
   secd_time[5] = getTime[8];
   secd_time[6] = getTime[9];
   secd_time[7] = getTime[10];
   secd_time[8] = getTime[11];
   secd_time[9] = ' ';

   get_time1 = (atoi(hour_time) * 3600.0) + (atoi(mint_time) * 60.0);
   // Convert from char to float
   sscanf(secd_time,"%f", &get_time2);
   total_getTime = get_time1 + get_time2;

   hour_time[0] = sendTime[0];
   hour_time[1] = sendTime[1];
   hour_time[3] = '\0';
   mint_time[0] = sendTime[2];
   mint_time[1] = sendTime[3];
   mint_time[3] = '\0';
   secd_time[0] = sendTime[4];
   secd_time[1] = sendTime[5];
   secd_time[2] = '.';
   secd_time[3] = sendTime[6];
   secd_time[4] = sendTime[7];
   secd_time[5] = sendTime[8];
   secd_time[6] = sendTime[9];
   secd_time[7] = sendTime[10];
   secd_time[8] = sendTime[11];
   secd_time[9] = ' ';

   snd_time1 = (atoi(hour_time) * 3600.0) + (atoi(mint_time) * 60.0);
   // Convert from char to float
   sscanf(secd_time,"%f", &snd_time2);
   total_sndTime = snd_time1 + snd_time2;

   elapse_time = total_sndTime - total_getTime;

#ifdef DEBUG
 cout << "elapseTime=" << elapse_time << endl;
#endif

   if (elapse_time > 0)
        return elapse_time;
   else return -1;

}


////////////////////////////////////////////////
//  FUNCTION:  readParmFile
//
//  PURPOSE:  get valid parm values
//
//  METHOD:
//
//  PARAMETERS: char* fileName
//              int   waitTimeInt < 61 seconds
//              int   maxMsgsIn   < 3001
//
//  RETURNS:  Returns 0 for valid parmValue,
//            otherwise returns -1
////////////////////////////////////////////////
int readParmFile(char* fileName, int* waitTimeInt, int* maxMsgsIn)
{
#ifdef DEBUG
 cout << "in readParmFile, file name=" << fileName << endl;
#endif

   FILE* fp;
   char  tmp0[50];
   char  tmp1[50];

   if ((fp = fopen(fileName, "r")) != NULL)
     { fscanf(fp,"%s %s", tmp0, tmp1);
       if (strcmp(tmp1,"") != NULL)
         { if ((atoi(tmp1) > 0) && (atoi(tmp1) < 60001))
               *waitTimeInt = atoi(tmp1);
           else
               *waitTimeInt = 0;
         }
       else
         { fclose(fp);
           return -1;
         }
       tmp0[0] = '\0';
       tmp1[0] = '\0';
       fscanf(fp,"%s %s", tmp0, tmp1);
       if (strcmp(tmp1,"") != NULL)
         { if ((atoi(tmp1) > 0) && (atoi(tmp1) < 3001))
               *maxMsgsIn = atoi(tmp1);
           else
               *maxMsgsIn = 1;
           fclose(fp);
           return 0;
         }
       else
         { fclose(fp);
           return -1;
         }
     }
   else return -1;

}

////////////////////////////////////////////////
//  FUNCTION:  readRqstSopId
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char*  fileName
//              char*  rqstSopId
//
//  RETURNS:  Returns  0  rqstSopId load complete,
//            returns -1  cannot open file
////////////////////////////////////////////////
int readRqstSopId(char* fileName,  char* rqstSopId)
{
#ifdef DEBUG
 cout << "in readRqstSopId, file name=" << fileName << endl;
#endif

   FILE* fp;
   char  tmp0[50];
   int ijk = 1;

   if ((fp = fopen(fileName, "r")) != NULL)
     {
       while (ijk=1)
         { tmp0[0] = '\0';
           fscanf(fp,"%s", tmp0);
           if (strcmp(tmp0,"") != NULL)
             {
               strcat(rqstSopId, tmp0);
               strcat(rqstSopId, " ");
             }
           else
             { fclose(fp);
               strcat(rqstSopId, "xxxxx");
               strcat(rqstSopId, " ");
               return 0;
             }
         }
      }
   else 
     { strcpy(rqstSopId, "xxxxx");
       strcat(rqstSopId, " ");
       return -1;
     }
}

////////////////////////////////////////////////
//  FUNCTION:  readExpryInfo
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char*  fileName
//              char*  expiryInfo
//
//  RETURNS:  Returns  0  expiryInfo load complete,
//            returns -1  cannot open file
////////////////////////////////////////////////
int readExpryInfo(char* fileName,  char* expiryInfo)
{
#ifdef DEBUG
 cout << "in readExpryInfo, file name=" << fileName << endl;
#endif

   FILE* fp;
   char  tmp0[50];
   int ijk = 1;

   if ((fp = fopen(fileName, "r")) != NULL)
     {
       while (ijk=1)
         { tmp0[0] = '\0';
           fscanf(fp,"%s", tmp0);
           if (strcmp(tmp0,"") != NULL)
             {
               strcat(expiryInfo, tmp0);
               strcat(expiryInfo, " ");
             }
           else
             { fclose(fp);
               strcat(expiryInfo, "xxxxx");
               strcat(expiryInfo, " ");
               return 0;
             }
         }
      }
   else
     { strcpy(expiryInfo, "xxxxx");
       strcat(expiryInfo, " ");
       return -1;
     }
}
     /*void DSOILog1(char* msg, char* msg1){
	  FILE *fp;
	  fp = fopen("/opt/DSOI/logfiles/report/DSOITest1.Log","a+");
	  fprintf(fp,"%s%s\n",msg,msg1);
	  fclose(fp);
     }
     void DSOILog2(char* msg2 , long int size){
	  FILE *fp1;
	  fp1 = fopen("/opt/DSOI/logfiles/report/DSOITest2.Log" , "a+");
	  fprintf(fp1,"%s%d\n",msg2,size);
     }*/

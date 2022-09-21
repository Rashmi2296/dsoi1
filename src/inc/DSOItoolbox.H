///////////////////////////////////////////////////////
//  FILE:  DSOItoolbox.H
//
//  PURPOSE:
//  
//  RELATED HEADER FILES:
//
//  REVISION HISTORY:
//  version	date	 author	  description
//  7.00.00     12/23/99 rfuruta  initial development
//  7.01.00     08/20/00 rfuruta  CR3452, date rtns for computing purge date
//  8.09.00     01/12/03 rfuruta  CR8010, set waitInterval time
//  9.02.00     09/22/03 rfuruta  CR8582, return SopId to requesting application
//  9.04.00     02/11/04 rfuruta  CR8911, setExpiry time

//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <cmqc.h>
#include <iostream.h>
#include <stdlib.h>
// #include "DSOIerrorHandler.H"
#include "DSOIconstants.H"

	void convertTime(char*);   // for the REPORTQ
	void calculateTime(char*); // for the REPORTQ
	void calculateDate(char*); // for the REPORTQ
        int computeChangeDate(int*, char*, int*, int*, char*, char*);
        int computeJulianDate(char*, int*, char*, char*);
        int computeGregDate(int*, char*, char*, char*);
	void formatReport(char*,
	        const char*,
		const char*, const char*,
		const char*, const char*, 
		const char*, const char*,
		const unsigned char*, const char*);
        int findTagValue(char*,const char*,char*);
        int getFidValue(char*,char*,char*);
        int getSection(char*,char*);
        int routeToSop(char*, char*);
        int replyToSop(char*, char*);
        int requestToDSOI(char*, char*);
        void buildMsgTrunc(char*, char*, char*, char*);
        char* binarySearch(char**, char*, int, int);
        double computeElapseTime(char*, char*);
        int readParmFile(char*, int*, int*);
        // int readRqstSopId(char*, char[20][7]);
        int readRqstSopId(char*, char*);
        int readExpryInfo(char*, char*);

	static char _reportMsg[DSOI_STRING_1024];		
	static char _putTime[DSOI_STRING_15];		
	static char _getTime[DSOI_STRING_15];		
	static char _putApplDate[DSOI_STRING_9];
	static char _putApplName[DSOI_STRING_29];
	static char _putApplTime[DSOI_STRING_9];		
	static char _Date[DSOI_STRING_9];		
	static char _Time[DSOI_STRING_9];		
	static const int _sizeOfTime = 14;
	static const int _sizeOfDate = 11;

        static char _errorMessage[DSOI_STRING_1024];
	//void DSOILog1(char *, char *);
	//void DSOILog2(char* , long int);

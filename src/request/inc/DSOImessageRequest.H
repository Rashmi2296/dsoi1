#ifndef DSOImessageRequest_H
#define DSOImessageRequest_H

///////////////////////////////////////////////////////
//  FILE: DSOImessageRequest.H
//
//  PURPOSE:
//  
//  RELATED HEADER FILES:
//
//  REVISION HISTORY:
//  version	date	 author	description
//  1.00.00     06/24/97 lparks initial release for IMA
//  2.00.01     09/24/97 lparks initial release for eastern(DSOI)
//  2.00.04     10/16/97 lparks added checkNXX for 208 NPA
//  4.00.00     11/02/98 rfuruta CR3151, add ownerCode tag
//  5.00.00     05/14/99 rfuruta CR454, send cuid to SOPAD instead of typists initials
//  6.00.00     06/15/99 rfuruta CR468, new function AM, Special Billing Number
//  6.00.00     06/15/99 rfuruta CR469, new function CK, Circuit Identifier
//  6.01.00     12/15/99 rfuruta CR3064, add SbnType tag to Special Billing Number
//  8.00.00     09/20/00 rfuruta CR4767, remove 4P connection
//  8.05.00     03/19/02 rfuruta CR7541, Compute OwnershipCode and pass to SOPs
//  8.09.00     03/02/03 rfuruta CR8037, Add owner code to logfile report
//  8.09.00     03/02/22 rfuruta CR8180, fix memory leaks
//  9.08.00     01/03/05 rfuruta CR9745, set Priority
// 10.02.00     09/20/05 rfuruta CR10269, new function 'DX' for SOLAR- retrieve CSR data
//
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <iostream.h>
#include <stdlib.h>
#include "DSOIerrorHandler.H"


class DSOImessageRequest{
public:
        static char _QueueName[7];

	static char* newMessage;
	static char _msgTrunc[450];
	static int returnFunctionType() {return _FunctionType; };
	static int readDataRef(char*); // read
	static int messageMassage(char*,MQLONG,DSOIsendMQ*,char*,char*,char*,char*,char*,char*,char*);
        static char ** _npaSplit;
        static char ** _routeInfo;
        static char ** _ownerInfo;
protected:
private:
	static void reInitMembers(); //reinitialize all members
	static void deleteTagVals(); //reinitialize all members
	static void endOfMessage(char*);
	static void mergeSOC(char*);
	static void fillerSOP(char*);
	static void setPreSoc(char*, char*, int, char*);
	static void setKeyType(char*);
	static void setHeader(char*, char*);
	static void setCLHeader(char*, char*);
	static void setDXHeader(char*, char*);
	static int verifySopId(char*,char*,char*,char*,char*); //verify SopId is valid
        static int verifyNPA(char*, char*);          //verify NPA is valid
        static int verifyNXX(char*);                 //verify NXX is valid
        static int verifyOWN(char*, char*, char*);     //verify Owner is valid
	static void setUsocTagName(char* ); //special case
	static int setTagName(char* ); //for all but USOC
	static void setPreServiceOrder(char*);
	static void setAMHeader(char*);
	static void setCKHeader(char*);
	static int parseServiceOrder(char*);
	static int elementLookup(char*); //lookup possible fields in message
	static int checkValues(char*, int); // lookup possible fields in message
	static int checkUserIdTable(char*); // validate UserId
	static int findEqualTags( char*, int, int); // find tags
        static int getSopId(char*,char*,char*,DSOIsendMQ*,char*);
	static int parseMessage(char*,char*,char*,char*,char*); // find tags
	static void fillSpace(char*,int); // fill spaces in message
	static int createErrorMsg(char* );
	static void setNewRsolarMessage(char*,char*,char*,char*);

	static DSOIerrorHandler messageErrObj;
	static DSOIsendMQ* _sendErrQ;
	static char* data;
	static char* QueryData;
	static int actionCode;
	static int _sizeOfNewMessage;
	static int _sizeOfNpaSplit;
	static int _sizeOfRouteInfo;
	static int _sizeOfOwnerInfo;
	static int _sizeOfTag;
	static int _FunctionType;
	static char _formatCode[2];
        // static char ** _npaSplit;
        // static char ** _routeInfo;
        // static char ** _ownerInfo;

	static char ownCode[2];
	static char elementType[2];
	static char QueryListType[4];
	static char orderType[3];
	static char dataLength[3];
	static char npa[4];
	static char tagName[6];
	static char lineCount[7];
	static char orderNumber[9];
	static char _userId[9];
	static char* tagVal[25];
	static char negotiationNumber[11];
	static char _errorMessage[1024];
	static char NpaNxx[7];
	static char KeyType[2];
	static char Exchange[5];
	static char CustName[20];
	static char SbnQty[3];
	static char SbnType[2];
	static char AUXIND[2];
	static char ClassSvc[6];
	static char PrivateLine[2];
	static char ServiceCode[5];
	static char CiPrefix[3];
	static char CiQty[3];
     	static char useridtemp[9];
}; //end of DSOImessageRequest class


#endif //DSOImessageRequest_H

#ifndef DSOIrouteRequest_H
#define DSOIrouteRequest_H

#include <iostream.h>
#include <cstdlib>
#include "DSOIerrorHandler.H"


class DSOIrouteRequest{
public:
	static char* newMessage;
	static char* SopId;
	static char* Is4P;
	static char _msgTrunc[150];
	static int returnFunctionType() {return _FunctionType; };
	static int readDataRef(char*); // read
	static int messageMassage(char*,DSOIsendMQ*,char*,char*,char*,char*,char*,char*);
protected:
private:
	static void fillerSOP(char*);
	static void setHeader(char*);
	static void setSOLmessage(char*);
	static void setALSmessage(char*);
	static int verifyNPA(char*, char*, char*);          //verify NPA is valid 
	static int verifyNXX(char*); //verify NPA is valid 
	static int createErrorMsg(char* );

	static DSOIerrorHandler messageErrObj;
	static DSOIsendMQ* _sendErrQ;
	static char* data;
	static char *QueryData;
	static char ** _npaSplit;
	static char ** _routeInfo;
	static int actionCode;
	static int _sizeOfNewMessage;
	static int _sizeOfNpaSplit;
	static int _sizeOfRouteInfo;
	static int _FunctionType;
	static char functionCode[3];
	static char orderType[3];
	static char orderNumber[9];
	static char _userId[9];
	static char _errorMessage[1024];
}; //end of DSOIrouteRequest class


#endif //DSOIrouteRequest_H

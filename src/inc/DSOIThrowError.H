#ifndef DSOIThrowError_H
#define DSOIThrowError_H

///////////////////////////////////////////////////////
//  FILE: 
//
//  PURPOSE:
//  
//  RELATED HEADER FILES:
//
//  REVISION HISTORY:
//  version	date	author	description
//
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <cmqc.h>
#include <cstring>
#include <iostream.h>

class ThrowError {
public:
	ThrowError(char * , MQLONG  ) ;
	~ThrowError();

	char* getErrorMessage();
	void printMessage() const {
		cout << getMessage() <<  endl;
		cout << "for reasonCode = " << getReason() << endl;
	}
	char* getMessage() const { return message; };
	MQLONG getReason() const { return reason; };

private:
	char *message;
	MQLONG reason;
};

inline ThrowError::ThrowError(char* msg, MQLONG returnCode)
{
	// cout << "in ThrowError Constructor " << endl;
	register int length = strlen(msg);
	message = new char[length+1];
	strcpy(message,msg);
	reason = returnCode;
}

inline char* ThrowError::getErrorMessage()
{
	int size = strlen(message);
	return message;
}

inline ThrowError::~ThrowError()
{
	delete [] message;
}

#endif // DSOI_ThrowError_H

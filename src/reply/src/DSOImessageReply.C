//  FILE:  DSOImessageReply.C
//
//  PURPOSE: Routes messages from a 'Requestor' application
//	to the SOPS and
//	formats the message accordingly.
//  
//  REVISION HISTORY:
//-----------------------------------------------------
//  version	date		author		description
//-----------------------------------------------------
//  1.00.00 	06/24/97	lparks 		Initial release IMA
//  2.00.01 	09/24/97	lparks 		Initial release Central
//  2.00.02 	10/09/97	lparks 		added memsets to
//						prevent core dumps
//  2.00.04 	10/16/97	lparks 		corrected data being
//						chopped by one character
//  2.00.04 	11/03/98	rfuruta 	CR3151 add owner code tag/value
//  5.00.00     05/14/99        rfuruta         CR459, send cuid to SOPAD instead of typists initials
//  6.00.00     06/15/99        rfuruta         CR468, new function BN, SpecialBillingNumber
//  6.00.00     06/15/99        rfuruta         CR469, new function CI, CircuitIdentifier
//  6.01.00     11/29/99        rfuruta         CR2980, SOLAR CircuitId list contains a hex9F
//  6.02.00	02/01/00	rfuruta		CR3299, accept 2 character OrderType
//  7.00.00     12/23/99        bxcox           CR3126, return SopId= tag/value if client is SOSE; new function getClientSopId()
//  7.01.00     07/03/00        rfuruta 	CR 4275, 4P connection
//  7.01.00     08/25/00        rfuruta         CR 4783, removed '<' and '>' from error msg
//  8.03.00     07/25/01        rfuruta         CR 6781, Select Section from service order
//  8.06.00     05/17/02        rfuruta         CR 7617, Assign a '88' section number to an invalid section name
//  8.08.00     01/02/03        rfuruta         CR 8000, upgrade to HP-UX 11.0
//  8.09.00     03/22/03        rfuruta         CR 8180, fix memory leaks
//  9.03.00     12/08/03        rfuruta         CR 8761, New function codes(NQ, NT) for WebSOP
//  9.06.00     08/17/04        rfuruta         CR 9194, New function codes (NU, NR, NE)
//  9.08.00     01/19/05        rfuruta         CR 9665, RSOLAR and SOLAR expanding the QS message
//  9.08.00     01/19/05        rfuruta         CR 9667, new function 'SQ' for RSOLAR
//  9.08.00     01/24/05        rfuruta         CR 9791, new function 'NL' for all SOPs
//  9.10.00     05/16/05        rfuruta         CR 9898, new function 'RQ' for RSOLAR
// 10.01.00     07/05/05        rfuruta         CR10094, ECRIS error correction
// 10.04.00     02/01/07        rfuruta 	CR10999, migrate DSOI to Solaris
// 10.04.01     08/09/07        rfuruta         CR11307, fix errors associated with migrating to Solaris
// 10.04.02     08/16/07        rfuruta         CR11357, fix data conversion problem with the RQ function
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOImessageReply.H"
#include "DSOIThrowError.H"
#include "DSOItoolbox.H"
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <climits>
#include <new>

const int TOKENBLANK 			= 	' ';
const int ERRORCODEPOSITION		=	75;
const int ERRORCOUNTPOSITION 		= 	77; //start position of error count
const int ERRORMESSAGECOUNTPOSITION	=	50;
const int ERRORMESSAGEPOSITION		=	80;
const int USERIDPOSITION		=	14;
const int LSRIDPOSITION	      		=	63;
const int OWNERCODEPOSITION		=	22;
const int FUNCTIONCODEPOSITION		=	29;
const int ORIGINALFUNCCODEPOSITION	=	73;
const int REPLYADDCHARS			=	125;
const int PARAMETERCHARS		= 	200;	
const int DATAPOSITION			=	106;
const int LINECOUNTPOSITION		=	23;
const int CANDIDATECOUNTPOSITION	=	75;
const int CANDIDATEDATAPOSITION		=	78;
const int SBNQTYPOSITION		=	76;
const int SBNDATAPOSITION		=	78;
const int CIRCUITIDQTYPOSITION	 	=	76;
const int CIRCUITIDDATAPOSITION		=	78;
const int STATUSCOUNTPOSITION	        =	84;
const int STATUSDATAPOSITION		=	88;
const int ENTTMPOSITION			=	75;
const int FACSENTPOSITION	        =	83;

int DSOImessageReply::_ONSflag =0;
int DSOImessageReply::_sizeNeeded = 0;
int DSOImessageReply::_sizeOfServiceOrder =0;
char* DSOImessageReply::_serviceOrder = '\0';
char* DSOImessageReply::_data = '\0';
int DSOImessageReply::_sizeOfData =100;
int DSOImessageReply::_FunctionType = 0;
char DSOImessageReply::_elementType[] = {'9','9'};
char DSOImessageReply::_Lock[] = {'9','9'};
char DSOImessageReply::_actionCode[] = {' ',' '};
char DSOImessageReply::_returnCode[] = {'9','9','9'};
char DSOImessageReply::_OriginalFunctionCode[] = {'0','0','0'};
char DSOImessageReply::_FunctionCode[] = {'0','0','0'};
char DSOImessageReply::_UserId[]  = {'x','x','x','x','x','x','x','x','x'};
char DSOImessageReply::_UserId2[] = {'x','x','x','x','x','x','x','x','x'};
char DSOImessageReply::_OrderType[] = {'0','0','0'};
char DSOImessageReply::_SectionId[] = {'0','0','0'};
char DSOImessageReply::errorMessageCount[] = {'9','9','9','9'};
char DSOImessageReply::_candidateCount[] = {'0','0' ,'0','0'};
char DSOImessageReply::_statusCount[] = {'0','0' ,'0','0'};
char DSOImessageReply::_SbnQty[] = {'0','0','0'};
char DSOImessageReply::_CiQty[] = {'0','0','0'};
char DSOImessageReply::_tagName[] = {'0','0','0','0','0','0'};
char DSOImessageReply::_ParentSeq[] = {'0','0','0','0','0','0','0'};
char DSOImessageReply::_SeqNo[] = {'0', '0','0', '0', '0', '0', '0'};
char DSOImessageReply::_OrderNumber[] = {'0','0','0','0','0','0','0','0'};
char DSOImessageReply::_LsrId[] = {'0','0','0','0','0','0','0','0','0','0','0'};
char DSOImessageReply::_ownCode[] = {'X'};
DSOIerrorHandler DSOImessageReply::messageErrObj;

char * DSOImessageReply::_requestQueue = '\0';
char DSOImessageReply::_errorMessage[1024] = "";
char DSOImessageReply::_npaNxx[7] = "";
DSOIsendMQ* DSOImessageReply::_sendErrQ = 0;


////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::messageMassage(char* msg, char* holdmsg,
//			MQLONG msgLengthReceived, DSOIsendMQ* sendErrorQ)
//
//  PURPOSE: Formats a message for the 'Requestor' application
//	received from the SOP.
//
//  METHOD: Calls a parse function to format the message
//	in 'Requestor' application expected format.
//
//  PARAMETERS: char* msg - the message to format,
//		MQLONG msgLengthReceived - message length
//  
//  OTHER OUTPUTS: Errored messages when parseMessage fails 
//  
//  OTHER INPUTS:_sendErrQ - the queue to send error
//			messages only
//
//  RETURNS: One if it does not format the message correctly or
//	parseMessage fails, zero otherwise.
// 
//  FUNCTIONS USED: DSOImessageReply::parseMessage(),
//	DSOImessageReply::endOfMessage(), 
//	DSOIerrorHandler::sendErrorMesage(), strcpy(),
//	memset()
//
//////////////////////////////////////////////////////
int DSOImessageReply::messageMassage(char* msg, char* holdmsg, MQLONG msgLengthReceived, char* sopId,
                          DSOIsendMQ* sendErrorQ, char* newMessage)
{
        /* variable introduced to capture the size of the reply message for large order tracing
	long int size_reply_message;
	char* msgPtr1 = msg;
	size_reply_message = strlen(msgPtr1);
	if(size_reply_message > 4000){
	DSOILog2("The size of the reply message : " , size_reply_message);
	DSOILog1("The content of the reply message : " , msg);
	DSOILog1("*******************************************************************" ," ");
	}*/
	char  FunCdValue[3] = "  ";
	char  OldFunCdVal[3] = "  ";
	char  RetCdValue[3] = "00";
	char  ErrMsgCntValue[4] = "000";
	char  OwnerCodeValue[2] = " ";
	char* ErrMsgValue;
	const char* SopIdTag = "SopId=";
	const char* FunCdTag = "FunctionCode=";
	const char* RetCdTag = "ReturnCode=";
	const char* ErrMsgCntTag = "ErrorMessageCount=";
	const char* ErrMsgTag = "ErrorMessage=";
	const char* OwnerCodeTag = "OWN=";
	char  tagValue[101];

	_sendErrQ = sendErrorQ;
	int returnCode = 0;
	int length = strlen(msg);

	OldFunCdVal[0] = '\0';
	tagValue[0] = '\0';
        if (findTagValue(holdmsg, (char *)FunCdTag, tagValue) ==0)
          { strcpy(OldFunCdVal, tagValue);
          }
      
	sopId[0] = '\0';
	tagValue[0] = '\0';
        if (findTagValue(holdmsg, (char *)SopIdTag, tagValue) ==0)
             { strcpy(sopId, tagValue);  }
        else returnCode = getClientSopId(sopId, holdmsg);
        
	tagValue[0] = '\0';

	if( (length > 0 ) && (_ONSflag) )
	{
		// ONS

		int returnCode = 0;

		if(returnCode= setONS(msg, newMessage) !=0)
		{
			//error
		}

		return 0;
	}

	else if((length > 0 ) && ((returnCode = checkIfRsolar(msg) )==0 ))
	{
		// RSOLAR

#ifdef DEBUG
cout << "message is from RSOLAR " << msg << endl;
#endif

		if (findTagValue(msg, (char *)FunCdTag, tagValue) == 0)
			{ strcpy(FunCdValue, tagValue); }
		if ((strcmp(FunCdValue, "OE") == 0) || (strcmp(FunCdValue, "NL") == 0))
		{
			if (findTagValue(msg, (char *)RetCdTag, tagValue) == 0)
				{ strcpy(RetCdValue, tagValue); }
			else { return -1; }     //error
                        
			// CR 9194    CR 9791- add NL
                        if (((strncmp(OldFunCdVal, "NE",2) ==0) || (strncmp(OldFunCdVal, "NR",2) ==0) || 
                             (strncmp(OldFunCdVal, "NL",2) ==0)) &&
                            ((strncmp(RetCdValue, "00",2) ==0)  || (strncmp(RetCdValue, "02",2) ==0)) )
		          { strcat(newMessage,msg);
	                     return 0;
		          }

			if (findTagValue(msg, (char *)ErrMsgCntTag, tagValue) == 0)
				{ strcpy(ErrMsgCntValue, tagValue); }
			else { return -1; }     //error

			if (atoi(ErrMsgCntValue) > 0)
			{
				// error exist

				if ((ErrMsgValue = new char[(atoi(ErrMsgCntValue)*100)+1]) ==NULL)
				{
					messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because malloc(new) FAILED" , (char *) "", -1,(char *) "messageMassage", (char *) __FILE__, __LINE__);
					messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
					return -1;
				}

				if (findTagValue(msg, (char *)ErrMsgTag, ErrMsgValue) == 0)
				  {
			 	    sprintf(newMessage,"%s%s%c%s%s%c%s%s%c",
						"ReturnCode=",RetCdValue,0xff,
						"ErrorMessageCount=",ErrMsgCntValue,0xff,
						"ErrorMessage=",ErrMsgValue,0xff);
				}
				else { return -1; }     //error

				delete []ErrMsgValue;
			}

			else 
			{
                              // no errors

 			        sprintf(newMessage,"%s%s%c%s%s%c",
					"ReturnCode=",RetCdValue,0xff,
					"ErrorMessageCount=",ErrMsgCntValue,0xff);
			        // CR 9194    CR 9791- add NL
                                if (((strncmp(OldFunCdVal, "NE",2) ==0) || (strncmp(OldFunCdVal, "NR",2) ==0) || 
                                     (strncmp(OldFunCdVal, "NL",2) ==0)) &&
                                    ((strncmp(RetCdValue, "00",2) ==0)  || (strncmp(RetCdValue, "02",2) ==0)) )
                                     {   char * str_ptr;
                                         if ((str_ptr = strstr(msg,"ServiceOrder=00000000000000L")) != NULL)
                                           { strncat(newMessage, msg,  str_ptr - msg);
                                             strncat(newMessage, "ServiceOrder=",13);
                                             strcat(newMessage, str_ptr+30);
                                           }
                                         else
                                           { strcat(newMessage,msg);
                                           }
                                     }

			}

		}	// end if (strcmp(FunCdValue, "OE") == 0)

		else
                        { if (strncmp(OldFunCdVal, "RQ",2) ==0)
		             { strcat(newMessage,msg);
	                       return 0;
		             }
			  if ((strcmp(FunCdValue, "SO") == 0) || (strcmp(FunCdValue, "SL") == 0))
		            { char * str_ptr;
	                      if ((str_ptr = strstr(msg,"ServiceOrder=00000000000000L")) != NULL)
				{ strncpy(newMessage, msg,  str_ptr - msg);
				  strncat(newMessage, "ServiceOrder=",13);
				  strcat(newMessage, str_ptr+30);
				}
			      else {strcpy(newMessage,msg);}
                            }
			  else {strcpy(newMessage,msg);}
	                  return 0;
		        }

	     return 0;
	}

	if(returnCode == -2)
	{
		//error
        	return -1;
	}
#ifdef DEBUG
cout << "message is not RSOLAR " << endl;
#endif
        msg[0] = ' ';
        msg[1] = ' ';
        msg[2] = ' ';
        msg[3] = ' ';

	for(int i = 0; i < 29; i++)
	{
		if (msg[i] < ' ')
			msg[i] = ' ';
	}

	char* msgPtr = msg+ORIGINALFUNCCODEPOSITION;
	strncpy(_OriginalFunctionCode,msgPtr,2);
	_OriginalFunctionCode[2] = '\0';
	msgPtr = msg;
	
        returnCode = getFunction(msgPtr);
	int rc=0;

        switch (returnCode)
        {
	   case 1:     // SO function code

                // CR 9194
                if ((strncmp(_OriginalFunctionCode,"NE",2)==0) || (strncmp(_OriginalFunctionCode,"NR",2)==0))
                  {
                    _returnCode[0] = msg[106];
                    _returnCode[1] = msg[107];
                    _returnCode[2] = '\0';
                    int ixy = atoi(_returnCode);
                    if ((ixy==0) || (ixy==2))
                      {
                        memset(newMessage,0,1048576);
                        newMessage[0] = '\0';
                        if (rc=serviceOrderSetup(msg, msgLengthReceived, newMessage) !=0)
                          { messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because serviceOrderSetup() failed", (char *) "", rc, (char *) "messageMassage", (char *) __FILE__, __LINE__);
                            messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
                            return rc;
                          }
                        memset(msgPtr,0,strlen(newMessage)+14);
                        strcpy(msgPtr,"ReturnCode=");
                        strncat(msgPtr,_returnCode,2);
                        strcat(msgPtr, "\xff");
                        strcat(msgPtr,newMessage);
                        memset(newMessage,0,strlen(msgPtr));
                        strcpy(newMessage,msgPtr);
                        return 0;
                      }
                  }

		rc = serviceOrderSetup(msg, msgLengthReceived, newMessage); 
		if (rc !=0)
		  {
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because serviceOrderSetup() failed", (char *) "", rc, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return rc;
			//error
		  }

                break;
            
            case 2:     // OE function code
            case 3:     // EE function code
		msgPtr = msg;
		returnCode = parseErrorMessage(msgPtr, newMessage);

		if (returnCode !=0)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseErrorMessage() failed", (char *) "", returnCode, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return returnCode;
		}
                memset(msgPtr,0,strlen(msg));   // CR 9194
                strcpy(msgPtr,newMessage);      // CR 9194
                break;

            case 4:     // CL function code
		if(rc=queryListSetup(msg, newMessage) !=0)
		{
			//error
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because queryListSetup() failed", (char *) "", rc, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return rc;
		}
                break;

            case 5:     // BN function code
		if(rc=BillingNumberSetup(msg, newMessage) !=0)
		{
			//error
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because BillingNumberSetup() failed", (char *) "", rc, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return rc;
		}
                break;

            case 6:     // CI function code
		if(rc=CircuitIdSetup(msg, newMessage) !=0)
		{
			//error
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because CircuitIdSetup() failed", (char *) "", rc, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return rc;
		}
                break;

            case 7:     // SL function code
		if(rc=OrderStatusSetup(msg, newMessage) !=0)
		{
			//error
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because OrderStatusSetup() failed", (char *) "", rc, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return rc;
		}
                break;

            default:
                if(returnCode < 0)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because getFunction() failed", (char *) "", returnCode, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return returnCode;
                }
                else
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because getFunction() failed",(char *) "",returnCode,(char *) "messageMassage",(char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                }
                break;
        }

        return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::serviceOrderSetup ()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::serviceOrderSetup(char* msg, 
				MQLONG msgLengthReceived, char* newMessage) 
{
		char* msgPtr = msg;
		int returnCode = setParameters(msgPtr);
		if (returnCode !=0)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because setParameters() failed",
			(char *) "",
			returnCode,
			(char *) "messageMassage",
			(char *) __FILE__,
			__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return returnCode;
		}
		msgPtr = msg;
		returnCode = parseServiceOrder(msgPtr,msgLengthReceived);
		if (returnCode !=0)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseServiceOrder() failed",
			(char *) "",
			returnCode,
			(char *) "messageMassage",
			(char *) __FILE__,
			__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return returnCode;
		}
		mergeMessage(newMessage);
		return 0;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::queryListSetup ()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::queryListSetup(char* msg, char* newMessage) 
{
	char* msgPtr = msg;
	int returnCode = parseCandidateList(msgPtr, newMessage);
	if (returnCode !=0)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseCandidateList() failed",
			(char *) "",
			returnCode,
			(char *) "messageMassage",
			(char *) __FILE__,
			__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return returnCode;
	}	
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::BillingNumberSetup ()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::BillingNumberSetup(char* msg, char* newMessage) 
{
	char* msgPtr = msg;
	int returnCode = parseBillingNumberList(msgPtr, newMessage);
	if (returnCode !=0)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseBillingNumberList() failed",
			(char *) "",
			returnCode,
			(char *) "messageMassage",
			(char *) __FILE__,
			__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return returnCode;
	}	
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::CircuitIdSetup ()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::CircuitIdSetup(char* msg, char* newMessage) 
{
	char* msgPtr = msg;
	int returnCode = parseCircuitIdList(msgPtr, newMessage);
	if (returnCode !=0)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseCircuitIdList() failed",
			(char *) "",
			returnCode,
			(char *) "messageMassage",
			(char *) __FILE__,
			__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return returnCode;
	}	
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::OrderStatusSetup ()
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS:
//
//  OTHER OUTPUTS:
//
//  OTHER INPUTS:
//
//  RETURNS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////
int DSOImessageReply::OrderStatusSetup(char* msg, char* newMessage)
{
        char* msgPtr = msg;
        int returnCode = parseOrderStatusList(msgPtr, newMessage);
        if (returnCode !=0)
        {
                messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseOrderStatusList() failed",
                        (char *) "",
                        returnCode,
                        (char *) "messageMassage",
                        (char *) __FILE__,
                        __LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
                return returnCode;
        }
        return 0;
}


////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::mergeMessage ()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
void DSOImessageReply::mergeMessage(char* newMessage) 
{
	sprintf(newMessage,"%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c%s%s%c",
		"FunctionCode=",_FunctionCode, 0xff,
		"UserID=", _UserId, 0xff,
		"NPANXX=", _npaNxx, 0xff,
		"OrderType=",_OrderType,0xff,
		"OrderNumber=", _OrderNumber, 0xff,
		"LSRID=", _LsrId, 0xff,
		"Lock=", _Lock, 0xff,
		"OWN=", _ownCode, 0xff,
		"ServiceOrder=", _serviceOrder,0xff);
#ifdef DEBUG
cout << "     message:" << newMessage << endl;
#endif
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::setParameters (char* message
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: char* message - the message 
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::setParameters(char* msg) 
{
	char* ParameterTable[] = {_FunctionCode, _UserId, _OrderType,
		  _OrderNumber, _LsrId, _Lock, _UserId2, _ownCode}; //parameters
	int messagePosition[] = { 29, 82, 90, 92, 63, 101, 14, 22}; //in message
	const int size[] = { 3, 9, 3, 9, 11, 2, 9, 2}; //in bytes

	char* msgPtr = msg;
	for(int i =0; i < 8; i++)
	{
		msgPtr += messagePosition[i];
		if(msgPtr == NULL)
			return -1;
		strncpy(ParameterTable[i],msgPtr,size[i]);
		ParameterTable[i][size[i]-1] = '\0';
#ifdef DEBUG
cout << "PArameterTable[i]= " << ParameterTable[i] << endl;
#endif
		msgPtr = msg;
	}

	if (_OrderType[1] == ' ')
		_OrderType[1] = '\0';

	return 0;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::parseCandidateLis
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: char* msg - the message to format,
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::parseCandidateList(char* msg, char* newMessage)
{
	char* messagePtr = msg; // filler data

	int rc =0;
	if(rc = parseMessage(messagePtr,
				CANDIDATECOUNTPOSITION,
				(char *) "CandidateCount",3,1, newMessage) != 0 ) //find error code in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseCandidateList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	int countInt = atoi(_candidateCount);
	if(countInt > 0 )
	{
		int x = CANDIDATEDATAPOSITION;
		if((rc =parseMessage(messagePtr,
				CANDIDATEDATAPOSITION,
				(char *) "CandidateList", countInt*92,1, newMessage) )!=0)
		{
		        messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseCandidateList", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return 1;
		}
		endOfMessage(newMessage);
	}//end of if(countInt > 0 )

	memset(msg,0,strlen(msg));	//clear the message sent in  
	strcpy(msg,newMessage);

	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::parseBillingNumberList
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: char* msg - the message to format,
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::parseBillingNumberList(char* msg, char* newMessage)
{
	char* messagePtr = msg; // filler data

	int rc =0;
	if(rc = parseMessage(messagePtr,
				FUNCTIONCODEPOSITION,
				(char *) "FunctionCode",2,1, newMessage) != 0 ) //find function code in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseBillingNumberList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	if(rc = parseMessage(messagePtr,
				USERIDPOSITION,
				(char *) "UserID",8,1, newMessage) != 0 ) //find UserId in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseBillingNumberList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	if(rc = parseMessage(messagePtr,
				LSRIDPOSITION,
				(char *) "LSRID",10,1, newMessage) != 0 ) //find LSRID in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseBillingNumberList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	if(rc = parseMessage(messagePtr,
				SBNQTYPOSITION,
				(char *) "SbnQty",2,1, newMessage) != 0 ) //find error code in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseBillingNumberList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	int countInt = atoi(_SbnQty);
	if(countInt > 0 )
        {
		if((rc =parseMessage(messagePtr,
				SBNDATAPOSITION,
				(char *) "SpBillNo", countInt*11, 1, newMessage) )!=0)
		{
		        messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseBillingNumberList", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return 1;
		}
	}

	endOfMessage(newMessage);
	memset(msg,0,strlen(msg));	//clear the message sent in  
	strcpy(msg,newMessage);

	return 0;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::parseCircuitIdList
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: char* msg - the message to format,
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::parseCircuitIdList(char* msg, char* newMessage)
{
	char* messagePtr = msg; // filler data

	int rc =0;
	if(rc = parseMessage(messagePtr,
				FUNCTIONCODEPOSITION,
				(char *) "FunctionCode",2,1, newMessage) != 0 ) //find function code in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseCircuitIdList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	if(rc = parseMessage(messagePtr,
				USERIDPOSITION,
				(char *) "UserID",8,1, newMessage) != 0 ) //find UserId in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseCircuitIdList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	if(rc = parseMessage(messagePtr,
				LSRIDPOSITION,
				(char *) "LSRID",10,1, newMessage) != 0 ) //find LSRID in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseCircuitIdList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	if(rc = parseMessage(messagePtr,
				CIRCUITIDQTYPOSITION,
				(char *) "CiQty",2,1, newMessage) != 0 ) //find error code in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseCircuitIdList", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	int countInt = atoi(_CiQty);
	if(countInt > 0 )
	{
		if((rc =parseMessage(messagePtr,
			CIRCUITIDDATAPOSITION,
			(char *) "CircuitIdentifier", countInt*15, 1, newMessage) )!=0)
		{
		        messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseCircuitIdList", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return 1;
		}
        }

	endOfMessage(newMessage);
	memset(msg,0,strlen(msg));	//clear the message sent in  
	strcpy(msg,newMessage);

	return 0;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::parseOrderStatusList
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char* msg - the message to format,
//
//  OTHER OUTPUTS:
//
//  OTHER INPUTS:
//
//  RETURNS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////
int DSOImessageReply::parseOrderStatusList(char* msg, char* newMessage)
{
        char* messagePtr = msg; // filler data
        
	int rc =0;
        if(rc = parseMessage(messagePtr,
                                ENTTMPOSITION,
                                (char *) "ENTTM",8,1, newMessage) != 0 ) //find error code in message
        {
                messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseOrderStatusList", (char *) __FILE__,__LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
                return 1;
        }
        endOfMessage(newMessage);

	rc =0;
        if(rc = parseMessage(messagePtr,
                                FACSENTPOSITION,
                                (char *) "FACSENT",1,1, newMessage) != 0 ) //find error code in message
        {
                messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseOrderStatusList", (char *) __FILE__,__LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
                return 1;
        }
        endOfMessage(newMessage);

        rc =0;
        if(rc = parseMessage(messagePtr,
                                STATUSCOUNTPOSITION,
                                (char *) "StatusListCount",4,1, newMessage) != 0 ) //find error code in message
        {
                messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseOrderStatusList", (char *) __FILE__,__LINE__);
                messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
                return 1;
        }
        endOfMessage(newMessage);

        int countInt = atoi(_statusCount);
        if(countInt > 0 )
        {
                int x = STATUSDATAPOSITION;
                if((rc =parseMessage(messagePtr,
                                STATUSDATAPOSITION,
                                (char *) "StatusList", countInt*93,1, newMessage) )!=0)
                {
                        messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseOrderStatuslist", (char *) __FILE__,__LINE__);
                        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
                        return 1;
                }
                endOfMessage(newMessage);
        }//end of if(countInt > 0 )

        memset(msg,0,strlen(msg));      //clear the message sent in
        strcpy(msg,newMessage);

        return 0;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::parseErrorMessage(char* msg,
//			MQLONG msgLengthReceived)
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: char* msg - the message to format,
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::parseErrorMessage(char* msg, char* newMessage)
{
	char* messagePtr = msg; // filler data

	int rc =0;

/*** Following new code for CR459  05/21/99 
        if(rc = parseMessage(messagePtr,
				USERIDPOSITION,
				(char *) "UserID",8,1, newMessage) != 0 ) //find UserID in message
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed",(char *) "",rc,(char *) "parseErrorMessage",(char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return 1;
		}
	endOfMessage(newMessage);
	if(rc = parseMessage(messagePtr,
				OWNERCODEPOSITION,
				(char *) "OWN",1,1, newMessage) != 0 ) //find ownCode in message
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed",(char *) "",rc, (char *) "parseErrorMessage", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return 1;
		}
	endOfMessage(newMessage);
***/
        if(rc = parseMessage(messagePtr,
				ERRORCODEPOSITION,
				(char *) "ReturnCode",2,1, newMessage) != 0 ) //find error code in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed ", (char *) "", rc, (char *) "parseErrorMessage", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}

#ifdef DEBUG
cout << "returnCode before check " << _returnCode[0] << endl;
#endif

	// do some checking on a valid returnCode 
	int error = atoi(_returnCode);
	if(error < 0 || error > 3)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because returnCode not valid ", (char *) "", (long)_returnCode, (char *) "parseErrorMessage", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);

	if(rc = parseMessage(messagePtr,ERRORCOUNTPOSITION,
		(char *) "ErrorMessageCount",3,1, newMessage) != 0) //find error code in message
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed", (char *) "", rc, (char *) "parseErrorMessage", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,
			_sendErrQ);
		return 1;
	}
	endOfMessage(newMessage);
	int countInt = atoi(errorMessageCount);
	if(countInt > 0 )
	{
		if((rc =parseMessage(messagePtr,
				ERRORMESSAGEPOSITION,
				(char *) "ErrorMessage", countInt*74,1, newMessage) )!=0)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because parseMessage() failed",(char *) "",rc,(char *) "parseErrorMessage",(char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return 1;
		}
		endOfMessage(newMessage);
	}//end of if(atoi(errorMessageCount) > 0 )

	// memset(msg,0,strlen(msg));	//clear the message sent in      CR 9194
	// strcpy(msg,newMessage);

	return 0;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::parseServiceOrder(char* msg,
//			MQLONG msgLengthReceived)
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: char* msg - the message to format,
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
int DSOImessageReply::parseServiceOrder(char* msg, MQLONG msgLengthReceived)
{
	int seqNo = 0;
	int secId = 0;
	int returnCode =0;
	char* msgPtr = msg + LINECOUNTPOSITION;
	char lineCtCh[7];
	char _sectionSeqNo[7] = {'0','0','0','0','0','0','0'}; 
	char _ULsectionSeqNo[7] = {'0','0','0','0','0','0','0'}; 
	strncpy(lineCtCh,msgPtr,6);
	lineCtCh[6] = '\0';
#ifdef DEBUG
cout << "lineCount=" << lineCtCh << endl;
#endif
	int action=0;
	int lineCnt = atoi(lineCtCh);
	int lengthNeeded = (int)msgLengthReceived + ((lineCnt+2)*20)  + REPLYADDCHARS;

	if( lengthNeeded  > _sizeOfServiceOrder)
	{
		if(_serviceOrder != '\0')
		{
			delete [] _serviceOrder;
		}

		_serviceOrder = new char[lengthNeeded];

		if( _serviceOrder == NULL )
		{
			messageErrObj.createReplyErrMsg((char *) "DSOIREPLY:unable to allocate new memory, DSOI must exit!");
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory, DSOI must exit!", (char *) "", -1, (char *) "parseServiceOrder", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return -1;
		}

		_sizeOfServiceOrder = lengthNeeded;
	}
	memset(_serviceOrder,0,_sizeOfServiceOrder);
	if(_data == '\0')
	{
		if( (_data = new char[100]) ==NULL)
		{
			messageErrObj.createReplyErrMsg((char *) "DSOIREPLY:failed because DSOI can't malloc:DSOI must exit!");
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because DSOI can't malloc:DSOI must exit!",
			(char *) "",
			-1,
			(char *) "parseServiceOrder",
			(char *) __FILE__,
			__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			return -1;
		}
		_sizeOfData = 100;
	}
	memset(_data,0,_sizeOfData);
	int dataLength;
	char CdataLength[3];
        if ((strncmp(_OriginalFunctionCode,"NE",2)==0) || (strncmp(_OriginalFunctionCode,"NR",2)==0))
	     msgPtr= msg+DATAPOSITION + 2; // CR 9194
	else msgPtr= msg+DATAPOSITION;     //advance to the Service Order data
	register int i;
	for(i =1; i <= lineCnt; i++)
	{
		seqNo++;
		sprintf(_SeqNo,"%06d",seqNo);
		strncpy(CdataLength,msgPtr,2);
		CdataLength[2] = '\0';
		dataLength = atoi(CdataLength);
		if(_sizeOfData < dataLength) 
		{
			if(_data != '\0')
				delete [] _data;
			_data = new char[dataLength+100];

			if( _data == NULL )
			{
				messageErrObj.createReplyErrMsg((char *) "DSOIREPLY:unable to allocate new memory, DSOI must exit!");
				messageErrObj.formatErrMsg((char *) "DSOIREPLY:unable to allocate new memory, DSOI must exit!", (char *) "", -1, (char *) "parseServiceOrder", (char *) __FILE__, __LINE__);
				messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
				return -1;
			}

			_sizeOfData = dataLength +100;
		}
		memset(_data,0,_sizeOfData);
		msgPtr+=2; //advance to element type
		strncpy(_elementType,msgPtr,1);
		_elementType[1]= '\0';
		int j =0;
		switch(_elementType[0])
		{
		case 'C':   //an 'L' left hand FID
	 		strcpy(_ULsectionSeqNo,_SeqNo); 
			strcpy(_elementType,"L");
			msgPtr++; //advance to action code OR tagName
			if( (strcmp(_SectionId,"00")!=0 ) &&
			   (strcmp(_SectionId,"03")!=0)  )
			{
				returnCode = checkActionCode(msgPtr);
				if(returnCode != -1) //action code present
				{
					action=1;
				}
			}
			if(action==1)
			{
				strncpy(_actionCode,msgPtr,1);
				_actionCode[1]= '\0';
				msgPtr++;
				strncpy(_tagName,msgPtr,4);
				_tagName[4] = ' ';
				_tagName[5] = '\0';
				msgPtr+=4;
				action=0;
			}
			else
			{
				strncpy(_tagName,msgPtr,5);
				_tagName[5]= '\0';
				msgPtr+=5;
			}
			strcpy(_ParentSeq,_sectionSeqNo);
			if(dataLength>0)
			{
				strncpy(_data,msgPtr,dataLength);
				_data[dataLength] = '\0';
			}
			break;
		case 'S':   // section header
			msgPtr++; //advance to the tag name
			strncpy(_tagName,msgPtr,5);
			_tagName[5]= '\0';
                	if(setSectionId(_tagName) != 0)    return -1;
			sprintf(_ParentSeq,"%06d",1);
			sprintf(_sectionSeqNo,"%06d",seqNo);
			msgPtr+=5; //advance to data
			strncpy(_data,msgPtr,dataLength);
			if(dataLength>0)
				_data[dataLength] = '\0';
			break;
		case 'F':   //floated FID
			strcpy(_elementType,"F");
			strcpy(_ParentSeq,_ULsectionSeqNo);
			msgPtr++; //advance to tagName
			strncpy(_tagName,msgPtr,5);
			_tagName[5] = '\0';
			msgPtr+=5; //advance to data
			strncpy(_data,msgPtr,dataLength);
			if(dataLength>0)
				_data[dataLength] = '\0';
			break;

		case 'A':   // USOC
	 		strcpy(_ULsectionSeqNo,_SeqNo); 
			strcpy(_ParentSeq,_sectionSeqNo);
			strcpy(_elementType,"U");
			msgPtr++; //advance to action code
			strncpy(_actionCode,msgPtr,1);
			_actionCode[1] = '\0';
			msgPtr++; //advance past action code
			char* msgPtr2 ;
			msgPtr2 = msgPtr;
			char* blankPtr;
			blankPtr = strchr(msgPtr,TOKENBLANK);
			strncpy(_data,msgPtr,blankPtr-msgPtr);
			if(blankPtr-msgPtr > 0)
				_data[blankPtr-msgPtr] = '\0';
			msgPtr = msgPtr2;
			msgPtr+=4;
			strncpy(_tagName,msgPtr,dataLength);
			while(j < 5-dataLength)
			{
				strcat(_tagName," ");
				j++;
			}
			break;
		case ' ': //blank means continuation of previous data
	 		strcpy(_ULsectionSeqNo,_SeqNo); 
			strcpy(_elementType," ");
			msgPtr++; //advance to tagName
			strncpy(_tagName,msgPtr,5);
			_tagName[5] = '\0';
			msgPtr+=5; //advance to data
			strncpy(_data,msgPtr,dataLength);
			if(dataLength>0)
				_data[dataLength] = '\0';
			break;
		case 'N': //continuation of previous data
	 		strcpy(_ULsectionSeqNo,_SeqNo); 
			strcpy(_elementType," ");
			msgPtr++; //advance to tagName
			strncpy(_tagName,msgPtr,5);
			_tagName[5] = '\0';
			msgPtr+=5; //advance to data
			strncpy(_data,msgPtr,dataLength);
			if(dataLength>0)
				_data[dataLength] = '\0';
			break;
		default:
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because element Type is invalid",_elementType,1,(char *) "parseServiceOrder",(char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			sprintf(_errorMessage,"%s%s%s","DSOIREPLY:failed because element Type is invalid  ",_elementType," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			return -1;
		} // end of switch
		mergeServiceOrder();
		endOfLine(_serviceOrder);
		msgPtr+=dataLength; //advance past data
		//some elements don't have action codes 
		//but a blank space instead so reset _actionCode
		memset(_actionCode,' ',2); 
		// and other members  clear data
		memset(_tagName,0,6);
		memset(_elementType,0,2);
	} // end of for loop
	memset(_data,0,_sizeOfData);
	register int length = strlen(_serviceOrder);
	_sizeNeeded = length + PARAMETERCHARS;
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::mergeServiceOrder ()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS:
//
//  RETURNS: 
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
void DSOImessageReply::mergeServiceOrder() 
{
		strncat(_serviceOrder,_SeqNo,6);
		strncat(_serviceOrder,_ParentSeq,6);
		strncat(_serviceOrder,_SectionId,2);
		strncat(_serviceOrder,_elementType,1);
		strncat(_serviceOrder,_actionCode,1);
		strncat(_serviceOrder,_tagName,5);
		strcat(_serviceOrder,_data);
}
////////////////////////////////////////////////
//  FUNCTION:
//	int DSOImessageReply::checkActionCode()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
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
int DSOImessageReply::checkActionCode(char* message)
{
	// the type of service without action codes
	const char* actionITable[] = { "IB   ", "ICM  ", "ICO  ", "ICR  ", 
				"IDGP ", "IDP  ", "ILT  ", "IMP  ", 
				"INNP ", "IOC  ", "IR   ", "ISC  ", 
				"ISHG ", "ITM  "};
	register int iAction = 14;

	// the type of service without action codes
	const char* actionOTable[] = {"OAB  ", "OBP  ", "OBS  ", "OCB  ", 
				"OCO  ", "OCP  ", "OCP3 ", "OCT  ", 
				"OE   ", "OFR  ", "OFN  ", "OGO  ", 
				"OPC  ", "ORG  ", "OST  ", "OTB  ", 
				"OTRAK"};
	register int oAction = 17;
	char* msgPtr = message;
	register int i;
	switch(msgPtr[0])
	{
		case 'I':
			for(i=0; i < iAction; i++ )
			{
				if(strncmp(msgPtr,actionITable[i],5)==0)
				{
					return -1;
				}
			}
			return 0;
		case 'O':
			for(i=0; i < oAction; i++ )
			{
				if(strncmp(msgPtr,actionOTable[i],5)==0)
				{
					return -1;
				}
			}
			return 0;
		default:
			return -1;
	}
}

////////////////////////////////////////////////
//  FUNCTION:
//	int DSOImessageReply::setSectionId()
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: 
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
int DSOImessageReply::setSectionId(char* typeOfSection)
{
	const char* sectionIdTable[] = {"NEW  ", "START", "PREAM",
  			 "ID   ", "IDEX ", "LIST ", "CTL  ",
			 "DIR  ", "TFC  ", "BILL ", "RMKS ",
			 "S&E  ", "STAT ", "ASGM ", "ZASGM",
			 "ERR  ", "END  ", "HDR  ", "ZAP  "};
	const char* idNumTable[] = {"00", "00", "01", 
			"03", "04", "05", "06",
			"07", "08", "09", "11",
			"10", "12", "13", "14",
			"15", "99", "02", "16"};
	register int numberOfEntries =  19;
	for(register int i=0; i < numberOfEntries; i++)
	{
// testing CR 2378  if(strcmp(typeOfSection,"LIST ") ==0)  i=20;
		if(strcmp(sectionIdTable[i], typeOfSection) ==0)
		{
			strcpy(_SectionId,idNumTable[i]);
			return 0;
		}
	}
	strcpy(_SectionId,"88");   // CR 7617
	return 0;
//	messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because Section Id is invalid",typeOfSection,1,(char *) "setSectionId", (char *) __FILE__,__LINE__);
//	messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
//	sprintf(_errorMessage,"%s%s%s","DSOIREPLY:failed because Section Id is invalid  ",typeOfSection," ");
//	messageErrObj.createReplyErrMsg(_errorMessage);
//	return -1;
}

////////////////////////////////////////////////
//  FUNCTION:
//	int DSOImessageReply::fillSpace(char* message, int number)
//
//  PURPOSE: To fill the message with blanks if the message
//	length is less than the 'Requestor' application
//	required message
//
//  METHOD: Using strcat() to fill the message with blanks.
//
//  PARAMETERS: char* message - the message to add the blanks
//		int number - the number of blanks
//  
//  RETURNS: One if 
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
void DSOImessageReply::fillSpace(char* message, int number)
{
	for(int i=number; i != 0; i--)
	{
		strcat(message," ");
	}
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::getFunction(char* message)
//
//  PURPOSE: 
//
//  METHOD:
//
//  PARAMETERS:
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS:
//
//  RETURNS:
// 
//  KNOWN USERS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////

int DSOImessageReply::getFunction(char* message)
{
	char* msgPtr = message + FUNCTIONCODEPOSITION;
#ifdef DEBUG
cout << "msg received = " <<  msgPtr << endl;
#endif
	if(strncmp(msgPtr,"SO",2)==0)
	{
#ifdef DEBUG
cout << "FunctionCode == SO" << endl;
#endif
		return 1;
	}
	if(strncmp(msgPtr,"OE",2)==0)
	{
#ifdef DEBUG
cout << "FunctionCode == OE" << endl;
#endif
		return 2;
	}
	if(strncmp(msgPtr,"EE",2)==0)
	{
#ifdef DEBUG
cout << "FunctionCode == EE" << endl;
#endif
		return 3;
	}
	if(strncmp(msgPtr,"CL",2)==0)
	{
#ifdef DEBUG
cout << "FunctionCode == CL" << endl;
#endif
		return 4;
	}
	if(strncmp(msgPtr,"BN",2)==0)
	{
#ifdef DEBUG
cout << "FunctionCode == BN" << endl;
#endif
		return 5;
	}
	if(strncmp(msgPtr,"CI",2)==0)
	{
#ifdef DEBUG
cout << "FunctionCode == CI" << endl;
#endif
		return 6;
	}
	if(strncmp(msgPtr,"SL",2)==0)
	{
#ifdef DEBUG
cout << "FunctionCode == SL" << endl;
#endif
		return 7;
	}
	char Function[3];
	strncpy(Function,msgPtr,2);
	Function[2] = '\0';
	messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because Function Code not found! ",Function,1,(char *) "getFunction", (char *) __FILE__,__LINE__);
	messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
	sprintf(_errorMessage,"%s%s%s","DSOIREPLY:failed because Function Code not found!  ",Function," ");
	messageErrObj.createReplyErrMsg(_errorMessage);
	return -1;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::parseMessage
//		(char* message,int bytePosition, 
//		char* flag , int offset, int equal
//
//  PURPOSE: Format the message to the 'Requestor' applciation
//	expected format
//	with ReturnCode=,ErrorMessageCount=, and ErrorMessage=
//
//  METHOD:
//
//  PARAMETERS:
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS: _sendErrQ - error queue to send error messages
//
//  RETURNS:
// 
//  KNOWN USERS:
//
//  FUNCTIONS USED:
//
//////////////////////////////////////////////////////
int DSOImessageReply::parseMessage(char* message,int bytePosition, 
				char* flag , int offset, int equal, char* newMessage)
{
	int length = strlen(message); 
	// check for pointer past the end of message
	
        if(bytePosition > length)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:failed because message length is less than bytes needed to find ",flag,1,(char *) "parseMessage", (char *) __FILE__,__LINE__);
		sprintf(_errorMessage,"%s%s%s","DSOIREPLY:failed because message length is less than bytes needed to find  ",flag," ");
		messageErrObj.createReplyErrMsg(_errorMessage);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		return -1;
	}
	char* messagePtr = message+bytePosition;
	if(equal)
	{
		strcat(newMessage,flag);
		strcat(newMessage,"=");
		if(strcmp(flag,"ErrorMessageCount") ==0 )
		{
			char* errorCountPtr = messagePtr;
			strncpy(errorMessageCount,errorCountPtr,offset);
			errorMessageCount[offset]='\0';
		}
		else if(strcmp(flag,"ReturnCode") ==0 )
		{
			char* returnCodePtr = messagePtr;
			strncpy(_returnCode,returnCodePtr,offset);
			_returnCode[offset]='\0';
		}
		else if(strcmp(flag,"FunctionCode") ==0 )
		{
			char* FunctionCodePtr = messagePtr;
			strncpy(_FunctionCode,FunctionCodePtr,offset);
			_FunctionCode[offset]='\0';
		}
		else if(strcmp(flag,"UserID") ==0 )
		{
			char* UserIdPtr = messagePtr;
			strncpy(_UserId,UserIdPtr,offset);
			_UserId[offset]='\0';
		}
		else if(strcmp(flag,"LSRID") ==0 )
		{
			char* LsrIdPtr = messagePtr;
			strncpy(_LsrId,LsrIdPtr,offset);
			_LsrId[offset]='\0';
		}
		else if(strcmp(flag,"CandidateCount") ==0 )
		{
			char* CandidateCountPtr = messagePtr;
			strncpy(_candidateCount,CandidateCountPtr,offset);
			_candidateCount[offset]='\0';
		}
		else if(strcmp(flag,"CandidateList") ==0 )
		{
	                strncat(newMessage,messagePtr,offset);
			return 0;
		}
		else if(strcmp(flag,"SbnQty") ==0 )
		{
			char* SbnQtyPtr = messagePtr;
			strncpy(_SbnQty,SbnQtyPtr,offset);
			_SbnQty[offset]='\0';
		}
		else if(strcmp(flag,"SpBillNo") ==0 )
		{      char * idx;  
                       idx=strchr(messagePtr,'\x9f'); //search for hex9F
                       if (idx != NULL)  
                         { offset = idx - messagePtr;
                         }
	                strncat(newMessage,messagePtr,offset);
			return 0;
		}
		else if(strcmp(flag,"CiQty") ==0 )
		{
			char* CiQtyPtr = messagePtr;
			strncpy(_CiQty,CiQtyPtr,offset);
			_CiQty[offset]='\0';
		}
		else if(strcmp(flag,"CircuitIdentifier") ==0 )
		{      char * idx;  
                       idx=strchr(messagePtr,'\x9f'); //search for hex9F
                       if (idx != NULL)  
                         { offset = idx - messagePtr;
                         }
	                strncat(newMessage,messagePtr,offset);
			return 0;
		}
		else if(strcmp(flag,"OWN") ==0 )
		{
			char* OwnerCodePtr = messagePtr;
			strncpy(_ownCode,OwnerCodePtr,offset);
			_ownCode[offset]='\0';
		}
		else if(strcmp(flag,"StatusListCount") ==0 )
		{
			char* StatusCountPtr = messagePtr;
			strncpy(_statusCount,StatusCountPtr,offset);
			_statusCount[offset]='\0';
		}
		else if(strcmp(flag,"StatusList") ==0 )
		{
	                strncat(newMessage,messagePtr,offset);
			return 0;
		}
	}
	strncat(newMessage,messagePtr,offset);
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION:
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS:
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
int DSOImessageReply::endOfLine(char* message)
{
	int length = strlen(message);
	message[length] = '\n';
	return 0;
}
////////////////////////////////////////////////
//  FUNCTION:
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS:
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
int DSOImessageReply::endOfMessage(char* message)
{
	const char* hexName = "\xff";
	int length = strlen(message);
	char* strPtr = message+length;
	strcat(strPtr,hexName);
#ifdef DEBUG
cout << "newMessage in endOfMessage " << message << endl;
#endif
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::checkIfRsolar(char* msg, 
//						MQLONG msgLengthReceived)
//
//  PURPOSE: Checks the input to DSOI to verify that there
//	are parameters sent in the standard way.  For example,
//	FunctionCode="string if data". 
//
//  METHOD: Has a table of all possible required parameters.
// 	Compares what is in the message sent to what is required
//  and determines of all the parameters are sent. 
//
//  PARAMETERS:
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS:
//
//  RETURNS:
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
int DSOImessageReply::checkIfRsolar(char* msg)
{
#ifdef DEBUG
cout << "message sent to checkIfRsolar" << msg << endl;
cout << " size of message sent " << strlen(msg) << endl;
#endif
	char  functionCode[3] = "  ";


	// char* parameterList[] = {"FunctionCode=","UserID=","NPANXX=",
        //                          "LSRID=","OWN=","SpBillNo=",
        //                          "CiQty=","CircuitIdentifier"};
	// const int NUMBEROFPARAMETERS=8;

	const char* parameterList[] = {"FunctionCode=","UserID=","LSRID=",
               		         "NPANXX="};
	const int NUMBEROFPARAMETERS=3;

	functionCode[0] = '\0'; 
	int foundOne=0;
	char* msgPtr = msg;
	for (int i =0; i < NUMBEROFPARAMETERS; i++)
	{
		msgPtr = strstr(msgPtr,parameterList[i]);
		if(msgPtr !=NULL)
		{
			foundOne++;
			if (i == 0)
			  { int idx = msgPtr - msg;
			    functionCode[0] = msg[idx+13];
			    functionCode[1] = msg[idx+14];
			    functionCode[2] = '\0';
			  }
		}
		msgPtr = msg;
	}
	if(foundOne > 0 )
	{
		if(foundOne <  NUMBEROFPARAMETERS)
		{
		//error
			char errorMsg[100];
			strncpy(errorMsg,msg,99);
			errorMsg[99] = '\0';
			messageErrObj.formatErrMsg((char *) "DSOIREPLY:1011:CRITICAL:failed because not all parameters were sent from the RSOLAR SOP",errorMsg,-1,(char *) "checkIfRsolar", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
			messageErrObj.createReplyErrMsg((char *) "DSOIREPLY:failed because not all parameters were sent from the SOP");
			return -2;
		}
                if (strncmp(functionCode, "RQ",2) !=0)
		     { int returnCode=0;
		       if (returnCode = find9F(msg) !=0)
		          {
	        	    return -2;   //error
		          }
		     }
		else { int returnCode=0;
		       if (returnCode = parseMsgRQ(msg) !=0)
		          {
	        	    return -2;   //error
		          }
		     }
		return 0;
	}
	//not RSOLAR
		return -1;
}
////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::findNPANXX ()
//
//  PURPOSE:  To find the NPANXX value in the HOLDQ 
//	information so that it can be sent back to the 
//  requesting applciation.
//
//  METHOD: The message received from the HOLDQ is 
//	sent in as the msg parameter.  The NPANXX token is
//	looked for in the message.  It is copied into the 
//	_npaNxx private member and held until DSOI sends a reponse
//	back to the requesting application. It is updated with 999999
//	if the NPANXX value is not found in the msg.
//
//  PARAMETERS: cahr* msg - the message received from the HOLDQ
//  
//  OTHER OUTPUTS:  _npaNxx is updated with the NPANXX value
//	or 999999 if it is not found
//  
//  RETURNS:  -1 if the NPANXX value is not found, 0 otherwise.
// 
//  FUNCTIONS USED:  strstr(), sprintf(), strncpy()
//
//////////////////////////////////////////////////////
int DSOImessageReply::findNPANXX(char* msg) 
{
	char* npaPtr;
	if( (npaPtr=strstr(msg,"NPANXX=")) != NULL)
	{
		strncpy(_npaNxx,npaPtr+7,6);
		_npaNxx[6] = '\0';
		return 0;
	}
	sprintf(_npaNxx,"%6d",999999);
	return -1;
}


////////////////////////////////////////////////
//  FUNCTION:
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS:
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS:
//
//  RETURNS:
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
int DSOImessageReply::setONS(char* msg, char* newMessage)
{
	const char* hexName = "\xff";
	sprintf(newMessage,"%s%s%s%s%s",
					"FunctionCode=ON",hexName,
					"ONSMessage=",msg,hexName);
#ifdef DEBUG
cout << "newMessage in setONS" << newMessage << endl;
#endif
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::find9F(char * str)
//
//  PURPOSE: To find a character (9F) that MQSeries
//	convert the EBCIDIC hexFF character to this 
//	9F character.
//
//  METHOD: To spin through the message returned from
//	the MQ Series queue after it has been converted from
//	EBCIDIC to ASCII.  If the 9F(ascii unknown character)
//	character is found then change
//	it to a hexFF.  
//
//  PARAMETERS: char* str - the message returned from getMsg()
//	call to MQGET.
//  
//  OTHER OUTPUTS: Changes the message to contain the hexFF instead
//	of the 9F character.
//  
//  RETURNS: 0 if the character is found and -1 if it is not found
// 
//  KNOWN USERS: DSOImessageReply::checkIfRsolar()
//
//////////////////////////////////////////////////////
int DSOImessageReply::find9F(char * str)
{
	int i,l,e,found =0 ;
   
   	l = strlen(str);
   	for (i=0;i < l;++i)
   	{
		e = (255 - (USHRT_MAX - (unsigned short) str[i]));
		if (e < 0)
		{
			e = 65280 + e;
		}
		if(e == 159)
		{
			str[i] = 255; //a hexFF character in ASCII
			found =1;
		}
#ifdef DEBUG
 cout << "ASCII Character" << i <<  " is" << str[i] << endl;
#endif
 	}
	if(!found)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREPLY:1010:CRITICAL:failed because the expected ASCII 9F character was not found from MQSeries converting the RSOLAR message, DSOIReply can not continue!",(char *) "",-1,(char *) "find9F", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg,_sendErrQ);
		messageErrObj.createReplyErrMsg((char *) "DSOIREPLY failed because the expected ASCII 9F character was not found from MQSeries converting the RSOLAR message  DSOIReply can't continue!");
		return -1;
	}
 
   return(0);
}


////////////////////////////////////////////////
//  FUNCTION: int DSOImessageReply::getClientSopId(char*)
//
//  PURPOSE: Add SOP Id tag/value to reply message if client wants it
//
//  METHOD:
//
//  PARAMETERS:
//
//  OTHER OUTPUTS:
//
//  RETURNS: 0; 1 on error
//
//  KNOWN USERS:
//
//////////////////////////////////////////////////////

int DSOImessageReply::getClientSopId(char* sopIdValue, char* holdmsg)
{
        const char* SopIdTag = "SopId=";
        char* tagValue=  new char[10];
        int returnCode = 0;

	// SOLAR South
	if ( strstr(_requestQueue, "_XJ_") )
		strcpy(sopIdValue, "ESS");

	// SOLAR North
	else if ( strstr(_requestQueue, "_XK_") )
		strcpy(sopIdValue, "ESN");

	// SOPAD Utah
	else if ( strstr(_requestQueue, "_QS_") )
		strcpy(sopIdValue, "SOU");

	// SOPAD Colorado
	else if ( strstr(_requestQueue, "_RN_") )
		strcpy(sopIdValue, "SOC");

	// RSOLAR
	else if ( strstr(_requestQueue, "_GI_") )
		strcpy(sopIdValue, "RSO");

	// ONS
	else if ( strstr(_requestQueue, "_ONS_") )
		strcpy(sopIdValue, "RSO");

	// 4P RSOLAR
	else if ( strstr(_requestQueue, "_4P_") )
		strcpy(sopIdValue, "4RS");

	// SOP unknown
	else
	  { if ( strstr(_requestQueue, "DSOIROUTEREPLY") )
              { memset(tagValue,0,10);
                if (findTagValue(holdmsg, (char *)SopIdTag, tagValue) == 0)
                     strcpy(sopIdValue, tagValue);
                else strcpy(sopIdValue, "XXX");
              }
 	    else strcpy(sopIdValue, "XXX");
          }
        delete []tagValue;
        return returnCode;
}

////////////////////////////////////////////////
//  FUNCTION:int findTag9f
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
int DSOImessageReply::parseMsgRQ(char * str)
{ 
	char* tagVal=  new char[100];
	char* tmpMsg=  new char[strlen(str)];
	char* str_ptr;
	int lngth = strlen(str);

	tmpMsg[0] = '\0';
	tagVal[0] = '\0';
	if (findTagVal9f(str, "OWN=", tagVal) == 0)
	  { strcat(tmpMsg,"OWN=");
	    strcat(tmpMsg,tagVal);
	    strcat(tmpMsg,"\xff");
	  }
	tagVal[0] = '\0';
	if (findTagVal9f(str, "FunctionCode=", tagVal) == 0)
	  { strcat(tmpMsg,"FunctionCode=");
	    strcat(tmpMsg,tagVal);
	    strcat(tmpMsg,"\xff");
	  }
	tagVal[0] = '\0';
	if (findTagVal9f(str, "LSRID=", tagVal) == 0)
	  { strcat(tmpMsg,"LSRID=");
	    strcat(tmpMsg,tagVal);
	    strcat(tmpMsg,"\xff");
	  }
	tagVal[0] = '\0';
	if (findTagVal9f(str, "OrderType=", tagVal) == 0)
	  { strcat(tmpMsg,"OrderType=");
	    strcat(tmpMsg,tagVal);
	    strcat(tmpMsg,"\xff");
	  }
	tagVal[0] = '\0';
	if (findTagVal9f(str, "OrderNumber=", tagVal) == 0)
	  { strcat(tmpMsg,"OrderNumber=");
	    strcat(tmpMsg,tagVal);
	    strcat(tmpMsg,"\xff");
	  }
	tagVal[0] = '\0';
	if (findTagVal9f(str, "UserID=", tagVal) == 0)
	  { strcat(tmpMsg,"UserID=");
	    strcat(tmpMsg,tagVal);
	    strcat(tmpMsg,"\xff");
	  }
	tagVal[0] = '\0';
	if (findTagVal9f(str, "RQMSG=", tagVal) == 0)
	  { strcat(tmpMsg,"RQMSG=");
	    strcat(tmpMsg,tagVal);
	    strcat(tmpMsg,"\xff");
	  }
	tagVal[0] = '\0';
	if ((str_ptr = strstr(str,"RQDATA=")) != NULL)
	  { strncat(tmpMsg,"RQDATA=",7);
	    strcat(tmpMsg,str_ptr+7);
	    tmpMsg[lngth - 2] = '\xff';
	  }
	str[0] = '\0';
	strcpy(str, tmpMsg);
	delete tagVal;
	delete tmpMsg;

  return 0;
}

////////////////////////////////////////////////
//  FUNCTION:int findTag9f
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
int DSOImessageReply::findTagVal9f(char* msg, const char* tagType, char *tagValue)
{
	char* msgPtr = msg;
	char* equalPtr;
	char* hexPtr;
	int size =0;
	msgPtr = msg;
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
	hexPtr = strchr(msgPtr,'\x9f');  //locate delimiter x9f
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

	return 0;
}



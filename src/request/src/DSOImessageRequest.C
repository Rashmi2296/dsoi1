/////////////////////////////////////////////////////  
//  REVISION HISTORY:
//---------------------------------------------------------------------
//  version	date		author	description
//---------------------------------------------------------------------
//  1.00.00	06/11/97	lparks	initial checkin IMA
//  2.00.01	09/24/97	lparks	initial checkin Central
//  2.00.02	10/09/97	lparks	removed code for hostName
//  2.00.04	10/16/97	lparks	added '435' NPA and '208' NXX
//  2.00.04	10/00/97	lparks	release for Eastern with data reference
//					update capability
//  2.00.04	10/00/97	lparks	changed the linecount to the last six
//					characters of the filler info
//  2.00.04	10/00/97	lparks	added the userid for IMA
//  4.00.00	02/26/98	lparks	western release and ONS message
//  4.00.00	11/02/98	rfuruta	CR3151, add ownerCode tag
//  5.00.00     05/14/99        rfuruta CR338, changed required tags for an ONS request
//  5.00.00     05/14/99        rfuruta CR441, allow completion function CP for all regions
//  5.00.00	05/14/99	rfuruta	CR454, send cuid to SOPAD instead of typists initials
//  6.00.00	06/15/99	rfuruta	CR468, new function AM, Special Billing Number
//  6.00.00	06/15/99	rfuruta	CR469, new function CK, Circuit Identifier
//  6.01.00     12/15/99        rfuruta CR3064, add SbnType tag to Special Billing Number
//  6.02.00     02/01/00        rfuruta CR3299, accept 2 character OrderType
//  7.00.00     12/23/99        rfuruta CR3123, changed DSOIrouteMQ to DSOIdistribute
//  7.01.00     07/03/00        rfuruta CR 4275, 4P connection
//  7.01.00     08/25/00        rfuruta CR 4783, removed '<' and '>' from error msg
//  8.00.00     08/23/00        rfuruta CR 4767, remove 4P connection
//  8.03.00     06/07/01        rfuruta CR 5391, add LSRID to report
//  8.03.00     07/25/01        rfuruta CR 6781, Select Section from service order
//  8.05.00     03/19/02        rfuruta CR 7541, Compute OwnershipCode and pass to SOPs
//  8.07.00     08/22/02        rfuruta CR 7826, Allow for variable length LSRID
//  8.08.00     01/02/03        rfuruta CR 8000, upgrade to HP-UX 11.0
//  8.09.00     03/02/03        rfuruta CR 7999, Shorten error message
//  8.09.00     03/02/03        rfuruta CR 8037, Add owner code to logfile report
//  8.09.00     03/22/03        rfuruta CR 8180, fix memory leaks
//  8.09.00     03/27/03        rfuruta CR 8144, ROMS officeCd and typistId
//  9.00.02     07/15/03        rfuruta CR 8410, _npaSplit changed to Northern Idaho Exchanges
//  9.00.01     07/15/03        rfuruta CR 8509, shorten DSOIRequest error message
//  9.03.00     12/08/03        rfuruta CR 8761, New function codes(NQ, NT) for WebSOP
//  9.04.00     02/08/04        rfuruta CR 8948, write 'IQ' txns to separate queue
//  9.04.00     03/08/04        rfuruta CR 8710, New function NQ for SOPAD
//  9.06.00     05/10/04        rfuruta CR 9194, New function codes (NU, NR, NE)
//  9.06.00     08/16/04        rfuruta CR 9413, Check for other border towns, in addition to 208
//  9.08.00     01/03/05        rfuruta CR 9745, set Priority
//  9.08.00     01/18/05        rfuruta CR 9665, pass all QueryListTypes to SOLAR
//  9.08.00     01/20/05        rfuruta CR 9667, new function 'SQ' for RSOLAR
//  9.08.00     01/20/05        rfuruta CR 9791, new function 'NL' nolock for RSOLAR, SOLAR, SOPAD
//  9.09.00     03/15/05        rfuruta CR 9664, new tags for SBN-  add AUXIND for SOLAR
//                                               add BILLPERIOD,BULKNMBR,BROADCAST,OVERRIDE for RSOLAR
//  9.10.00     05/16/05        rfuruta CR 9898, new rate quote function 'RQ' for RSOLAR
// 10.01.00     07/05/05        rfuruta CR10094, ECRIS error correction
// 10.00.00     07/26/05        rfuruta CR10186, write 'CL' txns to separate queue
// 10.02.00     09/20/05        rfuruta CR10269, new function 'DX' for SOLAR- retrieve CSR data
// 10.04.00     02/01/07        rfuruta CR10999, migrate DSOI to Solaris
// 10.05.00     07/01/12        rfuruta CR11307, make the ServiceOrder tag/values optional on a DL function
// 11.17.00     01/31/11        agujjal CR 21,   Border town changes added new npa nxx combinations
// 11.18.00     02/07/11        agujjal CR 22,   Border town changes removed npa 507, and added new npa nxx combinations
//  
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOImessageRequest.H"
#include "DSOIThrowError.H"
#include "DSOItoolbox.H"
#include <cstdio>   
#include <cstddef>
#include <ctime>
#include <cctype>
#include <new>
#include <unistd.h>
#include <string.h>

char* DSOImessageRequest::data = '\0';
char** DSOImessageRequest::_npaSplit = '\0';
char** DSOImessageRequest::_routeInfo = '\0';
char** DSOImessageRequest::_ownerInfo = '\0';
int DSOImessageRequest::_sizeOfNpaSplit = 0;
int DSOImessageRequest::_sizeOfRouteInfo = 0;
int DSOImessageRequest::_sizeOfOwnerInfo = 0;

#define EQUAL    '='
#define HEXFF    '\xff'
#define TOKENNEWLINE	'\n'
#define TYPEOFELEMENT 		14 	// position in Service Order 
#define TAGNAME 			16 	// position in Service Order 
#define USOCTAGNAME			15
#define USOCDATA			16
#define CHARSSUBTRACTED 	21	//characters not counted for data
#define FILLERINFO			23
#define DSOIADDCHARS		130

const int SIZEOFARRAY = 	35;
const int FOURTEENSTATES=	14;
const int TAGVALFORNPA = 	9;
const int TAGVALFORROUTE = 	10;
const int CHANNELPOSITION = 22;
const int SOPNAMEPOSITION = 8;

char DSOImessageRequest::_QueueName[] = {'n','o','n','e'};

// define the static members of the DSOImessageRequest class

//private members
DSOIerrorHandler DSOImessageRequest::messageErrObj;
char* DSOImessageRequest::QueryData = '\0';
int DSOImessageRequest::actionCode = 0;
int DSOImessageRequest::_sizeOfNewMessage = 100000;
int DSOImessageRequest::_sizeOfTag = 0;
int DSOImessageRequest::_FunctionType =0;
char DSOImessageRequest::_formatCode[] = {'X','X'};
char DSOImessageRequest::elementType[] = {0, 0};
char DSOImessageRequest::QueryListType[] = {'0','0','0','0'};
char DSOImessageRequest::orderType[] = {0,0,0};
char DSOImessageRequest::dataLength[] = {0,0,0};
char DSOImessageRequest::npa[] = {'9','9','9', '9'};
char DSOImessageRequest::tagName[] = {0,0,0,0,0,0};
char DSOImessageRequest::lineCount[] = {'0','0','0','0','0','0'};
char DSOImessageRequest::orderNumber[] = {0,0,0,0,0,0,0,0,0};
char DSOImessageRequest::_userId[] = {0,0,0,0,0,0,0,0,0};
char* DSOImessageRequest::tagVal[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char DSOImessageRequest::negotiationNumber[] = {0,0,0,0,0,0,0,0,0,0,0};
char DSOImessageRequest::ownCode[] = {' ',' '};
char DSOImessageRequest::NpaNxx[] = {' ',' ',' ',' ',' ',' ',' '};
char DSOImessageRequest::KeyType[] = {' ',' '};
char DSOImessageRequest::Exchange[] = {' ',' ',' ',' ',' '};
char DSOImessageRequest::CustName[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
char DSOImessageRequest::SbnQty[] = {0,0,0};
char DSOImessageRequest::SbnType[] = {' ',' '};
char DSOImessageRequest::AUXIND[] = {' ',' '};
char DSOImessageRequest::ClassSvc[] = {' ',' ',' ',' ',' ',' '};
char DSOImessageRequest::ServiceCode[] = {' ',' ',' ',' ',' '};
char DSOImessageRequest::CiPrefix[] = {' ',' ',' '};
char DSOImessageRequest::CiQty[] = {0,0,0};
char DSOImessageRequest::_msgTrunc[450] = "";
DSOIsendMQ* DSOImessageRequest::_sendErrQ = 0;
char DSOImessageRequest::_errorMessage[1024] = "";
char DSOImessageRequest::useridtemp[] = {0,0,0,0,0,0,0,0,0};

//public members
char* DSOImessageRequest::newMessage=  new char[_sizeOfNewMessage];

////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOImessageRequest::messageMassage
//
//  PURPOSE: To manipulate the message received from a "REQUESTOR"
//			 application on the 
//           MQ Series message queue for readiness to send to
//           the Service Order Processor.
//
//  METHOD: Parses the message received into acceptable format for the
//          SOP. 
//
//  PARAMETERS: char* msg - the message to format received from the 
//	'Requestor' .
//		MQLONG msgLengthReceived - the length of the message received
//			from the MQGET() call
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS:_sendErrQ  - the message queue to send error messages
//
//  RETURNS: Returns one if any of the function calls fail or the message
//	was not successfully formatted and zero otherwise.
// 
//  FUNCTIONS USED: DSOImessageRequest::findEqualTags(),
//	DSOImessageRequest::fillerSOP(),
//	DSOImessageRequest::setPreSoc(),
//	DSOImessageRequest::elementLookup(),
//	DSOImessageRequest::mergeSOC(), memset(),strcpy(),strcat(),
//	sprintf()
//
////////////////////////////////////////////////////////////////////////
int DSOImessageRequest::messageMassage(char* functionCode, MQLONG msgLengthReceived,
                        DSOIsendMQ* sendErrQ, char* sopId, char* sopToSend, char* replyFromSop,
                        char* ownerName, char* ownerCode, char* setPriority, char* msg)
{       
	const char* OwnerTag = "OWN=";
        char* tagData=  new char[10];

	int returnCode = 0;
#ifdef DEBUG
cout << "in messageMassage " << msg << endl;
#endif
	_sendErrQ = sendErrQ;
	char* msgPtr = msg;
	
        memset(tagData,0,10);
        tagData[0] = '\0';
        if (findTagValue(msg, OwnerTag, tagData) == 0)
          { strcpy(ownerCode, tagData);
            ownerCode[1] = '\0';
            char* ownerTemp=  new char[2];
            if (verifyOWN(ownerName,ownerTemp, setPriority) != 0)
              { strcpy(setPriority,"0");
              }
            setPriority[1] = '\0';
            delete ownerTemp;
          }
        else
          { if (verifyOWN(ownerName,ownerCode, setPriority) != 0)
              { strcpy(ownerCode,"D");
                strcpy(setPriority,"0");
              }
            setPriority[1] = '\0';
            ownerCode[1] = '\0';
          }

        delete [] tagData;

        returnCode = 0;
        returnCode = getSopId(functionCode, ownerCode, sopId, sendErrQ, msgPtr);
	if(returnCode < 0)
        {
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because getSopId failed", (char *) "", -1,(char *) "messageMassage", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		deleteTagVals();
		return returnCode;
        }
        returnCode = 0;
	returnCode = parseMessage (functionCode, sopId, sopToSend, replyFromSop, msgPtr);
      
	if(returnCode < 0)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because findEqualTags failed" , (char *) "", returnCode, (char *) "messageMassage", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);

		deleteTagVals();
		return returnCode;
	}
	if(_sizeOfNewMessage < msgLengthReceived+DSOIADDCHARS)
	{
		delete [] newMessage;
		if( (newMessage = new char[msgLengthReceived+DSOIADDCHARS]) ==NULL)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" , (char *) "", -1, (char *) "messageMassage",(char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		_sizeOfNewMessage = (int)msgLengthReceived+DSOIADDCHARS;
	}
	memset(newMessage,0,_sizeOfNewMessage);
	if(strcmp(functionCode,"ON")==0)
	{
		strcpy(newMessage,tagVal[11]);
		return 0;
	}

	if(strcmp("RSO", sopId) == 0)
	{
		memset(newMessage,0,_sizeOfNewMessage);
		setNewRsolarMessage(sopId, ownerCode, functionCode,  msg);
#ifdef DEBUG
cout << "RSOLAR new Message = " << newMessage << endl;
#endif
		return 0;
	}

	fillerSOP(newMessage);
	setPreSoc(functionCode, ownerCode, returnCode, newMessage);

	switch (returnCode)
	{   case 1:  //EN || CO || RP ||DD || CP || NT || NE || NR
		setPreServiceOrder(newMessage);
		if(parseServiceOrder(newMessage)!=0)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because parseServiceOrder failed", (char *) "", -1,(char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			deleteTagVals();
			return -1;
		}
		if(tagVal[6] !=0)
		{
			tagVal[6] = 0;
		}
                break;
	
	    case 2:  //IQ || UP || QS || SS || NQ || NU
		setKeyType(newMessage);
		endOfMessage(newMessage);
                break;

	    case 3:  //UN || NL
		setHeader(functionCode, newMessage);
		endOfMessage(newMessage);
                break;
	 
	    case 4:  //CL
		setCLHeader(sopId, newMessage);
		endOfMessage(newMessage);
                break;
	    
            case 7:  //AM
		setAMHeader(newMessage);
		endOfMessage(newMessage);
                break;
            
            case 8:  //CK
		setCKHeader(newMessage);
		endOfMessage(newMessage);
                break;
	 
	    case 11:  //DX
		setDXHeader(sopId, newMessage);
		endOfMessage(newMessage);
                break;
	    
            default:
	     {
		memset(_errorMessage,0,1024);
		sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because returnType from getFunction invalid   ", functionCode," ");
		messageErrObj.createReplyErrMsg(_errorMessage);
        	messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because returnType from getFunction invalid", (char *) "", -1, (char *) "messageMassage", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		deleteTagVals();
		return -1;
	     }

        } //end of switch

#ifdef DEBUG
cout << "newMesage= " << newMessage << endl;
#endif
	deleteTagVals();
	return 0;
}

////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOImessageRequest::parseServiceOrder
//
//  PURPOSE: 
//
//  METHOD: Parses the Service Order as received from the requestor
//
//  PARAMETERS: char* msg - the message 
//		MQLONG msgLengthReceived - the length of the message received
//			from the MQGET() call
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS: _sendErrQ  - the message queue to send error messages
//
//  RETURNS:
// 
//  FUNCTIONS USED:
//
////////////////////////////////////////////////////////////////////////
int DSOImessageRequest::parseServiceOrder(char* new_Msg)
{
	char*	line = tagVal[6];
	long int count=0;
	int returnCode = 0;
	while ( (line= strchr(tagVal[6], TOKENNEWLINE)) != NULL)
	{
		returnCode = elementLookup(tagVal[6]);
		if(returnCode == 0) //element found
		{
			if(elementType[0] == 'A') //USOC
			{
				char* linePtr = tagVal[6] + USOCDATA;
				int length =0;
				char* movePtr = linePtr;
				for(int i = 0; i<5; i++)
				{
					if(*movePtr != ' ')
					{
						length++;
					}
					movePtr++;
				}
				if( (data = new char[length+1]) ==NULL)
				{
					messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" );
					messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" , (char *) "", -1, (char *) "parseServiceOrder",(char *) __FILE__, __LINE__);
					messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
					return -1;
				}
				memset(data,0,length+1);
				strncpy(data,linePtr,length);
				data[length] = '\0';
				//find all the lower case letters and change
				// to upper case because the REQUESTOR may
				//send lower case letters and the SOP will
				// not accept them (maybe).
				//for(i =0; i< length; i++)
				//{
				//	if(islower(data[i]))
				//		data[i] = (char)toupper(data[i]);
				//}
				sprintf(dataLength,"%02d",length);
			} // end of if elementType[0] == 'A'
			else	// not a USOC 
		 	{	
				if(line-tagVal[6]-CHARSSUBTRACTED <=0)// no data
				{
					strncpy(dataLength,"00",2);
					data = '\0';
				}
				else // data
				{
					sprintf(dataLength,"%02d",line-tagVal[6]-CHARSSUBTRACTED);
					char* dataPtr = tagVal[6] + CHARSSUBTRACTED;
					if( (data = new char[line-tagVal[6]-CHARSSUBTRACTED+1]) ==NULL)
					{
						messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" );
						messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" , (char *) "", -1, (char *) "parseServiceOrder",(char *) __FILE__, __LINE__);
						messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
						return -1;
					}
					memset(data,0,line-tagVal[6]-CHARSSUBTRACTED+1);
					strncpy(data,dataPtr,line-tagVal[6]-CHARSSUBTRACTED);
					data[line-tagVal[6]-CHARSSUBTRACTED] = '\0';
					int length = strlen(data);
					//for(int i =0; i< length; i++)
					//{
					//	if(islower(data[i]))
					//		data[i] = (char)toupper(data[i]);
					//}
#ifdef DEBUG
cout << "length of data= " << length << endl;
#endif
					if(length > 99 ) //data length only two characters
					{
						memset(_errorMessage,0,1024);
						sprintf(_errorMessage,"%s%06d%s","DSOIREQUEST:failed because length of data greater than 99  " ,dataLength," ");
						messageErrObj.createReplyErrMsg(_errorMessage);
						messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because length of data greater than 99= " ,dataLength, -1, (char *) "parseServiceOrder",(char *) __FILE__, __LINE__);
						messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
						return -1;
					}
				} // end of else data
			} // end of not a USOC
			count++; //count number of lines in SOC
			mergeSOC(new_Msg);
		} // end of if returnCode == 0
		else if (returnCode == 1)	//element NOT FOUND
		{ //error  
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because elementLookup failed", 
					(char *) "",
					returnCode, 
					(char *) "parseServiceOrder",
					(char *) __FILE__,
					__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return 1;
		}
		line++;
		tagVal[6] = line;
		reInitMembers();
	} // end of while


	char* lineCountPtr = new_Msg+FILLERINFO;
#ifdef DEBUG
cout << "count= " << count << endl;
#endif
	sprintf(lineCount,"%06d", count);
	strncpy(lineCountPtr,lineCount,6); 	//does not insert a NULL
	endOfMessage(new_Msg);
#ifdef DEBUG
cout << "after endOfMessage " << new_Msg << endl;
#endif
	sprintf(lineCount,"%06d", 000000);		//reinitialize lineCount
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION:int DSOImessageRequest::parseMessage
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char* msg - message to parse functionCode type
//	values parsed from the message. 
//	int start - the index of the member of the tagType table to start
//	the search
//	int end - the index of the tagTaype table where the search ends
//  
//  OTHER OUTPUTS:  tagVal[] - this array is updated with the data
//	of the tagTaype member searched.
//	_FunctionType - updates the type of function code so that the
//	comparison does not have to be done by any other function.
//  
//  OTHER INPUTS: _sendErrQ - send queue to send error messages
//
//  RETURNS: -1 if the fucntion code is not found or it is not a valid
//	function code, returns zero otherwise.
// 
//  FUNCTIONS USED: DSOIerrorHandler::createReplyErrMsg(),
//		DSOIerrorHandler::sendErrorMessage(),
//		strcat(), strcmp()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::parseMessage(char* functionCode, char* sopId, 
                                     char* sopToSend,  char* replyFromSop, char* msgPtr)
{
	_FunctionType = 0;
	int returnCode = 0;
//	char* msgPtr = msg;

        if(findEqualTags(msgPtr,0,0)!=0)         // get FunctionCode value
        {       return -1;
        }

	if(strcmp("ON", tagVal[0] ) != 0) 
        {
	  if(findEqualTags(msgPtr,1,3)!=0) 
	  {
#ifdef DEBUG
cout << "findEqualTags FAILED" << endl;
#endif
		return -1;
          }
	}

        if (verifySopId(functionCode,sopId,useridtemp,sopToSend,replyFromSop) !=0) //added for userId to routed to seperate Q
	      {
                  //send the error message from verifySopId
		  messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because INVALID SopId ");
	          messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because INVALID SopId ", (char *) "", -1, (char *) "parseMessage", (char *) __FILE__, __LINE__);
	          messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                  return -1;
              }

	if (strcmp("ON", tagVal[0] ) != 0) 
          { strncpy(npa,tagVal[3],3);
          }

	returnCode = 0;
	if(     (strcmp("EN", tagVal[0] ) == 0) ||
		(strcmp("CO", tagVal[0] ) == 0) ||
		(strcmp("RP", tagVal[0] ) == 0) ||
		(strcmp("DD", tagVal[0] ) == 0) ||
		(strcmp("NT", tagVal[0] ) == 0) ||
		(strcmp("NE", tagVal[0] ) == 0) ||
		(strcmp("NR", tagVal[0] ) == 0) ||
		(strcmp("CP", tagVal[0] ) == 0) )
	{
		returnCode = findEqualTags(msgPtr,4,6); 
		strcpy(_formatCode,"A");
		_FunctionType =1;
	}
	else if((strcmp("IQ", tagVal[0] ) == 0) ||
		(strcmp("UP", tagVal[0] ) == 0) ||
		(strcmp("NQ", tagVal[0] ) == 0) ||
		(strcmp("NU", tagVal[0] ) == 0) ||
		(strcmp("QS", tagVal[0] ) == 0) )
	{
		returnCode = findEqualTags(msgPtr,4,5);
		strcpy(_formatCode,"C");
		_FunctionType =2;
	}
	else if(strcmp("UN", tagVal[0] ) == 0) 
	{
		returnCode = findEqualTags(msgPtr,4,5);
		strcpy(_formatCode,"E");
		_FunctionType =3;
	}
	else if(strcmp("NL", tagVal[0] ) == 0) 
	{
		if (findEqualTags(msgPtr,4,5) == 0) 
		  { returnCode = findEqualTags(msgPtr,23,23);   // LockTyp
                  }
		strcpy(_formatCode,"E");
		_FunctionType =3;
	}
	else if(strcmp("CL", tagVal[0] ) == 0)  
	{
		returnCode = findEqualTags(msgPtr,7,8);
                if (strstr(sopToSend, "SOLAR")!=NULL )
                    strcpy(_formatCode,"H");
                else
                    strcpy(_formatCode,"C");
		_FunctionType =4;
	}
	else if(strcmp("DX", tagVal[0] ) == 0)  
	{
		returnCode = findEqualTags(msgPtr,7,8);
                strcpy(_formatCode,"H");
		_FunctionType =11;
	}
	else if(strcmp("DL", tagVal[0] ) == 0)  
	{
		returnCode = findEqualTags(msgPtr,4,5);    // ServiceOrder optional for 'DL'
		strcpy(_formatCode,"A");
		_FunctionType =5;
	}
	else if(strcmp("ON", tagVal[0] ) == 0)  
	{
		returnCode = findEqualTags(msgPtr,11,11); 
		_FunctionType =6;
	}
	else if(strcmp("AM", tagVal[0] ) == 0)
        {
	        if (strstr(sopToSend, "RSOLAR")!=NULL  || 
	            strstr(sopToSend, "4PRSOLAR")!=NULL )
		  { returnCode = findEqualTags(msgPtr,4,5);
	            returnCode = findEqualTags(msgPtr,17,17);    // ClassSvc
                  }
	        else if (strstr(sopToSend, "SOLAR")!=NULL ) 
	          { returnCode = findEqualTags(msgPtr,22,22);    // SbnType is optional
	            returnCode = findEqualTags(msgPtr,24,24);    // AUXIND is optional
	         // returnCode = findEqualTags(msgPtr,14,16);    // CR7464, Exchange is not required for SOLAR
	            returnCode = findEqualTags(msgPtr,15,16);    // CustName,SbnQty
                  }
	        else if (strstr(sopToSend, "SOPAD")!=NULL ) 
	      	  { returnCode = findEqualTags(msgPtr,16,16);    // SbnQty
                  } 
		strcpy(_formatCode,"C");
		_FunctionType =7;
	}
	else if(strcmp("CK", tagVal[0] ) == 0)  
	{
		returnCode = findEqualTags(msgPtr,19,21);       // ServiceCode,CiPrefix,CiQty
		strcpy(_formatCode,"C");
		_FunctionType =8;
	}
	else if(strcmp("SQ", tagVal[0] ) == 0)      // SQ applies to only RSOLAR 
	{
		strcpy(_formatCode,"A");
		_FunctionType =9;     
	}
	else if(strcmp("RQ", tagVal[0] ) == 0)      // RQ applies to only RSOLAR 
	{
		strcpy(_formatCode,"A");
		_FunctionType =10;     
	}
	else //not a valid FunctionCode 
	{
	// error here
		memset(_errorMessage,0,1024);
		sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because incorrect Function Code failed  ",tagVal[0]," ");
		messageErrObj.createReplyErrMsg(_errorMessage);
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because incorrect Function Code failed",tagVal[0], returnCode,(char *) "parseMessage",(char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
	}
	if(returnCode <0)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because findEqualTags failed",(char *) "", returnCode, (char *) "parseMessage", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
	}
        returnCode = 0;
	if( (returnCode = checkValues(functionCode, _FunctionType)) < 0)
	{
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because checkValues failed", (char *) "",returnCode, (char *) "parseMessage", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
	}	
	return _FunctionType;
}

////////////////////////////////////////////////
//  FUNCTION:int DSOImessageRequest::findEqualTags
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char* msg - message to parse for equal tags
//			and the values.
//	char
//  
//  OTHER OUTPUTS: Sets the tagVals array elements to the 
//	message if there is an error in the message sent from the
//	Requestor application. Logs error messages to the 
//	error message queue.
//  
//  OTHER INPUTS:_sendErrQ - send queue to send error messages
//
//  RETURNS: -1 if the tagType is not found, zero otherwise.
// 
//  FUNCTIONS USED: DSOIerrorHandler::createReplyErrMsg(),
//		DSOIerrorHandler::sendErrorMessage(),
//		DSOImessageRequest::checkValues(),
//		strcpy(), strcat(), sprintf(), strstr(), strncpy()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::findEqualTags( char* msg, int start, int end)
{
#ifdef DEBUG
cout << "in findEqualTags " << endl;
#endif

	const char* tagType[] = {"FunctionCode","UserID","LSRID", 
		"NPANXX","OrderType","OrderNumber",
		"ServiceOrder","QueryListType", "QueryData",
		"npaSplit", "routeInfo","ONSMessage","UserIDS","OWN",
                "Exchange","CustName","SbnQty","ClassSvc","PrivateLine",
                "ServiceCode","CiPrefix","CiQty","SbnType","LockTyp","AUXIND"};

	char* msgPtr;
	char* equalPtr;
	char* hexPtr;
	int size =0;

#ifdef DEBUG
cout << "in findEqualTags start=" << start << endl;
cout << "in findEqualTags   end=" << end  << endl;
cout << "in findEqualTags   tagType=" << tagType  << endl;
for(int i1=start; i1 <= end; i1++)
   { cout << "in findEqualTags tagType=" << tagType[i1] << endl;
   }
#endif

	for(int i=start; i <= end; i++)
	{
		// test msgPtr = msg;
		// test msgPtr = strstr(msgPtr,tagType[i]); //locate each tagType
		msgPtr = strstr(msg, tagType[i]); //locate each tagType
		if(msgPtr == NULL) 
		{
                        if (strstr(tagType[i], "OWN") != NULL)          return -1;
                        if (strstr(tagType[i], "SbnType") != NULL)      return 0;
                        if (strstr(tagType[i], "AUXIND") != NULL)       return 0;
                        if (strstr(tagType[i], "LockTyp") != NULL)      return 0;
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because tag NOT FOUND ", (char *)tagType[i]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because tag NOT FOUND: ", (char *)tagType[i], -1, (char *) "findEqualTags", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		equalPtr = strchr(msgPtr, EQUAL);
		if(equalPtr == NULL)
		{
#ifdef DEBUG
cout << "equalPtr == NULL" << endl;
#endif
                        if (strstr(tagType[i], "OWN") != NULL)          return -1;
                        if (strstr(tagType[i], "SbnType") != NULL)      return 0;
                        if (strstr(tagType[i], "AUXIND") != NULL)       return 0;
                        if (strstr(tagType[i], "LockTyp") != NULL)      return 0;
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because an equal sign was not found after tag ", (char *)tagType[i]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because an equal sign was not found after tag ", (char *)tagType[i], -1, (char *) "findEqualTags", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		equalPtr++;
		hexPtr = strchr(msgPtr, HEXFF);
		if(hexPtr == NULL) 
		{
#ifdef DEBUG
cout << "hexPtr == NULL" << endl;
#endif
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because the \'xff\' was NOT FOUND for tag ", (char *)tagType[i]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because the \'xff\' was NOT FOUND for tag ", (char *)tagType[i],-1, (char *) "findEqualTags", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		size = (hexPtr-equalPtr);
		if(size <= 0)
		{
                        if (strstr(tagType[i], "OWN") != NULL)          return -1;
                        if (strstr(tagType[i], "SbnType") != NULL)      return 0;
                        if (strstr(tagType[i], "AUXIND") != NULL)       return 0;
                        if (strstr(tagType[i], "LockTyp") != NULL)      return 0;
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because the \'xff\'data was NOT FOUND for tag ", (char *)tagType[i]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because the \'xff\'data was NOT FOUND for tag ", (char *)tagType[i],-1, (char *) "findEqualTags", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		if (tagVal[i] != 0)
                {	delete []tagVal[i];
                	tagVal[i] = 0;
        	}
		if( (tagVal[i] = new char[size+1]) ==NULL)
		{
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" , (char *) "", -1, (char *) "messageMassage", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		_sizeOfTag = size+1; //to use for creating new arrays
		strncpy(tagVal[i],equalPtr,size);
		tagVal[i][size] = '\0';
#ifdef DEBUG
cout << "tagVal[i] = " << tagVal[i] << endl;
#endif
		for(int x =0; x< size; x++)
		{
			if(islower(tagVal[i][x]))
				tagVal[i][x] = (char)toupper(tagVal[i][x]);
		}
	}
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::checkValues
//
//  PURPOSE: Validates the tag values received from the "Requestor".
//
//  METHOD: Defines what the valid values are for each 
//	tag and verifies that the values are valid.
//
//  PARAMETERS: 
//	messages
//  
//  OTHER OUTPUTS: Creates a error logged message if there is an
//	invalid data and creates a response to the "Requestor" but in 
//	SOP format so that DSOIReply will understand the
//	format.
//  
//  OTHER INPUTS:_sendErrQ - the message queue to send error 
//				and the  tagVal array 
//
//  RETURNS: 1 if any value is invalid, 0 otherwise.
// 
//  FUNCTIONS USED:  strcmp(), strcpy(), strcat(),
//	             strlen(), strcmp(), sprintf()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::checkValues(char* functionCode, int functionType)
{
	int size;
	int retCd = 0;
	const char* QueryListTypeTable[] = {"T","TN","LN","LA","HML","PO","CLS","SC"};
	int found =0;
        int x=0;
	size = strlen(tagVal[0]);  //FunctionCode value
#ifdef DEBUG
cout << "strlen of tagVal[0]= " << size << endl;
#endif
	if( size != 2  )
	{
#ifdef DEBUG
cout << "bad FunctionCode value " << tagVal[0] << endl;
#endif
		memset(_errorMessage,0,1024);
		sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because bad FunctionCode value  ", (char *)tagVal[0]," ");
		messageErrObj.createReplyErrMsg(_errorMessage);
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because bad FunctionCode value ", (char *)tagVal[0], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
	}
	else
	{
		strcpy(functionCode,tagVal[0]);
	}

	if(functionType == 6) //ONS
	{
		size =  strlen(tagVal[11]);
		if(size <= 0 )
		{
		//error
			messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because no ONSMessage data ");
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because no ONSMessage data ",(char *) "",-1, (char *) "checkValues", (char *) __FILE__,__LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		strncpy(DSOImessageRequest::_QueueName,"ONS",3);
		DSOImessageRequest::_QueueName[3] = '\0';
		return 0;
	}
	if(functionType==4) //CL
	{
		memset(functionCode,0,3);
		strcpy(functionCode,"RN"); //requirement of SOP to change CL to RN
		strcpy(tagVal[0],"RN"); //requirement of SOP to change CL to RN
	}
	
        if( size=strlen(tagVal[1]) < 1)      //UserId value 
	{
		memset(_errorMessage,0,1024);
		sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because bad UserID value  ", (char *)tagVal[1]," ");
		messageErrObj.createReplyErrMsg(_errorMessage);
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because bad UserID value ", 
					(char *)tagVal[1], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
	}
	//Commented for SCR IT_DSOI_SCR_11 ATACS
       //	memset(_userId,0,9);
         	strcpy(_userId, tagVal[1]);
	size = strlen(tagVal[2]); //LSRID (negotiationNumber)

#ifdef DEBUG
cout << "size OF NEGtiation number" << size << endl;
#endif
	// if(size != 10)	//LSRID
	if((size > 0) && (size < 10))	//LSRID
	{
		strcpy(negotiationNumber,"0");
		for (int i =0; i < (9-size); i++)
			strncat(negotiationNumber,"0",10-size);
		strncat(negotiationNumber,tagVal[2],size);
		negotiationNumber[10] = '\0';
	}
	else // LSRID is greater than 9
	{	strncpy(negotiationNumber,tagVal[2], 10);
		negotiationNumber[10] = '\0';
		// strcpy(negotiationNumber,tagVal[2]);
	}

	switch(functionType)
	{  case 1: 
           case 2:
           case 3:
           case 5:
                if (functionType == 1)
		  { size = strlen(tagVal[6]); //ServiceOrder
		    if (size <= 0)
		      { messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because NO SERVICE ORDER ");
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because NO SERVICE ORDER ", (char *) "", -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		      }
		  }
		size = strlen(tagVal[4]); //OrderType
                if(size < 1 || size > 2)        //orderType
		{
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because bad OrderType value  ", (char *)tagVal[4]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because bad OrderType value ", (char *)tagVal[4], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
                if(size == 1)   //orderType
                  { strcpy(orderType,tagVal[4]);
                    strcat(orderType," ");
                  }
                else
                  { strcpy(orderType,tagVal[4]);
                  }
		size = strlen(tagVal[5]); //OrderNumber
		if(size != 8)	//orderNumber
		{
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because orderNumber not= 8 characters ", (char *)tagVal[5]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because orderNumber not= 8 characters ", (char *)tagVal[5], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		strcpy(orderNumber,tagVal[5]);
		
                break;

           case 4:   //CL
		if((strcmp(DSOImessageRequest::_QueueName, "SOPAD")==0))    //SOPAD 
		{
			if(strcmp("T",tagVal[7]) != 0 &&
			   strcmp("TN",tagVal[7]) != 0 ) 
			{
			//error
				memset(_errorMessage,0,1024);
				sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because QueryListType not valid for SOPAD ServiceOrder Processor  ", (char *)tagVal[7]," ");
				messageErrObj.createReplyErrMsg(_errorMessage);
				messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because QueryListType not valid for SOPAD ServiceOrder Processor : ", (char *)tagVal[7], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
				messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
				return -1;
			}
		}
		for (x=0; x < 8; x++)
		{
			if(strcmp((const char*)QueryListTypeTable[x],tagVal[7])==0)
			{
                                switch(strlen(tagVal[7]))
                                 { case 1:
                                     tagVal[7][1] = ' ';
                                     tagVal[7][2] = ' ';
                                     tagVal[7][3] = '\0';
                                     break;
                                   case 2:
                                     tagVal[7][2] = ' ';
                                     tagVal[7][3] = '\0';
                                     break;
                                   case 3:
                                     tagVal[7][3] = '\0';
                                     break;
                                 } //end of switch
				found =1;
				break;
			}
		}
		if(!found)
		{
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because bad QueryListType value  ", (char *)tagVal[7]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because bad QueryListType value ", (char *)tagVal[7], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
		size = strlen(tagVal[8]); //QueryData
#ifdef DEBUG
cout << "size of QueryData" << size << endl;
#endif
		if(size <= 0)
		{	
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because bad QueryData value  ", (char *)tagVal[8]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because bad QueryData value ", (char *)tagVal[8], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		}
                break;

           case 7:   //AM
		if (strcmp(DSOImessageRequest::_QueueName, "RSOLAR")==0) 
		  { strcpy(ClassSvc,tagVal[17]);
                  }
		else if (strcmp(DSOImessageRequest::_QueueName, "SOLAR")==0) 
		  { strcpy(KeyType,"S");
		    strcpy(NpaNxx,tagVal[3]);
                    strcpy(Exchange,"    ");   // CR7464, Exchange is not required for SOLAR
                    Exchange[4] = '\0';

		    strcpy(CustName,tagVal[15]);
	            if ((strlen(CustName) < 1) || (strlen(CustName) > 19))
                      {   memset(_errorMessage,0,1024);
                          sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because invalid CustName value  ", CustName," ");
                          messageErrObj.createReplyErrMsg(_errorMessage);
                          messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because invalid CustName value ", CustName, -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
                          messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                          return -1;
                      }
		    strcpy(SbnQty,tagVal[16]);
	            switch(strlen(SbnQty))
	              { case 1: 
                          SbnQty[2] = '\0';
                          SbnQty[1] = SbnQty[0];
                          SbnQty[0] = '0';
                          break;
                        case 2:
                          break;
                        default:
                          memset(_errorMessage,0,1024);
                          sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because invalid SbnQty value  ", SbnQty," ");
                          messageErrObj.createReplyErrMsg(_errorMessage);
                          messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because invalid SbnQty value ", SbnQty, -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
                          messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                          return -1;
                      }
	            if(tagVal[22] == 0)               
		      { SbnType[0] = 'S';           // Default SbnType
                        SbnType[1] = '\0';
                      }
                    else
		      { strcpy(SbnType,tagVal[22]);
                        SbnType[1] = '\0';
                      }
	            if(tagVal[24] == 0)               
		      { AUXIND[0] = ' ';           // Default AUXIND
                        AUXIND[1] = '\0';
                      }
                    else
		      { strcpy(AUXIND,tagVal[24]);
                        AUXIND[1] = '\0';
                      }
                  }
		else if (strcmp(DSOImessageRequest::_QueueName, "SOPAD")==0) 
		  { strcpy(KeyType,"S");
		    strcpy(NpaNxx,tagVal[3]);
		    strcpy(SbnQty,tagVal[16]);
	            switch(strlen(SbnQty))
	              { case 1: 
                          SbnQty[2] = '\0';
                          SbnQty[1] = SbnQty[0];
                          SbnQty[0] = '0';
                          break;
                        case 2:
                          break;
                        default:
                          memset(_errorMessage,0,1024);
                          sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because invalid SbnQty value  ", SbnQty," ");
                          messageErrObj.createReplyErrMsg(_errorMessage);
                          messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because invalid SbnQty value ", SbnQty, -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
                          messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                          return -1;
                      }
                  }
                break;

           case 8:   //CK
		strcpy(KeyType,"C");
		strcpy(ServiceCode,tagVal[19]);
                if ((strstr(ServiceCode, " ") != NULL) || (strlen(ServiceCode) != 4 ))
                  {   memset(_errorMessage,0,1024);
                      sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because invalid ServiceCode value  ", ServiceCode," ");
                      messageErrObj.createReplyErrMsg(_errorMessage);
                      messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because invalid ServiceCode value ", ServiceCode, -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
                      messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                      return -1;
                  }
		strcpy(CiPrefix,tagVal[20]);
	        switch(strlen(CiPrefix))
	          { case 1: 
                      CiPrefix[2] = '\0';
                      CiPrefix[1] = CiPrefix[0];
                      CiPrefix[0] = ' ';
                      break;
                    case 2:
                      break;
                    default:
                      memset(_errorMessage,0,1024);
                      sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because invalid CiPrefix value  ", CiPrefix," ");
                      messageErrObj.createReplyErrMsg(_errorMessage);
                      messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because invalid CiPrefix value ", CiPrefix, -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
                      messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                      return -1;
                  }
		strcpy(CiQty,tagVal[21]);
	        switch(strlen(CiQty))
	          { case 1: 
                      CiQty[2] = '\0';
                      CiQty[1] = CiQty[0];
                      CiQty[0] = '0';
                      break;
                    case 2:
                      break;
                    default:
                      memset(_errorMessage,0,1024);
                      sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because invalid CiQty value  ", CiQty," ");
                      messageErrObj.createReplyErrMsg(_errorMessage);
                      messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because invalid CiQty value ", CiQty, -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
                      messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                      return -1;
                  }
                break;

           case 9:                     // SQ applies only for RSOLAR
                break;

           case 10:                    // RQ applies only for RSOLAR
                break;

           case 11:                    // DX applies only for SOLAR
		if((strcmp(DSOImessageRequest::_QueueName, "SOLAR")==0))    //SOLAR
		  { if(strcmp("CR",tagVal[7]) != 0 )
		      {
			memset(_errorMessage,0,1024);
			sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because QueryListType not valid: ", (char *)tagVal[7]," ");
			messageErrObj.createReplyErrMsg(_errorMessage);
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because QueryListType not valid: : ", (char *)tagVal[7], -1, (char *) "checkValues", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return -1;
		      }
                    tagVal[7][2] = ' ';
                    tagVal[7][3] = '\0';
                    break;
		 }
  
           default:
	      {	memset(_errorMessage,0,1024);
		sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because FunctionCode invalid  ", functionCode," ");
		messageErrObj.createReplyErrMsg(_errorMessage);
		messageErrObj.formatErrMsg(_errorMessage, functionCode, -1, (char *) "checkValues", (char *) __FILE__,__LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
              }
  
        } //end of switch

	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::verifySopId
//
//  PURPOSE: Verify that the NPA is valid for
//	the SOP the DSOI is set to send messages to
//
//  METHOD:
//
//  PARAMETERS: char* Npa - the NPA parsed from the tag value
//			of the REQUESTOR message
// 
//  OTHER OUTPUTS: sopsOpen array is updated to the SOP valid for
//	the NPA
//  
//  OTHER INPUTS: A table of all the valid NPAs,
//		_sendErrQ - message queue to send errors
//
//  RETURNS: One if the NPA is not valid, zero otherwise.
// 
//  FUNCTIONS USED: strcmp(), strcpy()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::verifySopId(char* functionCode, char* SopId, char* UserId,
                                    char* sopToSend,    char* replyFromSop)
{
#ifdef DEBUG
cout << "in verifySopId=" << SopId << endl;
cout << "      function=" << functionCode << endl;
#endif
        int returnCode = 0;
        char tmpId[4];

        tmpId[0] = SopId[0];
        tmpId[1] = SopId[1];
        tmpId[2] = SopId[2];
	if (strncmp(functionCode, "CL",2)==0 )
             tmpId[3] = 'C';
        else
	if (strncmp(functionCode, "IQ",2)==0 )
             tmpId[3] = 'I';
        else
	if (strncmp(functionCode, "DX",2)==0) 
             tmpId[3] = 'X';
        else
	if (strncmp(functionCode, "UP",2)==0  ||
	    strncmp(functionCode, "UN",2)==0  ||
	    strncmp(functionCode, "NL",2)==0  ||
	    strncmp(functionCode, "AM",2)==0  ||
	    strncmp(functionCode, "CK",2)==0  ||
	    strncmp(functionCode, "DL",2)==0  ||
	    strncmp(functionCode, "NQ",2)==0  ||
	    strncmp(functionCode, "NU",2)==0  ||
	    strncmp(functionCode, "QS",2)==0  ||
	    strncmp(functionCode, "SQ",2)==0  ||
	    strncmp(functionCode, "RQ",2)==0  )
             tmpId[3] = 'Q';
        else
	if (strncmp(functionCode, "ON",2)==0)
          { memset(tmpId,0,4);
            strcpy(tmpId,"ONSR");
          }
        else
	if (strncmp(functionCode, "DD",2)==0  ||
	    strncmp(functionCode, "CC",2)==0  ||
	    strncmp(functionCode, "CO",2)==0  ||
	    strncmp(functionCode, "RP",2)==0  ||
	    strncmp(functionCode, "SS",2)==0  ||
	    strncmp(functionCode, "NR",2)==0  ||
	    strncmp(functionCode, "NE",2)==0  )
	      tmpId[3] = 'D';
        else
        {
     
            if(((strncmp(SopId,"SOU",3) == 0) ||(strncmp(SopId,"SOC",3)==0) ||
	        (strncmp(SopId,"RSO",3) == 0) ||(strncmp(SopId,"ESS",3)==0) ||
		(strncmp(SopId,"ESN",3) == 0)) && 
		((strncmp(UserId, "ATACSCO", 7)==0) || 
		(strncmp(UserId, "ATACSSB", 7)==0) || 
		(strncmp(UserId, "MS2SYN", 6)==0) ||
		(strncmp(UserId, "NGDLCBF", 7)==0) ||
		(strncmp(UserId, "MACRO31", 7)==0) ||
		(strncmp(UserId, "LURFTS", 6)==0) ||
		(strncmp(UserId, "CAJUNDMN", 7)==0)))
	       tmpId[3] = 'A';
           else
               tmpId[3] = 'D';
        }
        tmpId[4] = '\0';
        memset(useridtemp,0,sizeof(useridtemp));
        returnCode = routeToSop(tmpId, sopToSend);
        returnCode = replyToSop(tmpId, replyFromSop);

#ifdef DEBUG
cout << "    sopToSend=" << sopToSend      << endl;
cout << " replyFromSop=" << replyFromSop   << endl;
#endif

	if (strncmp(SopId, "ONS",3)==0)
	  { strncpy(DSOImessageRequest::_QueueName,"RSOLAR",6);
	    DSOImessageRequest::_QueueName[6] = '\0';
	    return 0;
	  }
	if (strncmp(SopId, "4RS",3)==0)
	  { strncpy(DSOImessageRequest::_QueueName,"4PRSOLAR",8);
	    DSOImessageRequest::_QueueName[8] = '\0';
	    return 0;
	  }
	if (strncmp(SopId, "RSO",3)==0)
	  { strncpy(DSOImessageRequest::_QueueName,"RSOLAR",6);
	    DSOImessageRequest::_QueueName[6] = '\0';
	    return 0;
	  }
	if (strncmp(SopId, "ESN",3)==0)
	  { strncpy(DSOImessageRequest::_QueueName,"SOLAR",5);
	    DSOImessageRequest::_QueueName[5] = '\0';
	    return 0;
	  }
	if (strncmp(SopId, "ESS",3)==0)
	  { strncpy(DSOImessageRequest::_QueueName,"SOLAR",5);
	    DSOImessageRequest::_QueueName[5] = '\0';
	    return 0;
	  }
	if (strncmp(SopId, "SOU",3)==0)
	  { strncpy(DSOImessageRequest::_QueueName,"SOPAD",5);
	    DSOImessageRequest::_QueueName[5] = '\0';
	    return 0;
	  }
	if (strncmp(SopId, "SOC",3)==0)
	  { strncpy(DSOImessageRequest::_QueueName,"SOPAD",5);
	    DSOImessageRequest::_QueueName[5] = '\0';
	    return 0;
	  }

	return -1;
} //end if verifySopId()

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::elementLookup
//
//  PURPOSE: To look up the element type(e.g. EN) 
//	from the message sent from the REQUESTOR
//
//  METHOD: Finds the element type in the constant position
//	of the REQUESTOR SOC message to determine the type
//	of element.
//
//  PARAMETERS: char* word -  the portion of the message to parse the element
//  
//  OTHER OUTPUTS: Assigns the elementType array element of the 
//	DSOImessageRequest class member.
//  
//  OTHER INPUTS: ELEMENTTYPE - The position in the message where 
//		the element type can be found. Creates an error message
//		if the element type is not found.
//		_sendErrQ - the queue to send errors to
//
//  RETURNS: Returns 2 if the element type is determined to be the
//	START or END of the SOC message, 1 if the element type is
//	not found, and 0 if the element type is found and is valid.
// 
//  FUNCTIONS USED: DSOImessageRequest::setTagName(), 
//	DSOImessageRequest::setUsocTagName(word);
//	DSOIerrorHandler::sendErrorMessage()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::elementLookup(char* word)
{
	int returnCode =0;
	char* wordPtr = word;
	wordPtr += TYPEOFELEMENT;
	switch(wordPtr[0])
	{
		case 'X':    // lookup TAG name for transalated type
			elementType[0] = 'C';
			//elementType[0] = 'X';
			if( (returnCode = setTagName(word)) !=0)
			{
				//START or END in SOC message
				return 2;
			}
			break;
		case 'L':    // change to 'C'
			*wordPtr = 'C';
			elementType[0] = 'C';
			if(wordPtr[1] != ' ' )
			{
				actionCode = 1;
			}
			if(returnCode = setTagName(word) != 0)
			{
				//START or END in SOC message
				return 2;
			}
			break;
		case 'S':    // pass through 
			elementType[0] = 'S';
			if(returnCode = setTagName(word) != 0)
			{
				//START or END in SOC message
				return 2;
			}
			break;
		case 'F':    // pass through
			elementType[0] = 'F';
			if(returnCode = setTagName(word) != 0)
			{
				//START or END in SOC message
				return 2;
			}
			break;
		case 'U':    // USOC change to 'A'
			*wordPtr = 'A';
			setUsocTagName(word);
			elementType[0] = 'A';
			break;
		case 'C':    // change to blank then 'C'
			elementType[0] = ' ';
			if(returnCode = setTagName(word) != 0)
			{
				return 2;
			}
			break;
		case 'N':    // pass through
			elementType[0] = 'N';
			if(returnCode = setTagName(word) != 0)
			{
				return 2;
			}
			break;
		case ' ':    // pass through
			elementType[0] = ' ';
			if(returnCode = setTagName(word) != 0)
			{
				return 2;
			}
			break;
		default:
		{
#ifdef DEBUG
cout << "    fidType=" << wordPtr     << endl;
#endif
			messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because elementType of X,L,S,F,U, or C is NOT FOUND!");
			messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because elementType of X,L,S,F,U, or C is NOT FOUND!", (char *) "", 1, (char *) "elementLookup", (char *) __FILE__, __LINE__);
			messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
			return 1;
		}
	} // end of switch
	return 0;
}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::setTagName
//
//  PURPOSE: To copy the tagname in the message
//	into the tagName[].
//
//  METHOD: Moves to the static position in the SOC message
//	to where the tag name can be found and copies the
//	next five characters into the tagName.
//
//  PARAMETERS: char* line - the message to find the
//	tag name from. 
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS: actionCode - defines if there is an action
//	code in the input then the tagname starts in 
//	the next position one less than if there was
//	not a action code in the SOC message.
//
//	TAGNAME - the position in the SOC where the
//	tag name can be found.
//
//  RETURNS: 1 if the tagname is START or END,
//	0 otherwise. 
// 
//  FUNCTIONS USED: strncpy(), strcmp()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::setTagName(char* line)
{
	char* linePtr = line;
	if(actionCode)
		linePtr +=TAGNAME-1;
	else
		linePtr += TAGNAME;
	strncpy(tagName,linePtr,5);
	tagName[5] = '\0';
	if( ( strcmp(tagName,"START") == 0)
	   || (strcmp(tagName,"SOEND")==0 ))
		return 1;
	return 0;
}


////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setUsocTagName
//
//  PURPOSE: To set the tag name for the element type
//	of a USOC in the SOC.
//
//  METHOD: Points to the position in the SOC where
//	the USOC tagname is found and copies only one
//	character(usually action code in all other element types)
//	then advances six characters where the rest of the tag name
//	is positioned.
//
//  PARAMETERS: char* line - the SOC message
//  
//  OTHER OUTPUTS: tagName is assigned to USOC tag name.
//  
//  OTHER INPUTS: USOCTAGNAME -  the position in the SOC where
//	a USOC tag name is found
//
//  RETURNS: void
// 
//  FUNCTIONS USED: DSOImessageRequest::fillSpace(),strncpy(), strncat()
//
//////////////////////////////////////////////////////
void DSOImessageRequest::setUsocTagName(char* line)
{
	char* linePtr = line;
	linePtr += USOCTAGNAME;
	strncpy(tagName,linePtr,1);
	linePtr += 6;
	char* endOfLinePtr = strchr(linePtr,TOKENNEWLINE);
	int quantityLength = endOfLinePtr-linePtr;
	if(quantityLength > 0 ) 
		strncat(tagName,linePtr,quantityLength);
	int tagNameLength = strlen(tagName);
	if(tagNameLength < 5)
		fillSpace(tagName,5-tagNameLength);
}	

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::deleteTagVals()
//
//  PURPOSE:
//
//  METHOD: 
//
//  PARAMETERS: None
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS: 
//
//  RETURNS: void
// 
//  FUNCTIONS USED: 
//
//////////////////////////////////////////////////////
void DSOImessageRequest::deleteTagVals()
{
	if(tagVal[0] !=0)
	{
		delete [] tagVal[0]; //Function Code
		tagVal[0] =0;
	}
	if(tagVal[1] !=0)
	{
		delete [] tagVal[1]; //User ID
		tagVal[1] =0;
	}
	if(tagVal[2] !=0)
	{
		delete [] tagVal[2]; // LSRID
		tagVal[2] =0;
	}
	if(tagVal[3] !=0)
	{
		delete [] tagVal[3]; // NPANXX
		tagVal[3] =0;
	}
	if(tagVal[4] != 0) //Order Type
	{
		delete [] tagVal[4];
		tagVal[4] = 0;
	}
	if(tagVal[5] != 0) //Order Number
	{
		delete [] tagVal[5];
		tagVal[5] = 0;
	}
	if(tagVal[6] != 0) //Service Order
	{
		delete [] tagVal[6];
		tagVal[6] = 0;
	}
	if(tagVal[7] != 0) //Query Order List
	{
		delete [] tagVal[7];
		tagVal[7] = 0;
	}
	if(tagVal[8] != 0) //Query Data
	{
		delete [] tagVal[8];
		tagVal[8] = 0;
	}
	if(tagVal[9] != 0) //NpaSplit
	{
		delete [] tagVal[9];
		tagVal[9] = 0;
	}
	if(tagVal[10] != 0) //RouteInfo
	{
		delete [] tagVal[10];
		tagVal[10] = 0;
	}
	if(tagVal[11] != 0) //ONSMessage
	{
		delete [] tagVal[11];
		tagVal[11] = 0;
	}
	if(tagVal[12] != 0) 
	{
		delete [] tagVal[12];
		tagVal[12] = 0;
	}
	if(tagVal[13] != 0) //ownerCode
	{
		delete [] tagVal[13];
		tagVal[13] = 0;
	}
	if(tagVal[14] != 0) //Exchange
	{
		delete [] tagVal[14];
		tagVal[14] = 0;
	}
	if(tagVal[15] != 0) //CustName
	{
		delete [] tagVal[15];
		tagVal[15] = 0;
	}
	if(tagVal[16] != 0) //SbnQty
	{
		delete [] tagVal[16];
		tagVal[16] = 0;
	}
	if(tagVal[17] != 0) //ClassSvc
	{
		delete [] tagVal[17];
		tagVal[17] = 0;
	}
	if(tagVal[18] != 0) //PrivateLine
	{
		delete [] tagVal[18];
		tagVal[18] = 0;
	}
	if(tagVal[19] != 0) //ServiceCode
	{
		delete [] tagVal[19];
		tagVal[19] = 0;
	}
	if(tagVal[20] != 0) //CiPrefix
	{
		delete [] tagVal[20];
		tagVal[20] = 0;
	}
	if(tagVal[21] != 0) //CiQty
	{
		delete [] tagVal[21];
		tagVal[21] = 0;
	}
	if(tagVal[22] != 0) //SbnType
	{
		delete [] tagVal[22];
		tagVal[22] = 0;
	}
	if(tagVal[23] != 0) //LockTyp
	{
		delete [] tagVal[23];
		tagVal[23] = 0;
	}
	if(tagVal[24] != 0) //AUXIND
	{
		delete [] tagVal[24];
		tagVal[24] = 0;
	}
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::reInitMembers()
//
//  PURPOSE: To re-initialize all data information parsed
//	from the SOC to prepare for a new line of the SOC.
//
//  METHOD: Set all members of the DSOImessageRequest
//	class to default values. 
//
//  PARAMETERS: None
//  
//  OTHER OUTPUTS: 
//  
//  OTHER INPUTS: DSOImessageRequest::tagName,DSOImessageRequest::data, 
//	DSOImessageRequest::dataLength,DSOImessageRequest::elementType,
//	DSOImessageRequest::actionCode,
//
//  RETURNS: void
// 
//  FUNCTIONS USED: memset(), delete();
//
//////////////////////////////////////////////////////
void DSOImessageRequest::reInitMembers()
{
	memset(tagName,0,sizeof(tagName));
	memset(dataLength,0,sizeof(dataLength));
	memset(elementType,0,sizeof(elementType) );
	actionCode = 0;
	if(data != '\0')
	{
	int size = strlen(data);
	memset(data,0,size);
		delete [] data ;
		data = '\0';
	}
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::mergeSOC
//
//  PURPOSE: To concatenate all of the data as parsed from
//	one line of the SOC
//
//  METHOD: Concatenates the DSOI reformatted SOC data with the
//	previous reformated SOC data.
//
//  PARAMETERS: char* message - reformatted SOC message so far
//  
//  OTHER OUTPUTS: Concatenated DSOI reformatted SOP message.
//  
//  OTHER INPUTS: DSOImessageRequest::dataLength,DSOImessageRequest::tagName,
//	DSOImessageRequest::elementType,DSOImessageRequest::data
//
//  RETURNS: void
// 
//  FUNCTIONS USED: strcat()
//
//////////////////////////////////////////////////////
void DSOImessageRequest::mergeSOC(char* message)
{
	char* messagePtr = message;
	strcat(message,dataLength); //dataLength first character overwrites the message terminating NULL character
	strcat(message,elementType);
	strcat(message,tagName);
	if(data!='\0')
	strcat(message,data); // concats the NULL terminating character
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::fillerSOP
//
//  PURPOSE: To add characters at the start of
//	the SOP message that are not read by the SOP.
//
//  METHOD: Added the character 'F' for 23 bytes.
//
//  PARAMETERS: char* message - the DSOI reformatted message
//	to be sent to the SOP
//  
//  RETURNS: void
// 
//  FUNCTIONS USED: strcat()
//
//////////////////////////////////////////////////////
void DSOImessageRequest::fillerSOP(char* message)
{
	strcat(message,"FFFFFFFFFFFFFFFFFFFFFF");
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::fillSpace
//
//  PURPOSE: To add blanks to portions of the SOC message
//	when exact length of data is required (e.g. tag value).
//
//  METHOD: Adds the number of defined blanks.
//
//  PARAMETERS: char* message - message where blanks need to be added
//		int number -  the number of blanks to add
//  
//  RETURNS: void
// 
//  FUNCTIONS USED: strcat()
//
//////////////////////////////////////////////////////
void DSOImessageRequest::fillSpace(char* message, int number)
{
	for(int i=number; i != 0; i--)
	{
		strcat(message," ");
	}
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setPreSoc
//
//  PURPOSE: To add required information to the 
//	message sent to the SOP
//
//  METHOD: Add time of day, date, hostname where DSOI process
//	is executing, line count of how many lines of the SOC,
//	and hard code the company code, userid,
//	office code, typist location,id, and type, order type
//
//  PARAMETERS: char* message -  the message to add the required
//	data sent to the SOP.
//  
//  OTHER INPUTS: DSOImessageRequest::lineCount,
//  DSOImessageRequest::negotiationNumber(LSRID),DSOImessageRequest::orderNumber,
//
//  RETURNS:void
// 
//  FUNCTIONS USED: time(), strcat(),strftime(), gethostname(),
//	DSOImessageRequest::fillSpace()	
//	
//
//////////////////////////////////////////////////////
void DSOImessageRequest::setPreSoc(char* functionCode, char* ownerCode, 
                            int functionType, char* message)
{
	strncat(message,ownerCode,1);             // the ownerCode
	strcat(message,lineCount);
        strcat(message,functionCode);
	strncat(message,_formatCode,2); // the format code

	// do some calculations for date and time
	time_t timeNow;
	static int dateSize=7;
	int timeSize=9;
	static char curTime[9];
	time(&timeNow);

	// get the current time
	strftime(curTime,dateSize,"%H%M%S",localtime(&timeNow) );
	strcat(curTime,"0");
	strcat(curTime,"0");

	// get the current date
	static char curDate[9];
	strftime(curDate,dateSize,"%m%d%y",localtime(&timeNow) );
	strcat(message,curDate);
	strcat(message,curTime);

	strcat(message,"USW"); // company code
	strcat(message,"DSI"); // office code

	// get the host name
	static char hostName[9];
	gethostname(hostName,8);
	int hostNameLength = strlen(hostName);
	hostName[hostNameLength] = '\0';
	strcat(message,hostName);
	if(hostNameLength < 8)
	{
		int fillWithSpace = 8 - hostNameLength;
		if(fillWithSpace >0)
			fillSpace(message,fillWithSpace);
	}
	strcat(message,"SOP"); 	//Host System ID
	strcat(message,negotiationNumber);// Negotiation number(LSRID)
        if(functionType==1)
		fillSpace(message,2);	
	strcat(message,"   ");               // replace typistId with 3 spaces
	strcat(message,"   ");               // replace officeCd with 3 spaces
	strcat(message," ");
	int userNameLength = strlen(_userId);
	strcat(message,_userId);
	if(userNameLength < 8)
	{
		int fillWithSpace = 8 - userNameLength;
		if(fillWithSpace >0)
			fillSpace(message,fillWithSpace);
	}
	//fillSpace(message,8);	// Typist Terminal ID
#ifdef DEBUG
   cout << "    Request msg=" << message << endl;
#endif
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setKeyType
//
//  PURPOSE: 
//
//  METHOD: 
//
//  PARAMETERS: char* message -  the message to add the required
//	data sent to the SOP.
//  
//  OTHER INPUTS: DSOImessageRequest::lineCount,
//	DSOImessageRequest::negotiationNumber(LSRID),
//	DSOImessageRequest::orderNumber
//
//  RETURNS: None
// 
//  FUNCTIONS USED: 
//	
//
//////////////////////////////////////////////////////
void DSOImessageRequest::setKeyType(char* message)
{
	strcat(message,"O");
	strcat(message,orderType);	// Order Type
	strcat(message,orderNumber);	// Order Number
	fillSpace(message,18);
	strncat(message,tagVal[3],3);
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setCLHeader(char* message)
//
//  PURPOSE: For FunctionCode CL only it concatenates the
//	required data for the SOLAR+ and SOPAD SOPS only!
//
//  METHOD: Concatenates the Query List Type, 
//	Query Data, and  npa. 
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
void DSOImessageRequest::setCLHeader(char* sopId, char* message)
{
	 if ((strcmp(sopId, "SOC")==0) || (strcmp(sopId, "SOU")==0))
           { //strcat(message,tagVal[7]);	// Query List Type
	     strcat(message,"T");	        // Query List Type
	     //fillSpace(message,6);	// compressed NPA, lock indicator, error indicator
	     strcat(message,tagVal[8]);	//  Query Data
	     fillSpace(message,18);	// compressed NPA, lock indicator, error indicator
	     strncat(message,npa,3);	//  NPA
           }
         else
	   { strcat(message,tagVal[7]);	//  Query List Type
	     strcat(message,tagVal[8]);	//  Query Data
	     // fillSpace(message,18);	
           }
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setDXHeader(char* message)
//
//  PURPOSE: For FunctionCode DX only it concatenates the
//	required data for the SOLAR+ 
//
//  METHOD: Concatenates the Query List Type, 
//	Query Data, and  npa. 
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
void DSOImessageRequest::setDXHeader(char* sopId, char* message)
{
	   strcat(message,tagVal[7]);	//  Query List Type
	   strcat(message,tagVal[8]);	//  Query Data
	   // fillSpace(message,18);	
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setAMHeader(char* message)
//
//  PURPOSE: For FunctionCode AM only it concatenates the
//	required data for the SOLAR+ and SOPAD SOPS only!
//
//  METHOD: Concatenates the  Exchange,CustName,SbnQty,SbnType,AUXIND
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
void DSOImessageRequest::setAMHeader(char* message)
{
	if(strcmp(DSOImessageRequest::_QueueName, "SOLAR")==0) 
	  { strcat(message,KeyType);	// KeyType
	    strcat(message,NpaNxx);	// NPANXX
	    strcat(message,Exchange);	// Exchange
	    strcat(message,SbnQty);	// SbnQty
	    strcat(message,SbnType);	// SbnType
	    strcat(message,AUXIND);	// AUXIND
	    strcat(message,CustName);	// CustName
          }
        else
	  { strcat(message,KeyType);	// KeyType
	    strcat(message,NpaNxx);	// NPANXX
	    strcat(message,SbnQty);	// SbnQty
          }
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setCKHeader(char* message)
//
//  PURPOSE: For FunctionCode CK only it concatenates the
//	required data for the SOLAR+ and SOPAD SOPS only!
//
//  METHOD: Concatenates the  CiPrefix,ServiceCode,CiQty
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
void DSOImessageRequest::setCKHeader(char* message)
{
	strcat(message,KeyType);	// KeyType
	strcat(message,CiPrefix);	//  CiPrefix
	strcat(message,ServiceCode);	//  ServiceCode
	strcat(message,CiQty);	        //  CiQty
}

////////////////////////////////////////////////
//  FUNCTION:void DSOImessageRequest::setHeader(char* message)
//
//  PURPOSE: To update the newMessage with the required 
//	information for the SOLAR+ and SOPAD SOP for the UN 
//	and NL FunctionCode only.
//
//  METHOD: concatenate the orderType and orderNumber and
//	three blanks to the newMessage.
//
//  PARAMETERS: char* message - newMessage that is global and 
//	can eventually be taken out
//  
//  RETURNS: void
// 
//  KNOWN USERS: DSOImessageRequest::messageMassage()
//
//  FUNCTIONS USED: strcat()
//
//////////////////////////////////////////////////////
void DSOImessageRequest::setHeader(char* functionCode, char* message)
{
	strcat(message,orderType);	// Order Type
	strcat(message,orderNumber);	// Order Number
        if (strcmp(functionCode, "NL")==0)
          {
	     fillSpace(message,3);	// Compressed NPA, lock indicator, error indicator
             if (strlen(tagVal[23]) > 0)
	       {
                 strcat(message,tagVal[23]);      // LockTyp
               }
          }
        else
          {
	     fillSpace(message,3);	// Compressed NPA, lock indicator, error indicator
          }

}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setPreServiceOrder
//
//  PURPOSE: To set all characters for one string to be sent
//	to the SOLAR+ and SOPAD SOPS only.
//
//  METHOD: Fills orderType and orderNumber in the message sent.
//  This message is the newMessage and may be removed at some time.
//	It does not have to be a parameter since it is a global member.
//
//  PARAMETERS: char* message -  the message to add the required
//  
//  OTHER INPUTS: DSOImessageRequest::orderNumber,
//
//  RETURNS:void
// 
//  FUNCTIONS USED: strcat()
//	
//////////////////////////////////////////////////////
void DSOImessageRequest::setPreServiceOrder(char* message)
{
	strcat(message,orderType);	// Order Type
	strcat(message,orderNumber);	// Order Number
	fillSpace(message,6);	// compressed NPA, lock indicator, 
				// error inidicator, use to be line count
}

////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::endOfMessage
//
//  PURPOSE: To add a HEX FF to the end of the DSOI
//	reformated message to send to the SOP
//
//  METHOD: Determine the length of the message and add 
//			HEX FF character.
//
//  PARAMETERS: char* message -  the DSOI reformatted message
//	to add the HEX value to end.
//  
//  RETURNS: void
// 
//  FUNCTIONS USED: strlen(), strcat()
//
//////////////////////////////////////////////////////
void DSOImessageRequest::endOfMessage(char* message)
{
	//int hexName = 0xff;
	char hexName = 0xff;
	int length = strlen(message);
	message[length] = 0xff;
	message[length+1] = '\0';
}

////////////////////////////////////////////////
//  FUNCTION: char* getEnvVariable
//
//  PURPOSE:
//
//  METHOD:
//
//  PARAMETERS: char* envVarReq -  environment variable
//	to get the value of.
//  
//  RETURNS: Returns the environment value for the endVarReq parameter
// 
//  FUNCTIONS USED: getenv()
//
//////////////////////////////////////////////////////
char* getEnvVariable(char* envVarReq)
{
	char* environVariable = getenv(envVarReq);
	if(environVariable == NULL)
	{
		return NULL;
	}
	return environVariable;
}


////////////////////////////////////////////////
//  FUNCTION: void DSOImessageRequest::setNewRsolarMessage
//				(char* msg)
//
//  PURPOSE: To send the same data and parameters that
//	the 'Requestor' application sent only send all data
//	as capitals (this is done in findEqualTags()).
//
//  METHOD: Look for parameters in the message sent and
//	update the newMessage with those parameters and 
//	the tagVal[]s that have the data all caps.
//
//  PARAMETERS: char* msg - the message sent from the 'Requestor'
//  
//  OTHER OUTPUTS: newMessage - the newMessage that is sent to 
//	the SOP
//  
//  OTHER INPUTS: tagVal[]s - the tag values that were updated 
//	with the data values sent from the 'Requestor" only all 
//	characters that were sent as non-cap characters are converted
//	to cap cahracters.
//
//  RETURNS: void
// 
//  FUNCTIONS USED: strstr(), strcat()
//
//////////////////////////////////////////////////////
void DSOImessageRequest::setNewRsolarMessage(char* sopId, char* ownerCode, 
				char* functionCode, char* msg)
{
	const char* tagType[] = {"FunctionCode","UserID","LSRID", 
		"NPANXX","OrderType","OrderNumber",
		"ServiceOrder","QueryListType", "QueryData",
		"npaSplit", "routeInfo","ONSMessage","UserIDS","OWN",
                "Exchange","CustName","SbnQty","ClassSvc","Filler",
                "ServiceCode","CiPrefix","CiQty","SbnType","LockTyp"};

	char* msgPtr = msg;
	msgPtr = msg;
        if (strcmp("RQ", functionCode ) == 0)
          { 
            strcpy(newMessage,msgPtr);
            strcat(newMessage,"\xff");
	    return;
	  }

	if (strstr(msgPtr,"FunctionCode") != NULL)
          { strcat(newMessage,tagType[0]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[0]);
	    strcat(newMessage,"\xff");
          }
	if (strstr(msgPtr,"UserID") != NULL)
          { strcat(newMessage,tagType[1]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[1]);
	    strcat(newMessage,"\xff");
	  }
	if (strstr(msgPtr,"LSRID") != NULL)
          { strcat(newMessage,tagType[2]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[2]);
	    strcat(newMessage,"\xff");
	  }
	if (strstr(msgPtr,"NPANXX") != NULL)
          { strcat(newMessage,tagType[3]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[3]);
	    strcat(newMessage,"\xff");
	  }
        strcat(newMessage, "OWN=");             // the Owner Code
	strncat(newMessage,ownerCode,1);            
        strcat(newMessage,"\xff");
	   
        strcat(newMessage,tagType[23]);         // SopId
	strcat(newMessage,"=");
        strcat(newMessage,sopId);
	strcat(newMessage,"\xff");
	
	if (strstr(msgPtr,"OrderType") != NULL)
          { strcat(newMessage,tagType[4]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[4]);
	    strcat(newMessage,"\xff");
	  }
	if (strstr(msgPtr,"OrderNumber") != NULL)
          { strcat(newMessage,tagType[5]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[5]);
	    strcat(newMessage,"\xff");
	  }
	if (strstr(msgPtr,"ServiceOrder") != NULL)
          { if (strcmp("DL", tagVal[0] ) != 0)
              { strcat(newMessage,tagType[6]);
	        strcat(newMessage,"=");
                strcat(newMessage,tagVal[6]);
	        strcat(newMessage,"\xff");
	      }
	  }
	if (strstr(msgPtr,"QueryListType") != NULL)
          { strcat(newMessage,tagType[7]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[7]);
	    strcat(newMessage,"\xff");
	  }
	if (strstr(msgPtr,"QueryData") != NULL)
          { strcat(newMessage,tagType[8]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[8]);
	    strcat(newMessage,"\xff");
	  }
	if (strstr(msgPtr,"Exchange") != NULL)
          { strcat(newMessage,tagType[14]);
	    strcat(newMessage,"=");
            strcat(newMessage,tagVal[14]);
	    strcat(newMessage,"\xff");
	  }
        if (strcmp("AM", tagVal[0] ) == 0)
	  {if (strstr(msgPtr,"CustName") != NULL)
             { strcat(newMessage,tagType[15]);
	       strcat(newMessage,"=");
               strcat(newMessage,tagVal[15]);
	       strcat(newMessage,"\xff");
	     }
	   if (strstr(msgPtr,"SbnQty") != NULL)
             { strcat(newMessage,tagType[16]);
	       strcat(newMessage,"=");
               strcat(newMessage,tagVal[16]);
	       strcat(newMessage,"\xff");
	     }
	   if (strstr(msgPtr,"ClassSvc") != NULL)
             { strcat(newMessage,tagType[17]);
	       strcat(newMessage,"=");
               strcat(newMessage,tagVal[17]);
	       strcat(newMessage,"\xff");
	     }
           char* tagValue=  new char[100];
           memset(tagValue,0,100);
           if (findTagValue(msgPtr, "PrivateLine", tagValue) == 0)
             { strcat(newMessage,"PrivateLine=");
               strcat(newMessage,tagValue);
               strcat(newMessage,"\xff");
             }
           memset(tagValue,0,100);
           if (findTagValue(msgPtr, "BILLPERIOD", tagValue) == 0)
             { strcat(newMessage,"BILLPERIOD=");
               strcat(newMessage,tagValue);
               strcat(newMessage,"\xff");
             }
           memset(tagValue,0,100);
           if (findTagValue(msgPtr, "BULKNMBR", tagValue) == 0)
             { strcat(newMessage,"BULKNMBR=");
               strcat(newMessage,tagValue);
               strcat(newMessage,"\xff");
             }
           if (findTagValue(msgPtr, "BROADCAST", tagValue) == 0)
             { strcat(newMessage,"BROADCAST=");
               strcat(newMessage,tagValue);
               strcat(newMessage,"\xff");
             }
           if (findTagValue(msgPtr, "OVERRIDE", tagValue) == 0)
             { strcat(newMessage,"OVERRIDE=");
               strcat(newMessage,tagValue);
               strcat(newMessage,"\xff");
             }
           delete [] tagValue;
	  }
        if (strcmp("CK", tagVal[0] ) == 0)
	  {if (strstr(msgPtr,"ServiceCode") != NULL)
             { strcat(newMessage,tagType[19]);
	       strcat(newMessage,"=");
               strcat(newMessage,tagVal[19]);
	       strcat(newMessage,"\xff");
	     }
	   if (strstr(msgPtr,"CiPrefix") != NULL)
             { strcat(newMessage,tagType[20]);
	        strcat(newMessage,"=");
                strcat(newMessage,tagVal[20]);
	        strcat(newMessage,"\xff");
	      }
	   if (strstr(msgPtr,"CiQty") != NULL)
             { strcat(newMessage,tagType[21]);
	       strcat(newMessage,"=");
               strcat(newMessage,tagVal[21]);
	       strcat(newMessage,"\xff");
	     }
	  }

        if (strcmp("NL", tagVal[0] ) == 0)
          { char* tagValue=  new char[100];
            memset(tagValue,0,100);
            if (findTagValue(msgPtr, "LockTyp", tagValue) == 0)
               { strcat(newMessage,"LockTyp=");
                 strcat(newMessage,tagValue);
	         strcat(newMessage,"\xff");
               }
            delete [] tagValue;
	  }
        if (strcmp("SQ", tagVal[0] ) == 0)
          { char* tagValue=  new char[100];
            memset(tagValue,0,100);
            if (findTagValue(msgPtr, "SQUSER", tagValue) == 0)
              { strcat(newMessage,"SQUSER=");
                strcat(newMessage,tagValue);
                strcat(newMessage,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msgPtr, "SQSLS", tagValue) == 0)
              { strcat(newMessage,"SQSLS=");
                strcat(newMessage,tagValue);
                strcat(newMessage,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msgPtr, "SQNPA", tagValue) == 0)
              { strcat(newMessage,"SQNPA=");
                strcat(newMessage,tagValue);
                strcat(newMessage,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msgPtr, "SQORDN", tagValue) == 0)
              { strcat(newMessage,"SQORDN=");
                strcat(newMessage,tagValue);
                strcat(newMessage,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msgPtr, "SQSTAT", tagValue) == 0)
              { strcat(newMessage,"SQSTAT=");
                strcat(newMessage,tagValue);
                strcat(newMessage,"\xff");
              }
            delete [] tagValue;
	  }

}

////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOImessageRequest::getSopId
//
//  PURPOSE: To manipulate the message received from a "REQUESTOR"
//			 application on the 
//           MQ Series message queue for readiness to send to
//           the Service Order Processor.
//
//  METHOD: Parses the message received into acceptable format for the
//          SOP. 
//
//  PARAMETERS: char* msg - the message to format received from the 
//	'Requestor' .
//		MQLONG msgLengthReceived - the length of the message received
//			from the MQGET() call
//  
//  OTHER OUTPUTS:
//  
//  OTHER INPUTS:_sendErrQ  - the message queue to send error messages
//
//  RETURNS: Returns one if any of the function calls fail or the message
//	was not successfully formatted and zero otherwise.
// 
//  FUNCTIONS USED: 
//	DSOImessageRequest::memset(),strcpy(),strcat(),
//	sprintf()
//
////////////////////////////////////////////////////////////////////////
int DSOImessageRequest::getSopId(char* functionCode, char* ownerCode, char* DefaultSopId,
                                 DSOIsendMQ* sendErrQ, char* msg)
{
        char  GivenSopId[10] = "     ";
        const char* FunCdTag = "FunctionCode=";
        const char* OrdTypTag = "OrderType=";
        char  OrdTypVal[3] = "  ";
        const char* OrdNumTag = "OrderNumber=";
        char  OrdNumVal[9] = "        ";
        const char* NPANXXTag = "NPANXX=";
        char  NPANXXVal[7] = "      ";
        const char* UserIdTag = "UserID=";
        char  UserIdVal[9] = "        ";
        const char* LSRIDTag = "LSRID=";
        char  LSRIDVal[30] = "                             ";
        const char* SopIdTag = "SopId=";
        char  SopIdVal[10] = "     ";
        const char* queryListTypeTag = "QueryListType=";
        char  queryListTypeVal[4] = "   ";
        const char* queryDataTag = "QueryData=";
        char  queryDataVal[46] = "                                            ";
        const char* querySectnTag = "QuerySection=";
        char  querySectnVal[6] = "     ";
        const char* LockTypTag = "LockTyp=";
        char  LockTypVal[46] = "                                            ";

        char* tagValue=  new char[100];
	
        char hexName = 0xff;
	int returnCode = 0;
#ifdef DEBUG
cout << "in messageMassage " << msg << endl;
#endif
	_sendErrQ = sendErrQ;
	char* msgPtr = msg;

        memset(_msgTrunc,0,450);
        memset(NPANXXVal,0,7);
        memset(SopIdVal,0,10);
        memset(UserIdVal,0,9);
        memset(LSRIDVal,0,30);
        memset(GivenSopId,0,10);

        memset(tagValue,0,100);
        if (findTagValue(msg, FunCdTag, tagValue) == 0)
          { strcpy(functionCode, tagValue); 
            functionCode[2] = '\0';
          }
        else
	  { messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because missing FuncCd");
	    messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because missing FuncCd ", (char *) "", -1, (char *) "findTagValue", (char *) __FILE__, __LINE__);
	    messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            delete [] tagValue;
            return -1;
          }
        memset(tagValue,0,100);
        if (findTagValue(msg, SopIdTag, tagValue) == 0)
          { strcpy(SopIdVal, tagValue); 
            strcpy(GivenSopId,   SopIdVal); 
          }
        else
          { GivenSopId[0] = '\0';}

        strcat(_msgTrunc,"OWN=");
        strcat(_msgTrunc,ownerCode);
        strcat(_msgTrunc,"\xff");

        memset(tagValue,0,100);
        if (findTagValue(msg, NPANXXTag, tagValue) == 0)
          { strcpy(NPANXXVal, tagValue); 
          }
        else
	  { if (strcmp(functionCode,"ON")==0)       //  NPANXX is optional for 'ON'
              { strcpy(NPANXXVal, "206451");        //  ON always go to RSO
                strcat(_msgTrunc,"SopId=RSO");
	        strncpy(DefaultSopId,"ONS",3);
                strcat(_msgTrunc,"\xff");
                strcat(_msgTrunc,msg);
                delete [] tagValue;
	        return 0;
              }
            else
	      { messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because missing NpaNxx");
	        messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because missing NpaNxx ", (char *) "", -1, (char *) "findTagValue", (char *) __FILE__, __LINE__);
	        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                delete [] tagValue;
                return -1;
              }
          }

        if (GivenSopId[0] == '\0')
          { if (verifyNPA(NPANXXVal,DefaultSopId) !=0)
              {  memset(_errorMessage,0,1024);
                 sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because INVALID NPA  ", NPANXXVal," ");
                 messageErrObj.createReplyErrMsg(_errorMessage);
                 messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because INVALID NPA",
                               NPANXXVal, returnCode, (char *) "messageMassage", (char *) __FILE__, __LINE__);
                 messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                 return -1;
              }
            else
              { memset(SopIdVal,0,10);
                strncpy(SopIdVal, DefaultSopId,3);
                strcpy(GivenSopId, DefaultSopId);
              }
          }
        else
          { strcpy(DefaultSopId, GivenSopId);
            memset(SopIdVal,0,10);
            strncpy(SopIdVal, DefaultSopId,3);
          }

        memset(tagValue,0,100);
        if (findTagValue(msg, UserIdTag, tagValue) == 0)
          { strncpy(UserIdVal,"         ",8);
	    UserIdVal[8] = '\0';
            strncpy(UserIdVal, tagValue, strlen(tagValue)); 
	    strncpy(useridtemp, UserIdVal,strlen(UserIdVal));
	    for(int i=0; i < strlen(useridtemp); i++)
	    {
	       if(islower(useridtemp[i]))
	       {
		  useridtemp[i] = toupper(useridtemp[i]);
               }
           }  
          }
        else
	  { // messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because missing UserId");
	    // messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because missing UserId ", (char *) "", -1, (char *) "findTagValue", (char *) __FILE__, __LINE__);
	    // messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            // return -1;
          }
	
        memset(tagValue,0,100);
        if (findTagValue(msg, LSRIDTag, tagValue) == 0)
          { if (strlen(tagValue) < 30)
	      { strncpy(LSRIDVal, tagValue, strlen(tagValue)); 
                LSRIDVal[strlen(tagValue)+1] = '\0';
              }
	    else
	      { strncpy(LSRIDVal, tagValue, 30); 
                LSRIDVal[31] = '\0';
              }
          }
        else
          { LSRIDVal[0] = '\0';
          }


#ifdef DEBUG
cout << "   funcCode=" << functionCode  << endl;
cout << "     userId=" << UserIdVal   << endl;
cout << "     NpaNxx=" << NPANXXVal   << endl;
cout << " GivenSopId=" << GivenSopId   << endl;
cout << " DefltSopId=" << DefaultSopId << endl;
#endif

        if ((strcmp(functionCode,"AM")==0) || (strcmp(functionCode,"CK")==0))
          { strcat(_msgTrunc,"SopId=");
            strcat(_msgTrunc,DefaultSopId);
            strcat(_msgTrunc,"\xff");
            strcat(_msgTrunc,msg);
            delete [] tagValue;
	    return 0;
          }

        strcat(_msgTrunc,"FunctionCode=");
        strcat(_msgTrunc,functionCode);
        strcat(_msgTrunc,"\xff");
        strcat(_msgTrunc,"NPANXX=");
        strcat(_msgTrunc,NPANXXVal);
        strcat(_msgTrunc,"\xff");
        strcat(_msgTrunc,"SopId=");
        strcat(_msgTrunc,DefaultSopId);
        strcat(_msgTrunc,"\xff");
        strcat(_msgTrunc,"UserID=");
        strcat(_msgTrunc,UserIdVal);
        strcat(_msgTrunc,"\xff");
        if (LSRIDVal[0] != '\0')
          { strcat(_msgTrunc,"LSRID=");
            strcat(_msgTrunc,LSRIDVal);
            strcat(_msgTrunc,"\xff");
          }
	
	if (strcmp(functionCode,"EN")==0  || 
	    strcmp(functionCode,"CO")==0  ||
	    strcmp(functionCode,"RP")==0  ||
	    strcmp(functionCode,"DD")==0  ||
	    strcmp(functionCode,"CP")==0  ||
	    strcmp(functionCode,"IQ")==0  ||
	    strcmp(functionCode,"UP")==0  ||
	    strcmp(functionCode,"QS")==0  ||
	    strcmp(functionCode,"UN")==0  ||
	    strcmp(functionCode,"NL")==0  ||
	    strcmp(functionCode,"DL")==0  ||
	    strcmp(functionCode,"NQ")==0  ||
	    strcmp(functionCode,"NT")==0  ||
	    strcmp(functionCode,"NU")==0  ||
	    strcmp(functionCode,"NE")==0  ||
	    strcmp(functionCode,"RQ")==0  ||
	    strcmp(functionCode,"NR")==0 )
          { memset(tagValue,0,100);
            if (findTagValue(msg, OrdTypTag, tagValue) == 0)
              { strcpy(OrdTypVal, tagValue); 
              }
            else
	      { // messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because missing OrderType");
	        // messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because missing OrderType ", (char  *) "", -1, (char *) "findTagValue", (char *) __FILE__, __LINE__);
	        // messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                // return -1;
              }
            memset(tagValue,0,100);
            if (findTagValue(msg, OrdNumTag, tagValue) == 0)
              { strcpy(OrdNumVal, tagValue); 
              }
            else
	      { // messageErrObj.createReplyErrMsg((char  *) "DSOIREQUEST:failed because missing OrderNumber");
	        // messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because missing OrderNumber ", (char *) "", -1, (char *) "findTagValue", (char *) __FILE__, __LINE__);
	        // messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                // return -1;
              }
#ifdef DEBUG
cout << "    OrdTyp=" << OrdTypVal   << endl;
cout << "    OrdNum=" << OrdNumVal   << endl;
#endif
              strcat(_msgTrunc,"OrderType=");
              strcat(_msgTrunc,OrdTypVal);
              strcat(_msgTrunc,"\xff");
              strcat(_msgTrunc,"OrderNumber=");
              strcat(_msgTrunc,OrdNumVal);
              strcat(_msgTrunc,"\xff");

	      if (strcmp(functionCode,"NL")==0 )
                { memset(LockTypVal,0,46);
                  memset(tagValue,0,100);
                  if (findTagValue(msg, LockTypTag, tagValue) == 0)
                    { strcpy(LockTypVal, tagValue); 
                      strcat(_msgTrunc,"LockTyp=");
                      strcat(_msgTrunc,LockTypVal);
                      strcat(_msgTrunc,"\xff");
                    }
                }

	  }  // end if (strcmp(functionCode,"EN")==0  || 
       
	if (strcmp(functionCode,"SQ")==0)
          { memset(tagValue,0,100);
            if (findTagValue(msg, "SQUSER", tagValue) == 0)
              { strcat(_msgTrunc,"SQUSER=");
                strcat(_msgTrunc,tagValue);
                strcat(_msgTrunc,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msg, "SQSLS", tagValue) == 0)
              { strcat(_msgTrunc,"SQSLS=");
                strcat(_msgTrunc,tagValue);
                strcat(_msgTrunc,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msg, "SQNPA", tagValue) == 0)
              { strcat(_msgTrunc,"SQNPA=");
                strcat(_msgTrunc,tagValue);
                strcat(_msgTrunc,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msg, "SQORDN", tagValue) == 0)
              { strcat(_msgTrunc,"SQORDN=");
                strcat(_msgTrunc,tagValue);
                strcat(_msgTrunc,"\xff");
              }
            memset(tagValue,0,100);
            if (findTagValue(msg, "SQSTAT", tagValue) == 0)
              { strcat(_msgTrunc,"SQSTAT=");
                strcat(_msgTrunc,tagValue);
                strcat(_msgTrunc,"\xff");
              }
	  }  // end  if (strcmp(functionCode,"SQ")==0)

	if (strcmp(functionCode,"CL")==0)
          { memset(queryListTypeVal,0,4);
            memset(tagValue,0,100);
            if (findTagValue(msg, queryListTypeTag, tagValue) == 0)
              { strcpy(queryListTypeVal, tagValue);
              }
            else
	      { // messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because missing Query List Type");
	        // messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because missing Query List Type ", (char *) "", -1, (char *) "findTagValue", (char *) __FILE__, __LINE__);
	        // messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                // return -1;
              }
            memset(queryDataVal,0,46);
            memset(tagValue,0,100);
            if (findTagValue(msg, queryDataTag, tagValue) == 0)
              { strcpy(queryDataVal, tagValue);
              }
            else
	      { // messageErrObj.createReplyErrMsg((char *) "DSOIREQUEST:failed because missing Query Data");
	        // messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because missing Query Data ", (char *) "", -1, (char *) "findTagValue", (char *) __FILE__, __LINE__);
	        // messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                // return -1;
              }
            strcat(_msgTrunc,"QueryListType=");
            strcat(_msgTrunc,queryListTypeVal);
            strcat(_msgTrunc,"\xff");
            strcat(_msgTrunc,"QueryData=");
            strcat(_msgTrunc,queryDataVal);
            strcat(_msgTrunc,"\xff");
	  }  // end  if (strcmp(functionCode,"CL")==0)

        if (strcmp(functionCode,"DX")==0)
           { memset(queryListTypeVal,0,4);
             memset(tagValue,0,100);
             if (findTagValue(msg, queryListTypeTag, tagValue) == 0)
               { strcpy(queryListTypeVal, tagValue);
               }
             memset(queryDataVal,0,46);
             memset(tagValue,0,100);
             if (findTagValue(msg, queryDataTag, tagValue) == 0)
               { strcpy(queryDataVal, tagValue);
               }
             strcat(_msgTrunc,"QueryListType=");
             strcat(_msgTrunc,queryListTypeVal);
             strcat(_msgTrunc,"\xff");
             strcat(_msgTrunc,"QueryData=");
             strcat(_msgTrunc,queryDataVal);
             strcat(_msgTrunc,"\xff");
         }  // end  if (strcmp(functionCode,"DX")==0)

        delete [] tagValue;

	return 0;

}

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::verifyNPA
//
//  PURPOSE: Verify that the NPA is valid for
//	the SOP the DSOI is set to send messages to
//
//  METHOD:
//
//  PARAMETERS: char* Npa - the NPA parsed from the tag value
//			of the REQUESTOR message
// 
//  OTHER OUTPUTS: sopsOpen array is updated to the SOP valid for
//	the NPA
//  
//  OTHER INPUTS: A table of all the valid NPAs,
//		_sendErrQ - message queue to send errors
//
//  RETURNS: One if the NPA is not valid, zero otherwise.
// 
//  FUNCTIONS USED: strcmp(), strcpy()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::verifyNPA(char* Npa, char* SopId)
{
	const char* binPtr;
	int tempSopId=0;

#ifdef DEBUG
cout << "routeInfo= " << _routeInfo << endl;
#endif
	if( (binPtr = binarySearch(_routeInfo,Npa,_sizeOfRouteInfo,3) )==NULL)
	  {
		memset(_errorMessage,0,1024);
		sprintf(_errorMessage,"%s%s%s","DSOIREQUEST:failed because npa not FOUND NPANXX  ", Npa, " ");
		messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because npa not FOUND NPANXX=", Npa, -1, (char *) "verifyNPA", (char *) __FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
	  }

        binPtr +=21;
        strncpy(SopId,binPtr,3);                   // CR 9413 get default SopId
        binPtr +=4;
	// QCD 1112 - SCR 21 for DSOI Border town  Begin

        if (!strcmp (SopId, "SOU")) {
	    tempSopId=1;
        }
        if (!strcmp (SopId, "ESS")) {
	    tempSopId=2;
        }
	if (!strcmp (SopId, "SOC")) {
	    tempSopId=3;
        }
	if (!strcmp (SopId, "ESN")) {
	    tempSopId=4;
        }
     
        if (( !strncmp(binPtr,"YY",2) && (verifyNXX(Npa) ==0))) { // CR 9413,8410 Check for Bordertowns
            SopId[0] = '\0';
            switch  (tempSopId) {  

	    case 1: 
                if ((strncmp(Npa,"208",3)==0)) 
                {	        
		    if (!(strncmp(Npa,"208225",6)==0) && !(strncmp(Npa,"208C25",6)==0))
		    {
		        strncpy(SopId,"RSO",3);         // CR 9413 RSOLAR Washington
                        SopId[3] = '\0';
			return 0;
                    } else {
                        strncpy(SopId,"SOC",3);         // Border Town Change SOPAD Colo
                        SopId[3] = '\0';
			return 0;
		    }
                }
	    
                if ((strncmp(Npa,"406",3)==0) || (strncmp(Npa,"435289",6)==0))
	        {
                    strncpy(SopId,"SOC",3);         // Border Town Change SOPAD Colo
                    SopId[3] = '\0';
                    return 0; 
	         }
            break; 

	    case 2:
                if ((strncmp(Npa,"308",3)==0))
                {
                    strncpy(SopId,"SOC",3);         // Border Town Change SOPAD Colo
                    SopId[3] = '\0';
                    return 0;
                }
	/*	if ((strncmp(Npa,"605862",6)==0))
	        {
		    strncpy(SopId,"ESN",3);         // Border Town Change SOLAR North
	            SopId[3] = '\0';
	       	    return 0;
                }*/
	     break;

             case 3:
	         if ((strncmp(Npa,"307849",6)==0) || (strncmp(Npa,"307J49",6)==0))
                 {
                     strncpy(SopId,"SOU",3);         // Border Town Change SOPAD Utah
                     SopId[3] = '\0';
                     return 0;
                 }
                 if ((strncmp(Npa,"307643",6)==0) || (strncmp(Npa,"307896",6)==0) ||
	             (strncmp(Npa,"307H88",6)==0) || (strncmp(Npa,"307D11",6)==0) ||
		     (strncmp(Npa,"307W11",6)==0) || (strncmp(Npa,"307Z01",6)==0) ||
		     (strncmp(Npa,"307Z22",6)==0) || (strncmp(Npa,"970",3)==0)) 
	         {
                     strncpy(SopId,"ESS",3);         // Border Town Change SOLAR South
                     SopId[3] = '\0';
                     return 0;
                  }                  
              break;

	      case 4 :
	          if ((strncmp(Npa,"701",3)==0))
	          {
                      strncpy(SopId,"SOU",3);         // Border Town Change SOPAD Utah
                      SopId[3] = '\0';
                      return 0;
                  }
              break;

              }
        }
        // QCD 1112 - SCR 21 for DSOI Border town  End	
        return 0;
} //end if verifyNPA()

////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::verifyNXX(char* nxx)
//
//  PURPOSE: To verify that the  NXX portion of the NPANXX
//	parameter sent from the "Requestor" is valid.  
//
//  METHOD: By using the npaSplit array that was created in
//	readDataRef() it calls binarySearch to look for the NXX 
//	in that array.  
//
//  PARAMETERS: const char* nxx - the NXX that will be searched for.
//  
//  OTHER INPUTS: npaSplit - the array that holds all valid NXX's
//	for the valid NPAs  
//	_sizeOfNpaSplit - the size of the array that holds all valid
//	NXXs, updated by readDataRef()
//
//  RETURNS: Returns -1 if the NXX is NOT found, 0 if the 
//	NXX is valid and found
// 
//  FUNCTIONS USED:  binarySearch()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::verifyNXX(char* ws_npa)
{
#ifdef DEBUG
cout << "npaSplit= " << _npaSplit << endl;
#endif
	const char* binPtr;
	if( (binPtr = binarySearch(_npaSplit, ws_npa, _sizeOfNpaSplit,6)) == NULL)
	{
    	return -1;
	}
	else
	{
	return 0;
	}
}


/////////////////////////////////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::verifyOWN(char* ownerName, char* ownerCode)
//
//  PURPOSE: To  retrieve Ownership Code
//
//  METHOD: 
//
//  PARAMETERS: const char* own  - the OWN that will be searched for.
// 
//  OTHER INPUTS:
//
//  RETURNS: 
//
//  FUNCTIONS USED:  binarySearch()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::verifyOWN(char* ownerName, char* ownerCode, char* setPriority)
{
#ifdef DEBUG
cout << "ownerInfo= " << _ownerInfo << endl;
#endif
        const char* binPtr;
        if (strstr(ownerName, "REQUEST") != NULL)   
          { strcpy(ownerCode,"D");             // default OwnershipCode
            strcpy(setPriority,"0");           // default Priority
	    ownerCode[1] = '\0';
	    setPriority[1] = '\0';
            return 0;
          }
        if ((binPtr = binarySearch(_ownerInfo, ownerName, _sizeOfOwnerInfo,5) )==NULL)
          {
            strcpy(ownerCode,"D");             // default OwnershipCode
            strcpy(setPriority,"0");           // default Priority
	    ownerCode[1] = '\0';
	    setPriority[1] = '\0';
            return -1;
          }
        else
	  { binPtr +=10;
            strncpy(setPriority,binPtr,1);     // Get Priority
	    setPriority[1] = '\0';
	    binPtr +=2;
            strncpy(ownerCode,binPtr,1);       // Set OwnershipCode
	    ownerCode[1] = '\0';
	  }
        if (atoi(setPriority) < 0  || atoi(setPriority) > 9 )
          { strcpy(setPriority,"0");           // default Priority
	    setPriority[1] = '\0';
            return -1;
          }
        return 0;

}


////////////////////////////////////////////////
//  FUNCTION: int DSOImessageRequest::readDataRef(char* msg)
//
//  PURPOSE: to read a  message  that holds the
//	data that is dynamically read from the QL_DSOI_DATAREFQ.
//  This data contains 
//	information about the names of the routing queues
//	and the npa split information and valid U S West User Ids.
//
//  METHOD: Reads the message received on the QL_DSOI_DATAREFQ.
//	Each type of dynamic information is delemited by a Keyword,
//	equal sign, and ending in \Xff. The data is on of routeInfo,
//	npa_split info. An array of pointers is creating with
//	each pointer of the array pointing to the data value of the routing,
//	npa_split, or userid.
// 
//  PARAMETERS: char* msg - the  message read from the QL_DSOI_DATAREFQ
//  
//  OTHER OUTPUTS: npaSplit - the pointer to the array of npa split data
// 	routeInfo - pointer to the array of route information pointers,
//  
//  OTHER INPUTS:
//
//  RETURNS: Returns 0 if data is found for DataRef,
// -1 otherwise.
// 
//  FUNCTIONS USED:  
//	DSOIerrorHandler::sendErrorMessage(), 
//	DSOIerrorHandler::formatErrMsg(), new()
//
//////////////////////////////////////////////////////
int DSOImessageRequest::readDataRef(char* msg)
{
#ifdef DEBUG
cout << " in readDataRef msg=" << msg << endl;
cout << " in readDataRef lng=" << strlen(msg) << endl;
#endif
   const char* npaSplitTag  = "npaSplit=";
   const char* routeInfoTag = "routeInfo=";
   const char* ownerInfoTag = "ownerInfo=";
   const char* msgPtr = msg;
   char* hexPtr;

   _sizeOfNpaSplit  = 0;
   _sizeOfRouteInfo = 0;
   _sizeOfOwnerInfo = 0;
   msgPtr = msg;
   msgPtr = strstr(msgPtr, npaSplitTag); 
   if (msgPtr != NULL)
     {
       msgPtr += 9;
       hexPtr = strchr((char *)msgPtr, HEXFF);
       _sizeOfNpaSplit = (hexPtr - msgPtr)/7;
       if ((_npaSplit = new char*[_sizeOfNpaSplit * 7]) ==NULL)
         {  messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because realloc(new) FAILED" ,
                                       (char *) "", -1, (char *) "readDataRef",(char *) __FILE__, __LINE__);
            messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            return -1;
         }

       for(int i=0; i < _sizeOfNpaSplit; ++i)
          {
            // test if ((_npaSplit[i] = (char *) malloc(7)) == NULL)
            if ((_npaSplit[i] = new char[7]) == NULL)
              {  messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" ,
                                       (char *) "", -1, (char *) "readDataRef", (char *) __FILE__, __LINE__);
                 messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                 return -1;
              }
            strncpy(_npaSplit[i], msgPtr, 6); 
            msgPtr += 7;
          }
     }
#ifdef DEBUG
    cout << "npaSplit_size=" << _sizeOfNpaSplit  << endl;
    for(int j1=0; j1 < _sizeOfNpaSplit; ++j1)
       { if (j1 == _sizeOfNpaSplit)   break;
         cout << "npaSplit="  << _npaSplit[j1]  << endl;
       }
#endif

   
   msgPtr = msg;
   msgPtr = strstr(msgPtr, routeInfoTag); 
   if (msgPtr != NULL)
     {
       msgPtr += 10;
       hexPtr = strchr((char *)msgPtr, HEXFF);
       _sizeOfRouteInfo = (hexPtr - msgPtr)/28;
       if ((_routeInfo = new char*[_sizeOfRouteInfo * 28]) ==NULL)
         {  messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" ,
                                       (char *) "", -1, (char *) "readDataRef", (char *) __FILE__, __LINE__);
            messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            return -1;
         }

       for(int i=0; i < _sizeOfRouteInfo; ++i)
          {
            // test if ((_routeInfo[i] = (char *) malloc(27)) == NULL)
            if ((_routeInfo[i] = new char[27]) == NULL)
              {  messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" ,
                                       (char *) "", -1, (char *) "readDataRef", (char *) __FILE__, __LINE__);
                 messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                 return -1;
              }
            strncpy(_routeInfo[i], msgPtr, 27); 
            msgPtr += 28;
          }
     }

#ifdef DEBUG
    cout << "routeInfo_size=" << _sizeOfRouteInfo  << endl;
    for(int j2=0; j2 < _sizeOfRouteInfo; ++j2)
       { if (j2 == _sizeOfRouteInfo)   break;
         cout << "routeInfo="  << _routeInfo[j2]  << endl;
       }
#endif

   msgPtr = msg;
   msgPtr = strstr(msgPtr, ownerInfoTag);
   if (msgPtr != NULL)
     {
       msgPtr += 10;
       hexPtr = strchr((char *)msgPtr, HEXFF);
       _sizeOfOwnerInfo = (hexPtr - msgPtr)/14;
       if ((_ownerInfo = new char*[_sizeOfOwnerInfo * 14]) ==NULL)
         {  messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" ,
                                       (char *) "", -1, (char *) "readDataRef", (char *) __FILE__, __LINE__);
            messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            return -1;
         }

       for(int i=0; i < _sizeOfOwnerInfo; ++i)
          {
            // test if ((_ownerInfo[i] = (char *) malloc(13)) == NULL)
            if ((_ownerInfo[i] = new char[13]) == NULL)
              {  messageErrObj.formatErrMsg((char *) "DSOIREQUEST:failed because malloc(new) FAILED" ,
                                       (char *) "", -1, (char *) "readDataRef", (char *) __FILE__, __LINE__);
                 messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                 return -1;
              }
            strncpy(_ownerInfo[i], msgPtr, 13);
            msgPtr += 14;
          }
     }

#ifdef DEBUG
    cout << "ownerInfo_size=" << _sizeOfOwnerInfo  << endl;
    for(int j3=0; j3 < _sizeOfOwnerInfo; ++j3)
       { if (j3 == _sizeOfOwnerInfo)   break;
         cout << "ownerInfo="  << _ownerInfo[j3]  << endl;
       }
#endif

   return 0;
}


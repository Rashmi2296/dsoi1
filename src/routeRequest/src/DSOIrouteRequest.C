/////////////////////////////////////////////////////  
//  FILE NAME:  DSOIrouteRequest.C
//  
//  REVISION HISTORY:
//    -----------------------------------------------------------------
//  version	date		author	description
//---------------------------------------------------------------------
//  7.00.00	01/07/00	rfuruta	CR3123, initial development
//  7.01.00     07/03/00        rfuruta CR 4275, 4P connection
//  7.01.00     07/14/00        rfuruta CR 4329, 4P override
//  7.01.00     08/25/00        rfuruta CR 4329, 'P'order types go to legacy SOPs
//  7.01.00     08/25/00        rfuruta CR 4783, removed '<' and '>' from error msg
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include "DSOIrouteRequest.H"
#include "DSOIThrowError.H"
#include "DSOIroute.H"
#include "DSOItoolbox.H"
#include "DSOIconstants.H"
#include <cstdio>   
#include <cstddef>
#include <ctime>
#include <cctype>
#include <new>
//#include <unistd.h>

// define the static members of the DSOIrouteRequest class

//private members
DSOIerrorHandler DSOIrouteRequest::messageErrObj;
char* DSOIrouteRequest::data = '\0';
char** DSOIrouteRequest::_npaSplit = '\0';
char** DSOIrouteRequest::_routeInfo = '\0';
int DSOIrouteRequest::_sizeOfNpaSplit = 0;
int DSOIrouteRequest::_sizeOfRouteInfo = 0;
DSOIsendMQ* DSOIrouteRequest::_sendErrQ = 0;
char DSOIrouteRequest::_msgTrunc[150] = "";
char DSOIrouteRequest::_errorMessage[1024] = "";

//public members

////////////////////////////////////////////////////////////////////////
//  FUNCTION: DSOIrouteRequest::messageMassage
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
//	DSOIrouteRequest::memset(),strcpy(),strcat(),
//	sprintf()
//
////////////////////////////////////////////////////////////////////////
int DSOIrouteRequest::messageMassage(char* msg, DSOIsendMQ* sendErrQ, char* functionCode,
            char* userId, char* GivenSopId, char* DefaultSopId, char* Is4P, char* dtl4Pmsg)
{
        const char* FunCdTag = "FunctionCode=";
        const char* OrdTypTag = "OrderType=";
        char  OrdTypVal[3] = "  ";
        const char* OrdNumTag = "OrderNumber=";
        char  OrdNumVal[9] = "        ";
        const char* NPANXXTag = "NPANXX=";
        char  NPANXXVal[7] = "      ";
        const char* UserIdTag = "UserID=";
        const char* SopIdTag = "SopId=";
        char  SopIdVal[10] = "     ";
        const char* queryListTypeTag = "QueryListType=";
        char  queryListTypeVal[4] = "   ";
        const char* queryDataTag = "QueryData=";
        char  queryDataVal[46] = "                                            ";

        const char* TnFid  = "03L TN ";
        char  TnFidVal[13] = "            ";
        const char* CusFid_l = "03L CUS";   // Western and Central
        const char* CusFid_f = "03F CUS";   // Eastern
        char* tagValue=  new(nothrow) char[100];
        const char* classSvcFid  = "03L CS  ";
        const char* classSvcFid1 = "03L ICS ";
	
        char hexName = 0xff;
	int returnCode = 0;
#ifdef DEBUG
cout << "in messageMassage " << msg << endl;
#endif
	_sendErrQ = sendErrQ;
	char* msgPtr = msg;

        // 4P Platform SOL Message Layout
        char* regCode=   new(nothrow) char[2];
        char* NpaNxx=    new(nothrow) char[7];
        char* ordNo=     new(nothrow) char[10];
        char* blgTelNo=  new(nothrow) char[11];
        char* custCD=    new(nothrow) char[4];
        char* srchFidCD= new(nothrow) char[3];
        char* srchFidDA= new(nothrow) char[46];

        const char* filler = "                                                    ";

        strncpy(regCode,filler,2);
        strncpy(NpaNxx,filler,7);
        strncpy(ordNo,filler,10);
        strncpy(blgTelNo,filler,11);
        strncpy(custCD,filler,4);
        strncpy(srchFidCD,filler,3);
        strncpy(srchFidDA,filler,46);

        memset(_msgTrunc,0,150);
        memset(NPANXXVal,0,7);
        memset(SopIdVal,0,10);

        memset(tagValue,0,100);
        if (findTagValue(msg, (char *)FunCdTag, tagValue) == 0)
          { strcpy(functionCode, tagValue); 
          }
        else
	  { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed because missing FuncCd");
	    messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because missing FuncCd ", "", -1, "findTagValue",__FILE__, __LINE__);
	    messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            return -1;
          }
        memset(tagValue,0,100);
        if (findTagValue(msg, (char *)SopIdTag, tagValue) == 0)
          { strcpy(SopIdVal, tagValue); 
            strcpy(GivenSopId, SopIdVal); 
          }

        memset(tagValue,0,100);
        if (findTagValue(msg, (char *)NPANXXTag, tagValue) == 0)
          { strcpy(NPANXXVal, tagValue); 
            strncpy(NpaNxx, NPANXXVal,6); 
          }
        else
	  { if (strcmp(functionCode,"ON")==0)       //  NPANXX is optional for 'ON'
              { strcpy(NPANXXVal, "206451");        //  ON always go to RSO
                memset(Is4P,0,4);
	        strncpy(Is4P, "N4P", 3);
	        Is4P[3] = '\0';
	        strncpy(DefaultSopId,"RSO",3);
                strcat(_msgTrunc,"SopId=");
                strcat(_msgTrunc,DefaultSopId);
                strcat(_msgTrunc,"\xff");
                strcat(_msgTrunc,msg);
	        return 0;
              }
            else
	      { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed because missing NpaNxx");
	        messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because missing NpaNxx ", "", -1, "findTagValue",__FILE__, __LINE__);
	        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                return -1;
              }
          }

        if (verifyNPA(NPANXXVal,DefaultSopId,Is4P) !=0)
          {   memset(_errorMessage,0,1024);
              sprintf(_errorMessage,"%s%s%s","DSOIROUTEREQUEST:failed because INVALID NPA  ", NPANXXVal," ");
              messageErrObj.createReplyErrMsg(_errorMessage);
              messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because INVALID NPA",
                               NPANXXVal, returnCode,"messageMassage",__FILE__, __LINE__);
              messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
              return -1;
          }
        else
          { memset(SopIdVal,0,10);
            strncpy(SopIdVal, DefaultSopId,3);
            if (strcmp(GivenSopId,"YYY")==0)      // 4P exceptions,  CR4329
              { memset(Is4P,0,4);
	        strncpy(Is4P, "SOL", 3);
	        Is4P[3] = '\0';
              }
            else
              if (strcmp(GivenSopId,"NNN")==0)    // 4P exceptions,  CR4329
                { memset(Is4P,0,4);
	          strncpy(Is4P, "N4P", 3);
	          Is4P[3] = '\0';
                }
            else
              if (GivenSopId[0] != '\0')         // 4P exceptions
                { memset(Is4P,0,4);
	          strncpy(Is4P, "N4P", 3);
	          Is4P[3] = '\0';
                }
          }
        if ((strcmp(Is4P,"4P")==0) || (strcmp(Is4P,"SOL")==0))
          { memset(Is4P,0,4);
	    strncpy(Is4P, "SOL", 3);
	    Is4P[3] = '\0';
          }
        else
          { memset(Is4P,0,4);
	    strncpy(Is4P, "N4P", 3);
	    Is4P[3] = '\0';
          }

        memset(tagValue,0,100);
        if (findTagValue(msg, (char *)UserIdTag, tagValue) == 0)
          { strncpy(userId,"         ",8);
            strncpy(userId, tagValue, strlen(tagValue)); 
            userId[8] = '\0';
          }
        else
	  { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed because missing UserId");
	    messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because missing UserId ", "", -1, "findTagValue",__FILE__, __LINE__);
	    messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            return -1;
          }
	
#ifdef DEBUG
cout << "   funcCode=" << functionCode  << endl;
cout << "    regCode=" << regCode  << endl;
cout << "     userId=" << userId   << endl;
cout << "     NpaNxx=" << NpaNxx   << endl;
cout << " GivenSopId=" << GivenSopId   << endl;
cout << " DefltSopId=" << DefaultSopId << endl;
#endif

        strcat(_msgTrunc,"FunctionCode=");
        strcat(_msgTrunc,functionCode);
        strcat(_msgTrunc,"\xff");
        strcat(_msgTrunc,"NPANXX=");
        strcat(_msgTrunc,NpaNxx);
        strcat(_msgTrunc,"\xff");
        if (GivenSopId[0] == '\0')
          { strcat(_msgTrunc,"SopId=");
            strcat(_msgTrunc,DefaultSopId);
            strcat(_msgTrunc,"\xff");
          }
        else
          { strcat(_msgTrunc,"SopId=");
            strcat(_msgTrunc,GivenSopId);
            strcat(_msgTrunc,"\xff");
          }
        strcat(_msgTrunc,"UserID=");
        strcat(_msgTrunc,userId);
        strcat(_msgTrunc,"\xff");

        if ((strcmp(functionCode,"AM")==0) || (strcmp(functionCode,"CK")==0))  // 4P exceptions
          { memset(Is4P,0,4);
	    strncpy(Is4P, "N4P", 3);
	    Is4P[3] = '\0';
	    return 0;
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
	    strcmp(functionCode,"DL")==0 )
          { memset(tagValue,0,100);
            if (findTagValue(msg, (char *)OrdTypTag, tagValue) == 0)
              { strcpy(OrdTypVal, tagValue); 
                strncpy(ordNo, tagValue, 1); 
                ordNo[1] = '\0';
              }
            else
	      { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed because missing OrderType");
	        messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because missing OrderType ", "", -1, "findTagValue",__FILE__, __LINE__);
	        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                return -1;
              }
            memset(tagValue,0,100);
            if (findTagValue(msg, (char *)OrdNumTag, tagValue) == 0)
              { strcpy(OrdNumVal, tagValue); 
                strncat(ordNo, tagValue, 8); 
              }
            else
	      { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed because missing OrderNumber");
	        messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because missing OrderNumber ", "", -1, "findTagValue",__FILE__, __LINE__);
	        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                return -1;
              }
#ifdef DEBUG
cout << "     ordNo=" << ordNo   << endl;
#endif
              strcat(_msgTrunc,"OrderType=");
              strcat(_msgTrunc,OrdTypVal);
              strcat(_msgTrunc,"\xff");
              strcat(_msgTrunc,"OrderNumber=");
              strcat(_msgTrunc,OrdNumVal);
              strcat(_msgTrunc,"\xff");
	  }  // end if (strcmp(functionCode,"EN")==0  || 
       
        if ((strcmp(functionCode,"EN")==0) &&         //  4P exceptions
            ((ordNo[0] == 'N') || (ordNo[0] == 'P')))
          { memset(Is4P,0,4);
            strncpy(Is4P, "N4P", 3);
	    Is4P[3] = '\0';                             // 4RS does not accept new connects
	    return 0;                                   // route to legacy 
          }

	// if (strcmp(functionCode,"EN")==0  || 
	//     strcmp(functionCode,"CO")==0  ||
	//     strcmp(functionCode,"RP")==0  ||
	//     strcmp(functionCode,"DD")==0  ||
	//     strcmp(functionCode,"CP")==0 )
	if (strcmp(functionCode,"EN")==0)
          {    memset(tagValue,0,100);
               memset(TnFidVal,0,13);
	       if (getFidValue(msg, (char *)TnFid, tagValue)==0)
                 { strcpy(TnFidVal, tagValue);
                   strncpy(blgTelNo,tagValue,10);
                   blgTelNo[0] = tagValue[0];
                   blgTelNo[1] = tagValue[1];
                   blgTelNo[2] = tagValue[2];
                   blgTelNo[3] = tagValue[4];
                   blgTelNo[4] = tagValue[5];
                   blgTelNo[5] = tagValue[6];
                   blgTelNo[6] = tagValue[8];
                   blgTelNo[7] = tagValue[9];
                   blgTelNo[8] = tagValue[10];
                   blgTelNo[9] = tagValue[11];
                   blgTelNo[10] = '\0';
                 }
               else
	         { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed TN Fid not found in ID section");
	           messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed TN Fid not found in ID section", "", -1, "getFIDvalue",__FILE__, __LINE__);
	           messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                   return -1;
                 }
               memset(tagValue,0,100);
	       if (getFidValue(msg, (char *)CusFid_l, tagValue)==0)
                 { strncpy(custCD,tagValue,3);
                 }
	       else if (getFidValue(msg, (char *)CusFid_f, tagValue)==0)
                 { strncpy(custCD,tagValue,3);
                 }
               else
	         { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed CUS Fid not found in ID section");
	           messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed CUS Fid not found in ID section", "", -1, "getFIDvalue",__FILE__, __LINE__);
	           messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                   return -1;
                 }
#ifdef DEBUG
cout << "   blgTelNo=" << blgTelNo << endl;
cout << "     custCD=" << custCD   << endl;
#endif
            strcat(_msgTrunc,"TN=");
            strcat(_msgTrunc,blgTelNo);
            strcat(_msgTrunc,"\xff");
            strcat(_msgTrunc,"CUS=");
            strcat(_msgTrunc,custCD);
            strcat(_msgTrunc,"\xff");
	  }  // end if (strcmp(functionCode,"EN")==0)
        
	if (strcmp(functionCode,"CL")==0)
          { memset(queryListTypeVal,0,4);
            memset(tagValue,0,100);
            if (findTagValue(msg, (char *)queryListTypeTag, tagValue) == 0)
              { strcpy(queryListTypeVal, tagValue);
                if (strlen(tagValue) == 1)
                  { srchFidCD[0] = tagValue[0];
                    srchFidCD[1] = ' ';
                    srchFidCD[2] = '\0';
                  }
                else
                if (strlen(tagValue) == 2)
                 { strncpy(srchFidCD, tagValue, 2);
                   srchFidCD[2] = '\0';
                 }
                else
                 { srchFidCD[0] = tagValue[1];
                   srchFidCD[1] = tagValue[2];
                   srchFidCD[2] = '\0';
                 }
              }
            else
	      { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed because missing Query List Type");
	        messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because missing Query List Type ", "", -1, "findTagValue",__FILE__, __LINE__);
	        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                return -1;
              }
            memset(queryDataVal,0,46);
            memset(tagValue,0,100);
            if (findTagValue(msg, (char *)queryDataTag, tagValue) == 0)
              { strncpy(srchFidDA, tagValue, strlen(tagValue));
                strcpy(queryDataVal, tagValue);
              }
            else
	      { messageErrObj.createReplyErrMsg("DSOIROUTEREQUEST:failed because missing Query Data");
	        messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because missing Query Data ", "", -1, "findTagValue",__FILE__, __LINE__);
	        messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                return -1;
              }
#ifdef DEBUG
cout << "    srchFidCD=" << srchFidCD << endl;
cout << "    srchFidDA=" << srchFidDA << endl;
#endif
            strcat(_msgTrunc,"QueryListType=");
            strcat(_msgTrunc,queryListTypeVal);
            strcat(_msgTrunc,"\xff");
            strcat(_msgTrunc,"QueryData=");
            strcat(_msgTrunc,queryDataVal);
            strcat(_msgTrunc,"\xff");
	  }  // end  if (strcmp(functionCode,"CL")==0)

        if ((strcmp(Is4P,"N4P")==0) || (strcmp(Is4P,"N4")==0))   // 4P exceptions
          { memset(Is4P,0,4);
	    strncpy(Is4P, "N4P", 3);
	    Is4P[3] = '\0';

	    delete [] tagValue;
	    delete [] regCode;
	    delete [] NpaNxx;
	    delete [] ordNo;
	    delete [] blgTelNo;
	    delete [] custCD;
	    delete [] srchFidCD;
	    delete [] srchFidDA;

	    return 0;
          }

     // Populate SOL Request Msg
        strncat(dtl4Pmsg,regCode,1);
        strncat(dtl4Pmsg,NpaNxx,6);
        strncat(dtl4Pmsg,ordNo,9);
        strncat(dtl4Pmsg,blgTelNo,10);
        strncat(dtl4Pmsg,custCD,3);
        strncat(dtl4Pmsg,srchFidCD,2);
        strncat(dtl4Pmsg,srchFidDA,45);
        dtl4Pmsg[76]= '\0';
        memset(Is4P,0,4);
        strncpy(Is4P, "SOL", 3);
	Is4P[3] = '\0';
#ifdef DEBUG
cout << "  SOLmsg=" << dtl4Pmsg << endl;
cout << " Is4Pmsg=" << Is4P << endl;
#endif
	return 0;

}

////////////////////////////////////////////////
//  FUNCTION: int DSOIrouteRequest::verifyNPA
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
int DSOIrouteRequest::verifyNPA(char* Npa, char* SopId, char* Is4P)
{
	const char* binPtr;

	if( (binPtr = binarySearch(_routeInfo,Npa,_sizeOfRouteInfo,3) )==NULL)
	  {
	//error
		memset(_errorMessage,0,1024);
		sprintf(_errorMessage,"%s%s%s","DSOIROUTEREQUEST:failed because npa not FOUND NPANXX  ", Npa, " ");
		messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because npa not FOUND NPANXX=", Npa, -1, "verifyNPA",__FILE__, __LINE__);
		messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
		return -1;
	  }

	const char* infoPtr = binPtr;
	infoPtr += 25;
	strncpy(Is4P, infoPtr,2);
	Is4P[2] = '\0';

	binPtr +=4;
	if (strncmp(binPtr,"ID",2)==0)
          { if (verifyNXX(Npa) !=0)
	      { strncpy(SopId,"RSO",3);         // RSOLAR Washington
	        SopId[3] = '\0';
	        return 0;
	      }
            else
              { strncpy(SopId,"SOU",3);          // SOPAD Utah
                SopId[3] = '\0';
	        return 0;
              }
          }
        else
	  { binPtr +=17;
            strncpy(SopId,binPtr,3);       // SopId
	    SopId[3] = '\0';
	    return 0;
	  }
	
        return -1;
} //end if verifyNPA()

////////////////////////////////////////////////
//  FUNCTION: int DSOIrouteRequest::verifyNXX(char* nxx)
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
int DSOIrouteRequest::verifyNXX(char* npa)
{
#ifdef DEBUG
cout << "_npaSplit= " << _npaSplit << endl;
#endif
	const char* binPtr;
	if( (binPtr = binarySearch(_npaSplit, npa, _sizeOfNpaSplit,6)) == NULL)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}


////////////////////////////////////////////////
//  FUNCTION: int DSOIrouteRequest::readDataRef(char* msg)
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
int DSOIrouteRequest::readDataRef(char* msg)
{
#ifdef DEBUG
cout << " in readDataRef msg=" << msg << endl;
#endif
   const char* npaSplitTag  = "npaSplit=";
   const char* routeInfoTag = "routeInfo=";
   char* msgPtr = msg;
   char* hexPtr;

   _sizeOfNpaSplit  = 0;
   _sizeOfRouteInfo = 0;
   msgPtr = msg;
   msgPtr = strstr(msgPtr, npaSplitTag); 
   if (msgPtr != NULL)
     {
       msgPtr += 9;
       hexPtr = strchr(msgPtr, '\xff'); 
       _sizeOfNpaSplit = (hexPtr - msgPtr)/7;
       if ((_npaSplit = new(nothrow) char*[_sizeOfNpaSplit * 7]) ==NULL)
         {  messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because realloc(new) FAILED" ,
                                       "", -1, "readDataRef",__FILE__, __LINE__);
            messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            return -1;
         }

       for(int i=0; i < _sizeOfNpaSplit; ++i)
          {
            if ((_npaSplit[i] = (char *) malloc(6)) == NULL)
              {  messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because malloc(new) FAILED" ,
                                       "", -1, "readDataRef",__FILE__, __LINE__);
                 messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                 return -1;
              }
            strncpy(_npaSplit[i], msgPtr, 6); 
            msgPtr += 7;
          }
     }
   
   msgPtr = msg;
   msgPtr = strstr(msgPtr, routeInfoTag); 
   if (msgPtr != NULL)
     {
       msgPtr += 10;
       hexPtr = strchr(msgPtr, '\xff'); 
       _sizeOfRouteInfo = (hexPtr - msgPtr)/28;
       if ((_routeInfo = new(nothrow) char*[_sizeOfRouteInfo * 28]) ==NULL)
         {  messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because malloc(new) FAILED" ,
                                       "", -1, "readDataRef",__FILE__, __LINE__);
            messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
            return -1;
         }

       for(int i=0; i < _sizeOfRouteInfo; ++i)
          {
            if ((_routeInfo[i] = (char *) malloc(27)) == NULL)
              {  messageErrObj.formatErrMsg("DSOIROUTEREQUEST:failed because malloc(new) FAILED" ,
                                       "", -1, "readDataRef",__FILE__, __LINE__);
                 messageErrObj.sendErrorMessage(DSOIerrorHandler::_formatErrMsg, _sendErrQ);
                 return -1;
              }
            strncpy(_routeInfo[i], msgPtr, 27); 
            msgPtr += 28;
          }
     }

#ifdef DEBUG
    cout << "routeInfo_size=" << _sizeOfRouteInfo  << endl;
    for(int j=0; j < _sizeOfRouteInfo; ++j)
       { cout << "routeInfo="  << _routeInfo[j]  << endl;
       }
#endif

   return 0;
}


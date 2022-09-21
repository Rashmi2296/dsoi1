
//  8.08.00     01/02/03        rfuruta         CR8000, upgrade to HP-UX 11.0
//  9.04.00     02/11/04        rfuruta         CR8911, set Expiry time
//  9.08.00     01/03/05        rfuruta         CR9745, set Priority
// 10.04.00     02/01/07        rfuruta 	CR10999, migrate DSOI to Solaris

#include <iostream.h>
#include <cerrno>
#include <new> 		// used for exit() command
#include "DSOIThrowError.H"
#include "DSOIsendMQ.H"

const int NPAPOSITION=0;
const int NXXPOSITION=4;
const int ROUTEPOSITION=21;
const int ORIGINPOSITION=0;
const int OWNERPOSITION=12;
const int PARAMETER = 7;
const int SIZEOFBUFFER=100000;
int STRINGLENGTH=0;
int setExpiry=0;
int setPriority=0;
static  char    _putTime[DSOI_STRING_15];
unsigned char   _msgId[DSOI_STRING_25];
static MQBYTE24 _correlId;

int main (int argc, char** argv )
{
	if(argc != 4)
	{
		printf("USAGE: \n");
		printf("\'DSOIDRefLoader dataRefQ QueueManager fileDirectory ' \n");
		exit(1);
	}
	int returnCode =0;
	try{
                setExpiry=0;
                setPriority=0;
		DSOIsendMQ  *sendDataRefQueue = new DSOIsendMQ(setExpiry,setPriority,argv[1],argv[2]);
		DSOIsendMQ  *errorSendQ = new DSOIsendMQ(setExpiry,setPriority,(char *) "QL_DSOI_ERRORQ",argv[2]);
		const char* fileNames[] = {"npa_split","routeInfo","ownerInfo"};
		int filePositions =0;
		int length = 0;
		int positionValues[5];
		memset(positionValues,0,5);
		int positionValuesSize[5];
		memset(positionValuesSize,0,5);
		const int _numberOfFiles=3;
		char* testfile = new char[145];
		FILE *fp;
		char buffer[SIZEOFBUFFER];
		memset(buffer,0,SIZEOFBUFFER);
		int lengthBuf2 =SIZEOFBUFFER;
		char* bufPtr = new char[SIZEOFBUFFER];
		if( bufPtr == NULL)
		{
		//error
			cout << "new failed" << endl;
			exit(1);
		}
		memset (bufPtr,0,sizeof(bufPtr) );
		int _sizeNeeded = 0;
		for(int i=0; i < _numberOfFiles; i++)
		{
			memset(testfile,0,145);
			strcpy(testfile,argv[3]);
			strcat(testfile,"/");
			strcat(testfile,fileNames[i]);
			fp = fopen(testfile,"r");
			if (fp == NULL)
				cout << "can't open file" << testfile<<endl;
			else
			{
				char* dataPtr = 0;
				switch (i)
				{
				case 0:  //npa_split
					positionValues[0] = NPAPOSITION;
					positionValuesSize[0] = 3;
					positionValues[1] = NXXPOSITION;
					positionValuesSize[1] = 3;
					filePositions = 2;
					STRINGLENGTH += 9;
					strcat(bufPtr,"npaSplit=");
					break;	
				case 1:  //route_info
					positionValues[0] = NPAPOSITION;
					positionValuesSize[0] = 27;
					positionValues[1] = ROUTEPOSITION;
					positionValuesSize[2] = 7;
					filePositions = 1;
					STRINGLENGTH += 10;
					strcat(bufPtr,"routeInfo=");
					break;
				case 2:  //owner_info
					positionValues[0] = ORIGINPOSITION;
					positionValuesSize[0] = 12;
					positionValues[2] = OWNERPOSITION;
					positionValuesSize[2] = 1;
					filePositions = 3;
					STRINGLENGTH += 10;
					strcat(bufPtr,"ownerInfo=");
					break;	
				default:
					break;
				//error
				}; //end of switch
				while(( fgets(buffer,1000,fp) != NULL) )//fgets transfers the EOL
				{
					_sizeNeeded = strlen(bufPtr) + PARAMETER;
					if(lengthBuf2 < _sizeNeeded )
					{
cout << "buffer not large enough = " << lengthBuf2 << "need" << _sizeNeeded << endl;
						char* tmpBuf = bufPtr;
						char* bufPtr = new char[_sizeNeeded];
						lengthBuf2 = _sizeNeeded;

						memset(bufPtr,0,_sizeNeeded);
						strcpy(bufPtr,tmpBuf);
						delete[] tmpBuf;
					}
					int size =0;
					for(int i=0; i <  filePositions; i++)
					{
						dataPtr = buffer + positionValues[i];
						STRINGLENGTH = strlen(dataPtr);
#ifdef DEBUG
cout << "STRINGLENGTH= "<< STRINGLENGTH << "positionValuesSize = " << positionValuesSize[i] << endl;
#endif
						if (STRINGLENGTH-1 < positionValuesSize[i])
						{
							strncat(bufPtr,dataPtr,STRINGLENGTH-1);
						}
						else
						{
							strncat(bufPtr,dataPtr,positionValuesSize[i]);
						}
					}
					strcat(bufPtr,"\n");
					memset (buffer,0,1000);
				} // end of while loop
				length = strlen(bufPtr);
				bufPtr[length] = 0xFF;
				bufPtr[length+1] = '\0';
				fclose(fp);
			} //end of else 
		} // end of for loop

#ifdef DEBUG
cout << "strlen of buffer2 " << strlen(bufPtr) << endl;
cout << "buffer2 " << bufPtr << endl;
#endif
                        memset(_putTime,0,14);
                        memset(_msgId,0,25);
                        memset(_correlId,0,24);
                        returnCode = sendDataRefQueue->putMsg(_putTime,  _msgId,
                                                              _correlId, bufPtr);
			cout << "returnCode of putMsg " << returnCode << endl;
#ifdef DEBUG
cout << "buffer to send to configQ= " << bufPtr << endl;
#endif
	} // end of try
	catch ( ThrowError err )
	{
		cout << "ERROR=  ";
		err.printMessage() ;
		cout << endl;
		exit(1);
	}
	catch(...)
	{
		cout << "in catch(...) " << endl;
	}
} // end of main

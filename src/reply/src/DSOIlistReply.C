
///////////////////////////////////////////////////////
//  FILE:  DSOIlistReply.C
//
//  PURPOSE: To keep a list of the queues that are opened
//	for replies.  This allows the process to open
//	many queues for replies.  
//  
//  REVISION HISTORY:
//  version	date		autho	description
//-------- -----------------------------------------------
//  1.00.00	06/23/97	lparks	The initial release
// 10.04.00     02/01/07        rfuruta CR10999, migrate DSOI to Solaris
//
//
//		CONFIDENTIAL
//  Disclose and Distribute Solely To Employees Of 
//  U S West and Its Affiliates Having A Need To Know
///////////////////////////////////////////////////////

#include <new>
#include "DSOIlist.H"
#include "DSOIsendMQ.H"


////////////////////////////////////////////////
//  FUNCTION: int List::is_empty()
//
//  PURPOSE: Method to determine if there is 
//	any reply queues in the list.
//
//  METHOD: If the head of list is NULL then there
//	is not any items in the list.
//
//  PARAMETERS: NONE
//  
//  RETURNS: Return zero (success) if the list is empty
//		and one otherwise.
// 
//////////////////////////////////////////////////////

int List::is_empty()
{
	return list == 0 ? 0:1;
}

////////////////////////////////////////////////
//  FUNCTION:void List::insert(MQCHAR* queue, DSOIsendMQ* sendQ)
//
//  PURPOSE: Inserts a reply queue in the list.
//
//  METHOD: Creates a Reply class and extends the list at the end.
//
//  PARAMETERS: MQCHAR* queue - the name of the ReplyToQ
//		DSOIsendQ - the  send to it is connected to
//  
//  OTHER OUTPUTS: A Reply class
//  
//  RETURNS: void
// 
//  OTHER INPUTS: The pointer to the Reply class (list).
//
//////////////////////////////////////////////////////
Reply* List::insert(MQCHAR* queue, DSOIsendMQ* sendQ)
{
#ifdef DEBUG
cout << "in insert " << endl;
#endif
	Reply* replyPtr = new Reply(queue,sendQ);
	if(replyPtr == NULL)
	{
		return NULL;
	}

	if(list == NULL) //head of list
	{
		list = replyPtr;
		return replyPtr;
	}
	else
	{
		Reply* prevPtr = list;
		Reply* listPtr = prevPtr->next;
		while(listPtr != NULL)
		{
			prevPtr = prevPtr->next;
			listPtr = listPtr->next;
		}
		prevPtr->next = replyPtr;
	}
#ifdef DEBUG
cout << "in insert SUCCESS " << endl;
#endif
	return replyPtr;
}


////////////////////////////////////////////////
//  FUNCTION: int List::is_present(char* reply)
//
//  PURPOSE: To determine of a queue has been inserted in the list.
//
//  METHOD: Traverses the list and compares the queue name found
//	in each Reply object.
//
//  PARAMETERS: char* reply - name of ReplyToQ queue
//  
//  RETURNS: Return zero if reply is part of the list,
//	One otherwise.
// 
//////////////////////////////////////////////////////
Reply* List::is_present(char* reply)
{
	if(list == 0)
		return NULL;
	Reply* listPtr = list;
	while(listPtr)
	{
		if(strcmp(reply,listPtr->replyQ) == 0)
			return NULL;
		listPtr = listPtr->next;
	}
	return listPtr;
}

////////////////////////////////////////////////
//  FUNCTION: int List::remove(char* reply)
//
//  PURPOSE: To remove a Reply class object from the list.
//
//  METHOD: Arranges the pointers so that the Reply object
//	is no longer pointed to and deletes the object.
//
//  PARAMETERS: char* reply - the Reply object to be removed.
//  
//  RETURNS: Return zero if Reply object successfully
//	removed, one otherwise.
// 
//////////////////////////////////////////////////////
int List::remove(char* reply)
{
	Reply* listPtr = list;
	while(listPtr!=NULL)
	{
		if(strcmp(reply,listPtr->replyQ)==0)
		{
			Reply* tmpPtr = listPtr->next;
			delete listPtr;
			listPtr = tmpPtr;
			return 0;
		}
		listPtr = listPtr->next;
	}
	return 1;
}

////////////////////////////////////////////////
//  FUNCTION: void List::remove()
//
//  PURPOSE: Deletes all Reply objects from the list.
//
//  METHOD: 
//
//  PARAMETERS:
//  
//  OTHER OUTPUTS:
//  
//  RETURNS:
// 
//  OTHER INPUTS:
//
//////////////////////////////////////////////////////
void List::remove()
{
	Reply* listPtr = list;
	if(listPtr == NULL) //no list
	{
		return;
	}
	else
	{
		Reply* tmpPtr;
		while(listPtr != NULL)
		{
			tmpPtr = listPtr->next;
			//listPtr->~Reply();
			delete listPtr;
			listPtr = tmpPtr;
		}
	}
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
//  OTHER INPUTS:
//
//////////////////////////////////////////////////////
void List::display()
{
	cout << "(" << endl; ;
	for (Reply* replyPtr = list; replyPtr; replyPtr = replyPtr->next)
		cout << "replyQ= " << replyPtr->replyQ  << endl;
	cout << ")" << endl;
}

////////////////////////////////////////////////
//  FUNCTION: Reply::Reply(char* queue)
//
//  PURPOSE: Constructor for the Reply class
//
//  PARAMETERS: The name of the reply queue 
//  
//////////////////////////////////////////////////////
Reply::Reply(char* queue, DSOIsendMQ* sendQ)
{
	strcpy(replyQ,queue);
	sendQueue = sendQ;
	next = 0;

}
////////////////////////////////////////////////
//  FUNCTION: Reply::~Reply(
//
//  PURPOSE: Destructor for the Reply class
//
//  PARAMETERS: 
//  
//////////////////////////////////////////////////////
Reply::~Reply()
{
#ifdef DEBUG
cout << "in Reply destructor" << endl;
#endif
	delete this->sendQueue;
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

Reply::Reply()
{
	strcpy(replyQ,"");
	sendQueue = NULL;
	next = 0;
}

////////////////////////////////////////////////
//  FUNCTION: Reply* List:: get_reply(char* queue)
//
//  PURPOSE: To determine if the list of reply
//	queues has the queue in it and return a 
//	pointer to it if it does.
//
//  METHOD: Traverse the list of Reply objects and
//	comapre the name of the replyQ to the parameter.
//
//  PARAMETERS: char* queue - the name of the queue to
//	reply to.
//  
//  OTHER OUTPUTS:
//  
//  RETURNS: A pointer to the Reply object in the list
//	if found in the list, NULL otherwise.
// 
//  OTHER INPUTS: The pointer to the head of the Reply list(list).
//
//  FUNCTIONS USED: strcmp()
//
//////////////////////////////////////////////////////
Reply* List:: get_reply(char* queue)
{
	Reply* listPtr = list;
	while(listPtr)
	{
		if(strcmp(queue,listPtr->replyQ)==0)
		{
			return listPtr;
		}
		listPtr = listPtr->next;
	}
	return NULL;
}

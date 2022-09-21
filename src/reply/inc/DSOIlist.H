#ifndef DSOIlist_H
#define DSOIlist_H

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

#include <iostream.h>
#include <cmqc.h>

class DSOIsendMQ;

class Reply
{
	public:
		friend class List;
		DSOIsendMQ * sendQueue;
		MQCHAR48 replyQ;

	private:
		Reply(char *, DSOIsendMQ *);
		Reply();
		~Reply();
		Reply * next;
};


class List
{
	public:
		List() { list = NULL; }
		~List() { remove(); }
		Reply * insert(char *, DSOIsendMQ *);
		int remove(char *);
		void remove();
		void display();
		int is_empty();
		Reply * get_reply(char *);
		Reply * is_present(char *);

	private:
		Reply * list;	
};


#endif // DSOIlist_H

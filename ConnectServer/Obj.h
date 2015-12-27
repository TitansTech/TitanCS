#ifndef OBJ_H__
#define OBJ_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winsock2.h>
#include "iocp.h"

#define OBJMAX 1000

struct OBJECTSTRUCT
{
	int m_Index;
	_PER_SOCKET_CONTEXT* PerSocketContext;
	unsigned int m_socket;
	char Ip_addr[16];
	bool Connected;
	int ConnectedTime;
	int SendListCount;
};
typedef OBJECTSTRUCT * LPOBJ;
extern OBJECTSTRUCT Obj[OBJMAX];


short ObjAdd(SOCKET aSocket, char* ip, int aIndex);
short ObjDel(int index);
short ObjAddSearch(SOCKET aSocket, char* ip);
void OBJInit();
void OBJSecondProc();

#endif
#include "stdafx.h"
//#include "iocp.h"
#include "Obj.h"
#include "Log.h"
#include "ServerManager.h"

OBJECTSTRUCT Obj[OBJMAX];
extern void CloseClient(int index);

void OBJSecondProc()
{
	for(int i=0;i<serverManager._MaxConnections;i++)
	{
		if ( Obj[i].Connected == true )
		{
			Obj[i].ConnectedTime++;
			if(Obj[i].ConnectedTime >= serverManager.DisconnectAfter)
			{	
				LogAddC(2,"[AntiFlood] IP: %s was connected for %d seconds",Obj[i].Ip_addr,Obj[i].ConnectedTime);
				CloseClient(i);
			}
		}
	}
}

void OBJInit()
{
	for(int i=0;i<serverManager._MaxConnections;i++)
	{
		Obj[i].Connected = false;
		Obj[i].m_Index = -1;
		Obj[i].m_socket = INVALID_SOCKET;
		Obj[i].PerSocketContext = new _PER_SOCKET_CONTEXT;
		Obj[i].SendListCount=0;
		Obj[i].ConnectedTime=0;
		Obj[i].SendListCount=0;
		memset(Obj[i].Ip_addr,0,sizeof(Obj[i].Ip_addr)-1);
	}
}

short ObjAddSearch(SOCKET aSocket, char* ip)
{
	for(int count=0;count<serverManager._MaxConnections;count++)
	{
		if ( Obj[count].Connected == false )
		{
			return count;
		}
	}
	return -1;
}

short ObjAdd(SOCKET aSocket, char* ip, int aIndex)
{
	if ( Obj[aIndex].Connected != false )
	{
		return -1;
	}
	Obj[aIndex].m_Index = aIndex;
	Obj[aIndex].m_socket = aSocket;
	Obj[aIndex].Connected = true;
	Obj[aIndex].SendListCount=0;
	Obj[aIndex].ConnectedTime=0;
	Obj[aIndex].SendListCount=0;
	strcpy(Obj[aIndex].Ip_addr, ip);
	LogAdd("connect : [%d][%s]", aIndex, ip);

	return aIndex;
}

short ObjDel(int index)
{
	if ( index < 0 || index > serverManager._MaxConnections )
	{
		LogAdd ("(%s)(%d) = index over error (%d)", __FILE__, __LINE__, index);
		return 0;
	}

	LPOBJ lpObj = &Obj[index];

	if ( lpObj->Connected == false )
	{
		return 0;
	}

	memset(lpObj->Ip_addr,0,sizeof(lpObj->Ip_addr)-1);
	lpObj->Connected = false;
	lpObj->SendListCount=0;
	lpObj->ConnectedTime=0;
	lpObj->SendListCount=0;
	return 1;
}
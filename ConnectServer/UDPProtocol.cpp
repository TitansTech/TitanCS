#include "stdafx.h"
#include "WZUDP.h"
#include "ServerManager.h"
#include "UDPProtocol.h"
#include "Protocol.h"

WzUdp gUdpSoc;

struct PMSG_JOINSERVER_STAT
{
	PBMSG_HEAD	h;		
	int	iQueueCount;
};

struct PMSG_NOTIFY_GSSTATUS
{
	PBMSG_HEAD h;
	WORD ServerCode;
	BYTE State;
};


struct PMSG_NOTIFY_GSSTATUS2
{
	PBMSG_HEAD h;
	int Port;
	BYTE State;
};

struct PMSG_NOTIFY_USERS
{
	PBMSG_HEAD h;
	BYTE Unk1;
	WORD ServerCode;
	BYTE Unk2;
	WORD iCurUserCount;	// 8
	BYTE Unk3[3];
	WORD iMaxUserCount;	//	C
};

struct PMSG_NOTIFY_USERS2
{
	PBMSG_HEAD h;
	int Port;
	WORD iCurUserCount;	// 8
	WORD iMaxUserCount;	//	C
};

void UDPInit()
{
	if ( gUdpSoc.CreateSocket() == 0)
	{
		MessageBox(NULL,"UDP Socket create error",0,0);
		return;
	}

	gUdpSoc.RecvSet( serverManager.UDPPort );
	gUdpSoc.Run();
	gUdpSoc.SetProtocolCore(UDPProtocolCore);
}

void UDPProtocolCore(BYTE protoNum, unsigned char* aRecv, int aLen)
{
	switch ( protoNum )
	{
		//case 0x01:
		//	{
		//		PMSG_NOTIFY_USERS * pMsg = (PMSG_NOTIFY_USERS *)aRecv;
		//		serverManager.UpdateServerInfo(pMsg->ServerCode,pMsg->iCurUserCount,pMsg->iMaxUserCount);
		//	}break;
		case 0x02:
			{
				PMSG_JOINSERVER_STAT * pMsg = (PMSG_JOINSERVER_STAT *)aRecv;
			}break;
		case 0x03:
			{
				//PMSG_NOTIFY_GSSTATUS * pMsg = (PMSG_NOTIFY_GSSTATUS *)aRecv;
				//serverManager.UpdateServerVisible(pMsg->ServerCode,pMsg->State);
				PMSG_NOTIFY_GSSTATUS2 * pMsg = (PMSG_NOTIFY_GSSTATUS2 *)aRecv;
				serverManager.UpdateServerVisible2(pMsg->Port,pMsg->State);
			}break;
		case 0x04:
			{
				PMSG_NOTIFY_USERS2 * pMsg = (PMSG_NOTIFY_USERS2 *)aRecv;
				serverManager.UpdateServerInfo2(pMsg->Port,pMsg->iCurUserCount,pMsg->iMaxUserCount);
			}break;
	}
}



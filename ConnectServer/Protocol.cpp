#include "stdafx.h"
//#include "iocp.h"
#include "Protocol.h"
#include "Obj.h"
#include "Log.h"
#include "ServerManager.h"
#include "PacketUtils.h"

void JoinResultSend(int aIndex, BYTE result)
{
	PMSG_JOINRESULT pResult;
	PHeadSubSetB((LPBYTE)&pResult, 00, 0x01, sizeof(pResult));
	DataSend(aIndex, (unsigned char*)&pResult, pResult.h.size);
}

void ProtocolCore(int aIndex, LPBYTE aRecv, int aLen)
{
	BYTE protoNum = 0;
	if((aRecv[0] == 0xC1) || (aRecv[0] == 0xC3))
	{
		protoNum = aRecv[2];
	}else
	{
		protoNum = aRecv[3];
	}

	switch(protoNum)
	{
		case 0x05 :
			CSVersionRecv(aIndex,(PMSG_VERSION *)aRecv);
		break;	

		case 0x04 :		// HTTP
			CSHTTPVersionRecv(aIndex, (PMSG_VERSION *)aRecv);
		break;

		case 0xF4:
		{
			PMSG_DEFAULT2 * lpDef = (PMSG_DEFAULT2 *)aRecv;
			switch(lpDef->subcode)
			{
				case 3: //Connect to Server
				{
					//PMSG_DEFAULT2 encPacket ={0};
					//PHeadSubSetB((LPBYTE)&encPacket, 0xEE, 0x54, sizeof(encPacket));
					//DataSend(aIndex, (unsigned char*)&encPacket, encPacket.h.size);

					BYTE i=aRecv[5];
					BYTE j=aRecv[4];
					PMSG_SENDSERVER pResult;
					PHeadSubSetB((LPBYTE)&pResult, 0xF4, 0x03, sizeof(pResult));
					strcpy(pResult.IpAddress,serverManager.Server[i][j].IpAddress);
					pResult.port = serverManager.Server[i][j].Port;
					DataSend(aIndex, (unsigned char*)&pResult, pResult.h.size);
					LogAddTD("IP: %s is connected to server [%d,%d](%s:%d)",Obj[aIndex].Ip_addr,i,j,serverManager.Server[i][j].IpAddress,serverManager.Server[i][j].Port);
					serverManager.DelAutoAntiFloodIP(Obj[aIndex].Ip_addr);
				}break;
				case 6: //Send Server List
				{
					if(Obj[aIndex].SendListCount>=serverManager.MaxSendListCount)
					{
						LogAddC(2,"[AntiFlood] IP: %s requested serverlist %d times",Obj[aIndex].Ip_addr,Obj[aIndex].SendListCount);
						serverManager.AutoAntiFloodCheck(Obj[aIndex].Ip_addr);
						CloseClient(aIndex);
						return;
					}
					Obj[aIndex].SendListCount++;
					LogAddTD("IP: %s requested serverlist",Obj[aIndex].Ip_addr);
					DataSend(aIndex,serverManager.List,serverManager.ListSize);
				}break;
				default:
				{
					LogAdd("error-L2 : IP:%s HEAD:%x SUBCODE:%x",Obj[aIndex].Ip_addr,protoNum,lpDef->subcode);
					CloseClient(aIndex);
				}
			}
		}break;
		default:
		{
			LogAdd("error-L2 : IP:%s HEAD:%x",Obj[aIndex].Ip_addr,protoNum);
			CloseClient(aIndex);
		}break;
	}
}

void CFileServerInfoMake()
{
	PMSG_FILESERVERINFO	pMsg={0};
	PHeadSetB((LPBYTE)&pMsg, 1, sizeof(pMsg));

	pMsg.Version[0] = serverManager.ClientV2;
	pMsg.Version[1] = serverManager.ClientV3;
	//pMsg.Version[2] = serverManager.ClientV3;

	// FTP
	strcpy( pMsg.IpAddress,	serverManager.FTP.Address );
	strcpy( pMsg.FtpId,		serverManager.FTP.ID );
	strcpy( pMsg.FtpPass,	serverManager.FTP.Password );\
	strcpy( pMsg.Folder,	serverManager.FTP.Folder );
	
	pMsg.Port = serverManager.FTP.Port;

	BuxConvert(pMsg.IpAddress, 100);
	BuxConvert(pMsg.FtpId,   20);
	BuxConvert(pMsg.FtpPass, 20);
	BuxConvert(pMsg.Folder,  20);

	memcpy( serverManager.szFileServerInfoSendBuffer, (char*)&pMsg, sizeof(pMsg) );
	serverManager.nFileServerInfoBufferLen = sizeof( pMsg );
	
	// HTTP
	pMsg.h.headcode = 0x04;
	strcpy( pMsg.IpAddress,	serverManager.http.Address );
	strcpy( pMsg.FtpId,		serverManager.http.ID );
	strcpy( pMsg.FtpPass,	serverManager.http.Password );
	strcpy( pMsg.Folder,	serverManager.http.Folder );	
	pMsg.Port = serverManager.http.Port;

	BuxConvert(pMsg.IpAddress, 100);
	BuxConvert(pMsg.FtpId,   20);
	BuxConvert(pMsg.FtpPass, 20);
	BuxConvert(pMsg.Folder,  20);

	memcpy( serverManager.szHTTPFileServerInfoSendBuffer, (char*)&pMsg, sizeof(pMsg) );
	serverManager.nHTTPFileServerInfoBufferLen = sizeof( pMsg );
}

void SCNoUpdateSend(int aIndex)
{
	PMSG_DEFRESULT	pMsg={0};	
	PHeadSetB((LPBYTE)&pMsg, 2, sizeof(pMsg));

	pMsg.result     = 0;

	DataSend(aIndex, (LPBYTE)&pMsg, pMsg.h.size);
	ObjDel(aIndex);
}

void SCFileServerInfoSend(int aIndex)
{	
	if( serverManager.nFileServerInfoBufferLen == 0 ) 
	{		
		ObjDel(aIndex);
		return;
	}

	DataSend(aIndex, (LPBYTE)serverManager.szFileServerInfoSendBuffer, serverManager.nFileServerInfoBufferLen);
	ObjDel(aIndex);
}

void CSVersionRecv(int aIndex, PMSG_VERSION * lpMsg)
{	
	if( ((lpMsg->Version[0]*256)+lpMsg->Version[1]+lpMsg->Version[2]) != ((serverManager.ClientV1*256)+serverManager.ClientV2+serverManager.ClientV3))
	{
		SCFileServerInfoSend(aIndex);
	}
	else
	{
		SCNoUpdateSend(aIndex);
	}
}

void SCHTTPFileServerInfoSend(int aIndex)
{	
	if( serverManager.nHTTPFileServerInfoBufferLen == 0 ) 
	{
		ObjDel(aIndex);
		return;
	}

	DataSend(aIndex, (LPBYTE)serverManager.szHTTPFileServerInfoSendBuffer, serverManager.nHTTPFileServerInfoBufferLen);
}

void CSHTTPVersionRecv(int aIndex, PMSG_VERSION * lpMsg)
{	
	if( ((lpMsg->Version[0]*256)+lpMsg->Version[1]+lpMsg->Version[2]) != ((serverManager.ClientV1*256)+serverManager.ClientV2+serverManager.ClientV3))
	{
		SCHTTPFileServerInfoSend(aIndex);
	}
	else
	{
		SCNoUpdateSend(aIndex);
	}
}
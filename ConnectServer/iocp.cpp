#include "stdafx.h"
#include "iocp.h"
#include "ConnectServer.h"
#include "Protocol.h"
#include "ServerManager.h"
#include "Obj.h"
#include "Log.h"

HANDLE g_CompletionPort;
DWORD g_dwThreadCount;
enum SOCKET_FLAG;
CRITICAL_SECTION criti;
HANDLE g_ThreadHandles[MAX_IO_THREAD_HANDLES];
int g_ServerPort;
HANDLE g_IocpThreadHandle;


SOCKET g_Listen = INVALID_SOCKET;	// THIS IS NOT THE PLACE OF TTHIS VARIABLE

void iocpInit()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
	InitializeCriticalSection(&criti);
	CreateIocp(serverManager.Port);
	Sleep(100);
}

BOOL CreateIocp(int server_port)
{
	unsigned long ThreadID;
	
	g_ServerPort=server_port;

	g_IocpThreadHandle=CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)IocpServerWorker, NULL, 0, &ThreadID);

	if ( g_IocpThreadHandle == 0 )
	{
		LogAdd("CreateThread() failed with error %d", GetLastError());
		return 0;
	}
	else
	{
		return 1;
	}	
}

void DestroyIocp()
{
	return;
}


int CreateListenSocket()
{
	sockaddr_in InternetAddr;
	int nRet;

	g_Listen=WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if ( g_Listen == -1 )
	{
		LogAdd("WSASocket() failed with error %d", WSAGetLastError() );
		return 0;
	}
	else
	{
		InternetAddr.sin_family=AF_INET;
		InternetAddr.sin_addr.S_un.S_addr=htonl(0);
		InternetAddr.sin_port=htons(g_ServerPort);
		nRet=bind(g_Listen, (sockaddr*)&InternetAddr, 16);
		
		if ( nRet == -1 )
		{
			LogAddC(2,"bind() failed with error %d", WSAGetLastError());
			MsgBox("error-L3 : cant bind port: %d", g_ServerPort);
			exit(1);
			return 0 ;
		}
		else
		{
			nRet=listen(g_Listen, 5);
			if (nRet == -1)
			{
				LogAddC(2,"listen() failed with error %d", WSAGetLastError());
				MsgBox("error-L3 : listen() failed with error %d", WSAGetLastError());
				exit(1);
				return 0;
			}
			else
			{				
				LogAddC(4,"Server Listen OK. Port: %d",g_ServerPort);
				return 1;
			}
		}
	} 
}


unsigned long __stdcall IocpServerWorker(void * p)
{
	SYSTEM_INFO SystemInfo;
	DWORD ThreadID=0;
	SOCKET Accept;
	int nRet;
	int ClientIndex;
	sockaddr_in cAddr;
	in_addr cInAddr;
	int cAddrlen;
	_PER_SOCKET_CONTEXT * lpPerSocketContext;
	int RecvBytes;
	unsigned long Flags;

	cAddrlen=16;
	lpPerSocketContext=0;
	Flags=0;

	InitializeCriticalSection(&criti);
	GetSystemInfo(&SystemInfo);

	g_dwThreadCount = SystemInfo.dwNumberOfProcessors * 2;
	__try
	{

		g_CompletionPort=CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

		if ( g_CompletionPort == NULL )
		{
			LogAdd("CreateIoCompletionPort failed with error: %d", GetLastError());
			__leave;
		}

		for ( DWORD n = 0; n<g_dwThreadCount; n++ )
		{
			

			HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ServerWorkerThread, g_CompletionPort, 0, &ThreadID);

			if ( hThread == 0 )
			{
				LogAdd("CreateThread() failed with error %d", GetLastError() );
				__leave;
			}

			g_ThreadHandles[n] = hThread;

			CloseHandle(hThread);
		}

		if ( CreateListenSocket() == 0 )
		{

		}
		else
		{
			while ( true )
			{
				Accept = WSAAccept(g_Listen, (sockaddr*)&cAddr, &cAddrlen, NULL, 0 );

				if ( Accept == -1 )
				{
					EnterCriticalSection(&criti);
					LogAdd("WSAAccept() failed with error %d", WSAGetLastError() );
					LeaveCriticalSection(&criti);
					continue;
				}

				EnterCriticalSection(&criti);
				memcpy(&cInAddr, &cAddr.sin_addr  , sizeof(cInAddr) );

				ClientIndex = ObjAddSearch(Accept, inet_ntoa(cInAddr) );

				if(serverManager.IPCheck(inet_ntoa(cInAddr)) == false)
				{
					LogAddC(2,"[BlackList] Rejected IP %s",inet_ntoa(cInAddr));
					closesocket(Accept);
					LeaveCriticalSection(&criti);
					continue;
				}

				if ( ClientIndex == -1 )
				{
					LogAdd("error-L2 : ClientIndex = -1");
					closesocket(Accept);
					LeaveCriticalSection(&criti);
					continue;
				}

				if (UpdateCompletionPort(Accept, ClientIndex, 1) == 0 )
				{
					LogAdd("error-L1 : %d %d CreateIoCompletionPort failed with error %d", Accept, ClientIndex, GetLastError() );
					closesocket(Accept);
					LeaveCriticalSection(&criti);
					continue;
				}

				if (ObjAdd(Accept, inet_ntoa(cInAddr), ClientIndex) == -1 )
				{
					LogAdd("error-L1 : %d %d ObjAdd() failed with error %d", Accept, ClientIndex, GetLastError() );
					LeaveCriticalSection(&criti);
					closesocket(Accept);
					continue;
				}
				
				memset(&Obj[ClientIndex].PerSocketContext->IOContext[0].Overlapped, 0, sizeof(WSAOVERLAPPED));
				memset(&Obj[ClientIndex].PerSocketContext->IOContext[1].Overlapped, 0, sizeof(WSAOVERLAPPED));

				Obj[ClientIndex].PerSocketContext->IOContext[0].wsabuf.buf = (char*)&Obj[ClientIndex].PerSocketContext->IOContext[0].Buffer;
				Obj[ClientIndex].PerSocketContext->IOContext[0].wsabuf.len = MAX_IO_BUFFER_SIZE;
				Obj[ClientIndex].PerSocketContext->IOContext[0].nTotalBytes = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[0].nSentBytes = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[0].nWaitIO = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[0].nSecondOfs = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[0].IOOperation = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[1].wsabuf.buf = (char*)Obj[ClientIndex].PerSocketContext->IOContext[0].Buffer;
				Obj[ClientIndex].PerSocketContext->IOContext[1].wsabuf.len = MAX_IO_BUFFER_SIZE;
				Obj[ClientIndex].PerSocketContext->IOContext[1].nTotalBytes = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[1].nSentBytes = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[1].nWaitIO = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[1].nSecondOfs = 0;
				Obj[ClientIndex].PerSocketContext->IOContext[1].IOOperation = 1;
				Obj[ClientIndex].PerSocketContext->m_socket = Accept;
				Obj[ClientIndex].PerSocketContext->nIndex = ClientIndex;

				nRet = WSARecv(Accept, &Obj[ClientIndex].PerSocketContext->IOContext[0].wsabuf , 1, (unsigned long*)&RecvBytes, &Flags, 
						&Obj[ClientIndex].PerSocketContext->IOContext[0].Overlapped, NULL);

				if ( nRet == -1 )
				{
					if ( WSAGetLastError() != WSA_IO_PENDING )
					{
						LogAdd("error-L1 : WSARecv() failed with error %d", WSAGetLastError() );
						Obj[ClientIndex].PerSocketContext->IOContext[0].nWaitIO = 4;
						CloseClient(Obj[ClientIndex].PerSocketContext, 0);
						LeaveCriticalSection(&criti);
						continue;
					}
				}

				Obj[ClientIndex].PerSocketContext->IOContext[0].nWaitIO  = 1;
				Obj[ClientIndex].PerSocketContext->dwIOCount++;

				LeaveCriticalSection(&criti);
				JoinResultSend(ClientIndex, 1);
			}
		}
	}
	__finally
	{
		
		if ( g_CompletionPort != NULL )
		{
			for ( int i = 0 ; i < g_dwThreadCount ; i++ )
			{
				PostQueuedCompletionStatus( g_CompletionPort , 0, 0, 0);
			}
		}

		if ( g_CompletionPort != NULL )
		{
			CloseHandle(g_CompletionPort);
			g_CompletionPort = NULL;
		}
		if ( g_Listen != INVALID_SOCKET )
		{
			closesocket( g_Listen);
			g_Listen = INVALID_SOCKET;
		}
	}

	return 1;
		
}


unsigned long __stdcall ServerWorkerThread(HANDLE CompletionPortID)
{
	HANDLE CompletionPort;
	DWORD dwIoSize=0;
	unsigned long RecvBytes;
	unsigned long Flags;
	DWORD dwSendNumBytes;
	BOOL bSuccess;
	int nRet;

#ifdef _WIN64
	ULONG_PTR ClientIndex=0;
#else
	DWORD ClientIndex=0;
#endif

	_PER_SOCKET_CONTEXT * lpPerSocketContext;
	LPOVERLAPPED lpOverlapped;
	_PER_IO_CONTEXT * lpIOContext;
	
	

	CompletionPort=CompletionPortID;
	dwSendNumBytes=0;
	bSuccess=0;
	lpPerSocketContext=0;
	lpOverlapped=0;
	lpIOContext=0;
	
	while ( true )
	{
		bSuccess=GetQueuedCompletionStatus( CompletionPort, &dwIoSize, &ClientIndex, &lpOverlapped, -1); // WAIT_FOREVER

		if (bSuccess == 0)
		{
			if (lpOverlapped != 0)
			{
				int aError = GetLastError();
				//Win2k3 SP2 Fix
				//#define ERROR_SEM_TIMEOUT                121L
				//The semaphore timeout period has expired.
				if ( (aError != ERROR_SEM_TIMEOUT) && (aError != ERROR_NETNAME_DELETED) && (aError != ERROR_CONNECTION_ABORTED) && (aError != ERROR_OPERATION_ABORTED) )
				{
					EnterCriticalSection(&criti);
					LogAdd("Error Thread : GetQueueCompletionStatus( %d )", GetLastError());
					LeaveCriticalSection(&criti);
					return 0;
				}
				//LogAdd("[TRACE] Error Thread : GetQueueCompletionStatus( %d )", GetLastError());
			}
		}

		EnterCriticalSection(&criti);

		lpPerSocketContext=Obj[ClientIndex].PerSocketContext;
		lpPerSocketContext->dwIOCount --;
				
		if ( dwIoSize == 0 )
		{
			LogAdd("Connection Closed, dwIoSize == 0 (Index:%d)", lpPerSocketContext->nIndex);
			CloseClient(lpPerSocketContext, 0);
			LeaveCriticalSection(&criti);
			continue;
		}

		lpIOContext = (_PER_IO_CONTEXT *)lpOverlapped;

		if ( lpIOContext == 0 )
		{
			continue;
		}

		if ( lpIOContext->IOOperation == 1 )
		{
			lpIOContext->nSentBytes += dwIoSize;


			if ( lpIOContext->nSentBytes >= lpIOContext->nTotalBytes )
			{
				lpIOContext->nWaitIO = 0;
						
				if ( lpIOContext->nSecondOfs > 0)
				{
					IoSendSecond(lpPerSocketContext);
				}
			}
			else
			{
				IoMoreSend(lpPerSocketContext);
			}
			
		}
		else if ( lpIOContext->IOOperation == 0 )
		{
			RecvBytes = 0;
			lpIOContext->nSentBytes += dwIoSize;

			if ( RecvDataParse(lpIOContext, lpPerSocketContext->nIndex ) == 0 )
			{
				LogAdd("error-L1 : Socket Header error %d, %d", WSAGetLastError(), lpPerSocketContext->nIndex);
				CloseClient(lpPerSocketContext, 0);
				LeaveCriticalSection(&criti);
				continue;
			}

			lpIOContext->nWaitIO = 0;
			Flags = 0;
			memset(&lpIOContext->Overlapped, 0, sizeof (WSAOVERLAPPED));
			lpIOContext->wsabuf.len = MAX_IO_BUFFER_SIZE - lpIOContext->nSentBytes;
			lpIOContext->wsabuf.buf = (char*)&lpIOContext->Buffer[lpIOContext->nSentBytes];
			lpIOContext->IOOperation = 0;

			nRet = WSARecv(lpPerSocketContext->m_socket, &lpIOContext->wsabuf, 1, &RecvBytes, &Flags,
						&lpIOContext->Overlapped, NULL);

			if ( nRet == -1 )
			{
				if ( WSAGetLastError() != WSA_IO_PENDING)
				{
					LogAdd("WSARecv() failed with error %d", WSAGetLastError() );
					CloseClient(lpPerSocketContext, 0);
					LeaveCriticalSection(&criti);
					continue;
				}
			}

			lpPerSocketContext->dwIOCount ++;
			lpIOContext->nWaitIO = 1;
		}
		LeaveCriticalSection(&criti);
	}
	return 1;
}



BOOL RecvDataParse(_PER_IO_CONTEXT * lpIOContext, int uIndex)	
{
	unsigned char* recvbuf;
	int lOfs;
	WORD size;
	WORD headcode;

	// Check If Recv Data has More thatn 3 BYTES
	if ( lpIOContext->nSentBytes < 3 )
	{
		return TRUE;
	}

	// Initialize Variables
	lOfs=0;
	size=0;
	recvbuf = lpIOContext->Buffer;
	
	// Start Loop
	while ( true )
	{
				// Select packets with
		// C1 or C2 as HEader
		if ( recvbuf[lOfs] == 0xC1 ||
			 recvbuf[lOfs] == 0xC3 )
		{
			size = recvbuf[lOfs+1];
			if(serverManager.Encrypt != 0)
				if(recvbuf[lOfs+2] != 0x04 && recvbuf[lOfs+2] != 0x05)
					recvbuf[lOfs+2] ^= serverManager.Encrypt;
			headcode = recvbuf[lOfs+2];
		}
		else if ( recvbuf[lOfs] == 0xC2 ||
			      recvbuf[lOfs] == 0xC4 )
		{
			size = recvbuf[lOfs+1] * 256;
			size |= recvbuf[lOfs+2];
			if(serverManager.Encrypt != 0)
				if(recvbuf[lOfs+3] != 0x04 && recvbuf[lOfs+3] != 0x05)
					recvbuf[lOfs+3] ^= serverManager.Encrypt;
			headcode = recvbuf[lOfs+3];
		}
		// If HEader Differs - Second Generation Protocols
		else
		{
			LogAdd("error-L1 : header error %d",recvbuf[lOfs]);
			return false;
		}

		// Check Size is leess thant 0
		if ( size <= 0 )
		{
			LogAdd("error-L1 : size %d",size);
			return false;
		}

		// Check if Size is On Range
		if ( size <= lpIOContext->nSentBytes )
		{
			ProtocolCore(uIndex, &recvbuf[lOfs], size);

			lOfs += size;
			lpIOContext->nSentBytes  -= size;

			if ( lpIOContext->nSentBytes <= 0 )
			{
				break;
			}
		}
		else if ( lOfs > 0 )
		{
			if ( lpIOContext->nSentBytes < 1 )
			{
				LogAdd("error-L1 : recvbuflen 1 %s %d", __FILE__, __LINE__);
				break;
			}

			if ( lpIOContext->nSentBytes < MAX_IO_BUFFER_SIZE ) 
			{
				memcpy(recvbuf, &recvbuf[lOfs], lpIOContext->nSentBytes);
				LogAdd("Message copy %d", lpIOContext->nSentBytes);
			}
			break;
		
		}
		else
		{
			break;
		}
		
	}
		/*
		memcpy(&size,&recvbuf[0],sizeof(size));
		memcpy(&headcode,&recvbuf[2],sizeof(headcode));
		size += 6;

		// Check Size is leess thant 0
		if ( size <= 0 && headcode)
		{
			LogAdd("error-L1 : size %d",
				size);

			return false;
		}
		// Check if Size is On Range
		if ( size <= lpIOContext->nSentBytes )
		{
			ProtocolCore(uIndex, recvbuf, size);
			
			lOfs += size;
			lpIOContext->nSentBytes  -= size;

			if ( lpIOContext->nSentBytes <= 0 )
			{
				break;
			}
		}
		else if ( lOfs > 0 )
		{
			if ( lpIOContext->nSentBytes < 1 )
			{
				LogAdd("error-L1 : recvbuflen 1 %s %d", __FILE__, __LINE__);
				break;
			}

			if ( lpIOContext->nSentBytes < MAX_IO_BUFFER_SIZE ) 
			{
				memcpy(recvbuf, &recvbuf[lOfs], lpIOContext->nSentBytes);
				LogAdd("Message copy %d", lpIOContext->nSentBytes);
				//break;
			}
			break;
		
		}
		else
		{
			break;
		}
		
	}*/

	return true;
}

BOOL DataSend(int aIndex, unsigned char* lpMsg, DWORD dwSize)
{
	int i = 0;
	unsigned char changedProtocol = 0;
	int aLen = sizeof(lpMsg);
	unsigned long SendBytes;
	_PER_SOCKET_CONTEXT * lpPerSocketContext;
	unsigned char * SendBuf;
	EnterCriticalSection(&criti);

	if ( ((aIndex < 0)? FALSE : (aIndex > serverManager._MaxConnections-1)? FALSE : TRUE )  == FALSE )
	{
		LogAdd("error-L2 : Index(%d) %x %x %x ", dwSize, lpMsg[0], lpMsg[1], lpMsg[2]);
		LeaveCriticalSection(&criti);
		return false;
	}

	SendBuf = lpMsg;

	if ( Obj[aIndex].Connected == false )
	{
		LeaveCriticalSection(&criti);
		return FALSE;
	}

	lpPerSocketContext= Obj[aIndex].PerSocketContext;

	if ( dwSize > sizeof(lpPerSocketContext->IOContext[0].Buffer))
	{
		LogAdd("Error : Max msg(%d) %s %d", dwSize, __FILE__, __LINE__);
		CloseClient(aIndex);
		LeaveCriticalSection(&criti);
		return false;
	}

	_PER_IO_CONTEXT  * lpIoCtxt;

	lpIoCtxt = &lpPerSocketContext->IOContext[1];

	if ( lpIoCtxt->nWaitIO > 0 )
	{
		if ( ( lpIoCtxt->nSecondOfs + dwSize ) > MAX_IO_BUFFER_SIZE-1 )
		{
			LogAdd("(%d)error-L2 MAX BUFFER OVER %d %d %d", aIndex, lpIoCtxt->nTotalBytes, lpIoCtxt->nSecondOfs, dwSize);
			lpIoCtxt->nWaitIO = 0;
			CloseClient(aIndex);
			LeaveCriticalSection(&criti);
			return true;
		}

		memcpy( &lpIoCtxt->BufferSecond[lpIoCtxt->nSecondOfs], SendBuf, dwSize);
		lpIoCtxt->nSecondOfs += dwSize;
		LeaveCriticalSection(&criti);
		return true;
	}

	lpIoCtxt->nTotalBytes = 0;
	
	if ( lpIoCtxt->nSecondOfs > 0 )
	{
		memcpy(lpIoCtxt->Buffer, lpIoCtxt->BufferSecond, lpIoCtxt->nSecondOfs);
		lpIoCtxt->nTotalBytes = lpIoCtxt->nSecondOfs;
		lpIoCtxt->nSecondOfs = 0;
	}

	if ( (lpIoCtxt->nTotalBytes+dwSize) > MAX_IO_BUFFER_SIZE-1 )
	{
		LogAdd("(%d)error-L2 MAX BUFFER OVER %d %d", aIndex, lpIoCtxt->nTotalBytes, dwSize);
		lpIoCtxt->nWaitIO = 0;
		CloseClient(aIndex);
		LeaveCriticalSection(&criti);
		return FALSE;
	}

	memcpy( &lpIoCtxt->Buffer[lpIoCtxt->nTotalBytes], SendBuf, dwSize);
	lpIoCtxt->nTotalBytes += dwSize;
	lpIoCtxt->wsabuf.buf = (char*)&lpIoCtxt->Buffer;
	lpIoCtxt->wsabuf.len = lpIoCtxt->nTotalBytes;
	lpIoCtxt->nSentBytes = 0;
	lpIoCtxt->IOOperation = 1;
	

	if ( WSASend( Obj[aIndex].m_socket, &lpIoCtxt->wsabuf , 1, &SendBytes, 0, &lpIoCtxt->Overlapped, NULL) == -1 )
	{

		if ( WSAGetLastError() != WSA_IO_PENDING )	
		{
			lpIoCtxt->nWaitIO = 0;
			

			//if ( lpIoCtxt->wsabuf.buf[0] == 0xC1 )
			//{
			//	LogAdd("(%d)WSASend(%d) failed with error [%x][%x] %d %s ", __LINE__, aIndex, (BYTE)lpIoCtxt->wsabuf.buf[0],
			//		(BYTE)lpIoCtxt->wsabuf.buf[2], WSAGetLastError(), Obj[aIndex].Ip_addr);
			//}
			//else if ( lpIoCtxt->wsabuf.buf[0] == 0xC2 )
			//{
			//	LogAdd("(%d)WSASend(%d) failed with error [%x][%x] %d %s ", __LINE__, aIndex, (BYTE)lpIoCtxt->wsabuf.buf[0],
			//		(BYTE)lpIoCtxt->wsabuf.buf[3], WSAGetLastError(), Obj[aIndex].Ip_addr);
			//}
			CloseClient(aIndex);
			LeaveCriticalSection(&criti);
			return false;
		}
	}
	else
	{
		lpPerSocketContext->dwIOCount ++;
	}
	
	
	lpIoCtxt->nWaitIO = 1;
	LeaveCriticalSection(&criti);
	return true;
}





BOOL IoSendSecond(_PER_SOCKET_CONTEXT * lpPerSocketContext)
{
	unsigned long SendBytes;
	int aIndex;
	_PER_IO_CONTEXT * lpIoCtxt;

	EnterCriticalSection(&criti);
	aIndex = lpPerSocketContext->nIndex;
	lpIoCtxt = &lpPerSocketContext->IOContext[1];

	if ( lpIoCtxt->nWaitIO > 0 )
	{
		LeaveCriticalSection(&criti);
		return false;
	}

	lpIoCtxt->nTotalBytes = 0;
	if ( lpIoCtxt->nSecondOfs > 0 )
	{
		memcpy(lpIoCtxt->Buffer, lpIoCtxt->BufferSecond, lpIoCtxt->nSecondOfs);
		lpIoCtxt->nTotalBytes = lpIoCtxt->nSecondOfs;
		lpIoCtxt->nSecondOfs = 0;
	}
	else
	{
		LeaveCriticalSection(&criti);
		return false;
	}

	lpIoCtxt->wsabuf.buf = (char*)&lpIoCtxt->Buffer;
	lpIoCtxt->wsabuf.len = lpIoCtxt->nTotalBytes;
	lpIoCtxt->nSentBytes = 0;
	lpIoCtxt->IOOperation = 1;

	if ( WSASend(Obj[aIndex].m_socket, &lpIoCtxt->wsabuf, 1, &SendBytes, 0, &lpIoCtxt->Overlapped, NULL) == -1 )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			LogAdd("WSASend(%d) failed with error %d %s ", __LINE__, WSAGetLastError(), Obj[aIndex].Ip_addr);
			CloseClient(aIndex);
			LeaveCriticalSection(&criti);
			return false;
		}
	}
	else
	{
		lpPerSocketContext->dwIOCount ++;
	}
	
	lpIoCtxt->nWaitIO = 1;
	LeaveCriticalSection(&criti);
	
	return true;
}


BOOL IoMoreSend(_PER_SOCKET_CONTEXT * lpPerSocketContext)
{
	unsigned long SendBytes;
	int aIndex;
	_PER_IO_CONTEXT * lpIoCtxt;

	EnterCriticalSection(&criti);
	aIndex = lpPerSocketContext->nIndex;
	lpIoCtxt = &lpPerSocketContext->IOContext[1];

	if ( (lpIoCtxt->nTotalBytes - lpIoCtxt->nSentBytes) < 0 )
	{
		LeaveCriticalSection(&criti);
		return false;
	}

	lpIoCtxt->wsabuf.buf = (char*)&lpIoCtxt->Buffer[lpIoCtxt->nSentBytes];
	lpIoCtxt->wsabuf.len = lpIoCtxt->nTotalBytes - lpIoCtxt->nSentBytes;
	lpIoCtxt->IOOperation = 1;

	if ( WSASend(Obj[aIndex].m_socket, &lpIoCtxt->wsabuf, 1, &SendBytes, 0, &lpIoCtxt->Overlapped, NULL) == -1 )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			LogAdd("WSASend(%d) failed with error %d %s ", __LINE__, WSAGetLastError(), Obj[aIndex].Ip_addr);
			CloseClient(aIndex);
			LeaveCriticalSection(&criti);
			return false;
		}
	}
	else
	{
		lpPerSocketContext->dwIOCount ++;
	}
	
	
	lpIoCtxt->nWaitIO = 1;
	LeaveCriticalSection(&criti);
	return true;
}


BOOL UpdateCompletionPort(SOCKET sd, int ClientIndex, BOOL bAddToList)
{
	_PER_SOCKET_CONTEXT * lpPerSocketContext = NULL;

	HANDLE cp = CreateIoCompletionPort((HANDLE) sd, g_CompletionPort, ClientIndex, 0);

	if ( cp == 0 )
	{
		LogAdd("CreateIoCompletionPort: %d", GetLastError() );
		return FALSE;
	}

	Obj[ClientIndex].PerSocketContext->dwIOCount = 0;
	return TRUE;
}


void CloseClient(_PER_SOCKET_CONTEXT * lpPerSocketContext, int result)
{
	int index = -1;
	index = lpPerSocketContext->nIndex ;

	if ( index >= 0 && index < serverManager._MaxConnections )
	{
		if ( Obj[index].m_socket != INVALID_SOCKET )
		{
			if (closesocket(Obj[index].m_socket) == -1 )
			{
				if ( WSAGetLastError() != WSAENOTSOCK )
				{
					return;
				}
			}

			Obj[index].m_socket = INVALID_SOCKET;
		}

		ObjDel(index);
	}
}



void CloseClient(int index)
{
	if ( index < 0 || index > serverManager._MaxConnections-1 )
	{
		LogAdd("error-L1 : CloseClient index error");
		return;
	}

	if ( Obj[index].Connected == false )
	{
		LogAdd("error-L1 : CloseClient connect error");
		return;
	}

	EnterCriticalSection(&criti);

	if ( Obj[index].m_socket != INVALID_SOCKET )
	{
		closesocket(Obj[index].m_socket );
		Obj[index].m_socket = INVALID_SOCKET;
	}
	else
	{
		LogAdd("error-L1 : CloseClient INVALID_SOCKET");
	}

	LeaveCriticalSection(&criti);
}

void ResponErrorCloseClient(int index)
{
	if ( index < 0 || index > serverManager._MaxConnections-1 )
	{
		LogAdd("error-L1 : CloseClient index error");
		return;
	}

	if ( Obj[index].Connected == false )
	{
		LogAdd("error-L1 : CloseClient connect error");
		return;
	}

	EnterCriticalSection(&criti);
	closesocket(Obj[index].m_socket);
	Obj[index].m_socket = INVALID_SOCKET;

	if ( Obj[index].m_socket == INVALID_SOCKET )
	{
		LogAdd("error-L1 : CloseClient INVALID_SOCKET");
	}

	ObjDel(index);
	LeaveCriticalSection(&criti);
}

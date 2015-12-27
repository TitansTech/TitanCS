#include "stdafx.h"
//#include "iocp.h"
#include "Protocol.h"
#include "ServerManager.h"
#include "PacketUtils.h"
#include "Obj.h"
#include "Log.h"

ServerManager serverManager;

void ServerManager::Init()
{
	this->Port = GetPrivateProfileInt("General", "SCFTCPPort",44405,CONFIGFILE ) ;
	this->UDPPort = GetPrivateProfileInt("General", "SCFUDPPort",55557,CONFIGFILE ) ;
	this->_MaxConnections = GetPrivateProfileInt("General", "SCFMaxConnections",300,CONFIGFILE ) ;
	this->ShowAllServers = GetPrivateProfileInt("General", "SCFShowAllServers",1,CONFIGFILE ) ;
	this->Encrypt = GetPrivateProfileInt("General", "SCFEncryptKey",0,CONFIGFILE ) ;
	this->DisconnectAfter = GetPrivateProfileInt("AntiFlood", "SCFDisconnectAfter",60,CONFIGFILE ) ;
	this->MaxSendListCount = GetPrivateProfileInt("AntiFlood", "SCFMaxSendListCount",10,CONFIGFILE ) ;
	this->AutoBLCount = GetPrivateProfileInt("AntiFlood", "SCFBlackListAutoLearnAfterFloodCount",2,CONFIGFILE ) ;
	this->ReadList(SLISTFILE);
	this->ReadBlackList(BLISTFILE);
	this->CleanAutoAntiFloodList();
	this->ReadUpdateConfig();
	LogAddC(3, "[Configs] Loaded Successfull!");
}

void ServerManager::ReadUpdateConfig()
{
	char *cvstr;
	char ClientVersion[20]={0};
	this->ClientV1 = 0;
	this->ClientV2 = 0;
	this->ClientV3 = 0;

	this->nHTTPFileServerInfoBufferLen = 0;
	this->nFileServerInfoBufferLen = 0;
	memset(&this->szFileServerInfoSendBuffer[0],0,sizeof(this->szFileServerInfoSendBuffer));
	memset(&this->szHTTPFileServerInfoSendBuffer[0],0,sizeof(this->szHTTPFileServerInfoSendBuffer));

	GetPrivateProfileString("FtpServerInfo", "SCFAddress", "none", this->FTP.Address, 200, CONFIGFILE);
	this->FTP.Port = GetPrivateProfileInt("FtpServerInfo", "SCFPort", 21, CONFIGFILE);
	GetPrivateProfileString("FtpServerInfo", "SCFID", "", this->FTP.ID, 20, CONFIGFILE);
	GetPrivateProfileString("FtpServerInfo", "SCFPASS", "", this->FTP.Password, 20, CONFIGFILE);
	GetPrivateProfileString("GameServerInfo", "SCFVersionFileName", "version.wvd", this->FTP.Folder, 20, CONFIGFILE);

	GetPrivateProfileString("HttpServerInfo", "SCFHTTPAddress", "none", this->http.Address, 200, CONFIGFILE);
	this->http.Port = GetPrivateProfileInt("HttpServerInfo", "SCFPort", 21, CONFIGFILE);
	GetPrivateProfileString("HttpServerInfo", "SCFID", "", this->http.ID, 20, CONFIGFILE);
	GetPrivateProfileString("HttpServerInfo", "SCFPASS", "", this->http.Password, 20, CONFIGFILE);
	GetPrivateProfileString("GameServerInfo", "SCFVersionFileName", "version.wvd", this->http.Folder, 20, CONFIGFILE);

	GetPrivateProfileString("GameServerInfo", "SCFClientVersion", "00.00", ClientVersion, 10, CONFIGFILE);
	
	cvstr = strtok(ClientVersion, ".");
	this->ClientV1 = atoi(cvstr);
	cvstr = strtok(NULL, ".");
	this->ClientV2 = atoi(cvstr);
	cvstr = strtok(NULL, ".");
	this->ClientV3 = atoi(cvstr);

	CFileServerInfoMake();

	LogAddC(3, "[UpdateConfig] Loaded Successfull!");
}

void ServerManager::ListClean()
{
	for(int i=0;i<=MAXSERVERINDEX;i++)
	{
		for(int j=0;j<=MAXSERVERSUBINDEX;j++)
		{
			this->Server[i][j].Port = 0;
			this->Server[i][j].Show = FALSE;
			this->Server[i][j].State = FALSE;
			this->Server[i][j].ServerMax = 100;
			memset(this->Server[i][j].IpAddress,0,sizeof(this->Server[i][j].IpAddress)-1);
		}
	}
}


bool ServerManager::SearchServer(int Port, int & Index, int & SubIndex)
{
	Index = 0;
	SubIndex = 0;

	for(int i=0;i<=MAXSERVERINDEX;i++)
	{
		for(int j=0;j<=MAXSERVERSUBINDEX;j++)
		{
			if(this->Server[i][j].Port == Port)
			{
				Index = i;
				SubIndex = j;
				return true;
			}
		}
	}
	return false;
}


void ServerManager::BlackListClean()
{
	for(int i=0;i<=MAXIPINBLACKLIST;i++)
	{
		memset(this->BlackListIP[i],0,sizeof(this->BlackListIP[i])-1);
	}
}

void ServerManager::MakeList()
{
	this->ListSize = 0;
	BYTE cBUFFER[10000]={0};
	PMSG_SERVERLISTPROTOCOL * pResult = (PMSG_SERVERLISTPROTOCOL *)(cBUFFER);
	PMSG_SERVERLIST * list = (PMSG_SERVERLIST *)(cBUFFER + sizeof(PMSG_SERVERLISTPROTOCOL));
	int lOfs=sizeof(PMSG_SERVERLISTPROTOCOL);
	int Counter=0;
	for(int i=0;i<=MAXSERVERINDEX;i++)
	{
		for(int j=0;j<=MAXSERVERSUBINDEX;j++)
		{
			if(this->Server[i][j].Show == 1)
			{
				//if(this->ShowAllServers == 1 || (this->ShowAllServers == 0 && CheckPortTCP(this->Server[i][j].Port,this->Server[i][j].IpAddress)))
				//{
				if(this->Server[i][j].State == 1)
				{
					list = (PMSG_SERVERLIST *)(cBUFFER + lOfs);
					WORD ServerNum = j+(i*MAXSERVERINDEX);
					list->Index = LOBYTE(ServerNum);
					list->SubIndex = HIBYTE(ServerNum);
					list->unk1 = this->Server[i][j].UserCount;
					list->unk2 = 0x00; //0xCC
					lOfs+=sizeof(PMSG_SERVERLIST);
					Counter++;
				}
				//}
			}
		}
	}
	pResult->subcode = 0x06;
	pResult->unk1 = 0;
	pResult->Count = Counter;
	this->ListSize = (sizeof(PMSG_SERVERLISTPROTOCOL) + sizeof(PMSG_SERVERLIST) * Counter);
	PHeadSetW((LPBYTE)pResult, 0xF4,this->ListSize );

	List = cBUFFER;
}

void ServerManager::UpdateServerInfo(WORD ServerCode,WORD UserCount,WORD MaxUsers)
{
	BYTE ServerIndex = ServerCode/20;
	BYTE ServerSubIndex = ServerCode - (ServerIndex*20);
	BYTE UserPercent = (UserCount * 100) / MaxUsers;
	this->Server[ServerIndex][ServerSubIndex].UserCount = UserPercent;
	this->Server[ServerIndex][ServerSubIndex].ServerMax = MaxUsers;
	this->MakeList();
}

void ServerManager::UpdateServerInfo2(int Port,WORD UserCount,WORD MaxUsers)
{
	int ServerIndex=0;
	int ServerSubIndex=0;
	if(this->SearchServer(Port,ServerIndex,ServerSubIndex) == true)
	{
		BYTE UserPercent = (UserCount * 100) / MaxUsers;
		int CurrUser = this->Server[ServerIndex][ServerSubIndex].UserCount;
		int CurrMax = this->Server[ServerIndex][ServerSubIndex].ServerMax;

		this->Server[ServerIndex][ServerSubIndex].UserCount = UserPercent;
		this->Server[ServerIndex][ServerSubIndex].ServerMax = MaxUsers;

		if(this->Server[ServerIndex][ServerSubIndex].Show == 1 && this->Server[ServerIndex][ServerSubIndex].State == 0)
			this->Server[ServerIndex][ServerSubIndex].State = 1;

		if(CurrUser != this->Server[ServerIndex][ServerSubIndex].UserCount || CurrMax != this->Server[ServerIndex][ServerSubIndex].ServerMax)
			this->MakeList();
	}
}

void ServerManager::UpdateServerVisible(WORD ServerCode,BYTE State)
{
	BYTE ServerIndex = ServerCode/20;
	BYTE ServerSubIndex = ServerCode - (ServerIndex*20);
	this->Server[ServerIndex][ServerSubIndex].State = State;
	this->MakeList();
}

void ServerManager::UpdateServerVisible2(int Port,BYTE State)
{
	int ServerIndex=0;
	int ServerSubIndex=0;
	if(this->SearchServer(Port,ServerIndex,ServerSubIndex) == true)
	{
		this->Server[ServerIndex][ServerSubIndex].State = State;
		this->MakeList();
	}
}

void ServerManager::ReadList(LPSTR filename)
{
	this->ListClean();
	int Token;
	int index;
	int subindex;
	int port;
	char ip[16];
	BOOL show;

	SMDFile = fopen(filename, "r");

	if ( SMDFile == NULL )
	{
		MsgBox("Error reading file %s", filename);
		exit(1);
	}

	while ( true )
	{
		Token = GetToken();

		if ( Token == 2 )
		{
			break;
		}

		if ( Token == 1 )
		{
			index = TokenNumber;
			if(index < 0 || index > MAXSERVERINDEX)
			{
				MsgBox("error-L3 : Max Index = %d", MAXSERVERINDEX);
				exit(1);
			}

			Token = GetToken();
			subindex = TokenNumber;
			if(subindex < 0 || subindex > MAXSERVERSUBINDEX)
			{
				MsgBox("error-L3 : Max SubIndex = %d", MAXSERVERSUBINDEX);
				exit(1);
			}

			Token = GetToken();
			strncpy(ip,TokenString,sizeof(ip)-1);

			Token = GetToken();
			port = TokenNumber;

			Token = GetToken();
			if(!stricmp(TokenString,"SHOW"))
			{
				show = 1;
			}else if(!stricmp(TokenString,"HIDE"))
			{
				show = 0;
			}else
			{
				MsgBox("error-L3 : Status must be SHOW or HIDE");
				exit(1);
			}

			strcpy(this->Server[index][subindex].IpAddress,ip);
			this->Server[index][subindex].Port = port;
			this->Server[index][subindex].Show = show;
			this->Server[index][subindex].State = FALSE;
		}
	}

	fclose(SMDFile);
	this->MakeList();
	LogAddC(3, "[ServerList] Loaded Successfull!");
}

void ServerManager::ReadBlackList(LPSTR filename)
{
	this->BlackListClean();

	int Token;
	this->BLSize=0;

	SMDFile = fopen(filename, "r");

	if ( SMDFile == NULL )
	{
		MsgBox("Error reading file %s", filename);
		exit(1);
	}

	while ( true )
	{
		Token = GetToken();

		if ( Token == 2 )
		{
			break;
		}

		//if ( Token == 1 )
		//{
		if(this->AddToBlackList(TokenString) == false)
		{
			MsgBox("error-L3 : Max IP count in BlackList reached!");
			exit(1);
		}
		//}
	}

	fclose(SMDFile);
	LogAddC(3, "[BlackList] Loaded Successfull!");
}

bool ServerManager::IPCheck(LPSTR IP)
{
	for(int i=0;i<this->BLSize;i++)
		if(!stricmp(IP,this->BlackListIP[i]))
			return false;
	return true;
}

bool ServerManager::AddToBlackList(LPSTR IP)
{
	if(this->BLSize >= MAXIPINBLACKLIST)
	{
		return false;
	}
	strcpy(this->BlackListIP[this->BLSize],IP);
	this->BLSize++;
	return true;
}

int ServerManager::AutoAntiFloodIPSearch(LPSTR IP)
{
	for(int i=0;i<MAXAUTOANTIFLOODLIST;i++)
	{
		if(this->AutoBL[i].Active == TRUE)
		{
			if(!stricmp(IP,this->AutoBL[i].IP))
			{
				return i;
			}
		}
	}
	return -1;
}

int ServerManager::AutoAntiFloodIPAdd(LPSTR IP)
{
	for(int i=0;i<MAXAUTOANTIFLOODLIST;i++)
	{
		if(this->AutoBL[i].Active == FALSE)
		{
			this->AutoBL[i].Active = TRUE;
			strcpy(this->AutoBL[i].IP, IP);
			return i;
		}
	}
	return -1;
}

void ServerManager::AutoAntiFloodDel(int Num)
{
	this->AutoBL[Num].Active = FALSE;
	this->AutoBL[Num].Count = 0;
	memset(&this->AutoBL[Num].IP[0],0,16);
}

void ServerManager::AutoAntiFloodCheck(LPSTR IP)
{
	int Num = this->AutoAntiFloodIPSearch(IP);

	if(Num == -1)
	{
		Num = this->AutoAntiFloodIPAdd(IP);
	}

	if(Num != -1)
	{
		this->AutoBL[Num].Count++;
		LogAddC(2,"[AntiFlood] IP: %s Flood Attempt: %d",IP,this->AutoBL[Num].Count);

		if(this->AutoBL[Num].Count >= this->AutoBLCount)
		{
			if(this->AddToBlackList(IP) == true)
			{
				this->AutoAntiFloodDel(Num);
				char IP2FILE[20]={0};
				wsprintf(IP2FILE,"\"%s\"",IP);
				WriteTxt(BLISTFILE,IP2FILE);
				LogAddC(2,"[BlackList] IP: %s Added to Black List - Flood Attempt: %d",IP,this->AutoBLCount);
			}
		}
	}
}

void ServerManager::DelAutoAntiFloodIP(LPSTR IP)
{
	int Num = this->AutoAntiFloodIPSearch(IP);

	if(Num != -1)
	{
		this->AutoAntiFloodDel(Num);
	}
}

void ServerManager::CleanAutoAntiFloodList()
{	
	for(int i=0;i<MAXAUTOANTIFLOODLIST;i++)
	{
		this->AutoAntiFloodDel(i);
	}
}
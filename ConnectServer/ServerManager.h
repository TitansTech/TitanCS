

//----------------------------------------

#define MAXSERVERINDEX		20
#define MAXSERVERSUBINDEX	19
#define MAXIPINBLACKLIST	1024
#define MAXAUTOANTIFLOODLIST	20
#define SLISTFILE			".//Data//SCF_ServerList.cfg"
#define BLISTFILE			".//Data//SCF_BlackList.dat"
#define CONFIGFILE			".//Data//SCF_Config.ini"

struct ServerAttr
{
	char IpAddress[16];
	int Port;
	BOOL Show;
	BOOL State;
	int ServerMax;
	BYTE UserCount;
};

struct AutoBLStruct
{
	BOOL Active;
	char IP[16];
	int Count;
};

struct UpdateServerAttr
{
	char Address[200];
	short Port;
	char ID[20];
	char Password[20];
	char Folder[20];
};

class ServerManager
{
public:
	void Init();
	void ReadList(LPSTR filename);
	void ReadUpdateConfig();
	void UpdateServerInfo(WORD ServerCode,WORD UserCount,WORD MaxUsers);
	void UpdateServerInfo2(int Port,WORD UserCount,WORD MaxUsers);
	void UpdateServerVisible(WORD ServerCode,BYTE State);
	void UpdateServerVisible2(int Port,BYTE State);

	ServerAttr Server[MAXSERVERINDEX+1][MAXSERVERSUBINDEX+1];
	LPBYTE List;
	int ListSize;
	int Port;
	int UDPPort;
	BOOL ShowAllServers;
	BYTE Encrypt;
	int _MaxConnections;
	//
	bool IPCheck(LPSTR IP);
	void ReadBlackList(LPSTR filename);
	//
	void AutoAntiFloodCheck(LPSTR IP);
	void DelAutoAntiFloodIP(LPSTR IP);
	//
	int DisconnectAfter;
	int MaxSendListCount;
	//
	UpdateServerAttr FTP;
	UpdateServerAttr http;
	BYTE ClientV1;
	BYTE ClientV2;
	BYTE ClientV3;
	//	
	int nHTTPFileServerInfoBufferLen;
	int nFileServerInfoBufferLen;
	BYTE szFileServerInfoSendBuffer[1000];
	BYTE szHTTPFileServerInfoSendBuffer[1000];

private:	
	void MakeList();
	void ListClean();
	bool SearchServer(int Port, int & Index, int & SubIndex);
	//
	void BlackListClean();
	bool AddToBlackList(LPSTR IP);
	char BlackListIP[MAXIPINBLACKLIST][16];
	int AutoAntiFloodIPSearch(LPSTR IP);
	int AutoAntiFloodIPAdd(LPSTR IP);
	void CleanAutoAntiFloodList();
	void AutoAntiFloodDel(int Num);
	AutoBLStruct AutoBL[MAXAUTOANTIFLOODLIST];
	int BLSize;
	int AutoBLCount;
};

extern ServerManager serverManager;
//----------------------------------------
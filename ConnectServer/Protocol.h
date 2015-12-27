struct PWMSG_HEAD	// Packet - Word Type
{
public:

	void set( LPBYTE lpBuf, BYTE head, int size)
	{
		lpBuf[0] = 0xC2;
		lpBuf[1] = SET_NUMBERH(size);
		lpBuf[2] = SET_NUMBERL(size);
		lpBuf[3] = head;
	};

	void setE( LPBYTE lpBuf, BYTE head, int size)	// line : 49
	{
		lpBuf[0] = 0xC4;
		lpBuf[1] = SET_NUMBERH(size);
		lpBuf[2] = SET_NUMBERL(size);
		lpBuf[3] = head;
	};

	BYTE c;
	BYTE sizeH;
	BYTE sizeL;
	BYTE headcode;
};

struct PBMSG_HEAD	// Packet - Byte Type
{
public:
	void set ( LPBYTE lpBuf, BYTE head, BYTE size)	// line : 18
	{
		lpBuf[0] = 0xC1;
		lpBuf[1] = size;
		lpBuf[2] = head;
	};	// line : 22

	void setE ( LPBYTE lpBuf, BYTE head, BYTE size)	// line : 25
	{
		lpBuf[0] = 0xC3;
		lpBuf[1] = size;
		lpBuf[2] = head;
	};	// line : 29

	BYTE c;
	BYTE size;
	BYTE headcode;
};


struct PMSG_DEFAULT2
{
	PBMSG_HEAD h;
	BYTE subcode;
};

struct PMSG_SERVERLIST
{
	BYTE Index;
	BYTE SubIndex;
	BYTE unk1;
	BYTE unk2;
};

struct PMSG_SERVERLISTPROTOCOL
{
	PWMSG_HEAD h;
	BYTE subcode;
	BYTE unk1;
	BYTE Count;
};


struct PMSG_SENDSERVER
{
	PBMSG_HEAD h;
	BYTE subcode;
	char IpAddress[16];
	WORD port;
};



struct PMSG_JOINRESULT
{
	PBMSG_HEAD h;
	BYTE scode;
};

struct PMSG_VERSION
{
	PBMSG_HEAD	h;	
	BYTE		Version[3];
	BYTE		TEMP[100];
};


struct PMSG_FILESERVERINFO
{
	PBMSG_HEAD	h;

	BYTE	Version[2];
	char 	IpAddress[100];
	short	Port;
	char	FtpId[20];
	char	FtpPass[20];
	char	Folder[20];
};

struct PMSG_DEFRESULT
{
	PBMSG_HEAD	h;	
	BYTE		result;
};

void JoinResultSend(int aIndex, BYTE result);
void ProtocolCore(int aIndex, LPBYTE aRecv, int aLen);
void CSVersionRecv(int aIndex, PMSG_VERSION * lpMsg);
void CSHTTPVersionRecv(int aIndex, PMSG_VERSION * lpMsg);
void CFileServerInfoMake();
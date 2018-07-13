#include "osa_common.h"

//�ַ�����
#define   UP_ARROW			27
#define   DOWN_ARROW		28
#define   LEFT_ARROW		29
#define   RIGHT_ARROW		30
#define	  NEWLINE_CHAR		'\n'
#define   BACKSPACE_CHAR	8
#define   BLANK_CHAR		' '
#define   RETURN_CHAR		13
#define   TAB_CHAR			9
#define   DEL_CHAR			127
#define	  CTRL_S			19
#define   CTRL_R			18

#define  CMD_TABLE_SIZE			(UINT32)1024			//�������	
#define  CMD_NAME_LENGTH		(UINT8)20			//�������Ƴ��� 
#define  CMD_USAGE_LENGTH		(UINT8)80			//����˵������

#define  MAX_COMMAND_LENGTH		(UINT8)100			// telnet��������󳤶�, ��������

#define  MAX_PRTMSG_LEN			(UINT32)1024			//��ӡ��Ϣ����󳤶�
#define  MAX_PROMPT_LEN			(UINT8)20

#define	 TELNET_MAGIC_NUM		0xbeedbeef
#define  TASKWAIT_MS			2000  //�ȴ����������ʱ��
#define SELECT_TIMEOUT			200 //select ��ʱʱ��

#define OCCSTR_MAXLEN	8	//OCCתΪ�ַ�������󳤶�
#define TIMESTR_MAXLEN	14	//ʱ���ַ�������󳤶� [hh:mm:ss:xxx]
#define U64_SIZE		sizeof(UINT64)

#define OSA_LOG_FILE_MAXSIZE (10<<20)	//10M

static const char *g_pchTelnetPasswd = "minyuan2018";
//static const char *g_pchTelnetPasswd = "vda2013";

#ifdef _MSC_VER
typedef INT32 (* UniFunc)(INT32, INT32, INT32, INT32, INT32, INT32, INT32, INT32, INT32); //ͨ�õ������
#else
typedef INT32 (* UniFunc)(INT64, INT64, INT64, INT64, INT64, INT64, INT64, INT64, INT64); //ͨ�õ������
#endif

typedef struct tagCmdInfo //����ṹ
{
	char	m_achName[CMD_NAME_LENGTH+1];	//��������
	UniFunc m_funcCmd;						//�����
	char	m_achUsage[CMD_USAGE_LENGTH+1];	//����ʹ��
}TCmdInfo;

typedef struct tagCmdTable //�����
{
	UINT32 m_dwCurCmdNum;						//��ǰ�������
	TCmdInfo m_atCmdInfo[CMD_TABLE_SIZE]; 
}TCmdTable;


typedef struct tagRawPara
{
	char *paraStr;
	BOOL bInQuote;
	BOOL bIsChar;
}TRawPara;	

//��ȷ��ӡ��
typedef struct tagLabPrtTable 
{
	UINT32 m_dwCurNum;
	OCC m_aqwPrtLab[LABPRT_NAME_MAXNUM];
}TLabPrtTable;

typedef struct tagTelnetSvrStatis 
{
	UINT32 m_dwConNum;
	UINT32 m_dwDisConNum;
	UINT32 m_dwLoginNum; //��¼�ɹ���������������֤�ɹ�
}TTelnetSvrStatis;

typedef struct tagTelnetSvrInfo
{
	UINT32 m_dwMagicNum;
	UINT16 m_wPort;
	BOOL m_bInit;
	BOOL m_bExit;
	SOCKHANDLE m_hListenSock;		//���������Sock
	SOCKHANDLE m_hSaveAcceptSock;	//���ڶ���ͻ���������ʱ����ʱ����ǰһ������Sock
	SOCKHANDLE m_hAcceptSock;		//���浱ǰ����Sock
// 	UINT8	m_byPrtGate;
	TASKHANDLE m_hMsgPrcTask;
	TASKHANDLE m_hAcceptTask;
	TCmdTable  m_tCmdTable; //�����

	TLabPrtTable m_tLabPrtTable; //��ӡ��
	LockHandle m_hLockLabTable;	//���ڱ�����ȷ��ӡ�����б�

// 	BOOL m_bShowMs;  //�Ƿ���ʾ����
	TTelnetSvrStatis m_tStatis;
	BOOL m_bNeedPasswdVf;
	BOOL m_bBashOn; //�Ƿ�ֱͨ
	BOOL m_bPrintFile; //�Ƿ����뵽�ļ�
	char m_szLogFilePrefixName[32];
	LockHandle m_hLockLogFile;	//���ڱ������߳�д�ļ�
	FILE* m_pCurLogFile;	//�򿪵���־�ļ�
	UINT32 m_dwLogFileLen;	//��־�ļ�����
	UINT32 m_dwLogFileLoopIdx;
}TTelnetSvrInfo;


// static void sTelnetClientMsgPrc( TelnetHandle hTelHdl );
// static void sTelnetAcceptPrc( TelnetHandle hTelHdl );
static void sCmdProc(TelnetHandle hTelHdl, char* pchCmd, UINT8 byCmdLen, SOCKHANDLE *ptAcceptSock, SOCKHANDLE *ptSaveAcceptSock);

//����telnet��Tcp�����
static UINT32 sTelnetTcpSvrCreate( UINT16 wStartPort, OUT UINT16 *pwRealPort, OUT SOCKHANDLE *phListenSock )
{
	UINT32 dwRet = 0;
	SOCKHANDLE tListenSock = 0;	
	UINT16 wSearchPort = 0;
	UINT16 wRealPort = 0;  //ʵ��ʹ�õĶ˿�
	BOOL bSockCreate = FALSE;
	do 
	{	
		//����Tcp�׽���
		dwRet = OsaTcpSocketCreate( &tListenSock );
		if( dwRet != OSA_OK )
		{
			break;
		}
		bSockCreate = TRUE;

		//���׽��ֶ˿�
		dwRet = OsaSocketBind( tListenSock, wStartPort );
		if( dwRet != OSA_OK )
		{
			//�󶨺����˿�
			for(wSearchPort = wStartPort + 1; wSearchPort <= wStartPort + 1000; wSearchPort++)
			{
				dwRet = OsaSocketBind( tListenSock, wSearchPort );
				if( OSA_OK == dwRet )
				{
					wRealPort = wSearchPort;
					break;
				}
			}
		}
		else
		{
			wRealPort = wStartPort;
		}
	
		//����
		dwRet = OsaSocketListen( tListenSock, 20 );
		if( dwRet != OSA_OK )
		{
			break;
		}
	} while (0);

	if( dwRet != OSA_OK )
	{
		if( TRUE == bSockCreate )
		{
			OsaSocketClose( tListenSock );
		}
	}
	else
	{
		*phListenSock = tListenSock;
		*pwRealPort = wRealPort;
// 		iOsaLog( OSA_MSGOCC, "sTelnetTcpSvrCreate success, telnetsvr port is %d, startPort is %d\n", wRealPort, wStartPort);
	}
	return dwRet;
}

static void sTelnetTcpSvrExit( SOCKHANDLE hListenSock )
{
	OsaSocketClose( hListenSock );
}


/*====================================================================
���ܣ�֪ͨaccept�����˳�
�������˵����	dwTelHdl - telnet���������
====================================================================*/
static UINT32 sTelnetSvrAcceptNtf( TTelnetSvrInfo *ptTelnetSvr )
{
	UINT32 dwRet = 0;
	SOCKHANDLE hSock;
// 	int ret;
// 	struct sockaddr_in tSvrAddr;

	if(ptTelnetSvr == NULL)
	{
		printf("AcceptTaskExitInfo(), ptTelnetSvr is null\n");
		return COMERR_NULL_POINTER;
	}

	//����Tcp�׽���
	dwRet = OsaTcpSocketCreate( &hSock );
	if( dwRet != OSA_OK )
	{
		return dwRet;
	}

	//����
//     memset( &tSvrAddr, 0, sizeof(tSvrAddr) );    
//     tSvrAddr.sin_family = AF_INET; 
//     tSvrAddr.sin_port = htons(ptTelnetSvr->m_wPort);
//     tSvrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
// 
// 	ret = connect(hSock, (struct sockaddr *)&tSvrAddr, sizeof(tSvrAddr));
// 	if(ret != 0)
// 	{
// 		printf("AcceptTaskExitInfo(), connect() failed, ret=%d, errno=%d\n", ret, errno);
// 	}
	dwRet = OsaSocketConnect( hSock, "127.0.0.1", ptTelnetSvr->m_wPort );
	OsaSleep( 10 );
	OsaSocketClose( hSock );
	return dwRet;
}

//��ӡ���ͻ���
static void sPrintToClient(const char* pchMsg, SOCKHANDLE hAcceptSock, UINT16 wTelSvrPort)
{
	INT32 nSndBytes;
	nSndBytes = OsaSocketSend(hAcceptSock, pchMsg, (UINT32)strlen(pchMsg)+1, 0 );	

	/* ��Ƭ����һ���س�������Ƭ��������ֹ�������س��� */
// 	if(wTelSvrPort != TELNET_PORT_TARGET)
	{
		nSndBytes = OsaSocketSend(hAcceptSock, "\r", 1, 0);	
	}
	return;
}

//��Telnet����ʾ��ʾ��.
static void sPromptShow(SOCKHANDLE hAcceptSock)
{
	char achPrompt[MAX_PROMPT_LEN+1] = ">";	
#ifdef _MSC_VER
	OsaSocketSend(hAcceptSock, achPrompt, (UINT32)strlen(achPrompt)+1, 0);
#endif
#ifdef _LINUX_
	OsaSocketSend(hAcceptSock, achPrompt, (UINT32)strlen(achPrompt)+1, MSG_NOSIGNAL);  //��Linux��send�����һ������Ҫ����ΪMSG_NOSIGNAL �����ͳ�����п��ܵ��³����˳� add by yhq 20160302
#endif

	return;
}


//�ͻ���������֤
static void sClientPasswdVerify( TelnetHandle hTelHdl )
{
	INT32 nRet;
	INT8  cmdChar;
	UINT8 byCmdLen = 0;
	INT8 achCommand[MAX_COMMAND_LENGTH+1];
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)hTelHdl;
	BOOL bMatched = FALSE;
	UINT8 byPasswdCmpLen;
	INT8 tmpChar[3];
	fd_set tReadFd;
	struct timeval tTimeout;	
	INT32 ret;

	tmpChar[0] = BACKSPACE_CHAR;
	tmpChar[1] = BLANK_CHAR;	
	tmpChar[2] = '\0';

	if(ptTelnetSvr == NULL)
	{		
		return;
	}

	while(TRUE)
	{
		if(ptTelnetSvr->m_bExit == TRUE)
		{		
			printf("sClientPasswdVerify Task exited\n");			
			OsaThreadSelfExit();
			break;
		}		

		if(ptTelnetSvr->m_hAcceptSock != INVALID_SOCKET)
		{
			FD_ZERO(&tReadFd); 
			//tTimeout ÿ�ζ�Ҫ�裬linux���� select ����޸� tTimeout ��ֵ ������
			memset(&tTimeout, 0, sizeof(tTimeout));
			tTimeout.tv_sec = 0;
			tTimeout.tv_usec = SELECT_TIMEOUT * 1000;
			FD_SET(ptTelnetSvr->m_hAcceptSock, &tReadFd);

			ret = select(FD_SETSIZE, &tReadFd, NULL, NULL, &tTimeout);
			if(ret <= 0)
			{			
				continue;
			}

			if( ! FD_ISSET(ptTelnetSvr->m_hAcceptSock, &tReadFd) )
			{
				continue;
			}

			nRet = recv(ptTelnetSvr->m_hAcceptSock, &cmdChar, 1, 0);
		}
		else
		{
			OsaSleep( 200 );
			continue;			
		}

		/* �ͻ��˹ر� */
		if(nRet == 0)
		{
			OsaSocketClose(ptTelnetSvr->m_hAcceptSock);
			ptTelnetSvr->m_hAcceptSock = INVALID_SOCKET;
			ptTelnetSvr->m_hSaveAcceptSock = INVALID_SOCKET;
			continue;
		}

		/* ���˹ر� */
		if(nRet <= SOCKET_ERROR)
		{
			OsaSleep(100);
			continue;
		}		

		switch(cmdChar)
		{
		case NEWLINE_CHAR:		/* ���з�, �� 13, 10 ����,���Գ��ȼ�һ */

			byPasswdCmpLen = (byCmdLen > 1) ? byCmdLen - 1 : byCmdLen;	
			if( (byPasswdCmpLen == strlen(g_pchTelnetPasswd)) && 
				(strncmp(achCommand, g_pchTelnetPasswd, byPasswdCmpLen) == 0) )
			{		
				bMatched = TRUE;
			}
			else
			{
				sPrintToClient("\nenter password:\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
			}

			byCmdLen = 0;

			break;

		default:
			/* ���������ַ��� */
			if(byCmdLen < MAX_COMMAND_LENGTH)
			{				
				achCommand[byCmdLen++] = cmdChar;	
			}
			else
			{
				sPrintToClient("\nenter password:\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
				byCmdLen = 0;
			}

			/* ʹ�����ˣ���һ���ո����ԭ�ַ�����ʹ������ */
			OsaSocketSend(ptTelnetSvr->m_hAcceptSock, tmpChar, sizeof(tmpChar), 0);

			break;

		}//switch

		if(bMatched)
		{
			sPrintToClient("\npassword ok !!!\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
			sPromptShow(ptTelnetSvr->m_hAcceptSock);
			break;
		}
	}//while

	return;	
}

/*====================================================================
���ܣ��������ӵ�����
�������˵����	
====================================================================*/
void sTelnetAcceptPrc( void* pParam )
{
	INT32 iMode = 0;	
	struct sockaddr_in tAddrClient;
    INT32 nAddrLenIn = sizeof(tAddrClient);	
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)pParam;

	//�������
	if(ptTelnetSvr == NULL)
	{
		printf("AcceptLoop(), ptTelnetSvr is null\n");
		return;
	}

	while(TRUE)
	{
		ptTelnetSvr->m_hSaveAcceptSock = ptTelnetSvr->m_hAcceptSock;
#ifdef _MSC_VER
		ptTelnetSvr->m_hAcceptSock = accept(ptTelnetSvr->m_hListenSock, (struct sockaddr *)&tAddrClient, (int *)(&nAddrLenIn));
#else
		ptTelnetSvr->m_hAcceptSock = accept(ptTelnetSvr->m_hListenSock, (struct sockaddr *)&tAddrClient, (socklen_t *)(&nAddrLenIn));
#endif
		ptTelnetSvr->m_tStatis.m_dwConNum++;

		//���÷�����
#ifdef _MSC_VER
		iMode = 1;
		ioctlsocket(ptTelnetSvr->m_hAcceptSock, FIONBIO, (u_long FAR*)&iMode);
#else
		fcntl(ptTelnetSvr->m_hAcceptSock, F_SETFL, O_NONBLOCK);
#endif

		if(ptTelnetSvr->m_bExit == TRUE)
		{		
			printf("sTelnetAcceptPrc Task exited\n");			
			OsaThreadSelfExit();
			break;
		}		

		/* �ر�ԭ�������� */
		if(ptTelnetSvr->m_hSaveAcceptSock != INVALID_SOCKET) 
		{            
			OsaSocketClose(ptTelnetSvr->m_hSaveAcceptSock);
			ptTelnetSvr->m_hSaveAcceptSock = INVALID_SOCKET;
			ptTelnetSvr->m_tStatis.m_dwDisConNum++;
		}
		
		ptTelnetSvr->m_bNeedPasswdVf = TRUE;

		/* �����ʾ�� */
		sPrintToClient("***************************************\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
		sPrintToClient("welcome to telnet server\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
		sPrintToClient("***************************************\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);

		sPrintToClient("\nenter password:\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
	}
}

/*====================================================================
���ܣ�����ͻ��˵���Ϣ
�������˵����            
����ֵ˵������.
====================================================================*/
void sTelnetClientMsgPrc( void* pParam )
{
	INT32 nRet;
	INT8 cmdChar;
    INT8 achCommand[MAX_COMMAND_LENGTH+1];
	INT8 achPrevCommand[MAX_COMMAND_LENGTH+1];
    UINT8 byCmdLen = 0;
	UINT8 byPreCmdLen = 0;
	INT8 tmpChar[3];
	UINT8 byPreCmdSaved = 0;
	INT32 ret;
	fd_set tReadFd;
	struct timeval tTimeout;
//	UINT32 dwIdx;
	TelnetHandle hTelHdl = pParam;
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)pParam;
	//�������
	if(ptTelnetSvr == NULL)
	{
		printf("sTelnetClientMsgPrc(), ptTelnetSvr is null\n");
		return;
	}

    while(TRUE)
	{
		/* �˳������� */
		if(ptTelnetSvr->m_bExit == TRUE)
		{
			printf("TelnetClientMsgPrc Task exited\n");			
			OsaThreadSelfExit();
			break;
		}
		
		//������֤
		if(ptTelnetSvr->m_bNeedPasswdVf)
		{
			sClientPasswdVerify( pParam );
			ptTelnetSvr->m_bNeedPasswdVf = FALSE;
			ptTelnetSvr->m_tStatis.m_dwLoginNum++;
		}		

		if(ptTelnetSvr->m_hAcceptSock == INVALID_SOCKET)
		{
			OsaSleep( 200 );
			continue;
		}

		/* �����ؽ����û����� */
        /* ע�⣺�˴�����recv��Ϊ�����ֶԶ˹ر�����ִ������������
         * ��SockRecv��������Ϊ��һ������������Բ���ʹ�á�
         */

		FD_ZERO(&tReadFd); 
		//tTimeout ÿ�ζ�Ҫ�裬linux���� select ����޸� tTimeout ��ֵ ������
		memset(&tTimeout, 0, sizeof(tTimeout));
		tTimeout.tv_sec = 0;
		tTimeout.tv_usec = SELECT_TIMEOUT * 1000;

		FD_SET(ptTelnetSvr->m_hAcceptSock, &tReadFd);

		ret = select(FD_SETSIZE, &tReadFd, NULL, NULL, &tTimeout);
		if(ret <= 0)
		{			
			continue;
		}

		if( ! FD_ISSET(ptTelnetSvr->m_hAcceptSock, &tReadFd) )
		{
			continue;
		}

		nRet = recv(ptTelnetSvr->m_hAcceptSock, &cmdChar, 1, 0);

		/* �ͻ��˹ر� */
		if(nRet == 0)
		{
			OsaSocketClose(ptTelnetSvr->m_hAcceptSock);
			ptTelnetSvr->m_hAcceptSock = INVALID_SOCKET;
			ptTelnetSvr->m_hSaveAcceptSock = INVALID_SOCKET;
			continue;
		}

		/* ���˹ر� */
		if(nRet == SOCKET_ERROR)
		{
			OsaSleep(500);
			continue;
		}		

		/* �����û�����, ���Ե�Telnet�ͻ�����Ļ��, ������������ʵ�����Ӧ */
		switch(cmdChar)
		{
		case CTRL_S:
			sPromptShow(ptTelnetSvr->m_hAcceptSock);
			break;

		case CTRL_R:
			sPromptShow(ptTelnetSvr->m_hAcceptSock);
			break;

		case NEWLINE_CHAR:		/* ���з� */									
			if(byCmdLen > 0)
			{
				byPreCmdLen = byCmdLen;
				memcpy(achPrevCommand, achCommand, byCmdLen);
				byPreCmdSaved = 1;

				sCmdProc(hTelHdl, achCommand, byCmdLen, &(ptTelnetSvr->m_hAcceptSock), &(ptTelnetSvr->m_hSaveAcceptSock));			
				byCmdLen = 0;
			}
			sPromptShow(ptTelnetSvr->m_hAcceptSock);
			break;
        
		case RETURN_CHAR:         // �س���	
		case UP_ARROW:			  // �ϼ�ͷ�� "[A"
		case DOWN_ARROW:          // �¼�ͷ
		case LEFT_ARROW:          // ���ͷ
		case RIGHT_ARROW:         // �Ҽ�ͷ
			break;			
		case BACKSPACE_CHAR:         // �˸��
			
			if(byCmdLen <= 0)
			{
				continue;
			}
			
			byCmdLen--;			
			
			/* ʹ�����ˣ���һ���ո����ԭ�ַ�����ʹ������ */
			
           // tmpChar[0] = BACKSPACE_CHAR;
			tmpChar[0] = BLANK_CHAR;
			tmpChar[1] = BACKSPACE_CHAR;
			tmpChar[2] = '\0';
			
            OsaSocketSend(ptTelnetSvr->m_hAcceptSock, tmpChar, sizeof(tmpChar), 0);
			
			break;
			
		case '/':            // ��ʾ��һ������, ����defaultǰ,�� '/' Ϊ����(�� diskusage /)����ʱ,����ִ��default
#if 1	
			if((byPreCmdSaved == 1) && (byCmdLen == 0)) //ֻ�е�һ���ַ�����Ч
			{
				byCmdLen = byPreCmdLen;
				memcpy(achCommand, achPrevCommand, byCmdLen);
				achPrevCommand[byCmdLen] = '\0';
				OsaLabPrt( GENERAL_PRTLAB_OCC, "%s", achPrevCommand);				
				break;
			}
#else
			//printf("spytest\n");
			spytest(ptTelnetSvr->m_hAcceptSock);
#endif
		default:
			/* ���������ַ��� */
			if(byCmdLen < MAX_COMMAND_LENGTH)
			{				
				achCommand[byCmdLen++] = cmdChar;	
			}
			else
			{
				//OsaLabPrt( OSA_ERROCC, "\n");
				OsaLabPrt( GENERAL_PRTLAB_OCC,  "\n");
				byPreCmdLen = byCmdLen;
				memcpy(achPrevCommand, achCommand, byCmdLen);
				byPreCmdSaved = 1;

				sCmdProc(hTelHdl, achCommand, byCmdLen, &(ptTelnetSvr->m_hAcceptSock), &(ptTelnetSvr->m_hSaveAcceptSock));	
				byCmdLen = 0;
				sPromptShow(ptTelnetSvr->m_hAcceptSock);
			}
	
			break;
		}
    }
}

//��ѯ����
static UniFunc sFindCommand(TTelnetSvrInfo *ptTelnetSvr, const char* name )
{
	UINT32 dwIdx;
	UINT32 dwCurCmdNum;
	TCmdTable *ptCmdTable;
	if(ptTelnetSvr == NULL)
	{		 
		printf("FindCommand(), ptTelnetSvr is null\n");
		return NULL;
	}

	if((ptTelnetSvr->m_bInit == FALSE) || (ptTelnetSvr->m_dwMagicNum != TELNET_MAGIC_NUM))
	{		
		printf("FindCommand(), ptTelnetSvr binit=%d, magic=0x%x, TELNET_MAGIC_NUM=0x%x\n", ptTelnetSvr->m_bInit, ptTelnetSvr->m_dwMagicNum, TELNET_MAGIC_NUM);
		return NULL;
	}

	ptCmdTable = &(ptTelnetSvr->m_tCmdTable);

	dwCurCmdNum = ptCmdTable->m_dwCurCmdNum;
	for(dwIdx = 0; dwIdx < dwCurCmdNum; dwIdx++)
	{
		if (strncmp(ptCmdTable->m_atCmdInfo[dwIdx].m_achName, name, CMD_NAME_LENGTH) == 0)
		{
			return ptCmdTable->m_atCmdInfo[dwIdx].m_funcCmd;

		}
	}
	return NULL;
}

static INT32 sWordParse(char *word)
{
	INT32 tmp;

	if(word == NULL) return 0;

	tmp = atoi(word);
	if(tmp == 0 && word[0] != '0')
	{
#ifdef _MSC_VER
		return (INT32)word;
#else
		return (INT64)word;
#endif
	}
	return tmp;
}

//ִ������
static void sCmdRun( TelnetHandle hTelHdl, char *szCmd, SOCKHANDLE *ptAcceptSock, SOCKHANDLE *ptSaveAcceptSock)
{  
//	INT32 ret = 0;	
	INT8 *cmd = szCmd;
#ifdef _MSC_VER
	INT32 para[10];	
#else
	INT64 para[10];	
#endif
	TRawPara atRawPara[10];
	INT32 paraNum = 0;
	UINT8 count = 0;
	UINT8 chStartCnt = 0;
	BOOL bStrStart = FALSE;
	BOOL bCharStart = FALSE;
//	HMODULE hModule = NULL; //???
	UINT32 cmdLen = (UINT32)strlen(szCmd)+1;
//	INT8 achRealCmd[MAX_COMMAND_LENGTH + 1]; //�ڲ��������õ������dwTelHdl);
	UniFunc cmdFunc;
	INT8 achBashCmd[MAX_COMMAND_LENGTH];
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)hTelHdl;
	
	if(ptTelnetSvr == NULL)
	{
		return;
	}

	//���bashֱͨ������ԭʼ������
	if(ptTelnetSvr->m_bBashOn)
	{
		memcpy(achBashCmd, szCmd, MAX_COMMAND_LENGTH);
		achBashCmd[MAX_COMMAND_LENGTH-1] = '\0';
	}
	

	memset(para, 0, sizeof(para));
	memset(atRawPara, 0, sizeof(TRawPara)*10);

	/* �������������� */
	while(count < cmdLen)
	{	
		switch(szCmd[count])
		{
		case '\'':
			szCmd[count] = '\0';
			if(!bCharStart)
			{
				chStartCnt = count;
			}
			else
			{
				if(count > chStartCnt+2)
				{
					OsaLabPrt( GENERAL_PRTLAB_OCC, "telnetsvr: CmdRun(), input error ##1.\n");
					return;
				}
			}
			bCharStart = !bCharStart;
			break;

		case '\"':
			szCmd[count] = '\0';
			bStrStart = !bStrStart;
 			break;

		case ',':
		case ' ':
		case '\t':
		case '\n':
		case '(':
		case ')':
            if( ! bStrStart )
			{
				szCmd[count] = '\0';
			}
			break;

		default:
			/* ������ַ�Ϊ��Ч�ַ���ǰһ�ַ�ΪNULL����ʾ�ɵ��ʽ�����
			   �µ��ʿ�ʼ */
			if(count > 0 && szCmd[count-1] == '\0')
			{				
				atRawPara[paraNum].paraStr = &szCmd[count];
				if(bStrStart)
				{
					atRawPara[paraNum].bInQuote = TRUE;
				}
				if(bCharStart)
				{
					atRawPara[paraNum].bIsChar = TRUE;
				}
				if(++paraNum >= 10)
					break;
			}
		}
		count++;
	}

	if(bStrStart || bCharStart)
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "telnetsvr: CmdRun(), input error.\n");
		return;
	}

	for(count=0; count<10; count++)
	{
		if(atRawPara[count].paraStr == NULL)
		{
			para[count] = 0;
			continue;
		}

		if(atRawPara[count].bInQuote)
		{
#ifdef _MSC_VER
			para[count] = (INT32)atRawPara[count].paraStr;
#else
			para[count] = (INT64)atRawPara[count].paraStr;
#endif
			continue;
		}

		if(atRawPara[count].bIsChar)
		{
			para[count] = (INT8)atRawPara[count].paraStr[0];
			continue;
		}

		para[count] = sWordParse(atRawPara[count].paraStr);
	}

	/* ��ִ�� byee ���� */
    if ( strcmp("bye", cmd) == 0 )
    {
        OsaLabPrt( GENERAL_PRTLAB_OCC,"\n  bye......\n");
        OsaSleep(500);            // not reliable,
        OsaSocketClose(*ptAcceptSock);
        *ptAcceptSock = INVALID_SOCKET;
        *ptSaveAcceptSock = INVALID_SOCKET;
		return;
    }    


	//��ע���������в�ѯ�Ƿ��и�����о�ִ��

	cmdFunc = sFindCommand( ptTelnetSvr, cmd);
	if (cmdFunc != NULL)
	{
		(*cmdFunc)( para[0],para[1],para[2],para[3],para[4],para[5],para[6],para[7],para[8]);
	} 
	else
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "function '%s' doesn't exist!\n", szCmd);
		if(ptTelnetSvr->m_bBashOn)
		{
			system(achBashCmd);	
		}
	}
	return;
}

static void sCmdProc(TelnetHandle hTelHdl, char* pchCmd, UINT8 byCmdLen, SOCKHANDLE *ptAcceptSock, SOCKHANDLE *ptSaveAcceptSock)
{
	UINT8 count;
	INT32 nCpyLen = 0;
	INT8 achCommand[MAX_COMMAND_LENGTH];

	if((ptSaveAcceptSock == NULL) || (ptAcceptSock == NULL))
	{
		return;
	}

	//ȥͷ
	for(count=0; count<byCmdLen; count++)
	{
		INT8 chTmp;

		chTmp = pchCmd[count];

		if( ((chTmp >= '0') && (chTmp <= '9')) || ((chTmp >= 'a') && (chTmp <= 'z')) || ((chTmp >= 'A') && (chTmp <= 'Z')) )
		{
			break;
		}

	}

	nCpyLen = byCmdLen-count;
	if(nCpyLen <= 0)
	{
		return;
	}

	memcpy(achCommand, pchCmd+count, nCpyLen);   
	if(byCmdLen < MAX_COMMAND_LENGTH)
	{
		achCommand[nCpyLen] = '\0';
	}
	else
	{
		achCommand[MAX_COMMAND_LENGTH-1] = '\0';
	}

	sCmdRun(hTelHdl, achCommand, ptAcceptSock, ptSaveAcceptSock); 
}



/*===========================================================
���ܣ���ʼ��telnet������
����˵���� 	wStartPort -- telnet����˿ڣ�����ö˿ڱ�ռ�ã��ٵ�������1000���˿�
			phTelHdl -- telnet���������		
����ֵ˵�����ɹ�����CTL_OK��ʧ�ܷ��ش�����
˵����ʵ��ʹ��ʱ�����������Ҫ����һ��ʵ������CtlInit������
	  �ϲ�ͨ��CtlTelnetHdlGet�õ�������ϲ㲻���ڵ���TelnetSvrInit()
===========================================================*/
UINT32 InnerOsaTelnetSvrInit( UINT16 wStartPort, char* szLogFilePrefixName, OUT TelnetHandle *phTelHdl )
{
 	SOCKHANDLE tListenSock = INVALID_SOCKET;
	UINT32 dwRet = 0;
	UINT16 wRealPort = 0;  //ʵ��ʹ�õĶ˿�
	BOOL bTcpSvrCreate = FALSE;
	BOOL bMemAlloc = FALSE;
	BOOL bLabTableLockCreate = FALSE;
	BOOL bLogFileLockCreate = FALSE;

	TASKHANDLE hAcceptTask = TASK_NULL;
	TASKHANDLE hMsgPrcTask = TASK_NULL;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
#ifdef _i386_
	UINT32 dwMsgPrcStackSize = 64<<10;
	UINT32 dwAcpPrcStackSize = 64<<10;
#else
	UINT32 dwMsgPrcStackSize = 16<<10;
	UINT32 dwAcpPrcStackSize = 16<<10;
#endif

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();

	//�������
	if((phTelHdl == NULL) || (wStartPort == 0)) 
	{
		//����֧�ִ������TelnetSvr����������õ�ǰTelnet��ӡ
		iOsaLog( OSA_ERROCC, "TelnetSvrInit(), pdwTelHdl=0x%x, wStartPort=%d, return\n", phTelHdl, wStartPort);
		return COMERR_INVALID_PARAM;
	}

	do 
	{
		//����telnet��Tcp�����
		dwRet = sTelnetTcpSvrCreate( wStartPort, &wRealPort, &tListenSock );
		if( dwRet != OSA_OK )
		{
			break;
		}	
		bTcpSvrCreate = TRUE;

		//����������ֵ
		ptTelnetSvr = (TTelnetSvrInfo *)OsaMalloc(iOsaMemAllocGet(), sizeof(TTelnetSvrInfo), iOsaMRsrcTagGet() );
		if(ptTelnetSvr == NULL)
		{
			dwRet = COMERR_MEMERR;
			iOsaLog( OSA_ERROCC, "telnetsvr: TelnetSvrInit(), ptTelnetSvr malloc failed\n");
			break;
		}
		bMemAlloc = TRUE;

		memset(ptTelnetSvr, 0, sizeof(*ptTelnetSvr));
		ptTelnetSvr->m_wPort = wRealPort;
		ptTelnetSvr->m_hListenSock = tListenSock;
		ptTelnetSvr->m_dwMagicNum = TELNET_MAGIC_NUM;
		ptTelnetSvr->m_hAcceptSock = INVALID_SOCKET;
		strncpy(ptTelnetSvr->m_szLogFilePrefixName, szLogFilePrefixName, 31 );
		ptTelnetSvr->m_szLogFilePrefixName[31] = '\0';

		//����Lab������
		dwRet = OsaLightLockCreate(&(ptTelnetSvr->m_hLockLabTable));
		if( dwRet != OSA_OK )
		{
			break;
		}	
		bLabTableLockCreate = TRUE;

		//����д��־�ļ�������
		dwRet = OsaLightLockCreate(&(ptTelnetSvr->m_hLockLogFile));
		if( dwRet != OSA_OK )
		{
			break;
		}	
		bLogFileLockCreate = TRUE;
		
		/* ������ͻ���ͨ�ŵ����� */ 
		dwRet = OsaThreadCreate( sTelnetClientMsgPrc, "TelnetClientMsgPrcTask", OSA_THREAD_PRI_NORMAL,
								 dwMsgPrcStackSize, (void*)ptTelnetSvr, FALSE, &hMsgPrcTask );
		if( dwRet != OSA_OK || hMsgPrcTask == TASK_NULL)
		{
			iOsaLog( OSA_ERROCC, "telnetsvr: TelnetSvrInit(), TelnetClientMsgPrcTask create failed\n");
			break;
		}

		/* �������ӵ����� */
		dwRet = OsaThreadCreate( sTelnetAcceptPrc, "TelnetSvrAcceptPrcTask", OSA_THREAD_PRI_NORMAL, 
										dwAcpPrcStackSize, (void*)ptTelnetSvr, FALSE, &hAcceptTask);
		if( dwRet != OSA_OK || hAcceptTask == TASK_NULL )
		{
			iOsaLog( OSA_ERROCC, "telnetsvr: TelnetSvrInit(), telnetSvrAcceptTask create failed\n");
			return FALSE;
		}
		ptTelnetSvr->m_hAcceptTask = hAcceptTask;
		ptTelnetSvr->m_hMsgPrcTask = hMsgPrcTask;

	} while (0);

	if( dwRet != OSA_OK )
	{
		if( TRUE == bLogFileLockCreate )
		{
			OsaLightLockDelete( ptTelnetSvr->m_hLockLogFile );
		}

		if( TRUE == bLabTableLockCreate )
		{
			OsaLightLockDelete( ptTelnetSvr->m_hLockLabTable );
		}

		if( TRUE == bMemAlloc )
		{
			OsaMFree(iOsaMemAllocGet(), ptTelnetSvr );
		}

		if( TRUE == bTcpSvrCreate )
		{
			sTelnetTcpSvrExit( tListenSock );
		}
		*phTelHdl = NULL;
	}
	else
	{
		ptTelnetSvr->m_bInit = TRUE;
		*phTelHdl = (TelnetHandle)ptTelnetSvr;
	}
	return dwRet;
}

/*===========================================================
���ܣ��˳�telnet������
����˵���� 	hTelHdl -- telnet���������
����ֵ˵�����ɹ�����CTL_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 InnerOsaTelnetSvrExit( TelnetHandle hTelHdl )
{
	UINT32 dwRet = 0;
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)hTelHdl;
	//�������
	if(ptTelnetSvr == NULL)
	{
		printf("InnerOsaTelnetSvrExit(), ptTelnetSvr is null\n");
		return COMERR_NULL_POINTER;
	}

	//�������
	if( ptTelnetSvr->m_dwMagicNum != TELNET_MAGIC_NUM)
	{
		dwRet = COMERR_MEMERR;
		printf("InnerOsaTelnetSvrExit(), TelnetHandle memory error, magic:0x%x\n", ptTelnetSvr->m_dwMagicNum );
		return dwRet;
	}

	//�˳��߳�
	{
		//�����˳���־
		ptTelnetSvr->m_bExit = TRUE;

		//��� accept ����
		sTelnetSvrAcceptNtf( ptTelnetSvr );

		//�ȴ�������� ???
#ifdef _MSC_VER
		OsaThreadExitWait(ptTelnetSvr->m_hAcceptTask);
		OsaThreadExitWait(ptTelnetSvr->m_hMsgPrcTask);
// 		if(iWin_IsThreadActive(ptTelnetSvr->m_hAcceptTask))
// 		{
// 			WaitForSingleObject(ptTelnetSvr->m_hAcceptTask, TASKWAIT_MS);
// 		}
// 
// 		if(iWin_IsThreadActive(ptTelnetSvr->m_hMsgPrcTask))
// 		{
// 			WaitForSingleObject(ptTelnetSvr->m_hMsgPrcTask, TASKWAIT_MS);
// 		}
#else
		
		OsaThreadExitWait(ptTelnetSvr->m_hAcceptTask);
		OsaThreadExitWait(ptTelnetSvr->m_hMsgPrcTask);
#endif
	}

	if(ptTelnetSvr->m_hAcceptSock != INVALID_SOCKET)
	{
		OsaSocketClose(ptTelnetSvr->m_hAcceptSock);
		ptTelnetSvr->m_hAcceptSock = INVALID_SOCKET;
	}

	if(ptTelnetSvr->m_hSaveAcceptSock != INVALID_SOCKET)
	{
		OsaSocketClose(ptTelnetSvr->m_hSaveAcceptSock);
		ptTelnetSvr->m_hSaveAcceptSock = INVALID_SOCKET;
	}

	if(ptTelnetSvr->m_hListenSock != INVALID_SOCKET)
	{
		OsaSocketClose(ptTelnetSvr->m_hListenSock);
		ptTelnetSvr->m_hListenSock = INVALID_SOCKET;
	}

	//ɾ��д��־�ļ�������
	OsaLightLockDelete( ptTelnetSvr->m_hLockLogFile );
	//ɾ��Lab����
	OsaLightLockDelete( ptTelnetSvr->m_hLockLabTable );

	OsaMFree(iOsaMemAllocGet(), ptTelnetSvr );

	return dwRet;
}

void help( )
{
	cmdshow();
}

//��ʾ��ע��ĸ�ģ���������
void cmdshow( )
{
	TCmdTable *ptCmdTable = NULL;
	UINT32 dwCurNum;
	UINT32 dwIdx;
	INT8 *pchHelpPos = NULL;
	INT8 *pchCmdName = NULL;
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();

	if(ptTelnetSvr == NULL)
	{		 
		printf("cmdshow(), ptTelnetSvr=0x%x\n", ptTelnetSvr);
		return;
	}

	ptCmdTable = &(ptTelnetSvr->m_tCmdTable);
	dwCurNum = ptCmdTable->m_dwCurCmdNum;

	if(dwCurNum > CMD_TABLE_SIZE)
	{
		OsaLabPrt( OSA_ERROCC, "cmdshow(), cmdtable is overflow, num is %d, tablesize is %d\n", dwCurNum, CMD_TABLE_SIZE);
		return;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "common cmd: lc, usage: ������еĴ�ӡ��ǩ\n" );
	OsaLabPrt( GENERAL_PRTLAB_OCC, "common cmd: fpon,   usage: �����ļ���ӡ\n" );
	OsaLabPrt( GENERAL_PRTLAB_OCC, "common cmd: fpoff,  usage: �ر��ļ���ӡ\n" );

	for(dwIdx = 0; dwIdx < dwCurNum; dwIdx++)
	{
		pchCmdName = ptCmdTable->m_atCmdInfo[dwIdx].m_achName;
		pchHelpPos = pchCmdName + strlen(pchCmdName) - 4;
		if(strcmp(pchHelpPos, "help") != 0) //ֻ��ӡģ���������
		{
			continue;
		}

		OsaLabPrt( GENERAL_PRTLAB_OCC, "cmd: %s, usage: %s", 
			ptCmdTable->m_atCmdInfo[dwIdx].m_achName, ptCmdTable->m_atCmdInfo[dwIdx].m_achUsage);
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "\r\n");

	return;	
}

/*===========================================================
���ܣ���telnet������ע���������
����˵����	szCmdName -- �����������ƣ�����Ϊ NULL, ��󳤶�Ϊ20
			pCmdFunc -- ���������ָ�� 
			szCmdUsage -- �ӿ���;������Ϊ NULL����󳤶�Ϊ80
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaCmdReg( const char* szCmdName, void* pfuncCmd/*TfuncCmdCB pfuncCmd*/, const char* szCmdUsage)
{
	UINT32 dwRet = 0;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	TCmdTable *ptCmdTable = NULL;
	UINT32 dwCurNum = 0;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( szCmdName, "szCmdName" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pfuncCmd, "pfuncCmd" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( szCmdUsage, "szCmdUsage" );

	ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptTelnetSvr, "ptTelnetSvr" );

	ptCmdTable = &(ptTelnetSvr->m_tCmdTable);
	dwCurNum = ptCmdTable->m_dwCurCmdNum;

	if(dwCurNum >= CMD_TABLE_SIZE)
	{
		OsaLabPrt( OSA_ERROCC, "CmdReg(), cmdtable is full, num is %d, tablesize is %d\n", dwCurNum, CMD_TABLE_SIZE);
		return FALSE;
	}

	strncpy(ptCmdTable->m_atCmdInfo[dwCurNum].m_achName, szCmdName, CMD_NAME_LENGTH);
	ptCmdTable->m_atCmdInfo[dwCurNum].m_achName[CMD_NAME_LENGTH] = '\0';

	strncpy(ptCmdTable->m_atCmdInfo[dwCurNum].m_achUsage, szCmdUsage, CMD_USAGE_LENGTH);
	ptCmdTable->m_atCmdInfo[dwCurNum].m_achUsage[CMD_USAGE_LENGTH] = '\0';

	ptCmdTable->m_atCmdInfo[dwCurNum].m_funcCmd = (UniFunc)pfuncCmd;
	ptCmdTable->m_dwCurCmdNum += 1;

	return dwRet;
}

//�����ǩ�ַ�����������8����ת��ΪUINT64
/*====================================================================
���ܣ������ǩ������OCCֵ��ת������������CtlStr2Occ��CtlOcc2Str
����˵����	szOccLabName - �����ǩ���ƣ����8���ַ���
			tOccVal �� Occ��ֵ���ɷ����ǩ�ַ���ת����������ֵ��
			dwInLen - �����ַ������С��dwInLen>8
			pdwOutLen - ���ת������ַ�����
����ֵ˵����
====================================================================*/
OCC OsaStr2Occ(const char *szOccLabName )
{
	OCC qwVal = 0;
	UINT32 dwIdx;
	UINT32 dwShitBits = 0;

	if(szOccLabName == (void *)0)
	{
		return 0;
	}

	dwIdx = 0;
	while(szOccLabName[dwIdx] != '\0')
	{
		dwShitBits = dwIdx * 8;
		qwVal += ((OCC)(UINT8)(szOccLabName[dwIdx]) << dwShitBits);
		dwIdx++;

		if(dwIdx >= 8) //���8��
		{
			break;
		}
	}	 

	return qwVal;
}

#define ONEBYTE_BASE 256
BOOL OsaOcc2Str(OCC tOccVal, IN OUT char *szOccLabName, UINT32 dwInLen )
{
	UINT64 qwVal = tOccVal;
	UINT64 qwQot; //����
	UINT8 byRsd; //����
	UINT32 dwIdx;

	if((szOccLabName == (void *)0) || (dwInLen <= 8))
	{
		return FALSE;
	}

	dwIdx = 0;
	do
	{
		byRsd = (UINT8)(qwVal%ONEBYTE_BASE);
		qwQot =	qwVal/ONEBYTE_BASE;

		szOccLabName[dwIdx] = (char)byRsd;
		dwIdx++;
		qwVal = qwQot;

	}while(qwQot > 0);

	szOccLabName[dwIdx] = '\0';
// 	*pdwOutLen = dwIdx + 1;
	return TRUE;	
}

/*====================================================================
���ܣ�64λ���ݱȽ�
�������˵����	pValA - ָ������A
				pValB - ָ������B
����ֵ˵��
====================================================================*/
static int sU64Cmp(const void *pValA, const void *pValB)
{
	UINT64 qwValA = *((UINT64 *)pValA);
	UINT64 qwValB = *((UINT64 *)pValB);

	if(qwValA > qwValB)
	{
		return 1;
	}
	else if(qwValA < qwValB)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

/*====================================================================
��������FindKeyInU64Array
���ܣ�64λ�������
�㷨ʵ�֣�		
����ȫ�ֱ�����
�������˵����	qwKey - �ؼ���	
				pqwBase - �����׵�ַ
				dwEleNum - Ԫ�ظ���
����ֵ˵��
====================================================================*/
static UINT64 *sFindKeyInU64Array(UINT64 qwKey, UINT64 *pqwBase, UINT32 dwEleNum)
{
	void *pRet = NULL;

	if((pqwBase == NULL) || (dwEleNum == 0))
	{
		return NULL;
	}

	pRet = bsearch(&qwKey, pqwBase, dwEleNum, U64_SIZE, sU64Cmp);	
	return (UINT64 *)pRet;
}

static void sOsaLogFileOpen( TTelnetSvrInfo *ptTelnetSvr, const char* szLogFileName, BOOL bIsCreateNew )
{
	OsaLightLockLock( ptTelnetSvr->m_hLockLogFile );
	if( TRUE == bIsCreateNew )
	{
		//�������ļ�
		ptTelnetSvr->m_pCurLogFile = fopen( szLogFileName, "w" );
		ptTelnetSvr->m_dwLogFileLen = 0;
	}
	else
	{
		//�ڽ���д�ļ�ʱ��Ҫ���ж��ļ��Ĵ�С����������ض�ֵ����Ҫ����д�����������д
		UINT32 dwLogFileLen = (UINT32)OsaFileLengthGet( szLogFileName );
		if (dwLogFileLen >= OSA_LOG_FILE_MAXSIZE)
		{
			ptTelnetSvr->m_pCurLogFile = fopen( szLogFileName, "w" );
		}
		else
		{
			//�����ļ�д
			ptTelnetSvr->m_pCurLogFile = fopen( szLogFileName, "a+" );
			if( ptTelnetSvr->m_pCurLogFile )
			{
				//�ƶ����ļ�ĩβ����д����־�ļ���ʼ��־
				fseek(ptTelnetSvr->m_pCurLogFile, 0, SEEK_END);
			}
		}		
		ptTelnetSvr->m_dwLogFileLen = (UINT32)OsaFileLengthGet( szLogFileName );
	}	
	OsaLightLockUnLock( ptTelnetSvr->m_hLockLogFile );	
}

static void sOsaLogFileClose( TTelnetSvrInfo *ptTelnetSvr )
{
	OsaLightLockLock( ptTelnetSvr->m_hLockLogFile );
	if( ptTelnetSvr->m_pCurLogFile != NULL )
	{
		fclose( ptTelnetSvr->m_pCurLogFile );
		ptTelnetSvr->m_pCurLogFile = NULL;
		ptTelnetSvr->m_dwLogFileLen = 0;
	}
	OsaLightLockUnLock( ptTelnetSvr->m_hLockLogFile );	
}

#define OSA_FILEPATH_MAXLEN 512
static void sOsaWriteLog2File( TTelnetSvrInfo *ptTelnetSvr, const char* szLogInfo )
{
	UINT32 dwLogLen = (UINT32)strlen( szLogInfo );
	if( !ptTelnetSvr->m_pCurLogFile )
	{
		return;
	}
	ptTelnetSvr->m_dwLogFileLen += dwLogLen;
	if( ptTelnetSvr->m_dwLogFileLen > OSA_LOG_FILE_MAXSIZE )
	{
		BOOL bIsFileExist = FALSE;
		char szLogFileName[OSA_FILEPATH_MAXLEN] = {0};
		const char* szLogFilePrefix = ptTelnetSvr->m_szLogFilePrefixName;
		UINT32 dwIdx = ptTelnetSvr->m_dwLogFileLoopIdx%2 + 1;
		sprintf(szLogFileName, "%s%d.log", szLogFilePrefix, dwIdx);
		//�����ļ�
		sOsaLogFileClose( ptTelnetSvr );
		bIsFileExist = OsaIsPathExist( szLogFileName );
		sOsaLogFileOpen( ptTelnetSvr, szLogFileName, !bIsFileExist );
		ptTelnetSvr->m_dwLogFileLoopIdx++;
	}

	OsaLightLockLock( ptTelnetSvr->m_hLockLogFile );
	if( ptTelnetSvr->m_pCurLogFile != NULL )
	{
		fwrite( szLogInfo, 1, dwLogLen, ptTelnetSvr->m_pCurLogFile );
		fflush( ptTelnetSvr->m_pCurLogFile );
	}
	OsaLightLockUnLock( ptTelnetSvr->m_hLockLogFile );	
}

static void sOsaWriteLogHead2File( TTelnetSvrInfo *ptTelnetSvr )
{
	char szLogHead[ 1024*10] = {0};
	struct tm *ptTm = NULL;
	time_t t;

	time(&t);
	ptTm = localtime(&t);

	sprintf(szLogHead,
		"\r\n\r\n\r\n\r\n\r\n************************ %s log start  %d-%d-%d %d:%d:%d************************\n",
		ptTelnetSvr->m_szLogFilePrefixName,
		ptTm->tm_year + 1900, 
		ptTm->tm_mon + 1,
		ptTm->tm_mday,
		ptTm->tm_hour,
		ptTm->tm_min,
		ptTm->tm_sec);

	sOsaWriteLog2File( ptTelnetSvr, szLogHead );
}

static void sOsaFilePrtOpen( TTelnetSvrInfo *ptTelnetSvr )
{
	UINT32 dwLogFileLen = 0;
	BOOL bIsFileExist = FALSE;
	BOOL bIsCreateNew = FALSE;
	const char* szLogFileName = NULL;
	char szLogFileName1[OSA_FILEPATH_MAXLEN] = {0};
	char szLogFileName2[OSA_FILEPATH_MAXLEN] = {0};
	const char* szLogFilePrefix = ptTelnetSvr->m_szLogFilePrefixName;
	sprintf(szLogFileName1, "%s1.log", szLogFilePrefix);
	sprintf(szLogFileName2, "%s2.log", szLogFilePrefix);

	if( FALSE == ptTelnetSvr->m_bInit || TRUE == ptTelnetSvr->m_bExit )
	{		
		return;
	}

	do 
	{
		UINT64 qwMdfTimeFile1 = 0;
		UINT64 qwMdfTimeFile2 = 0;
		//�ļ�1��������ڣ���ʹ���ļ�1
		bIsFileExist = OsaIsPathExist( szLogFileName1 );
		if( !bIsFileExist )
		{
			szLogFileName = szLogFileName1;
			bIsCreateNew = TRUE;
			ptTelnetSvr->m_dwLogFileLoopIdx = 1;
			break;
		}

		//�ļ�1��������ڣ���ʹ���ļ�2
		bIsFileExist = OsaIsPathExist( szLogFileName2 );
		if( !bIsFileExist )
		{
			szLogFileName = szLogFileName2;
			bIsCreateNew = TRUE;
			ptTelnetSvr->m_dwLogFileLoopIdx = 2;
			break;
		}

		//�����ļ������ڣ�ʹ���޸�ʱ�����µ��Ǹ�
		OsaFileMdfTimeGet( szLogFileName1, &qwMdfTimeFile1 );
		OsaFileMdfTimeGet( szLogFileName2, &qwMdfTimeFile2 );
		if( qwMdfTimeFile1 > qwMdfTimeFile2 )
		{
			dwLogFileLen = (UINT32)OsaFileLengthGet( szLogFileName1 );
			if (dwLogFileLen >= OSA_LOG_FILE_MAXSIZE)
			{
				szLogFileName = szLogFileName2;
				ptTelnetSvr->m_dwLogFileLoopIdx = 2;
			}
			else
			{
				szLogFileName = szLogFileName1;
				ptTelnetSvr->m_dwLogFileLoopIdx = 1;
			}
		}
		else
		{
			dwLogFileLen = (UINT32)OsaFileLengthGet( szLogFileName2 );
			if (dwLogFileLen >= OSA_LOG_FILE_MAXSIZE)
			{
				szLogFileName = szLogFileName1;
				ptTelnetSvr->m_dwLogFileLoopIdx = 1;
			}
			else
			{
				szLogFileName = szLogFileName2;
				ptTelnetSvr->m_dwLogFileLoopIdx = 2;
			}
		}
	} while (0);

	if( !szLogFileName )
	{
		return;
	}
	//����־�ļ�
	sOsaLogFileOpen( ptTelnetSvr, szLogFileName, bIsCreateNew );
	//д����־ͷ
	sOsaWriteLogHead2File( ptTelnetSvr );
}

static void sOsaFilePrtClose( TTelnetSvrInfo *ptTelnetSvr )
{
	if( FALSE == ptTelnetSvr->m_bInit || TRUE == ptTelnetSvr->m_bExit )
	{		
		return;
	}
	if( ptTelnetSvr->m_pCurLogFile )
	{
		sOsaLogFileClose( ptTelnetSvr );
	}
}


/*====================================================================
���ܣ��Զ�������ǩ��ӡ, (��ӡ��Ϣ�Ŀ�ͷ�Ӵ�ӡ���ƣ�
����˵����	hTelHdl - telnet���������
				qwPrtLab �� ��ӡ��ǩ��ʾ
				format - �ɱ����			
����ֵ˵��
====================================================================*/
void OsaLabPrt( OCC qwPrtLab,  const char *format, ...)
{
	INT8 chStr[MAX_PRTMSG_LEN + 1];
	INT8 chPrtStr[MAX_PRTMSG_LEN + OCCSTR_MAXLEN + TIMESTR_MAXLEN + 2];
	va_list ap; 
	UINT64 *pqwRet;
	UINT64 qwKey;
	TLabPrtTable *ptLabPrtTable;
	INT8 achPrtName[OCCSTR_MAXLEN + 1];
	INT8 *pchPrt = NULL;
	BOOL bIsLogLab = FALSE;
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();

	if(ptTelnetSvr == NULL)
	{
		printf("AccuPrt(), para err, ptTelnetSvr=0x%x\n", ptTelnetSvr);
		return;
	}

	//��������Telnet�����Ѿ��˳���
	if( (ptTelnetSvr->m_bInit == FALSE) || ptTelnetSvr->m_dwMagicNum != TELNET_MAGIC_NUM )
	{
		return;
	}

	qwKey = qwPrtLab;
	//�����ӡ���Ʋ��ڴ�ӡ���Ʊ��У��˳�
	ptLabPrtTable = &(ptTelnetSvr->m_tLabPrtTable);
	pqwRet = sFindKeyInU64Array(qwKey, ptLabPrtTable->m_aqwPrtLab, ptLabPrtTable->m_dwCurNum);
	if(pqwRet == NULL)
	{
		return;
	}

	va_start(ap, format); 	
#ifdef _MSC_VER
	_vsnprintf(chStr, MAX_PRTMSG_LEN, format, ap); 		
#else
	vsnprintf(chStr, MAX_PRTMSG_LEN, format, ap); 		
#endif		

	va_end(ap); 	

	chStr[MAX_PRTMSG_LEN] = '\0';
	pchPrt = chStr;


	if(qwPrtLab != GENERAL_PRTLAB_OCC) //�ٹ���һ�Σ�����ӡ, ʱ�����ƷŽ�ȥ
	{
		struct tm *ptTm = NULL;
		time_t t;
		OsaOcc2Str(qwKey, achPrtName, OCCSTR_MAXLEN + 1);

		time(&t);
		ptTm = localtime(&t);
		if( NULL != ptTm )
		{


#ifdef _MSC_VER
			_snprintf(chPrtStr, MAX_PRTMSG_LEN + OCCSTR_MAXLEN, "[%s][%d-%d %d:%d:%d]: %s",
				achPrtName, ptTm->tm_mon+1, ptTm->tm_mday,ptTm->tm_hour,ptTm->tm_min, ptTm->tm_sec, chStr);
#else
			snprintf(chPrtStr, MAX_PRTMSG_LEN + OCCSTR_MAXLEN, "[%s][%d-%d %d:%d:%d]: %s", 
				achPrtName, ptTm->tm_mon+1, ptTm->tm_mday,ptTm->tm_hour, ptTm->tm_min, ptTm->tm_sec, chStr);
#endif
		}
		else
		{
			strncpy( chPrtStr, chStr, MAX_PRTMSG_LEN );
		}

		chPrtStr[MAX_PRTMSG_LEN + OCCSTR_MAXLEN] = '\0';
		pchPrt = chPrtStr;

		//��ӡ����־�ļ���
		{
			char* pchErrPos = achPrtName + strlen(achPrtName) - 3;
			if(0 == strcmp(pchErrPos, "Err")
				|| 0 == strcmp(pchErrPos, "ERR")
				|| 0 == strcmp(pchErrPos, "err"))//ֻ��ӡ�������־
			{
				sOsaWriteLog2File( ptTelnetSvr, pchPrt );
			}
			else if( 0 == strcmp(pchErrPos, "Log")
				|| 0 == strcmp(pchErrPos, "LOG")
				|| 0 == strcmp(pchErrPos, "log"))
			{
				bIsLogLab = TRUE;
				sOsaWriteLog2File( ptTelnetSvr, pchPrt );
			}
		}
	}

	//�����telnet�ͻ��ˣ���ӡ���ͻ���
	if( (ptTelnetSvr->m_hListenSock != INVALID_SOCKET) && (ptTelnetSvr->m_hAcceptSock != INVALID_SOCKET)  )
	{
		if( FALSE == bIsLogLab )
		{
			sPrintToClient(pchPrt, ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort );
		}
	}
	else
	{
		if( FALSE == bIsLogLab )
		{
			printf("%s", pchPrt);
		}
	}

	return;
}

/*====================================================================
���ܣ����Ʊ�ǩ��ӡ�Ƿ񱣴浽�ļ���
����˵����	hTelHdl - telnet���������
			bSaveFile �� �Ƿ񱣴浽�ļ�		
����ֵ˵��
====================================================================*/
UINT32 OsaLabPrt2FileSet( BOOL bSaveFile )
{
	UINT32 dwRet = 0;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();

	ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptTelnetSvr, "ptTelnetSvr" );

	if( bSaveFile == ptTelnetSvr->m_bPrintFile )
	{
		return 0;
	}

	if( bSaveFile )
	{
		//�򿪱�����ļ�,��Ҫ����ѭ��д�����ļ���ÿ���ļ��̶���С
		sOsaFilePrtOpen( ptTelnetSvr );
	}
	else
	{
		//�رմ򿪵��ļ�
		sOsaFilePrtClose( ptTelnetSvr );
	}

	ptTelnetSvr->m_bPrintFile = bSaveFile;
	return dwRet;
}

//64λ��������
static void sU64ArraySort(UINT64 *pqwBase, UINT32 dwEleNum)
{
	if((pqwBase == NULL) || (dwEleNum == 0))
	{
		printf("U64ArraySort(), para err, pqwBase=0x%x, dwEleNum=%d\n", pqwBase, dwEleNum);
		return;
	}

	qsort((void *)pqwBase, dwEleNum, U64_SIZE, sU64Cmp);
	return;
}


/*====================================================================
���ܣ����Ӿ�ȷ��ӡ����
����˵����	hTelHdl - telnet���������
			qwPrtLab �� ��ӡ��ǩ��ʾ
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
====================================================================*/
UINT32 OsaPrtLabAdd( OCC qwPrtLab )
{
	UINT32 dwRet = 0;
// 	UINT64 qwVal;
	TLabPrtTable *ptLabPrtTable = NULL;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	
	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();

	ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptTelnetSvr, "ptTelnetSvr" );

	OsaLightLockLock(ptTelnetSvr->m_hLockLabTable);
	ptLabPrtTable = &(ptTelnetSvr->m_tLabPrtTable);
	if(ptLabPrtTable->m_dwCurNum >= LABPRT_NAME_MAXNUM)
	{
		dwRet = COMERR_ABILITY_LIMIT;
		iOsaLog(OSA_ERROCC, "OsaPrtLabAdd(), PrtTable name num(%d) > maxnum(%d)\n", 
			ptLabPrtTable->m_dwCurNum, LABPRT_NAME_MAXNUM);
	}
	else
	{
		//�������֣�������
		ptLabPrtTable->m_aqwPrtLab[ptLabPrtTable->m_dwCurNum] = qwPrtLab; 
		ptLabPrtTable->m_dwCurNum++;

		sU64ArraySort(ptLabPrtTable->m_aqwPrtLab, ptLabPrtTable->m_dwCurNum);	
	}
	OsaLightLockUnLock(ptTelnetSvr->m_hLockLabTable);
	return dwRet;
}


/*====================================================================
���ܣ�ɾ����ȷ��ӡ����
����˵����	hTelHdl - telnet���������
			qwPrtLab �� ��ӡ��ǩ��ʾ
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
====================================================================*/
UINT32 OsaPrtLabDel(  OCC qwPrtLab )
{
	UINT32 dwRet = 0;
	TLabPrtTable *ptLabPrtTable = NULL;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	UINT64 *pqwRet;
	UINT64 qwKey;
	UINT32 dwPos;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptTelnetSvr, "ptTelnetSvr" );


	OsaLightLockLock(ptTelnetSvr->m_hLockLabTable);

	do 
	{
		ptLabPrtTable = &(ptTelnetSvr->m_tLabPrtTable);
		if(ptLabPrtTable->m_dwCurNum == 0)
		{
			dwRet = COMERR_NOT_FOUND;
			iOsaLog(OSA_ERROCC, "OsaPrtLabDel(), PrtTable is empty��\n");
			break;
		}

		qwKey = qwPrtLab;
		pqwRet = sFindKeyInU64Array(qwKey, ptLabPrtTable->m_aqwPrtLab, ptLabPrtTable->m_dwCurNum);
		if(pqwRet == NULL)
		{
			INT8 achPrtName[OCCSTR_MAXLEN + 1];
			OsaOcc2Str(qwKey, achPrtName, OCCSTR_MAXLEN + 1);

			dwRet = COMERR_NOT_FOUND;
			iOsaLog(OSA_ERROCC, "OsaPrtLabDel(), %s not found\n", achPrtName );
			break;
		}

		//�ҵ���ֵ, ��������
#ifdef _MSC_VER
		dwPos = ((UINT64)pqwRet - (UINT64)(ptLabPrtTable->m_aqwPrtLab)) / U64_SIZE; //???
#else
		dwPos = ((UINT64)pqwRet - (UINT64)(ptLabPrtTable->m_aqwPrtLab)) / U64_SIZE;
#endif
		memmove(pqwRet, pqwRet + 1, (ptLabPrtTable->m_dwCurNum - dwPos - 1) * U64_SIZE);
		ptLabPrtTable->m_aqwPrtLab[ptLabPrtTable->m_dwCurNum-1] = 0;
		ptLabPrtTable->m_dwCurNum--;
	} while (0);

	OsaLightLockUnLock(ptTelnetSvr->m_hLockLabTable);
	return dwRet;
}	

/*====================================================================
���ܣ�������������ǩ�����ر����еĴ�ӡ��ʾ���
����˵����	hTelHdl - telnet���������
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
====================================================================*/
UINT32 OsaPrtLabClearAll(  )
{
	UINT32 dwRet = 0;
	TLabPrtTable *ptLabPrtTable = NULL;
	TTelnetSvrInfo *ptTelnetSvr = NULL;

	OSA_CHECK_INIT_RETURN_U32();
	ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptTelnetSvr, "ptTelnetSvr" );

	OsaLightLockLock(ptTelnetSvr->m_hLockLabTable);

	ptLabPrtTable = &(ptTelnetSvr->m_tLabPrtTable);
	memset(ptLabPrtTable, 0, sizeof(TLabPrtTable));

	OsaLightLockUnLock(ptTelnetSvr->m_hLockLabTable);
	return dwRet;
}

/*====================================================================
���ܣ��жϴ�ӡ�����Ƿ��Ѵ��ڴ�ӡ����
����˵����	hTelHdl - telnet���������
			qwPrtLab �� ��ӡ��ǩ��ʾ
����ֵ˵��: TRUE, ���ڣ�FALSE��������
====================================================================*/
BOOL OsaIsPrtLabExist( OCC qwPrtLab )
{
	UINT64 *pqwRet;
	UINT64 qwKey;
	TLabPrtTable *ptLabPrtTable;
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();
	if(ptTelnetSvr == NULL)
	{
		printf("OsaIsPrtLabExist(), para err, ptTelnetSvr=0x%x\n", ptTelnetSvr);
		return FALSE;
	}

	qwKey = qwPrtLab;

	OsaLightLockLock(ptTelnetSvr->m_hLockLabTable);

	ptLabPrtTable = &(ptTelnetSvr->m_tLabPrtTable);
	pqwRet = sFindKeyInU64Array(qwKey, ptLabPrtTable->m_aqwPrtLab, ptLabPrtTable->m_dwCurNum);

	OsaLightLockUnLock(ptTelnetSvr->m_hLockLabTable);

	if(pqwRet == NULL)
	{
		return FALSE;
	}

	return TRUE;
}
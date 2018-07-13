#include "osa_common.h"

//字符定义
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

#define  CMD_TABLE_SIZE			(UINT32)1024			//命令表长度	
#define  CMD_NAME_LENGTH		(UINT8)20			//命令名称长度 
#define  CMD_USAGE_LENGTH		(UINT8)80			//命令说明长度

#define  MAX_COMMAND_LENGTH		(UINT8)100			// telnet下命令最大长度, 包含参数

#define  MAX_PRTMSG_LEN			(UINT32)1024			//打印消息的最大长度
#define  MAX_PROMPT_LEN			(UINT8)20

#define	 TELNET_MAGIC_NUM		0xbeedbeef
#define  TASKWAIT_MS			2000  //等待任务结束的时间
#define SELECT_TIMEOUT			200 //select 超时时间

#define OCCSTR_MAXLEN	8	//OCC转为字符串的最大长度
#define TIMESTR_MAXLEN	14	//时间字符串的最大长度 [hh:mm:ss:xxx]
#define U64_SIZE		sizeof(UINT64)

#define OSA_LOG_FILE_MAXSIZE (10<<20)	//10M

static const char *g_pchTelnetPasswd = "minyuan2018";
//static const char *g_pchTelnetPasswd = "vda2013";

#ifdef _MSC_VER
typedef INT32 (* UniFunc)(INT32, INT32, INT32, INT32, INT32, INT32, INT32, INT32, INT32); //通用的命令函数
#else
typedef INT32 (* UniFunc)(INT64, INT64, INT64, INT64, INT64, INT64, INT64, INT64, INT64); //通用的命令函数
#endif

typedef struct tagCmdInfo //命令结构
{
	char	m_achName[CMD_NAME_LENGTH+1];	//命令名称
	UniFunc m_funcCmd;						//命令函数
	char	m_achUsage[CMD_USAGE_LENGTH+1];	//命令使用
}TCmdInfo;

typedef struct tagCmdTable //命令表
{
	UINT32 m_dwCurCmdNum;						//当前命令个数
	TCmdInfo m_atCmdInfo[CMD_TABLE_SIZE]; 
}TCmdTable;


typedef struct tagRawPara
{
	char *paraStr;
	BOOL bInQuote;
	BOOL bIsChar;
}TRawPara;	

//精确打印表
typedef struct tagLabPrtTable 
{
	UINT32 m_dwCurNum;
	OCC m_aqwPrtLab[LABPRT_NAME_MAXNUM];
}TLabPrtTable;

typedef struct tagTelnetSvrStatis 
{
	UINT32 m_dwConNum;
	UINT32 m_dwDisConNum;
	UINT32 m_dwLoginNum; //登录成功次数，即密码验证成功
}TTelnetSvrStatis;

typedef struct tagTelnetSvrInfo
{
	UINT32 m_dwMagicNum;
	UINT16 m_wPort;
	BOOL m_bInit;
	BOOL m_bExit;
	SOCKHANDLE m_hListenSock;		//服务端侦听Sock
	SOCKHANDLE m_hSaveAcceptSock;	//用于多个客户端连接上时，临时缓存前一个连接Sock
	SOCKHANDLE m_hAcceptSock;		//保存当前连接Sock
// 	UINT8	m_byPrtGate;
	TASKHANDLE m_hMsgPrcTask;
	TASKHANDLE m_hAcceptTask;
	TCmdTable  m_tCmdTable; //命令表

	TLabPrtTable m_tLabPrtTable; //打印表
	LockHandle m_hLockLabTable;	//用于保护精确打印名字列表

// 	BOOL m_bShowMs;  //是否显示毫秒
	TTelnetSvrStatis m_tStatis;
	BOOL m_bNeedPasswdVf;
	BOOL m_bBashOn; //是否直通
	BOOL m_bPrintFile; //是否输入到文件
	char m_szLogFilePrefixName[32];
	LockHandle m_hLockLogFile;	//用于保护多线程写文件
	FILE* m_pCurLogFile;	//打开的日志文件
	UINT32 m_dwLogFileLen;	//日志文件长度
	UINT32 m_dwLogFileLoopIdx;
}TTelnetSvrInfo;


// static void sTelnetClientMsgPrc( TelnetHandle hTelHdl );
// static void sTelnetAcceptPrc( TelnetHandle hTelHdl );
static void sCmdProc(TelnetHandle hTelHdl, char* pchCmd, UINT8 byCmdLen, SOCKHANDLE *ptAcceptSock, SOCKHANDLE *ptSaveAcceptSock);

//创建telnet的Tcp服务端
static UINT32 sTelnetTcpSvrCreate( UINT16 wStartPort, OUT UINT16 *pwRealPort, OUT SOCKHANDLE *phListenSock )
{
	UINT32 dwRet = 0;
	SOCKHANDLE tListenSock = 0;	
	UINT16 wSearchPort = 0;
	UINT16 wRealPort = 0;  //实际使用的端口
	BOOL bSockCreate = FALSE;
	do 
	{	
		//创建Tcp套接字
		dwRet = OsaTcpSocketCreate( &tListenSock );
		if( dwRet != OSA_OK )
		{
			break;
		}
		bSockCreate = TRUE;

		//绑定套接字端口
		dwRet = OsaSocketBind( tListenSock, wStartPort );
		if( dwRet != OSA_OK )
		{
			//绑定后续端口
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
	
		//侦听
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
功能：通知accept任务退出
输入参数说明：	dwTelHdl - telnet服务器句柄
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

	//创建Tcp套接字
	dwRet = OsaTcpSocketCreate( &hSock );
	if( dwRet != OSA_OK )
	{
		return dwRet;
	}

	//建链
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

//打印到客户端
static void sPrintToClient(const char* pchMsg, SOCKHANDLE hAcceptSock, UINT16 wTelSvrPort)
{
	INT32 nSndBytes;
	nSndBytes = OsaSocketSend(hAcceptSock, pchMsg, (UINT32)strlen(pchMsg)+1, 0 );	

	/* 主片发送一个回车符，从片不发，防止有两个回车符 */
// 	if(wTelSvrPort != TELNET_PORT_TARGET)
	{
		nSndBytes = OsaSocketSend(hAcceptSock, "\r", 1, 0);	
	}
	return;
}

//在Telnet上显示提示符.
static void sPromptShow(SOCKHANDLE hAcceptSock)
{
	char achPrompt[MAX_PROMPT_LEN+1] = ">";	
#ifdef _MSC_VER
	OsaSocketSend(hAcceptSock, achPrompt, (UINT32)strlen(achPrompt)+1, 0);
#endif
#ifdef _LINUX_
	OsaSocketSend(hAcceptSock, achPrompt, (UINT32)strlen(achPrompt)+1, MSG_NOSIGNAL);  //在Linux下send的最后一个参数要设置为MSG_NOSIGNAL 否则发送出错后有可能导致程序退出 add by yhq 20160302
#endif

	return;
}


//客户端密码验证
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
			//tTimeout 每次都要设，linux上在 select 后会修改 tTimeout 的值 ！！！
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

		/* 客户端关闭 */
		if(nRet == 0)
		{
			OsaSocketClose(ptTelnetSvr->m_hAcceptSock);
			ptTelnetSvr->m_hAcceptSock = INVALID_SOCKET;
			ptTelnetSvr->m_hSaveAcceptSock = INVALID_SOCKET;
			continue;
		}

		/* 本端关闭 */
		if(nRet <= SOCKET_ERROR)
		{
			OsaSleep(100);
			continue;
		}		

		switch(cmdChar)
		{
		case NEWLINE_CHAR:		/* 换行符, 有 13, 10 构成,所以长度减一 */

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
			/* 加入命令字符串 */
			if(byCmdLen < MAX_COMMAND_LENGTH)
			{				
				achCommand[byCmdLen++] = cmdChar;	
			}
			else
			{
				sPrintToClient("\nenter password:\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
				byCmdLen = 0;
			}

			/* 使光标后退，用一个空格擦除原字符，再使光标后退 */
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
功能：处理连接的任务
输入参数说明：	
====================================================================*/
void sTelnetAcceptPrc( void* pParam )
{
	INT32 iMode = 0;	
	struct sockaddr_in tAddrClient;
    INT32 nAddrLenIn = sizeof(tAddrClient);	
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)pParam;

	//参数检查
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

		//设置非阻塞
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

		/* 关闭原来的连接 */
		if(ptTelnetSvr->m_hSaveAcceptSock != INVALID_SOCKET) 
		{            
			OsaSocketClose(ptTelnetSvr->m_hSaveAcceptSock);
			ptTelnetSvr->m_hSaveAcceptSock = INVALID_SOCKET;
			ptTelnetSvr->m_tStatis.m_dwDisConNum++;
		}
		
		ptTelnetSvr->m_bNeedPasswdVf = TRUE;

		/* 输出提示符 */
		sPrintToClient("***************************************\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
		sPrintToClient("welcome to telnet server\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
		sPrintToClient("***************************************\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);

		sPrintToClient("\nenter password:\n", ptTelnetSvr->m_hAcceptSock, ptTelnetSvr->m_wPort);
	}
}

/*====================================================================
功能：处理客户端的消息
输入参数说明：            
返回值说明：无.
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
	//参数检查
	if(ptTelnetSvr == NULL)
	{
		printf("sTelnetClientMsgPrc(), ptTelnetSvr is null\n");
		return;
	}

    while(TRUE)
	{
		/* 退出本任务 */
		if(ptTelnetSvr->m_bExit == TRUE)
		{
			printf("TelnetClientMsgPrc Task exited\n");			
			OsaThreadSelfExit();
			break;
		}
		
		//密码验证
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

		/* 阻塞地接收用户输入 */
        /* 注意：此处用了recv是为了区分对端关闭与出现错误两种情况；
         * 而SockRecv将两者作为了一种情况处理，所以不能使用。
         */

		FD_ZERO(&tReadFd); 
		//tTimeout 每次都要设，linux上在 select 后会修改 tTimeout 的值 ！！！
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

		/* 客户端关闭 */
		if(nRet == 0)
		{
			OsaSocketClose(ptTelnetSvr->m_hAcceptSock);
			ptTelnetSvr->m_hAcceptSock = INVALID_SOCKET;
			ptTelnetSvr->m_hSaveAcceptSock = INVALID_SOCKET;
			continue;
		}

		/* 本端关闭 */
		if(nRet == SOCKET_ERROR)
		{
			OsaSleep(500);
			continue;
		}		

		/* 解析用户输入, 回显到Telnet客户端屏幕上, 对于命令给出适当的响应 */
		switch(cmdChar)
		{
		case CTRL_S:
			sPromptShow(ptTelnetSvr->m_hAcceptSock);
			break;

		case CTRL_R:
			sPromptShow(ptTelnetSvr->m_hAcceptSock);
			break;

		case NEWLINE_CHAR:		/* 换行符 */									
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
        
		case RETURN_CHAR:         // 回车符	
		case UP_ARROW:			  // 上箭头， "[A"
		case DOWN_ARROW:          // 下箭头
		case LEFT_ARROW:          // 左箭头
		case RIGHT_ARROW:         // 右箭头
			break;			
		case BACKSPACE_CHAR:         // 退格键
			
			if(byCmdLen <= 0)
			{
				continue;
			}
			
			byCmdLen--;			
			
			/* 使光标后退，用一个空格擦除原字符，再使光标后退 */
			
           // tmpChar[0] = BACKSPACE_CHAR;
			tmpChar[0] = BLANK_CHAR;
			tmpChar[1] = BACKSPACE_CHAR;
			tmpChar[2] = '\0';
			
            OsaSocketSend(ptTelnetSvr->m_hAcceptSock, tmpChar, sizeof(tmpChar), 0);
			
			break;
			
		case '/':            // 显示上一条命令, 放在default前,当 '/' 为命令(如 diskusage /)参数时,可以执行default
#if 1	
			if((byPreCmdSaved == 1) && (byCmdLen == 0)) //只有第一个字符才有效
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
			/* 加入命令字符串 */
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

//查询命令
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

//执行命令
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
//	INT8 achRealCmd[MAX_COMMAND_LENGTH + 1]; //内部真正调用的命令（加dwTelHdl);
	UniFunc cmdFunc;
	INT8 achBashCmd[MAX_COMMAND_LENGTH];
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)hTelHdl;
	
	if(ptTelnetSvr == NULL)
	{
		return;
	}

	//如果bash直通，保留原始的命令
	if(ptTelnetSvr->m_bBashOn)
	{
		memcpy(achBashCmd, szCmd, MAX_COMMAND_LENGTH);
		achBashCmd[MAX_COMMAND_LENGTH-1] = '\0';
	}
	

	memset(para, 0, sizeof(para));
	memset(atRawPara, 0, sizeof(TRawPara)*10);

	/* 解析参数、命令 */
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
			/* 如果本字符为有效字符，前一字符为NULL，表示旧单词结束，
			   新单词开始 */
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

	/* 先执行 byee 命令 */
    if ( strcmp("bye", cmd) == 0 )
    {
        OsaLabPrt( GENERAL_PRTLAB_OCC,"\n  bye......\n");
        OsaSleep(500);            // not reliable,
        OsaSocketClose(*ptAcceptSock);
        *ptAcceptSock = INVALID_SOCKET;
        *ptSaveAcceptSock = INVALID_SOCKET;
		return;
    }    


	//从注册的命令表中查询是否有该命令，有就执行

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

	//去头
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
功能：初始化telnet服务器
参数说明： 	wStartPort -- telnet服务端口，如果该端口被占用，再递增尝试1000个端口
			phTelHdl -- telnet服务器句柄		
返回值说明：成功返回CTL_OK，失败返回错误码
说明：实际使用时，大多数仅需要创建一个实例，由CtlInit创建，
	  上层通过CtlTelnetHdlGet得到句柄，上层不需在调用TelnetSvrInit()
===========================================================*/
UINT32 InnerOsaTelnetSvrInit( UINT16 wStartPort, char* szLogFilePrefixName, OUT TelnetHandle *phTelHdl )
{
 	SOCKHANDLE tListenSock = INVALID_SOCKET;
	UINT32 dwRet = 0;
	UINT16 wRealPort = 0;  //实际使用的端口
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

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();

	//参数检查
	if((phTelHdl == NULL) || (wStartPort == 0)) 
	{
		//考虑支持创建多个TelnetSvr，这里可以用当前Telnet打印
		iOsaLog( OSA_ERROCC, "TelnetSvrInit(), pdwTelHdl=0x%x, wStartPort=%d, return\n", phTelHdl, wStartPort);
		return COMERR_INVALID_PARAM;
	}

	do 
	{
		//创建telnet的Tcp服务端
		dwRet = sTelnetTcpSvrCreate( wStartPort, &wRealPort, &tListenSock );
		if( dwRet != OSA_OK )
		{
			break;
		}	
		bTcpSvrCreate = TRUE;

		//分配句柄并赋值
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

		//创建Lab命令锁
		dwRet = OsaLightLockCreate(&(ptTelnetSvr->m_hLockLabTable));
		if( dwRet != OSA_OK )
		{
			break;
		}	
		bLabTableLockCreate = TRUE;

		//创建写日志文件互斥锁
		dwRet = OsaLightLockCreate(&(ptTelnetSvr->m_hLockLogFile));
		if( dwRet != OSA_OK )
		{
			break;
		}	
		bLogFileLockCreate = TRUE;
		
		/* 创建与客户端通信的任务 */ 
		dwRet = OsaThreadCreate( sTelnetClientMsgPrc, "TelnetClientMsgPrcTask", OSA_THREAD_PRI_NORMAL,
								 dwMsgPrcStackSize, (void*)ptTelnetSvr, FALSE, &hMsgPrcTask );
		if( dwRet != OSA_OK || hMsgPrcTask == TASK_NULL)
		{
			iOsaLog( OSA_ERROCC, "telnetsvr: TelnetSvrInit(), TelnetClientMsgPrcTask create failed\n");
			break;
		}

		/* 处理连接的任务 */
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
功能：退出telnet服务器
参数说明： 	hTelHdl -- telnet服务器句柄
返回值说明：成功返回CTL_OK，失败返回错误码
===========================================================*/
UINT32 InnerOsaTelnetSvrExit( TelnetHandle hTelHdl )
{
	UINT32 dwRet = 0;
	TTelnetSvrInfo *ptTelnetSvr = (TTelnetSvrInfo *)hTelHdl;
	//参数检查
	if(ptTelnetSvr == NULL)
	{
		printf("InnerOsaTelnetSvrExit(), ptTelnetSvr is null\n");
		return COMERR_NULL_POINTER;
	}

	//幻数检查
	if( ptTelnetSvr->m_dwMagicNum != TELNET_MAGIC_NUM)
	{
		dwRet = COMERR_MEMERR;
		printf("InnerOsaTelnetSvrExit(), TelnetHandle memory error, magic:0x%x\n", ptTelnetSvr->m_dwMagicNum );
		return dwRet;
	}

	//退出线程
	{
		//设置退出标志
		ptTelnetSvr->m_bExit = TRUE;

		//解除 accept 阻塞
		sTelnetSvrAcceptNtf( ptTelnetSvr );

		//等待任务结束 ???
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

	//删除写日志文件互斥锁
	OsaLightLockDelete( ptTelnetSvr->m_hLockLogFile );
	//删除Lab表锁
	OsaLightLockDelete( ptTelnetSvr->m_hLockLabTable );

	OsaMFree(iOsaMemAllocGet(), ptTelnetSvr );

	return dwRet;
}

void help( )
{
	cmdshow();
}

//显示已注册的各模块帮助命令
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

	OsaLabPrt( GENERAL_PRTLAB_OCC, "common cmd: lc, usage: 清除所有的打印标签\n" );
	OsaLabPrt( GENERAL_PRTLAB_OCC, "common cmd: fpon,   usage: 开启文件打印\n" );
	OsaLabPrt( GENERAL_PRTLAB_OCC, "common cmd: fpoff,  usage: 关闭文件打印\n" );

	for(dwIdx = 0; dwIdx < dwCurNum; dwIdx++)
	{
		pchCmdName = ptCmdTable->m_atCmdInfo[dwIdx].m_achName;
		pchHelpPos = pchCmdName + strlen(pchCmdName) - 4;
		if(strcmp(pchHelpPos, "help") != 0) //只打印模块帮助命令
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
功能：向telnet服务器注册调试命令
参数说明：	szCmdName -- 调试命令名称，可以为 NULL, 最大长度为20
			pCmdFunc -- 调试命令函数指针 
			szCmdUsage -- 接口用途，可以为 NULL，最大长度为80
返回值说明：成功返回OSA_OK，失败返回错误码
===========================================================*/
UINT32 OsaCmdReg( const char* szCmdName, void* pfuncCmd/*TfuncCmdCB pfuncCmd*/, const char* szCmdUsage)
{
	UINT32 dwRet = 0;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	TCmdTable *ptCmdTable = NULL;
	UINT32 dwCurNum = 0;

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
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

//分类标签字符串（不超过8个）转换为UINT64
/*====================================================================
功能：分类标签名称与OCC值的转换函数，包括CtlStr2Occ和CtlOcc2Str
参数说明：	szOccLabName - 分类标签名称（最多8个字符）
			tOccVal － Occ数值（由分类标签字符串转换过来的数值）
			dwInLen - 输入字符缓存大小，dwInLen>8
			pdwOutLen - 输出转换后的字符个数
返回值说明：
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

		if(dwIdx >= 8) //最多8个
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
	UINT64 qwQot; //商数
	UINT8 byRsd; //余数
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
功能：64位数据比较
输入参数说明：	pValA - 指向数据A
				pValB - 指向数据B
返回值说明
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
函数名：FindKeyInU64Array
功能：64位数组查找
算法实现：		
引用全局变量：
输入参数说明：	qwKey - 关键字	
				pqwBase - 数组首地址
				dwEleNum - 元素个数
返回值说明
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
		//创建新文件
		ptTelnetSvr->m_pCurLogFile = fopen( szLogFileName, "w" );
		ptTelnetSvr->m_dwLogFileLen = 0;
	}
	else
	{
		//在交替写文件时需要先判断文件的大小，如果大于特定值，需要重新写，否则接着着写
		UINT32 dwLogFileLen = (UINT32)OsaFileLengthGet( szLogFileName );
		if (dwLogFileLen >= OSA_LOG_FILE_MAXSIZE)
		{
			ptTelnetSvr->m_pCurLogFile = fopen( szLogFileName, "w" );
		}
		else
		{
			//接着文件写
			ptTelnetSvr->m_pCurLogFile = fopen( szLogFileName, "a+" );
			if( ptTelnetSvr->m_pCurLogFile )
			{
				//移动到文件末尾，并写下日志文件开始标志
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
		//更换文件
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
		//文件1如果不存在，就使用文件1
		bIsFileExist = OsaIsPathExist( szLogFileName1 );
		if( !bIsFileExist )
		{
			szLogFileName = szLogFileName1;
			bIsCreateNew = TRUE;
			ptTelnetSvr->m_dwLogFileLoopIdx = 1;
			break;
		}

		//文件1如果不存在，就使用文件2
		bIsFileExist = OsaIsPathExist( szLogFileName2 );
		if( !bIsFileExist )
		{
			szLogFileName = szLogFileName2;
			bIsCreateNew = TRUE;
			ptTelnetSvr->m_dwLogFileLoopIdx = 2;
			break;
		}

		//两个文件都存在，使用修改时间最新的那个
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
	//打开日志文件
	sOsaLogFileOpen( ptTelnetSvr, szLogFileName, bIsCreateNew );
	//写上日志头
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
功能：自定义分类标签打印, (打印信息的开头加打印名称）
参数说明：	hTelHdl - telnet服务器句柄
				qwPrtLab － 打印标签标示
				format - 可变参数			
返回值说明
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

	//检查幻数，Telnet可能已经退出了
	if( (ptTelnetSvr->m_bInit == FALSE) || ptTelnetSvr->m_dwMagicNum != TELNET_MAGIC_NUM )
	{
		return;
	}

	qwKey = qwPrtLab;
	//如果打印名称不在打印名称表中，退出
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


	if(qwPrtLab != GENERAL_PRTLAB_OCC) //再构建一次，将打印, 时间名称放进去
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

		//打印到日志文件中
		{
			char* pchErrPos = achPrtName + strlen(achPrtName) - 3;
			if(0 == strcmp(pchErrPos, "Err")
				|| 0 == strcmp(pchErrPos, "ERR")
				|| 0 == strcmp(pchErrPos, "err"))//只打印错误和日志
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

	//如果有telnet客户端，打印到客户端
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
功能：控制标签打印是否保存到文件中
参数说明：	hTelHdl - telnet服务器句柄
			bSaveFile － 是否保存到文件		
返回值说明
====================================================================*/
UINT32 OsaLabPrt2FileSet( BOOL bSaveFile )
{
	UINT32 dwRet = 0;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();

	ptTelnetSvr = (TTelnetSvrInfo *)OsaTelnetHdlGet();
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptTelnetSvr, "ptTelnetSvr" );

	if( bSaveFile == ptTelnetSvr->m_bPrintFile )
	{
		return 0;
	}

	if( bSaveFile )
	{
		//打开保存的文件,需要考虑循环写两个文件，每个文件固定大小
		sOsaFilePrtOpen( ptTelnetSvr );
	}
	else
	{
		//关闭打开的文件
		sOsaFilePrtClose( ptTelnetSvr );
	}

	ptTelnetSvr->m_bPrintFile = bSaveFile;
	return dwRet;
}

//64位数组排序
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
功能：增加精确打印名字
参数说明：	hTelHdl - telnet服务器句柄
			qwPrtLab － 打印标签标示
返回值说明：成功返回OSA_OK，失败返回错误码
====================================================================*/
UINT32 OsaPrtLabAdd( OCC qwPrtLab )
{
	UINT32 dwRet = 0;
// 	UINT64 qwVal;
	TLabPrtTable *ptLabPrtTable = NULL;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	
	//初始化检查
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
		//加入名字，并排序
		ptLabPrtTable->m_aqwPrtLab[ptLabPrtTable->m_dwCurNum] = qwPrtLab; 
		ptLabPrtTable->m_dwCurNum++;

		sU64ArraySort(ptLabPrtTable->m_aqwPrtLab, ptLabPrtTable->m_dwCurNum);	
	}
	OsaLightLockUnLock(ptTelnetSvr->m_hLockLabTable);
	return dwRet;
}


/*====================================================================
功能：删除精确打印名字
参数说明：	hTelHdl - telnet服务器句柄
			qwPrtLab － 打印标签标示
返回值说明：成功返回OSA_OK，失败返回错误码
====================================================================*/
UINT32 OsaPrtLabDel(  OCC qwPrtLab )
{
	UINT32 dwRet = 0;
	TLabPrtTable *ptLabPrtTable = NULL;
	TTelnetSvrInfo *ptTelnetSvr = NULL;
	UINT64 *pqwRet;
	UINT64 qwKey;
	UINT32 dwPos;

	//初始化检查
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
			iOsaLog(OSA_ERROCC, "OsaPrtLabDel(), PrtTable is empty！\n");
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

		//找到该值, 调整数组
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
功能：清除索引分类标签，将关闭所有的打印显示输出
参数说明：	hTelHdl - telnet服务器句柄
返回值说明：成功返回OSA_OK，失败返回错误码
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
功能：判断打印名字是否已存在打印表中
参数说明：	hTelHdl - telnet服务器句柄
			qwPrtLab － 打印标签标示
返回值说明: TRUE, 存在；FALSE，不存在
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
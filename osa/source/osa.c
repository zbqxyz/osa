#include "osa_common.h"

/*====================================================================
功能：Osa初始化
参数说明：无
返回值说明：成功返回OSA_OK，失败返回错误码
====================================================================*/
UINT32 OsaInit( TOsaInitParam tInitParam )
{
	UINT32 dwRet = 0;
	UINT16 wTelnetPort = OSA_DEFAULT_TELNET_PORT;
	TOsaMgr* ptMgr = iOsaMgrGet();
	if( ptMgr->m_bIsInit )
	{
		return COMERR_ALREADY_INIT;
	}

	memset( ptMgr, 0, sizeof(TOsaMgr) );

	if( tInitParam.wTelnetPort != 0 )
	{
		wTelnetPort = tInitParam.wTelnetPort;
	}

	do 
	{
		//初始化信号量监控链表
		EListInit(&ptMgr->m_tSemWatchList);
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), EListInit failed\n");
			break;
		}

		//创建用于保护信号量监控链表(LockHandle需要被监控，而监控这不行再被监控，这里用InnerLockHandle )
		iInnerLightLockCreate( &ptMgr->m_hSemWatchListLock );

		//初始化任务列表
		dwRet = EListInit( &ptMgr->m_tTaskList );
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), EListInit failed\n");
			break;
		}
		//创建用于保护任务列表的锁
		dwRet = OsaLightLockCreate( &ptMgr->m_hTaskListLock );
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), OsaLightLockCreate failed, ret=0x%x\n", dwRet);
			break;
		}

		//初始化内存库
		dwRet = iMemLibInit( );
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), iMemLibInit failed, ret=0x%x\n", dwRet);
			break;
		}

		//初始化套接口库，用于Telnet
		dwRet = OsaSocketInit();
		if( dwRet != OSA_OK )
		{
			break;
		}

	} while (0);

	if( OSA_OK == dwRet )
	{
		//运行到这里，就标志初始化成功了，后续创建Tel服务会用的多个内部功能，会检查初始化标志
		ptMgr->m_bIsInit = TRUE;
		ptMgr->m_tInitParam = tInitParam;

		do 
		{
			//申请Osa内存资源标签，
			dwRet = OsaMRsrcTagAlloc(OSA_MRSRC_TAG, (UINT32)strlen(OSA_MRSRC_TAG), &ptMgr->m_dwRsrcTag);
			if( dwRet != OSA_OK )
			{
				printf("OsaInit(), OsaMRsrcTagAlloc failed, ret=0x%x\n", dwRet);
				break;
			}	
			//创建内存分配器，供Osa内部使用
			dwRet = OsaMAllocCreate( 0, 0, &ptMgr->m_hMemAlloc );
			if( dwRet != OSA_OK || NULL == ptMgr->m_hMemAlloc )
			{
				printf("OsaInit(), OsaMAllocCreate failed, ret=0x%x\n", dwRet);
				break;
			}

			//创建telnet服务, 并注册调试命令
			dwRet = InnerOsaTelnetSvrInit( wTelnetPort, tInitParam.szLogFilePrefixName, &ptMgr->m_hTelHdl );
			if( dwRet != OSA_OK )
			{
				printf("OsaInit(), OsaTelnetSvrInit failed, port is %d, ret=0x%x\n", wTelnetPort, dwRet );
			}
			else
			{
				//本模块调试Telnet初始化
				OsaTelnetHdlGet();
				iOsaTelnetInit();
			}
		} while (0);
	}

	if( dwRet != 0 )
	{
		ptMgr->m_bIsInit = FALSE;
	}
	else
	{
		//开启日志打印
		fpon();
		//调试打印
		iOsaLog(OSA_MSGOCC, "OsaInit(), success.\n");
	}

	return dwRet;
}

/*====================================================================
功能：Osa退出
参数说明：无
返回值说明：成功返回OSA_OK，失败返回错误码
====================================================================*/
UINT32 OsaExit(void)
{
	UINT32 dwRet = 0;
	TelnetHandle hTelHdl = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//关闭日志打印
	fpoff();
	hTelHdl = ptMgr->m_hTelHdl;

	//退出Telnet后，句柄就无效了，先设置为NULL，内部打印将自动切换到printf，
	//同时确保退出Telnet退出后不再使用telnet，否则可能会有异常
	ptMgr->m_hTelHdl = NULL; 
	//退出telnet服务器
	InnerOsaTelnetSvrExit( hTelHdl );
	OsaSocketExit( );

	//释放内存分配器
	OsaMAllocDestroy( ptMgr->m_hMemAlloc );
	//退出内存库
	iMemLibExit( );

	//删除用于保护任务列表的锁
	OsaLightLockDelete( ptMgr->m_hTaskListLock );
	//删除用于保护任务列表的锁
	iInnerLightLockDelete( &ptMgr->m_hSemWatchListLock );

	//清空所有数据
	memset( ptMgr, 0, sizeof(TOsaMgr) );
	return OSA_OK;
}


char *FCC2STR(FCC tFccVal)
{
	UINT64 qwVal = tFccVal;
	UINT64 qwQot; //商数
	UINT8 byRsd; //余数
	UINT32 dwIdx;
	static char achFcc[5];


	dwIdx = 0;
	do
	{
		byRsd = (UINT8)(qwVal%ONEBYTE_BASE);
		qwQot =	qwVal/ONEBYTE_BASE;

		achFcc[dwIdx] = (char)byRsd;
		dwIdx++;
		qwVal = qwQot;

	}while(qwQot > 0);

	achFcc[4] = '\0';
	return achFcc;	
}

/*=================================================================
功    能: 得到Osa模块初始化创建的telnet服务器
参数说明: 
返回值:	 成功返回telnet服务器句柄，失败返回NULL
=================================================================*/
TelnetHandle OsaTelnetHdlGet(void)
{
//	UINT32 dwRet = 0;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//初始化检查
	if (FALSE == iIsOsaInit() )
	{
		return NULL;
	}

	return ptMgr->m_hTelHdl;
}

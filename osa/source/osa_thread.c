#include "osa_common.h"

#define TASKWAIT_MAXMS	2000 //创建任务时的最大同步时间

#ifdef _MSC_VER
#define  STACKSIZE_MIN   (UINT32)(16<<10)
#else
#define  STACKSIZE_MIN	PTHREAD_STACK_MIN
#endif
#define  STACKSIZE_MAX   (UINT32)(1<<20)


static BOOL g_bTaskNodeAdd; //是否已加入链表，用于同步
static BOOL g_bTaskNodeAddAck;

typedef struct tagDoTaskPara
{
	TfuncThreadEntry m_pfUserTaskFunc;	//用户的任务入口函数
	void *pUserParam;					//用户的任务入口函数输入参数	
	UINT32 m_dwStackSize;				//堆栈大小
}TDoTaskPara;

//////////////////////////////////////////////////////////////////////////
//线程链表管理

/*====================================================================
功能：增加节点到线程链表
输入参数说明：	pfTaskEntry: 线程入口，
				szName: 线程名，以NULL结束的字符串，
				byPriority: 线程优先级别，
				dwTaskID : 线程id
				dwSetStackSize: 线程堆栈大小

返回值说明：成功返回TRUE，失败返回FALSE
====================================================================*/
static BOOL sThreadListAddNode(TASKHANDLE hTaskHandle, TfuncThreadEntry pfTaskEntry, const char *szName,
	UINT8 byPriority, ULONG ulTaskID, UINT32 dwSetStackSize )
{
	UINT32 dwMaxNum = 0;
	TOsaThreadInfo *ptThreadInfo = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	ptThreadInfo = (TOsaThreadInfo *)OsaPoolMalloc( iOsaMemAllocGet(), sizeof(TOsaThreadInfo), iOsaMRsrcTagGet() );
	OSA_CHECK_NULL_POINTER_RETURN_BOOL( ptThreadInfo, "ptThreadInfo" );
	memset(ptThreadInfo, 0, sizeof(TOsaThreadInfo));

	OsaLightLockLock (ptMgr->m_hTaskListLock );
	
	ptThreadInfo->m_hTaskHandle = hTaskHandle;
	ptThreadInfo->m_byPri = byPriority;
	ptThreadInfo->m_pfTaskEntry = pfTaskEntry;
	ptThreadInfo->m_dwStackSize = dwSetStackSize;
	ptThreadInfo->m_ulTaskId = ulTaskID;

	if(szName != NULL)
	{
		strncpy(ptThreadInfo->m_achName, szName, TASKNAME_MAXLEN);
	}	
	ptThreadInfo->m_achName[TASKNAME_MAXLEN-1] = '\0';
	
	EListInsertLast(&ptMgr->m_tTaskList, &(ptThreadInfo->m_tNode));

	dwMaxNum = EListSize(&ptMgr->m_tTaskList);
	iOsaLog(OSA_LOGOCC, "%s() success, Name:%s, total tasknum is %d,\n",
		__FUNCTION__, szName, dwMaxNum);
	OsaLightLockUnLock( ptMgr->m_hTaskListLock );
	return TRUE;
}

/*====================================================================
功能：删除线程链表中相应节点
输入参数说明：dwTaskID : 线程id
返回值说明：成功返回线程的句柄，失败返回NULL.
====================================================================*/
static BOOL sThreadListDelNode( ULONG ulTaskID )
{
	BOOL bRet = FALSE;
	ENode *ptNode = NULL;
	TOsaThreadInfo *ptThreadInfo = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//处理已经退出的情况
	if( !ptMgr || !ptMgr->m_bIsInit )
	{
		return TRUE;
	}

	OsaLightLockLock( ptMgr->m_hTaskListLock );

	ptNode = EListGetFirst( &ptMgr->m_tTaskList );

	//while(ptNode != NULL)
	while(ptNode != NULL)
	{
		ptThreadInfo = HOST_ENTRY(ptNode, TOsaThreadInfo, m_tNode);

		if(ptThreadInfo->m_ulTaskId == ulTaskID)
		{
			iOsaLog(OSA_LOGOCC, "%s() Name:%s,\n", __FUNCTION__, ptThreadInfo->m_achName );
			EListRemove(&ptMgr->m_tTaskList, ptNode);
			memset(ptThreadInfo, 0, sizeof(TOsaThreadInfo));
			OsaMFree(iOsaMemAllocGet(), ptThreadInfo);
			bRet = TRUE;
			break;
		}

		ptNode = EListNext(&ptMgr->m_tTaskList, ptNode);		
	}
	OsaLightLockUnLock( ptMgr->m_hTaskListLock );
	return bRet;
}

static BOOL sThreadListDelNodeByHandle( TASKHANDLE hThread )
{
	BOOL bRet = FALSE;
	ENode *ptNode = NULL;
	TOsaThreadInfo *ptThreadInfo = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	OsaLightLockLock( ptMgr->m_hTaskListLock );

	ptNode = EListGetFirst( &ptMgr->m_tTaskList );

	//while(ptNode != NULL)
	while(ptNode != NULL)
	{
		ptThreadInfo = HOST_ENTRY(ptNode, TOsaThreadInfo, m_tNode);

		if(ptThreadInfo->m_hTaskHandle == hThread)
		{
			EListRemove(&ptMgr->m_tTaskList, ptNode);
			memset(ptThreadInfo, 0, sizeof(TOsaThreadInfo));
			OsaMFree(iOsaMemAllocGet(), ptThreadInfo);
			bRet = TRUE;
			break;
		}

		ptNode = EListNext(&ptMgr->m_tTaskList, ptNode);		
	}
	OsaLightLockUnLock( ptMgr->m_hTaskListLock );
	return bRet;
}

//通过线程ID查找，获取线程句柄
static BOOL sThreadHandleGetByID( ULONG ulTaskID, OUT TASKHANDLE *phThread )
{
	BOOL bRet = FALSE;
	ENode *ptNode = NULL;
	TOsaThreadInfo *ptThreadInfo = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	if( !phThread )
	{
		return FALSE;
	}

	OsaLightLockLock( ptMgr->m_hTaskListLock );

	ptNode = EListGetFirst( &ptMgr->m_tTaskList );

	while(ptNode != NULL)
	{
		ptThreadInfo = HOST_ENTRY(ptNode, TOsaThreadInfo, m_tNode);

		if( ulTaskID == ptThreadInfo->m_ulTaskId )
		{
			*phThread = ptThreadInfo->m_hTaskHandle;
			bRet = TRUE;
			break;
		}

		ptNode = EListNext(&ptMgr->m_tTaskList, ptNode);		
	}
	OsaLightLockUnLock( ptMgr->m_hTaskListLock );
	return bRet;
}

/*====================================================================
功能：获得调用线程的ID
输入参数说明：
====================================================================*/
ULONG iOsaThreadSelfID(void)
{
#ifdef _MSC_VER
	return GetCurrentThreadId();
#endif
	
#ifdef _LINUX_
	TASKHANDLE hTask;

	hTask = (TASKHANDLE)pthread_self();
	return (ULONG)hTask;
#endif
}

#ifdef _MSC_VER
//对用户线程入口函数的封装，这样便于线程的监控
static void do_task(void* pPara)
{
	TDoTaskPara *ptDoTaskPara = (TDoTaskPara *)pPara;
	TfuncThreadEntry pfUserTaskFunc = NULL;
	void* pUserPara = 0;
	UINT32 dwStackSize = 0;
	UINT32 dwBaseAddr = 0;
	BOOL bStackGrowLow = TRUE;

	OSA_CHECK_NULL_POINTER_RETURN_VOID( ptDoTaskPara, "ptDoTaskPara" );

	pfUserTaskFunc = ptDoTaskPara->m_pfUserTaskFunc;
	OSA_CHECK_NULL_POINTER_RETURN_VOID(pfUserTaskFunc, "pfUserTaskFunc" );

	pUserPara = ptDoTaskPara->pUserParam;
	dwStackSize = ptDoTaskPara->m_dwStackSize;

	//释放内存
	OsaMFree(iOsaMemAllocGet(), ptDoTaskPara );

	//得到进程号(对windows设非0值) 
// #ifdef _LINUX_
// 	g_dwPID = getpid();
// #else
// 	g_dwPID = 1;
// #endif	

	g_bTaskNodeAddAck = FALSE;
	//保证用户函数运行时线程节点已加入链表
	OSA_COND_WAIT(g_bTaskNodeAdd, TASKWAIT_MAXMS);
	g_bTaskNodeAddAck = TRUE;

	//用户线程入口
	pfUserTaskFunc( pUserPara );

	//从线程链表中删除
	sThreadListDelNode( iOsaThreadSelfID() );
	return;
}
#else
static void* do_task(void* dwPara)
{
	TDoTaskPara *ptDoTaskPara = (TDoTaskPara *)dwPara;
	TfuncThreadEntry pfUserTaskFunc = NULL;
	void* pUserPara = 0;
	UINT32 dwStackSize = 0;
// 	UINT32 dwBaseAddr = 0;
// 	BOOL bStackGrowLow = TRUE;

	OSA_CHECK_NULL_POINTER_RETURN_POINT( ptDoTaskPara, "ptDoTaskPara" );

	pfUserTaskFunc = ptDoTaskPara->m_pfUserTaskFunc;
	OSA_CHECK_NULL_POINTER_RETURN_POINT(pfUserTaskFunc, "pfUserTaskFunc" );

	pUserPara = ptDoTaskPara->pUserParam;
	dwStackSize = ptDoTaskPara->m_dwStackSize;

	//释放内存
	OsaMFree(iOsaMemAllocGet(), ptDoTaskPara );

	//得到进程号(对windows设非0值) 
	// #ifdef _LINUX_
	// 	g_dwPID = getpid();
	// #else
	// 	g_dwPID = 1;
	// #endif	

	g_bTaskNodeAddAck = FALSE;
	//保证用户函数运行时线程节点已加入链表
	OSA_COND_WAIT(g_bTaskNodeAdd, TASKWAIT_MAXMS);
	g_bTaskNodeAddAck = TRUE;

	//用户线程入口
	pfUserTaskFunc( pUserPara );

	//从线程链表中删除
	sThreadListDelNode( iOsaThreadSelfID() );
	return NULL;
}
#endif

#ifdef _MSC_VER
static TASKHANDLE sWin_ThreadCreate( TDoTaskPara *ptDoTaskPara, UINT8 byPriority, UINT32 dwStacksize, 
								   BOOL bSuspendThread, OUT ULONG *pulTaskID )
{
	TASKHANDLE  hTask = NULL;
	DWORD lTaskID = 0;
	int nPriority = THREAD_PRIORITY_NORMAL;
	UINT32 dwFlag = 0;

	if(byPriority == OSA_THREAD_PRI_CRITICAL)
	{
		nPriority = THREAD_PRIORITY_TIME_CRITICAL;
	}
	else if(byPriority == OSA_THREAD_PRI_HIGHEST)
	{
		nPriority = THREAD_PRIORITY_HIGHEST;
	}
	else if(byPriority == OSA_THREAD_PRI_ABOVE_NORMAL)
	{
		nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
	}
	else if(byPriority == OSA_THREAD_PRI_NORMAL)
	{
		nPriority = THREAD_PRIORITY_NORMAL;
	}
	else if(byPriority == OSA_THREAD_PRI_BELOW_NORMAL)
	{
		nPriority = THREAD_PRIORITY_BELOW_NORMAL;		
	}	
	else
	{
		nPriority = THREAD_PRIORITY_LOWEST;
	}

	dwFlag = (bSuspendThread) ? CREATE_SUSPENDED : 0;
	hTask = CreateThread(NULL, dwStacksize, (LPTHREAD_START_ROUTINE)do_task,
						 (void * )ptDoTaskPara, dwFlag, &lTaskID );
	if(hTask != NULL)
	{
		SetThreadPriority(hTask, nPriority);
		*pulTaskID = lTaskID;
	}
	else
	{
		iOsaLog(OSA_ERROCC, "CreateThread failed, errno=%d\n", GetLastError());
	}
	return hTask;
}

static UINT32 sWin_ThreadPrioritySet(TASKHANDLE hThread, UINT8 byPriority)
{
	UINT32 dwRet = 0;
	BOOL bRet = FALSE;
	int nPriority = THREAD_PRIORITY_NORMAL;

	if(byPriority == OSA_THREAD_PRI_CRITICAL)
	{
		nPriority = THREAD_PRIORITY_TIME_CRITICAL;
	}
	else if(byPriority == OSA_THREAD_PRI_HIGHEST)
	{
		nPriority = THREAD_PRIORITY_HIGHEST;
	}
	else if(byPriority == OSA_THREAD_PRI_ABOVE_NORMAL)
	{
		nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
	}
	else if(byPriority == OSA_THREAD_PRI_NORMAL)
	{
		nPriority = THREAD_PRIORITY_NORMAL;
	}
	else if(byPriority == OSA_THREAD_PRI_BELOW_NORMAL)
	{
		nPriority = THREAD_PRIORITY_BELOW_NORMAL;		
	}	
	else
	{
		nPriority = THREAD_PRIORITY_LOWEST;
	}

	bRet = SetThreadPriority(hThread, nPriority);
	if( !bRet )
	{
		iOsaLog(OSA_ERROCC, "SetThreadPriority failed, errno=%d\n", GetLastError());
		dwRet = COMERR_SYSCALL;
	}
	return dwRet;
}

static UINT32 sWin_ThreadPriorityGet(TASKHANDLE hThread, UINT8* pbyPri)
{
	int nRetPrior = GetThreadPriority( hThread );
	switch(nRetPrior)
	{
	case THREAD_PRIORITY_TIME_CRITICAL:
		*pbyPri = OSA_THREAD_PRI_CRITICAL;
		break;

	case THREAD_PRIORITY_HIGHEST:
		*pbyPri = OSA_THREAD_PRI_HIGHEST;
		break;

	case THREAD_PRIORITY_ABOVE_NORMAL:
		*pbyPri = OSA_THREAD_PRI_ABOVE_NORMAL;
		break;

	case THREAD_PRIORITY_NORMAL:
		*pbyPri = OSA_THREAD_PRI_NORMAL;
		break;

	case THREAD_PRIORITY_BELOW_NORMAL:
		*pbyPri = OSA_THREAD_PRI_BELOW_NORMAL;
		break;

	case THREAD_PRIORITY_LOWEST:
		*pbyPri = OSA_THREAD_PRI_LOWEST;
		break;
	default:
		*pbyPri = OSA_THREAD_PRI_NORMAL;
		break;
	}
	return 0;
}

#endif 

#ifdef _LINUX_
static TASKHANDLE sLinux_ThreadCreate( TDoTaskPara *ptDoTaskPara, UINT8 byPriority, UINT32 dwSetStackSize,
									 BOOL bSuspendThread, OUT ULONG *pulTaskID )
{
	TASKHANDLE  hTask = NULL;
	int nRet = 0;
	struct sched_param tSchParam;	
	pthread_attr_t tThreadAttr;
	int nSchPolicy;

	pthread_attr_init(&tThreadAttr);

	if(byPriority == OSA_THREAD_PRI_LOWEST)
	{
		// 设置调度策略
		pthread_attr_getschedpolicy(&tThreadAttr, &nSchPolicy);
		nSchPolicy = SCHED_OTHER;
		pthread_attr_setschedpolicy(&tThreadAttr, nSchPolicy);
	}
	else
	{
		// 设置调度策略
		pthread_attr_getschedpolicy(&tThreadAttr, &nSchPolicy);
		nSchPolicy = SCHED_FIFO;
		pthread_attr_setschedpolicy(&tThreadAttr, nSchPolicy);

		// 设置优先级
		pthread_attr_getschedparam(&tThreadAttr, &tSchParam);
		tSchParam.sched_priority = byPriority;
		pthread_attr_setschedparam(&tThreadAttr, &tSchParam);
	}

#ifdef _PPC_
	pthread_attr_setstacksize(&tThreadAttr, dwSetStackSize + (16<<10));
#else
	pthread_attr_setstacksize(&tThreadAttr, dwSetStackSize);
#endif

	nRet = pthread_create((pthread_t *)&hTask, &tThreadAttr, do_task, (void *)ptDoTaskPara);
	if(nRet == 0)
	{
		if(pulTaskID != NULL)
		{			
			*pulTaskID = (ULONG)hTask;
		}
	}
	else
	{
		iOsaLog(OSA_ERROCC, "pthread_create failed, errno=%d, %s\n", nRet, strerror(nRet) );
	}
	return hTask;
}

static UINT32 sLinux_ThreadPrioritySet(TASKHANDLE hThread, UINT8 byPriority)
{
	UINT32 dwRet = 0;
	int nRet = 0;
	struct sched_param tSchParam;	
	
	if(byPriority == OSA_THREAD_PRI_LOWEST)
	{
		tSchParam.sched_priority = 0;
		nRet = pthread_setschedparam((pthread_t)hThread, SCHED_OTHER, &tSchParam );
	}
	else
	{
		tSchParam.sched_priority = byPriority;
		nRet = pthread_setschedparam((pthread_t)hThread, SCHED_FIFO, &tSchParam);
	}	

	if( nRet != 0 )
	{
		iOsaLog(OSA_ERROCC, "pthread_setschedparam failed, errno=%d, %s\n", nRet, strerror(nRet) );
		dwRet = COMERR_SYSCALL;
	}
	return dwRet;
}

static UINT32 sLinux_ThreadPriorityGet(TASKHANDLE hThread, UINT8* pbyPri)
{
	int nRet = 0;
	struct sched_param tSchParam;
	int nPolicy;
	nRet = pthread_getschedparam((pthread_t)hThread, &nPolicy, &tSchParam);
	if( nRet != 0 )
	{
		iOsaLog(OSA_ERROCC, "pthread_getschedparam failed, errno=%d, %s\n", nRet, strerror(nRet) );
		return COMERR_SYSCALL;
	}

	if(SCHED_OTHER == nPolicy )
	{
		*pbyPri = OSA_THREAD_PRI_LOWEST;
	}
	else
	{
		*pbyPri = tSchParam.sched_priority;
	}
	return 0;
}

#endif

/*====================================================================
功能：创建并激活一个线程（任务）
参数说明：pfunThreadEntry: 线程入口。如：typedef void (*fThreadEntry)( UINT32 dwParam )
			pszName: 线程名，以NULL结束的字符串，
			byPriority: 线程优先级别，如OSA_THREAD_PRI_NORMAL
			uStacksize: 线程堆栈大小，传0表示采用默认大小
			pParam: 线程参数。
			bSuspendThread: 若要创建线程时，将线程先挂起，填TRUE，否则填0。
			puThreadID: 返回参数，线程ID.
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadCreate( TfuncThreadEntry pfunThreadEntry, const char *szName, UINT8 byPriority, UINT32 dwStacksize, 
					   void *pParam,  BOOL bSuspendThread, OUT TASKHANDLE *phThread )
{
	UINT32 dwRet = 0;
	TASKHANDLE  hTask = 0;
	ULONG ulTaskID = 0;
	UINT32 dwSetStackSize = 0;
	TDoTaskPara *ptDoTaskPara = NULL;

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();

	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( pfunThreadEntry, "pfunThreadEntry" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( szName, "szName" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( phThread, "phThread" );

	if( ! ( (byPriority == OSA_THREAD_PRI_LOWEST) || (byPriority == OSA_THREAD_PRI_BELOW_NORMAL) || (byPriority == OSA_THREAD_PRI_NORMAL) ||
		(byPriority == OSA_THREAD_PRI_ABOVE_NORMAL) || (byPriority == OSA_THREAD_PRI_HIGHEST) || (byPriority == OSA_THREAD_PRI_CRITICAL) ) )
	{
		iOsaLog(OSA_ERROCC, "OsaThreadCreate para err, byPriority=%d\n", byPriority);
		return COMERR_INVALID_PARAM;
	}

	//参数正确性保证
	dwSetStackSize = dwStacksize;
	if( 0 == dwSetStackSize )
	{
		dwSetStackSize = OSA_DFT_STACKSIZE;
	}
	else if(dwSetStackSize < STACKSIZE_MIN)
	{
		dwSetStackSize = STACKSIZE_MIN;
	}
	else if(dwSetStackSize > STACKSIZE_MAX)
	{
		dwSetStackSize = STACKSIZE_MAX;
	}

	//分配线程参数内存
	ptDoTaskPara = (TDoTaskPara *)OsaPoolMalloc( iOsaMemAllocGet(), sizeof(TDoTaskPara), iOsaMRsrcTagGet() );
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptDoTaskPara, "ptDoTaskPara" );

	memset(ptDoTaskPara, 0, sizeof(TDoTaskPara));
	ptDoTaskPara->pUserParam = pParam;
	ptDoTaskPara->m_pfUserTaskFunc = pfunThreadEntry;		
	ptDoTaskPara->m_dwStackSize = dwSetStackSize;

	//创建线程
	g_bTaskNodeAdd = FALSE;
#ifdef _MSC_VER
	hTask = sWin_ThreadCreate( ptDoTaskPara, byPriority, dwSetStackSize, bSuspendThread, &ulTaskID );
#endif

#ifdef _LINUX_
	hTask = sLinux_ThreadCreate( ptDoTaskPara, byPriority, dwSetStackSize, bSuspendThread, &ulTaskID );
#endif

	if( hTask != NULL )
	{
		sThreadListAddNode( hTask, pfunThreadEntry, szName, byPriority, ulTaskID, dwSetStackSize );
		g_bTaskNodeAdd = TRUE;
		OSA_COND_WAIT(g_bTaskNodeAddAck, TASKWAIT_MAXMS);

		*phThread = hTask;

		iOsaLog(OSA_LOGOCC, "%s() ThreadCreate success, Name:%s, StackSize=%d.\n",
			__FUNCTION__, szName, dwSetStackSize);
	}
	else
	{
// 		UINT32 dwMemtotal = 0;;
// 		UINT32 dwUsgPercent = 0;
// 		OsaTotalMemGet( &dwMemtotal );
// 		OsaMemUsgGet(&dwUsgPercent);
// 
// 		iOsaLog(OSA_ERROCC,"meminfo when create thread failed, total: %dM, Usg: %d\n", dwMemtotal, dwUsgPercent );
// 		osamemall(OsaTelnetHdlGet());

		OsaMFree(iOsaMemAllocGet(), ptDoTaskPara );
		dwRet = COMERR_SYSCALL;
	}
	return dwRet;
}

/*====================================================================
功能：线程函数内部自己退出
参数说明： 无
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadSelfExit(void)
{	
	UINT32 dwRet = 0;
	BOOL bRet = FALSE;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//直接退出线程，没有机会再回到do_task函数，
	//所以此处将线程节点释放
	bRet = sThreadListDelNode( iOsaThreadSelfID() );
	if( FALSE == bRet )
	{
		return COMERR_NOT_FOUND;
	}

#ifdef _MSC_VER
	ExitThread(0);
#endif

#ifdef _LINUX_
	int nRet = 0;
	pthread_exit(&nRet);
#endif	

	return dwRet;
}

/*====================================================================
功能：强制线程退出
参数说明：TASKHANDLE hThread 线程句柄
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadKill( TASKHANDLE hThread )
{
	UINT32 dwRet = 0;
	BOOL bRet = FALSE;
	int nRet = 0;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( hThread, "hThread" );

	//强制线程退出，没有机会再回到do_task函数，
	//所以此处将线程节点释放
	bRet = sThreadListDelNodeByHandle( hThread );
	if( FALSE == bRet )
	{
		return COMERR_NOT_FOUND;
	}

#ifdef _MSC_VER
	bRet = TerminateThread( hThread, 0);
	if( !bRet )
	{
		dwRet = COMERR_SYSCALL;
		iOsaLog(OSA_ERROCC, "TerminateThread failed, errno=%d\n", GetLastError());
	}

#endif

#ifdef _LINUX_
	//nRet = pthread_kill((pthread_t)hThread, 0); //???
	nRet = pthread_cancel((pthread_t)hThread); //???
	if( nRet != 0 )
	{
		iOsaLog(OSA_ERROCC, "pthread_getschedparam failed, errno=%d, %s\n", nRet, strerror(nRet) );
		dwRet = COMERR_SYSCALL;
	}

#endif

	return dwRet;
}

/*====================================================================
功能：获取当前运行线程的句柄（要求必须通过OsaThreadCreate创建的线程）
参数说明： phThread: 返回参数，线程句柄
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadSelfHandleGet( OUT TASKHANDLE *phThread )
{
	UINT32 dwRet = 0;
	BOOL bRet = FALSE;
	ULONG ulTaskID = 0;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( phThread, "phThread" );

	ulTaskID = iOsaThreadSelfID();
	bRet = sThreadHandleGetByID( ulTaskID, phThread );
	if( !bRet )
	{
		dwRet = COMERR_NOT_FOUND;
		//iOsaLog(OSA_ERROCC, "OsaThreadSelfHandleGet() failed, thread Id not found!\n" );
	}
	return dwRet;
}

/*====================================================================
功能：结束线程完成退出
参数说明：TASKHANDLE hThread 线程句柄
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadExitWait(TASKHANDLE hThread)
{
	UINT32 dwRet = 0;
#ifdef _MSC_VER
	DWORD dwWaitRet = 0;
#else
	UINT64 dwWaitRet = 0;
#endif
//	int nRet = 0;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( hThread, "hThread" );

#ifdef _MSC_VER
	dwWaitRet = WaitForSingleObject( hThread, INFINITE);
	if( WAIT_FAILED == dwWaitRet )
	{
		dwRet = COMERR_SYSCALL;
		iOsaLog(OSA_ERROCC, "WaitForSingleObject failed, errno=%d\n", GetLastError());
	}
#endif
#ifdef _LINUX_
	dwWaitRet = pthread_join((pthread_t)hThread, NULL);
	if( dwWaitRet != 0 )
	{
		iOsaLog(OSA_ERROCC, "pthread_join failed, errno=%d, %s\n", dwWaitRet, strerror(dwWaitRet) );
		dwRet = COMERR_SYSCALL;
	}
#endif

	return dwRet;
}

// /*====================================================================
// 功能：线程心跳检测间隔设置(在OsaThreadCreate后调用)
// 参数说明：	hThread - 线程句柄, 由OsaThreadCreate返回
// 				dwIntervalMs - 检测心跳的间隔(ms)
// 返回值说明：成功返回OSA_OK，失败返回错误码.
// ====================================================================*/
// UINT32 OsaThreadHeartBeatSet(TASKHANDLE hThread, UINT32 dwIntervalMs);
// 
// /*====================================================================
// 功能：线程心跳
// 参数说明：hThread - 线程句柄
// 返回值说明：成功返回OSA_OK，失败返回错误码.
// ====================================================================*/
// UINT32 OsaThreadHeartBeat(TASKHANDLE hThread);

/*====================================================================
功能：线程延时
参数说明：dwDelayMs: 延时时间（ms）
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
void OsaSleep(UINT32 dwDelayMs)
{
#ifdef _MSC_VER
	Sleep(dwDelayMs);
#endif

#ifdef _LINUX_
	usleep(dwDelayMs * 1000);
#endif	

}


/*====================================================================
功能：线程挂起
参数说明：hThread: 线程句柄 
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadSuspend(TASKHANDLE hThread)
{
	UINT32 dwRet = 0;
#ifdef _MSC_VER
	DWORD dwSysRet = 0;
#else
//	UINT64 dwSysRet = 0;
#endif

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( hThread, "hThread" );
#ifdef _MSC_VER
	dwSysRet = SuspendThread( hThread );
	if( -1 == dwSysRet )
	{
		iOsaLog(OSA_ERROCC, "SuspendThread failed, errno=%d\n", GetLastError());
		dwRet = COMERR_SYSCALL;
	}
#endif

#ifdef _LINUX_
	iOsaLog(OSA_ERROCC, "Linux not support OsaThreadSuspend()\n" );
	dwRet = COMERR_NOT_SUPPORT;	
#endif

	return dwRet;
}

/*====================================================================
功能：线程继续
参数说明：hThread: 线程句柄 
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadResume(TASKHANDLE hThread)
{
	UINT32 dwRet = 0;
#ifdef _MSC_VER
	DWORD dwSysRet = 0;
#else
//	UINT64 dwSysRet = 0;
#endif

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( hThread, "hThread" );
#ifdef _MSC_VER
	dwSysRet = ResumeThread(hThread);
	if( -1 == dwSysRet )
	{
		iOsaLog(OSA_ERROCC, "ResumeThread failed, errno=%d\n", GetLastError());
		dwRet = COMERR_SYSCALL;
	}
#endif

#ifdef _LINUX_		
	iOsaLog(OSA_ERROCC, "Linux not support OsaThreadResume()\n" );
	dwRet = COMERR_NOT_SUPPORT;	
#endif

	return dwRet;
}

/*====================================================================
功能：改变线程的优先级。
参数说明：hThread: 目标线程的句柄,
			uPriority: 要设置的优先级. 见宏定义，如OSA_THREAD_PRI_NORMAL 
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadSetPriority(TASKHANDLE hThread, UINT8 byPriority)
{	
	UINT32 dwRet = 0;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( hThread, "hThread" );

	if( ! ( (byPriority == OSA_THREAD_PRI_LOWEST) || (byPriority == OSA_THREAD_PRI_BELOW_NORMAL) || (byPriority == OSA_THREAD_PRI_NORMAL) ||
		(byPriority == OSA_THREAD_PRI_ABOVE_NORMAL) || (byPriority == OSA_THREAD_PRI_HIGHEST) || (byPriority == OSA_THREAD_PRI_CRITICAL) ) )
	{
		iOsaLog(OSA_ERROCC, "OsaThreadSetPriority para err, byPriority=%d\n", byPriority);
		return COMERR_INVALID_PARAM;
	}

#ifdef _MSC_VER		
	dwRet = sWin_ThreadPrioritySet(hThread, byPriority );
#endif

#ifdef _LINUX_
	dwRet = sLinux_ThreadPrioritySet(hThread, byPriority );
#endif

	return dwRet;
}

/*====================================================================
功能：获得线程的优先级。
参数说明：hThread: 目标线程的句柄,
			puPri: 返回参数, 成功返回线程的优先级.
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadGetPriority(TASKHANDLE hThread, UINT8* pbyPri)
{
	UINT32 dwRet = 0;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( hThread, "hThread" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pbyPri, "pbyPri" );

#ifdef _MSC_VER		
	dwRet = sWin_ThreadPriorityGet(hThread, pbyPri );
#endif

#ifdef _LINUX_
	dwRet = sLinux_ThreadPriorityGet(hThread, pbyPri );
#endif

	return dwRet;
}

/*====================================================================
功能：释放线程的句柄
参数说明：hThread: 目标线程的句柄
返回值说明：成功返回OSA_OK，失败返回错误码.
====================================================================*/
UINT32 OsaThreadHandleClose(TASKHANDLE hThread)
{
	UINT32 dwRet = 0;
	BOOL bRet = FALSE;
	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( hThread, "hThread" );

#ifdef _MSC_VER
	bRet = CloseHandle(hThread);
	if( FALSE == bRet )
	{
		iOsaLog(OSA_ERROCC, "%s(), call system CloseHandle failed, errno=%d\n", __FUNCTION__, GetLastError() );
		dwRet = COMERR_SYSCALL;
	}
#endif

#ifdef _LINUX_
	dwRet = 0;
#endif
	return dwRet;
}

#ifdef _MSC_VER
//判断指定线程是否存活
BOOL iWin_IsThreadActive( TASKHANDLE hThread )
{
	BOOL bGet = FALSE;
	DWORD dwExitCode = 0;
	bGet = GetExitCodeThread(hThread, (DWORD *)&dwExitCode);
	if(bGet == 0)
	{
		return FALSE;
	}

	return (dwExitCode == STILL_ACTIVE);
}
#endif

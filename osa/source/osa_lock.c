#include "osa_common.h"

static UINT32 g_dwSemTakeSeq = 0; //信号量（锁）take的序列号，用于定位死锁问题

//内部创建锁
void iInnerLightLockCreate( InnerLockHandle *phInnerLock )
{
#ifdef _MSC_VER
	InitializeCriticalSection(phInnerLock);
#else
	pthread_mutexattr_t attr; 
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE); 
	pthread_mutex_init(phInnerLock, &attr); 
#endif
}

//内部删除锁
void iInnerLightLockDelete( InnerLockHandle *phInnerLock )
{
#ifdef _MSC_VER
	DeleteCriticalSection(phInnerLock);
#else
	pthread_mutex_destroy(phInnerLock);
#endif
}

//不用监控的锁轻量级锁
void iInnerLightLockLockNoWatch( InnerLockHandle *phInnerLock )
{

#ifdef _MSC_VER
	EnterCriticalSection(phInnerLock);	
#endif

#ifdef _LINUX_
	pthread_mutex_lock(phInnerLock);
#endif
}
//不用监控的解锁轻量级锁
void iInnerLightLockUnLockNoWatch( InnerLockHandle *phInnerLock )
{
#ifdef _MSC_VER
	LeaveCriticalSection(phInnerLock);
#endif

#ifdef _LINUX_
	pthread_mutex_unlock(phInnerLock);
#endif
}

/*===========================================================
功能：创建轻量级锁
参数说明： LockHandle *phLock － 锁指针
返回值说明：成功返回OSA_OK，失败返回错误码. 
===========================================================*/
UINT32 OsaLightLockCreate( OUT LockHandle *phLock)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( phLock, "phLock" );
	*phLock = NULL;

	//内部也会创建锁，这里不能用内存库分配内存
	ptSemInfo = (TOsaSemInfo *)malloc(sizeof(TOsaSemInfo));
	OSA_CHECK_NULL_POINTER_RETURN_U32(ptSemInfo, "ptSemInfo" );
	memset(ptSemInfo, 0, sizeof(TOsaSemInfo));

	iInnerLightLockCreate(&(ptSemInfo->m_hInnerLightLock));

	ptSemInfo->m_bLightLock = TRUE;
	ptSemInfo->m_dwSemCount = 1;

	//加入到信号量监控链表中
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListInsertLast(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	*phLock = (LockHandle)(&(ptSemInfo->m_hInnerLightLock));
	return dwRet;
}

/*===========================================================
功能：删除轻量级锁
参数说明： hLock － 锁指针
返回值说明：成功返回OSA_OK，失败返回错误码. 
===========================================================*/
UINT32 OsaLightLockDelete( LockHandle hLock )
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	InnerLockHandle *phInnerLightLock = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( hLock, "hLock" );

	//取出内部锁的地址
	phInnerLightLock = (InnerLockHandle *)(hLock);

	//得到封装结构
	ptSemInfo = HOST_ENTRY(phInnerLightLock, TOsaSemInfo, m_hInnerLightLock );	

	//从监控链表中断链
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListRemove(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	//删除锁
	iInnerLightLockDelete( phInnerLightLock );

	//释放内存
	memset(ptSemInfo, 0, sizeof(TOsaSemInfo));
	free(ptSemInfo);
	ptSemInfo = NULL;
	return dwRet;
}

/*===========================================================
功能：锁轻量级锁，为了方便调试和死锁问题定位，请使用宏定义OsaLightLockLock进行加锁操作
参数说明：	hLock － 锁指针
			szLockName[]:锁名
			szFileName[]:调用所在的文件名
			UINT32  dwLine:调用所在的行
返回值说明：成功返回OSA_OK，失败返回错误码. 
===========================================================*/
UINT32 InnerOsaLightLockLock(LockHandle hLock, const char szLockName[], const char szFileName[], UINT32  dwLine)
{	
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	InnerLockHandle *phInnerLightLock = NULL;
//	INT32 nFileNameLen = 0;
	const char* pchFileNamePos = szFileName;

	if( NULL == hLock )
	{
		dwRet = COMERR_NULL_POINTER;
	}
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( hLock, "hLock" );
	//得到内部锁
	phInnerLightLock = (InnerLockHandle *)( hLock );

	iInnerLightLockLockNoWatch(phInnerLightLock);	

	//监控锁
	ptSemInfo = HOST_ENTRY(phInnerLightLock, TOsaSemInfo, m_hInnerLightLock);
	ptSemInfo->m_dwCurTakeThreadID = iOsaThreadSelfID();
	ptSemInfo->m_dwTakeCount++;
	ptSemInfo->m_dwTakeLine = dwLine;
	
// 	nFileNameLen = strlen( szFileName );
// 	if( nFileNameLen > OSA_FILENAME_MAXLEN )
// 	{
// 		pchFileNamePos = szFileName + nFileNameLen - OSA_FILENAME_MAXLEN ;
// 	}
// 	strncpy( ptSemInfo->m_szTakeFileName, pchFileNamePos, OSA_FILENAME_MAXLEN );
	sprintf(ptSemInfo->m_szTakeFileName,iOsaReverseTrim(pchFileNamePos,OSA_FILENAME_MAXLEN));
	ptSemInfo->m_szTakeFileName[OSA_FILENAME_MAXLEN] = '\0';

	strncpy( ptSemInfo->m_szSemName, szLockName, OSA_SEMNAME_MAXLEN );
	ptSemInfo->m_szSemName[OSA_SEMNAME_MAXLEN] = '\0';

	ptSemInfo->m_dwSemTakeSeq = g_dwSemTakeSeq;
	g_dwSemTakeSeq++;

	return dwRet;
}

/*===========================================================
功能：解锁轻量级锁
参数说明： hLock － 锁指针                   
返回值说明：成功返回OSA_OK，失败返回错误码.  
===========================================================*/
UINT32 OsaLightLockUnLock(LockHandle hLock)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo=NULL;
	InnerLockHandle *phInnerLightLock = NULL;

	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( hLock, "hLock" );
	//得到内部锁
	phInnerLightLock = (InnerLockHandle *)(hLock);
	OSA_CHECK_NULL_POINTER_RETURN_U32( phInnerLightLock, "phInnerLightLock" );

	iInnerLightLockUnLockNoWatch(phInnerLightLock);

	//监控信号量
	ptSemInfo = HOST_ENTRY(phInnerLightLock, TOsaSemInfo, m_hInnerLightLock);
	ptSemInfo->m_dwCurGiveThreadID = iOsaThreadSelfID();
	ptSemInfo->m_dwGiveCount++;

	return dwRet;
}

/*****************************************************************************
3.同步对象（慢速同步信号量）封装
******************************************************************************/
/*====================================================================
功能：创建一个二元信号量
参数说明：phSem: 返回的信号量句柄
返回值说明：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaSemBCreate(OUT SEMHANDLE *phSem)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem* ptOsaSem = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( phSem, "phSem" );

	*phSem = NULL;
	ptSemInfo = (TOsaSemInfo *)OsaMalloc( iOsaMemAllocGet(), sizeof(TOsaSemInfo), iOsaMRsrcTagGet() );
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptSemInfo, "ptSemInfo" );
	memset(ptSemInfo, 0, sizeof(TOsaSemInfo));

	do 
	{
#ifdef _MSC_VER
		ptSemInfo->m_tOsaSem = CreateSemaphore(NULL, 1, 1, NULL);
		if(ptSemInfo->m_tOsaSem == NULL)
		{
			dwRet = COMERR_SYSCALL;
			OsaLabPrt( OSA_ERROCC, "OsaSemBCreate(), CreateSemaphore() failed\n");
			break;
		}

#endif    

#ifdef _LINUX_
		ptOsaSem = &(ptSemInfo->m_tOsaSem);
		if(0 != pthread_mutex_init(&(ptOsaSem->mutex), NULL))
		{
			dwRet = COMERR_SYSCALL;
			OsaLabPrt( OSA_ERROCC, "OsaSemBCreate(),pthread_mutex_init failed\n");
			break;
		}

		if(0 != pthread_cond_init(&(ptOsaSem->condition), NULL))
		{
			dwRet = COMERR_SYSCALL;
			OsaLabPrt( OSA_ERROCC,"OsaSemBCreate(),pthread_cond_init failed\n");
			pthread_mutex_destroy( &(ptOsaSem->mutex) );
			break;
		}

		ptOsaSem->semCount = 1;
		ptOsaSem->semMaxCount = 1;	

#endif
	} while (0);

	if( dwRet != OSA_OK )
	{
		OsaMFree(iOsaMemAllocGet(), ptSemInfo );
		return dwRet;
	}

	ptSemInfo->m_dwSemCount = 1;

	//加入到信号量监控链表中
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListInsertLast(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	*phSem = (SEMHANDLE)(&(ptSemInfo->m_tOsaSem));
	return dwRet;
}

/*====================================================================
功能：删除信号量
参数说明：phSem: 待删除信号量的句柄
返回值说明：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaSemDelete(SEMHANDLE hSem)
{
	UINT32 dwRet = 0;
	BOOL bRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem* ptOsaSem = (TOsaSem*)hSem;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	ptSemInfo = HOST_ENTRY(ptOsaSem, TOsaSemInfo, m_tOsaSem);	

	//从监控链表中断链
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListRemove(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	//释放操作系统的信号量
#ifdef _MSC_VER 		
	bRet = CloseHandle( *ptOsaSem );
#endif

#ifdef _LINUX_

	ptOsaSem = &(ptSemInfo->m_tOsaSem);
	pthread_mutex_destroy(&(ptOsaSem->mutex));
	pthread_cond_destroy(&(ptOsaSem->condition));
	bRet = TRUE;    

#endif
	memset(ptSemInfo, 0, sizeof(TOsaSemInfo));
	OsaMFree(iOsaMemAllocGet(), ptSemInfo );

	if( !bRet )
	{
		dwRet = COMERR_SYSCALL;
	}
	return dwRet;
}

//不带监视的信号量p操作
static UINT32 sInnerSemTakeNoWatch(SEMHANDLE hSem)
{	
	UINT32 dwRet = 0;
	TOsaSem* ptOsaSem = (TOsaSem*)hSem;
	if (NULL == ptOsaSem)
	{
		return OSA_SEM_FAILED;
	}

#ifdef _MSC_VER	
	dwRet = WaitForSingleObject( *ptOsaSem, INFINITE);
	if (WAIT_OBJECT_0 == dwRet)
	{
		return OSA_SEM_SIGNALED;
	}
	if (WAIT_TIMEOUT == dwRet)
	{
		return OSA_SEM_TIMEOUT;
	}
	else 
	{
		return OSA_SEM_FAILED;
	}
#endif
    
#ifdef _LINUX_
	
	pthread_mutex_lock(&(ptOsaSem->mutex));

	while(ptOsaSem->semCount == 0)
	{
		pthread_cond_wait(&(ptOsaSem->condition), &(ptOsaSem->mutex));
	}
	
	ptOsaSem->semCount = ptOsaSem->semCount - 1;
	pthread_mutex_unlock(&(ptOsaSem->mutex));
	
	dwRet = OSA_SEM_SIGNALED;	
	return dwRet;	
#endif
}

//不带监视的带超时的信号量p操作
static UINT32  sInnerSemTakeByTimeNoWatch(SEMHANDLE hSem, UINT32 dwTimeout)
{
	UINT32 dwRet = 0;
	TOsaSem* ptOsaSem = (TOsaSem*)hSem;
	if (NULL == ptOsaSem)
	{
		return OSA_SEM_FAILED;
	}

#ifdef _MSC_VER
	dwRet = WaitForSingleObject(*ptOsaSem, dwTimeout) ;
	if (WAIT_OBJECT_0 == dwRet)
	{
		return OSA_SEM_SIGNALED;
	}
	if (WAIT_TIMEOUT == dwRet)
	{
		return OSA_SEM_TIMEOUT;
	}
	else 
	{
		return OSA_SEM_FAILED;
	}

#endif
    
#ifdef _LINUX_
	{
		INT32 rc = 0;
		struct timespec tm;
		struct timeb tp;
		INT32 sec, millisec;

		pthread_mutex_lock(&(ptOsaSem->mutex));

		sec = dwTimeout / 1000;
		millisec = dwTimeout % 1000;
		ftime( &tp );
		tp.time += sec;
		tp.millitm += millisec;
		if( tp.millitm > 999 )
		{
			tp.millitm -= 1000;
			tp.time++;
		}
		tm.tv_sec = tp.time;
		tm.tv_nsec = tp.millitm * 1000000 ;

		while((ptOsaSem->semCount == 0) && (rc != ETIMEDOUT))
		{
			rc = pthread_cond_timedwait(&(ptOsaSem->condition), &(ptOsaSem->mutex), &tm);
		}

		if(ETIMEDOUT == rc)
		{
			dwRet = OSA_SEM_TIMEOUT;
		}
		else
		{
			ptOsaSem->semCount--;
			dwRet = OSA_SEM_SIGNALED;
		}

		pthread_mutex_unlock(&(ptOsaSem->mutex));
		return dwRet;
	}
	
#endif
}

//不带监视的信号量v操作
static BOOL sInnerSemGiveNoWatch(SEMHANDLE hSem)
{
//	UINT32 dwRet = 0;
#ifdef _MSC_VER
	LPLONG previous=0;
#endif
	TOsaSem *ptOsaSem = (TOsaSem*)hSem;
	if (NULL == ptOsaSem)
	{
		return FALSE;
	}

#ifdef _MSC_VER
    return ReleaseSemaphore(*ptOsaSem, 1, (LPLONG)&previous);
#endif
    
#ifdef _LINUX_	
	
	pthread_mutex_lock(&(ptOsaSem->mutex));

	//判断give次数是否过多
	if(ptOsaSem->semCount >= ptOsaSem->semMaxCount)
	{
		pthread_mutex_unlock(&(ptOsaSem->mutex));
		return FALSE;		
	}

	if(ptOsaSem->semCount == 0)
	{
		pthread_cond_signal(&(ptOsaSem->condition));
	}
	
	ptOsaSem->semCount = ptOsaSem->semCount + 1;	
	pthread_mutex_unlock(&(ptOsaSem->mutex));
	return TRUE;	
#endif  
}

/*====================================================================
功能：信号量p操作，为了方便调试和死锁问题定位，请使用宏定义OsaSemTake进行加锁操作
参数说明：	hSem: 信号量句柄
			szSemName[]:信号量名
			szFileName[]:调用所在的文件名
			dwLine:调用所在的行
返回值说明：成功返回OSA_SEM_SIGNALED（OSA_OK)，否则返回OSA_SEM_TIMEOUT或者错误码
====================================================================*/
UINT32 InnerOsaSemTake(SEMHANDLE hSem,char szSemName[],char szFileName[],UINT32 dwLine)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem *ptOsaSem = (TOsaSem*)hSem;
	INT32 nFileNameLen = 0;
	char* pchFileNamePos = szFileName;

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	dwRet = sInnerSemTakeNoWatch(hSem);
	if(dwRet != OSA_SEM_SIGNALED)
	{
		OsaLabPrt( OSA_ERROCC, "InnerOsaSemTake(), sInnerSemTakeByTimeNoWatch error,dwRet: 0x%x\n", dwRet);
		return dwRet;
	}	

	//监控信号量
	ptSemInfo = HOST_ENTRY(ptOsaSem, TOsaSemInfo, m_tOsaSem);	
	ptSemInfo->m_dwCurTakeThreadID = iOsaThreadSelfID();
	ptSemInfo->m_dwTakeCount++;
	ptSemInfo->m_dwTakeLine = dwLine;

	nFileNameLen = (INT32)strlen( szFileName );
	if( nFileNameLen > OSA_FILENAME_MAXLEN )
	{
		pchFileNamePos = szFileName + nFileNameLen - OSA_FILENAME_MAXLEN ;
	}
	strncpy( ptSemInfo->m_szTakeFileName, pchFileNamePos, OSA_FILENAME_MAXLEN );
	ptSemInfo->m_szTakeFileName[OSA_FILENAME_MAXLEN] = '\0';

	strncpy( ptSemInfo->m_szSemName, szSemName, OSA_SEMNAME_MAXLEN );
	ptSemInfo->m_szSemName[OSA_SEMNAME_MAXLEN] = '\0';

	ptSemInfo->m_dwSemTakeSeq = g_dwSemTakeSeq;
	g_dwSemTakeSeq++;

	return dwRet;
}

/*====================================================================
功能：带超时的信号量p操作，为了方便调试和死锁问题定位，请使用宏定义OsaSemTakeByTime进行加锁操作
参数说明：	hSem: 信号量句柄
			dwTimeoutMs: 超时时间（ms）,最小精度为10ms
			szSemName[]:信号量名
			szFileName[]:调用所在的文件名
			dwLine:调用所在的行
返回值说明：成功返回OSA_SEM_SIGNALED（OSA_OK)，否则返回OSA_SEM_TIMEOUT或者错误码
====================================================================*/
UINT32 InnerOsaSemTakeByTime(SEMHANDLE hSem, UINT32  dwTimeoutMs,char szSemName[],char szFileName[],UINT32 dwLine)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem *ptOsaSem = (TOsaSem*)hSem;
	INT32 nFileNameLen = 0;
	char* pchFileNamePos = szFileName;

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	dwRet = sInnerSemTakeByTimeNoWatch(hSem, dwTimeoutMs );
	if(dwRet != OSA_SEM_SIGNALED && dwRet != OSA_SEM_TIMEOUT )
	{	
		OsaLabPrt( OSA_ERROCC, "InnerOsaSemTakeByTime(), sInnerSemTakeByTimeNoWatch error,dwRet: 0x%x\n", dwRet);
		return dwRet;
	}	

	//监控信号量
	ptSemInfo = HOST_ENTRY(ptOsaSem, TOsaSemInfo, m_tOsaSem);
	ptSemInfo->m_dwCurTakeThreadID = iOsaThreadSelfID();
	ptSemInfo->m_dwTakeCount++;
	ptSemInfo->m_dwTakeLine = dwLine;

	nFileNameLen = (INT32)strlen( szFileName );
	if( nFileNameLen > OSA_FILENAME_MAXLEN )
	{
		pchFileNamePos = szFileName + nFileNameLen - OSA_FILENAME_MAXLEN ;
	}
	strncpy( ptSemInfo->m_szTakeFileName, pchFileNamePos, OSA_FILENAME_MAXLEN );
	ptSemInfo->m_szTakeFileName[OSA_FILENAME_MAXLEN] = '\0';

	strncpy( ptSemInfo->m_szSemName, szSemName, OSA_SEMNAME_MAXLEN );
	ptSemInfo->m_szSemName[OSA_SEMNAME_MAXLEN] = '\0';

	ptSemInfo->m_dwSemTakeSeq = g_dwSemTakeSeq;
	g_dwSemTakeSeq++;

	return dwRet;
}


/*====================================================================
功能：信号量v操作
输入参数说明：hSem: 信号量句柄
返回值说明：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaSemGive(SEMHANDLE hSem )
{
	BOOL bGive=FALSE;
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem *ptOsaSem = (TOsaSem*)hSem;

	//初始化检查
	OSA_CHECK_INIT_RETURN_U32();
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	bGive = sInnerSemGiveNoWatch(hSem);	
	if(bGive)
	{
		//监控信号量
		ptSemInfo = HOST_ENTRY(ptOsaSem, TOsaSemInfo, m_tOsaSem);
		ptSemInfo->m_dwCurGiveThreadID = iOsaThreadSelfID();
		ptSemInfo->m_dwGiveCount++;
	}
	else
	{
		OsaLabPrt( OSA_ERROCC, "OsaSemGive(), sInnerSemGiveNoWatch error.\n" );
		dwRet = OSA_SEM_FAILED;
	}

	return dwRet;
}

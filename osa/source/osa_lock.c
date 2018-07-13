#include "osa_common.h"

static UINT32 g_dwSemTakeSeq = 0; //�ź���������take�����кţ����ڶ�λ��������

//�ڲ�������
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

//�ڲ�ɾ����
void iInnerLightLockDelete( InnerLockHandle *phInnerLock )
{
#ifdef _MSC_VER
	DeleteCriticalSection(phInnerLock);
#else
	pthread_mutex_destroy(phInnerLock);
#endif
}

//���ü�ص�����������
void iInnerLightLockLockNoWatch( InnerLockHandle *phInnerLock )
{

#ifdef _MSC_VER
	EnterCriticalSection(phInnerLock);	
#endif

#ifdef _LINUX_
	pthread_mutex_lock(phInnerLock);
#endif
}
//���ü�صĽ�����������
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
���ܣ�������������
����˵���� LockHandle *phLock �� ��ָ��
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����. 
===========================================================*/
UINT32 OsaLightLockCreate( OUT LockHandle *phLock)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( phLock, "phLock" );
	*phLock = NULL;

	//�ڲ�Ҳ�ᴴ���������ﲻ�����ڴ������ڴ�
	ptSemInfo = (TOsaSemInfo *)malloc(sizeof(TOsaSemInfo));
	OSA_CHECK_NULL_POINTER_RETURN_U32(ptSemInfo, "ptSemInfo" );
	memset(ptSemInfo, 0, sizeof(TOsaSemInfo));

	iInnerLightLockCreate(&(ptSemInfo->m_hInnerLightLock));

	ptSemInfo->m_bLightLock = TRUE;
	ptSemInfo->m_dwSemCount = 1;

	//���뵽�ź������������
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListInsertLast(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	*phLock = (LockHandle)(&(ptSemInfo->m_hInnerLightLock));
	return dwRet;
}

/*===========================================================
���ܣ�ɾ����������
����˵���� hLock �� ��ָ��
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����. 
===========================================================*/
UINT32 OsaLightLockDelete( LockHandle hLock )
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	InnerLockHandle *phInnerLightLock = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( hLock, "hLock" );

	//ȡ���ڲ����ĵ�ַ
	phInnerLightLock = (InnerLockHandle *)(hLock);

	//�õ���װ�ṹ
	ptSemInfo = HOST_ENTRY(phInnerLightLock, TOsaSemInfo, m_hInnerLightLock );	

	//�Ӽ�������ж���
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListRemove(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	//ɾ����
	iInnerLightLockDelete( phInnerLightLock );

	//�ͷ��ڴ�
	memset(ptSemInfo, 0, sizeof(TOsaSemInfo));
	free(ptSemInfo);
	ptSemInfo = NULL;
	return dwRet;
}

/*===========================================================
���ܣ�������������Ϊ�˷�����Ժ��������ⶨλ����ʹ�ú궨��OsaLightLockLock���м�������
����˵����	hLock �� ��ָ��
			szLockName[]:����
			szFileName[]:�������ڵ��ļ���
			UINT32  dwLine:�������ڵ���
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����. 
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
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( hLock, "hLock" );
	//�õ��ڲ���
	phInnerLightLock = (InnerLockHandle *)( hLock );

	iInnerLightLockLockNoWatch(phInnerLightLock);	

	//�����
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
���ܣ�������������
����˵���� hLock �� ��ָ��                   
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����.  
===========================================================*/
UINT32 OsaLightLockUnLock(LockHandle hLock)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo=NULL;
	InnerLockHandle *phInnerLightLock = NULL;

	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( hLock, "hLock" );
	//�õ��ڲ���
	phInnerLightLock = (InnerLockHandle *)(hLock);
	OSA_CHECK_NULL_POINTER_RETURN_U32( phInnerLightLock, "phInnerLightLock" );

	iInnerLightLockUnLockNoWatch(phInnerLightLock);

	//����ź���
	ptSemInfo = HOST_ENTRY(phInnerLightLock, TOsaSemInfo, m_hInnerLightLock);
	ptSemInfo->m_dwCurGiveThreadID = iOsaThreadSelfID();
	ptSemInfo->m_dwGiveCount++;

	return dwRet;
}

/*****************************************************************************
3.ͬ����������ͬ���ź�������װ
******************************************************************************/
/*====================================================================
���ܣ�����һ����Ԫ�ź���
����˵����phSem: ���ص��ź������
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����. 
====================================================================*/
UINT32 OsaSemBCreate(OUT SEMHANDLE *phSem)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem* ptOsaSem = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
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

	//���뵽�ź������������
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListInsertLast(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	*phSem = (SEMHANDLE)(&(ptSemInfo->m_tOsaSem));
	return dwRet;
}

/*====================================================================
���ܣ�ɾ���ź���
����˵����phSem: ��ɾ���ź����ľ��
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����. 
====================================================================*/
UINT32 OsaSemDelete(SEMHANDLE hSem)
{
	UINT32 dwRet = 0;
	BOOL bRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem* ptOsaSem = (TOsaSem*)hSem;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	ptSemInfo = HOST_ENTRY(ptOsaSem, TOsaSemInfo, m_tOsaSem);	

	//�Ӽ�������ж���
	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);
	EListRemove(&ptMgr->m_tSemWatchList, &(ptSemInfo->m_tNode) );
	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	//�ͷŲ���ϵͳ���ź���
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

//�������ӵ��ź���p����
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

//�������ӵĴ���ʱ���ź���p����
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

//�������ӵ��ź���v����
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

	//�ж�give�����Ƿ����
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
���ܣ��ź���p������Ϊ�˷�����Ժ��������ⶨλ����ʹ�ú궨��OsaSemTake���м�������
����˵����	hSem: �ź������
			szSemName[]:�ź�����
			szFileName[]:�������ڵ��ļ���
			dwLine:�������ڵ���
����ֵ˵�����ɹ�����OSA_SEM_SIGNALED��OSA_OK)�����򷵻�OSA_SEM_TIMEOUT���ߴ�����
====================================================================*/
UINT32 InnerOsaSemTake(SEMHANDLE hSem,char szSemName[],char szFileName[],UINT32 dwLine)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem *ptOsaSem = (TOsaSem*)hSem;
	INT32 nFileNameLen = 0;
	char* pchFileNamePos = szFileName;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	dwRet = sInnerSemTakeNoWatch(hSem);
	if(dwRet != OSA_SEM_SIGNALED)
	{
		OsaLabPrt( OSA_ERROCC, "InnerOsaSemTake(), sInnerSemTakeByTimeNoWatch error,dwRet: 0x%x\n", dwRet);
		return dwRet;
	}	

	//����ź���
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
���ܣ�����ʱ���ź���p������Ϊ�˷�����Ժ��������ⶨλ����ʹ�ú궨��OsaSemTakeByTime���м�������
����˵����	hSem: �ź������
			dwTimeoutMs: ��ʱʱ�䣨ms��,��С����Ϊ10ms
			szSemName[]:�ź�����
			szFileName[]:�������ڵ��ļ���
			dwLine:�������ڵ���
����ֵ˵�����ɹ�����OSA_SEM_SIGNALED��OSA_OK)�����򷵻�OSA_SEM_TIMEOUT���ߴ�����
====================================================================*/
UINT32 InnerOsaSemTakeByTime(SEMHANDLE hSem, UINT32  dwTimeoutMs,char szSemName[],char szFileName[],UINT32 dwLine)
{
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem *ptOsaSem = (TOsaSem*)hSem;
	INT32 nFileNameLen = 0;
	char* pchFileNamePos = szFileName;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	dwRet = sInnerSemTakeByTimeNoWatch(hSem, dwTimeoutMs );
	if(dwRet != OSA_SEM_SIGNALED && dwRet != OSA_SEM_TIMEOUT )
	{	
		OsaLabPrt( OSA_ERROCC, "InnerOsaSemTakeByTime(), sInnerSemTakeByTimeNoWatch error,dwRet: 0x%x\n", dwRet);
		return dwRet;
	}	

	//����ź���
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
���ܣ��ź���v����
�������˵����hSem: �ź������
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����. 
====================================================================*/
UINT32 OsaSemGive(SEMHANDLE hSem )
{
	BOOL bGive=FALSE;
	UINT32 dwRet = 0;
	TOsaSemInfo *ptSemInfo = NULL;
	TOsaSem *ptOsaSem = (TOsaSem*)hSem;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( hSem, "hSem" );

	bGive = sInnerSemGiveNoWatch(hSem);	
	if(bGive)
	{
		//����ź���
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

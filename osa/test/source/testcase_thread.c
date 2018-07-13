#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//���ܲ���
typedef struct tagThreadTestInfo
{
	TASKHANDLE hTestThread;
	SEMHANDLE hSemThreadSync;	//�߳�ͬ����
	UINT32 dwOptRet; //�������
	BOOL bIsTestRun; //�����̺߳����Ƿ�����
	TASKHANDLE hRunThreadSelfHandle; //�����̻߳�ȡ����������

	UINT32 dwSleepMs; //�̺߳������еĺ�����
}TThreadTestInfo;

//�ж�ָ���߳��Ƿ���
BOOL iIsTestThreadActive( TASKHANDLE hThread )
{
	BOOL bGet = FALSE;
#ifdef _MSC_VER
	DWORD dwExitCode = 0;
	bGet = GetExitCodeThread(hThread, (DWORD *)&dwExitCode);
	if(bGet == 0)
	{
		return FALSE;
	}

	return (dwExitCode == STILL_ACTIVE);
#else
	int pthread_kill_err;
	pthread_kill_err = pthread_kill(hThread,0);

	if(pthread_kill_err == ESRCH)
		return FALSE;
	else if(pthread_kill_err == EINVAL)
		return FALSE;
	else
		return TRUE;
#endif

}

//���ܲ����̺߳���
static void sFunTestThreadPrc( void* pParam )
{
	UINT32 dwRet = 0;
	TThreadTestInfo* ptThreadTest = (TThreadTestInfo*)pParam;
	if( NULL == ptThreadTest )
	{
		return;
	}

	if( NULL == ptThreadTest->hSemThreadSync )
	{
		return;
	}

	//�ȴ�1��
	OsaSleep( 1000 );
	ptThreadTest->bIsTestRun = TRUE;

	ptThreadTest->dwOptRet = OsaThreadSelfHandleGet( &(ptThreadTest->hRunThreadSelfHandle) );

	//�ͷ�ͬ��
	dwRet = OsaSemGive( ptThreadTest->hSemThreadSync );
	if( OSA_OK == ptThreadTest->dwOptRet )
	{
		ptThreadTest->dwOptRet = dwRet;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadPrc run success!\n" );
	return;
}

//����������һ���̣߳�����һ���̣߳��̺߳������򵥲�����
void FunTest_ThreadCreate( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadCreate\n");
	//1.�����߳�ͬ���ź���
	dwRet = OsaSemBCreate( &tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
	if( dwRet != OSA_OK )
	{
		OsaLabPrt( TEST_ERROCC, "OsaSemBCreate() Error, ret=0x%x\n", dwRet);
		return;
	}
	
	dwRet = OsaSemTake( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_SEM_SIGNALED == dwRet );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//1.�����߳�
	dwRet = OsaThreadCreate( sFunTestThreadPrc, "sFunTestThreadPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.�ȴ��̲߳���,30����߳�һ�����˳�
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 30*1000 );
	CU_ASSERT( OSA_OK == dwRet );

	//3.��֤�̲߳�����ȷ
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );
	//���Ի�ȡ�����߳̾������ȷ��
	CU_ASSERT( tThreadTest.hTestThread  == tThreadTest.hRunThreadSelfHandle );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.��֤�߳��Ѿ��˳�
	//�ȴ�1��
	OsaSleep( 1000 );
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.ɾ���߳�ͬ���ź���
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}


//�����߳��Լ��˳�����
static void sFunTestThreadSelfKillPrc( void* pParam )
{
	TThreadTestInfo* ptThreadTest = (TThreadTestInfo*)pParam;
	if( NULL == ptThreadTest )
	{
		return;
	}

	if( NULL == ptThreadTest->hSemThreadSync )
	{
		return;
	}

	//�ȴ�1��
	OsaSleep( 1000 );
	ptThreadTest->bIsTestRun = TRUE;
	ptThreadTest->dwOptRet = OSA_OK;
	OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadSelfKillPrc run success��and begin kill self!\n" );

	ptThreadTest->dwOptRet = OsaThreadSelfExit();

	//Ӧ�ò������е�����
	ptThreadTest->dwOptRet = -1;
	//�ͷ�ͬ��
	ptThreadTest->dwOptRet = OsaSemGive( ptThreadTest->hSemThreadSync );
}

//�̺߳����ڲ��Լ��˳�
void FunTest_ThreadSelfExit( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadSelfExit\n");
	//1.�����߳�ͬ���ź���
	dwRet = OsaSemBCreate( &tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
	if( dwRet != OSA_OK )
	{
		OsaLabPrt( TEST_ERROCC, "OsaSemBCreate() Error, ret=0x%x\n", dwRet);
		return;
	}

	dwRet = OsaSemTake( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_SEM_SIGNALED == dwRet );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//1.�����߳�
	dwRet = OsaThreadCreate( sFunTestThreadSelfKillPrc, "sFunTestThreadSelfKillPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.�ȴ��̲߳���,5����߳�һ�����˳�
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 5*1000 );
	//�����ǳ�ʱ
	CU_ASSERT( OSA_SEM_TIMEOUT == dwRet );

	//3.��֤�̲߳�����ȷ
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.��֤�߳��Ѿ��˳�
	//�ȴ�1��
	OsaSleep( 1000 );
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.ɾ���߳�ͬ���ź���
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}

//�����߳�����˳�
void FunTest_ThreadExitWait( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadExitWait\n");
	//1.�����߳�ͬ���ź���
	dwRet = OsaSemBCreate( &tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
	if( dwRet != OSA_OK )
	{
		OsaLabPrt( TEST_ERROCC, "OsaSemBCreate() Error, ret=0x%x\n", dwRet);
		return;
	}

	dwRet = OsaSemTake( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_SEM_SIGNALED == dwRet );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//1.�����߳�
	dwRet = OsaThreadCreate( sFunTestThreadSelfKillPrc, "sFunTestThreadSelfKillPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.�ȴ��̲߳���,5����߳�һ�����˳�
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 5*1000 );
	//�����ǳ�ʱ
	CU_ASSERT( OSA_SEM_TIMEOUT == dwRet );

	//3.��֤�̲߳�����ȷ
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.�ȴ��߳��˳�
	dwRet = OsaThreadExitWait( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.ɾ���߳�ͬ���ź���
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}


//�����߳�ǿ���˳�����
static void sFunTestThreadKillPrc( void* pParam )
{
	TThreadTestInfo* ptThreadTest = (TThreadTestInfo*)pParam;
	if( NULL == ptThreadTest )
	{
		return;
	}

	if( NULL == ptThreadTest->hSemThreadSync )
	{
		return;
	}

	ptThreadTest->bIsTestRun = TRUE;
	ptThreadTest->dwOptRet = OSA_OK;
	ptThreadTest->dwSleepMs = 0;
	do 
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadKillPrc run success��now is alive, run millisecond:%d!\n", ptThreadTest->dwSleepMs );
		//�ȴ�1��
		OsaSleep( 1000 );
		ptThreadTest->dwSleepMs +=1000;
		if( ptThreadTest->dwSleepMs > 3 * 1000 )
		{
			break;
		}
	} while (1);

	//Ӧ�ò������е�����
	ptThreadTest->dwOptRet = -1;	
	//�ͷ�ͬ��
	ptThreadTest->dwOptRet = OsaSemGive( ptThreadTest->hSemThreadSync );
}

//�����߳�����˳�
void FunTest_ThreadKill( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadKill\n");
	//1.�����߳�ͬ���ź���
	dwRet = OsaSemBCreate( &tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
	if( dwRet != OSA_OK )
	{
		OsaLabPrt( TEST_ERROCC, "OsaSemBCreate() Error, ret=0x%x\n", dwRet);
		return;
	}

	dwRet = OsaSemTake( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_SEM_SIGNALED == dwRet );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//1.�����߳�
	dwRet = OsaThreadCreate( sFunTestThreadKillPrc, "sFunTestThreadKillPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//�ȴ�500����
	OsaSleep( 500 );

	//2.ǿ���߳��˳�
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//3.�ȴ��̲߳���,5����߳�һ�����˳�
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 5*1000 );
	//�����ǳ�ʱ
	CU_ASSERT( OSA_SEM_TIMEOUT == dwRet );

	//3.��֤�̲߳�����ȷ
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );
	CU_ASSERT( tThreadTest.dwSleepMs < 5*1000 );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.��֤�߳��Ѿ��˳�
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.ɾ���߳�ͬ���ź���
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}




//�����߳����к���, ע�⣬û��ʹ���߳�ͬ���ź���
static void sFunTestThreadRunPrc( void* pParam )
{
	TThreadTestInfo* ptThreadTest = (TThreadTestInfo*)pParam;
	if( NULL == ptThreadTest )
	{
		return;
	}

	ptThreadTest->bIsTestRun = TRUE;
	ptThreadTest->dwOptRet = OSA_OK;
	ptThreadTest->dwSleepMs = 0;
	do 
	{
		//�ȴ�1��
		OsaSleep( 10 );
		ptThreadTest->dwSleepMs +=10;
		//OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadRunPrc run success��now is alive, run millisecond:%d!\n", ptThreadTest->dwSleepMs );
	} while (1);

	//Ӧ�ò������е�����
	ptThreadTest->dwOptRet = -1;	
}

//�̹߳������
void FunTest_ThreadSuspendResume( void )
{
#ifdef _MSC_VER	
	UINT32 dwRet = OSA_OK;
	UINT32 i = 0;
	UINT32 dwRunBaseMs = 0;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadSuspendResume\n");
	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//1.�����߳�
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
							 OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.��ͣ�߳�
	dwRet = OsaThreadSuspend( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//��֤�̲߳�������
	dwRunBaseMs = tThreadTest.dwSleepMs;
	for( i = 0; i < 10; i++ )
	{
		//�ȴ�20����
		OsaSleep( 20 );
		CU_ASSERT( dwRunBaseMs == tThreadTest.dwSleepMs );		
	}

	//3.�ָ��߳�
	dwRet = OsaThreadResume( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//��֤�̼߳�������
	for( i = 0; i < 10; i++ )
	{
		//dwRunBaseMs = tThreadTest.dwSleepMs;
		//�ȴ�20����
		OsaSleep( 20 );
	}
	CU_ASSERT( dwRunBaseMs != tThreadTest.dwSleepMs );		

	//4.ǿ���߳��˳�
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//�ȴ�1��
	OsaSleep( 1000 );
	//5.��֤�߳��Ѿ��˳�
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test End: FunTest_ThreadSuspendResume\n" );
#endif

}

//�ı��̵߳����ȼ�
void FunTest_SetPriority( void )
{
	UINT32 dwRet = OSA_OK;
	UINT8 byPrioritySet = OSA_THREAD_PRI_NORMAL;
	UINT8 byPriorityGet = OSA_THREAD_PRI_NORMAL;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: FunTest_SetPriority\n" );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//1.�����߳�
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadSetPriority OSA_THREAD_PRI_ABOVE_NORMAL\n" );
	//2.����߳����ȼ�
	byPrioritySet = OSA_THREAD_PRI_ABOVE_NORMAL;
	dwRet = OsaThreadSetPriority( tThreadTest.hTestThread, byPrioritySet );
	CU_ASSERT( OSA_OK == dwRet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadGetPriority\n" );
	dwRet = OsaThreadGetPriority( tThreadTest.hTestThread, &byPriorityGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( byPriorityGet == byPrioritySet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadSetPriority OSA_THREAD_PRI_BELOW_NORMAL\n" );
	//3.�����߳����ȼ�
	byPrioritySet = OSA_THREAD_PRI_BELOW_NORMAL;
	dwRet = OsaThreadSetPriority( tThreadTest.hTestThread, byPrioritySet );
	CU_ASSERT( OSA_OK == dwRet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadGetPriority\n" );
	dwRet = OsaThreadGetPriority( tThreadTest.hTestThread, &byPriorityGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( byPriorityGet == byPrioritySet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadKill\n" );
	//4.ǿ���߳��˳�
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//�ȴ�1��
	OsaSleep( 1000 );
	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority iIsTestThreadActive\n" );
	//5.��֤�߳��Ѿ��˳�
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}

//////////////////////////////////////////////////////////////////////////
//�쳣����
//����������һ���߳�
void ExcptTest_ThreadCreate( void )
{
	UINT32 dwRet = OSA_OK;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadCreate\n" );

	//���̺߳���
	dwRet = OsaThreadCreate( NULL, "NullFunc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK != dwRet );

	//��Ч�����ȼ�
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_CRITICAL + 1,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK != dwRet );

	//��Ч���߳̾������
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, NULL );
	CU_ASSERT( OSA_OK != dwRet );

}


//�����߳�����˳�
void ExcptTest_ThreadKill( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadKill\n" );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//���������߳�
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//1.ǿ���߳��˳�:NULL
	dwRet = OsaThreadKill( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//2.ǿ���߳��˳�:��Ч�ľ��
	dwRet = OsaThreadKill( (TASKHANDLE)123 );
	CU_ASSERT( OSA_OK != dwRet );

	//����ǿ���߳��˳�
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//�ȴ�1��
	OsaSleep( 1000 );
	//5.��֤�߳��Ѿ��˳�
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}

//���Ի�ȡ�����߳̾���쳣���к���
static void sExcptTestThreadSelfHandleGetPrc( void* pParam )
{
	UINT32 dwRet = 0;
	TThreadTestInfo* ptThreadTest = (TThreadTestInfo*)pParam;
	if( NULL == ptThreadTest )
	{
		return;
	}

	if( NULL == ptThreadTest->hSemThreadSync )
	{
		return;
	}

	//�ȴ�100����
	OsaSleep( 100 );
	ptThreadTest->bIsTestRun = TRUE;
	//NULL POINTER test
	ptThreadTest->dwOptRet = OsaThreadSelfHandleGet( NULL );

	//�ͷ�ͬ��
	dwRet = OsaSemGive( ptThreadTest->hSemThreadSync );
	if( OSA_OK == ptThreadTest->dwOptRet )
	{
		ptThreadTest->dwOptRet = dwRet;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadPrc run success!\n" );
	return;
}

//��ȡ��ǰ�����̵߳ľ��
void ExcptTest_ThreadSelfHandleGet( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TASKHANDLE hTestThreadGet = 0;
	TThreadTestInfo tThreadTest = { 0 };

	//////////////////////////////////////////////////////////////////////////
	//����1���̺߳����У���ָ���ȡ������
	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadSelfHandleGet\n" );

	//1.�����߳�ͬ���ź���
	dwRet = OsaSemBCreate( &tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
	if( dwRet != OSA_OK )
	{
		OsaLabPrt( TEST_ERROCC, "OsaSemBCreate() Error, ret=0x%x\n", dwRet);
		return;
	}

	dwRet = OsaSemTake( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_SEM_SIGNALED == dwRet );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//1.�����߳�
	dwRet = OsaThreadCreate( sExcptTestThreadSelfHandleGetPrc, "sExcptTestThreadSelfHandleGetPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.�ȴ��̲߳���,30����߳�һ�����˳�
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 30*1000 );
	CU_ASSERT( OSA_OK == dwRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//3.��֤�̲߳�������ȷ
	CU_ASSERT( OSA_OK != tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );
	//���Ի�ȡ�����߳̾���Ĳ���ȷ
	CU_ASSERT( tThreadTest.hTestThread != tThreadTest.hRunThreadSelfHandle );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//////////////////////////////////////////////////////////////////////////
	//����2����ǰ����OsaThreadCreate�������߳̾��
	dwRet = OsaThreadSelfHandleGet( &hTestThreadGet );
	CU_ASSERT( OSA_OK != dwRet );

	//4.ɾ���߳�ͬ���ź���
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

}


//�̹߳������
void ExcptTest_ThreadSuspendResume( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadSuspendResume\n" );
	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//���������߳�
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//1.�̹߳��� :NULL
	dwRet = OsaThreadSuspend( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//2.�̼߳��� :NULL
	dwRet = OsaThreadSuspend( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//����ǿ���߳��˳�
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//�ȴ�1��
	OsaSleep( 1000 );
	//5.��֤�߳��Ѿ��˳�
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}
//�ı��̵߳����ȼ�
void ExcptTest_SetPriority( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	UINT8 byPrioritySet = OSA_THREAD_PRI_NORMAL;
	UINT8 byPriorityGet = OSA_THREAD_PRI_NORMAL;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_SetPriority\n" );
	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//���������߳�
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//1.�ı��̵߳����ȼ� :NULL
	byPrioritySet = OSA_THREAD_PRI_BELOW_NORMAL;
	dwRet = OsaThreadSetPriority( NULL, byPrioritySet );
	CU_ASSERT( OSA_OK != dwRet );

	//2.�ı��̵߳����ȼ� :��Ч����
	byPrioritySet = 11;
	dwRet = OsaThreadSetPriority( tThreadTest.hTestThread, byPrioritySet );
	CU_ASSERT( OSA_OK != dwRet );

	//1.����̵߳����ȼ� :NULL
	dwRet = OsaThreadGetPriority( NULL, &byPriorityGet );
	CU_ASSERT( OSA_OK != dwRet );

	//����ǿ���߳��˳�
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//�ȴ�1��
	OsaSleep( 1000 );
	//5.��֤�߳��Ѿ��˳�
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}
#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//功能测试
typedef struct tagThreadTestInfo
{
	TASKHANDLE hTestThread;
	SEMHANDLE hSemThreadSync;	//线程同步锁
	UINT32 dwOptRet; //操作结果
	BOOL bIsTestRun; //测试线程函数是否运行
	TASKHANDLE hRunThreadSelfHandle; //运行线程获取到的自身句柄

	UINT32 dwSleepMs; //线程函数运行的毫秒数
}TThreadTestInfo;

//判断指定线程是否存活
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

//功能测试线程函数
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

	//等待1秒
	OsaSleep( 1000 );
	ptThreadTest->bIsTestRun = TRUE;

	ptThreadTest->dwOptRet = OsaThreadSelfHandleGet( &(ptThreadTest->hRunThreadSelfHandle) );

	//释放同步
	dwRet = OsaSemGive( ptThreadTest->hSemThreadSync );
	if( OSA_OK == ptThreadTest->dwOptRet )
	{
		ptThreadTest->dwOptRet = dwRet;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadPrc run success!\n" );
	return;
}

//创建并激活一个线程（创建一个线程，线程函数做简单操作）
void FunTest_ThreadCreate( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadCreate\n");
	//1.创建线程同步信号量
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
	//1.创建线程
	dwRet = OsaThreadCreate( sFunTestThreadPrc, "sFunTestThreadPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.等待线程操作,30秒后线程一定会退出
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 30*1000 );
	CU_ASSERT( OSA_OK == dwRet );

	//3.验证线程操作正确
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );
	//测试获取自身线程句柄的正确性
	CU_ASSERT( tThreadTest.hTestThread  == tThreadTest.hRunThreadSelfHandle );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.验证线程已经退出
	//等待1秒
	OsaSleep( 1000 );
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.删除线程同步信号量
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}


//测试线程自己退出函数
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

	//等待1秒
	OsaSleep( 1000 );
	ptThreadTest->bIsTestRun = TRUE;
	ptThreadTest->dwOptRet = OSA_OK;
	OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadSelfKillPrc run success，and begin kill self!\n" );

	ptThreadTest->dwOptRet = OsaThreadSelfExit();

	//应该不会运行到这里
	ptThreadTest->dwOptRet = -1;
	//释放同步
	ptThreadTest->dwOptRet = OsaSemGive( ptThreadTest->hSemThreadSync );
}

//线程函数内部自己退出
void FunTest_ThreadSelfExit( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadSelfExit\n");
	//1.创建线程同步信号量
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
	//1.创建线程
	dwRet = OsaThreadCreate( sFunTestThreadSelfKillPrc, "sFunTestThreadSelfKillPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.等待线程操作,5秒后线程一定会退出
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 5*1000 );
	//必须是超时
	CU_ASSERT( OSA_SEM_TIMEOUT == dwRet );

	//3.验证线程操作正确
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.验证线程已经退出
	//等待1秒
	OsaSleep( 1000 );
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.删除线程同步信号量
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}

//结束线程完成退出
void FunTest_ThreadExitWait( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadExitWait\n");
	//1.创建线程同步信号量
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
	//1.创建线程
	dwRet = OsaThreadCreate( sFunTestThreadSelfKillPrc, "sFunTestThreadSelfKillPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.等待线程操作,5秒后线程一定会退出
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 5*1000 );
	//必须是超时
	CU_ASSERT( OSA_SEM_TIMEOUT == dwRet );

	//3.验证线程操作正确
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.等待线程退出
	dwRet = OsaThreadExitWait( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.删除线程同步信号量
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}


//测试线程强制退出函数
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
		OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadKillPrc run success，now is alive, run millisecond:%d!\n", ptThreadTest->dwSleepMs );
		//等待1秒
		OsaSleep( 1000 );
		ptThreadTest->dwSleepMs +=1000;
		if( ptThreadTest->dwSleepMs > 3 * 1000 )
		{
			break;
		}
	} while (1);

	//应该不会运行到这里
	ptThreadTest->dwOptRet = -1;	
	//释放同步
	ptThreadTest->dwOptRet = OsaSemGive( ptThreadTest->hSemThreadSync );
}

//结束线程完成退出
void FunTest_ThreadKill( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	printf( "Start FunTest_ThreadKill\n");
	//1.创建线程同步信号量
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
	//1.创建线程
	dwRet = OsaThreadCreate( sFunTestThreadKillPrc, "sFunTestThreadKillPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//等待500毫秒
	OsaSleep( 500 );

	//2.强制线程退出
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//3.等待线程操作,5秒后线程一定会退出
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 5*1000 );
	//必须是超时
	CU_ASSERT( OSA_SEM_TIMEOUT == dwRet );

	//3.验证线程操作正确
	CU_ASSERT( OSA_OK == tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );
	CU_ASSERT( tThreadTest.dwSleepMs < 5*1000 );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//4.验证线程已经退出
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//5.删除线程同步信号量
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );
}




//测试线程运行函数, 注意，没有使用线程同步信号量
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
		//等待1秒
		OsaSleep( 10 );
		ptThreadTest->dwSleepMs +=10;
		//OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadRunPrc run success，now is alive, run millisecond:%d!\n", ptThreadTest->dwSleepMs );
	} while (1);

	//应该不会运行到这里
	ptThreadTest->dwOptRet = -1;	
}

//线程挂起继续
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
	//1.创建线程
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
							 OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.暂停线程
	dwRet = OsaThreadSuspend( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//验证线程不再运行
	dwRunBaseMs = tThreadTest.dwSleepMs;
	for( i = 0; i < 10; i++ )
	{
		//等待20毫秒
		OsaSleep( 20 );
		CU_ASSERT( dwRunBaseMs == tThreadTest.dwSleepMs );		
	}

	//3.恢复线程
	dwRet = OsaThreadResume( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//验证线程继续运行
	for( i = 0; i < 10; i++ )
	{
		//dwRunBaseMs = tThreadTest.dwSleepMs;
		//等待20毫秒
		OsaSleep( 20 );
	}
	CU_ASSERT( dwRunBaseMs != tThreadTest.dwSleepMs );		

	//4.强制线程退出
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//等待1秒
	OsaSleep( 1000 );
	//5.验证线程已经退出
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test End: FunTest_ThreadSuspendResume\n" );
#endif

}

//改变线程的优先级
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
	//1.创建线程
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadSetPriority OSA_THREAD_PRI_ABOVE_NORMAL\n" );
	//2.提高线程优先级
	byPrioritySet = OSA_THREAD_PRI_ABOVE_NORMAL;
	dwRet = OsaThreadSetPriority( tThreadTest.hTestThread, byPrioritySet );
	CU_ASSERT( OSA_OK == dwRet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadGetPriority\n" );
	dwRet = OsaThreadGetPriority( tThreadTest.hTestThread, &byPriorityGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( byPriorityGet == byPrioritySet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadSetPriority OSA_THREAD_PRI_BELOW_NORMAL\n" );
	//3.降低线程优先级
	byPrioritySet = OSA_THREAD_PRI_BELOW_NORMAL;
	dwRet = OsaThreadSetPriority( tThreadTest.hTestThread, byPrioritySet );
	CU_ASSERT( OSA_OK == dwRet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadGetPriority\n" );
	dwRet = OsaThreadGetPriority( tThreadTest.hTestThread, &byPriorityGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( byPriorityGet == byPrioritySet );

	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority OsaThreadKill\n" );
	//4.强制线程退出
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//等待1秒
	OsaSleep( 1000 );
	OsaLabPrt( GENERAL_PRTLAB_OCC, "FunTest_SetPriority iIsTestThreadActive\n" );
	//5.验证线程已经退出
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}

//////////////////////////////////////////////////////////////////////////
//异常测试
//创建并激活一个线程
void ExcptTest_ThreadCreate( void )
{
	UINT32 dwRet = OSA_OK;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadCreate\n" );

	//空线程函数
	dwRet = OsaThreadCreate( NULL, "NullFunc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK != dwRet );

	//无效的优先级
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_CRITICAL + 1,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK != dwRet );

	//无效的线程句柄参数
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, NULL );
	CU_ASSERT( OSA_OK != dwRet );

}


//结束线程完成退出
void ExcptTest_ThreadKill( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadKill\n" );

	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//正常创建线程
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//1.强制线程退出:NULL
	dwRet = OsaThreadKill( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//2.强制线程退出:无效的句柄
	dwRet = OsaThreadKill( (TASKHANDLE)123 );
	CU_ASSERT( OSA_OK != dwRet );

	//正常强制线程退出
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//等待1秒
	OsaSleep( 1000 );
	//5.验证线程已经退出
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}

//测试获取自身线程句柄异常运行函数
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

	//等待100毫秒
	OsaSleep( 100 );
	ptThreadTest->bIsTestRun = TRUE;
	//NULL POINTER test
	ptThreadTest->dwOptRet = OsaThreadSelfHandleGet( NULL );

	//释放同步
	dwRet = OsaSemGive( ptThreadTest->hSemThreadSync );
	if( OSA_OK == ptThreadTest->dwOptRet )
	{
		ptThreadTest->dwOptRet = dwRet;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "sFunTestThreadPrc run success!\n" );
	return;
}

//获取当前运行线程的句柄
void ExcptTest_ThreadSelfHandleGet( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TASKHANDLE hTestThreadGet = 0;
	TThreadTestInfo tThreadTest = { 0 };

	//////////////////////////////////////////////////////////////////////////
	//用例1：线程函数中，空指针获取自身句柄
	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadSelfHandleGet\n" );

	//1.创建线程同步信号量
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
	//1.创建线程
	dwRet = OsaThreadCreate( sExcptTestThreadSelfHandleGetPrc, "sExcptTestThreadSelfHandleGetPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//2.等待线程操作,30秒后线程一定会退出
	dwRet = OsaSemTakeByTime( tThreadTest.hSemThreadSync, 30*1000 );
	CU_ASSERT( OSA_OK == dwRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//3.验证线程操作不正确
	CU_ASSERT( OSA_OK != tThreadTest.dwOptRet );
	CU_ASSERT( TRUE == tThreadTest.bIsTestRun );
	//测试获取自身线程句柄的不正确
	CU_ASSERT( tThreadTest.hTestThread != tThreadTest.hRunThreadSelfHandle );

	dwRet = OsaSemGive( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

	//////////////////////////////////////////////////////////////////////////
	//用例2：当前不是OsaThreadCreate创建的线程句柄
	dwRet = OsaThreadSelfHandleGet( &hTestThreadGet );
	CU_ASSERT( OSA_OK != dwRet );

	//4.删除线程同步信号量
	dwRet = OsaSemDelete( tThreadTest.hSemThreadSync );
	CU_ASSERT( OSA_OK == dwRet );

}


//线程挂起继续
void ExcptTest_ThreadSuspendResume( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bRet = FALSE;
	TThreadTestInfo tThreadTest = { 0 };

	OsaLabPrt( GENERAL_PRTLAB_OCC, "Test Start: ExcptTest_ThreadSuspendResume\n" );
	tThreadTest.dwOptRet = -1;
	tThreadTest.bIsTestRun = FALSE;
	//正常创建线程
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//1.线程挂起 :NULL
	dwRet = OsaThreadSuspend( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//2.线程继续 :NULL
	dwRet = OsaThreadSuspend( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//正常强制线程退出
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//等待1秒
	OsaSleep( 1000 );
	//5.验证线程已经退出
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}
//改变线程的优先级
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
	//正常创建线程
	dwRet = OsaThreadCreate( sFunTestThreadRunPrc, "sFunTestThreadRunPrc", OSA_THREAD_PRI_NORMAL,
		OSA_DFT_STACKSIZE, (void*)&tThreadTest, FALSE, &tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( tThreadTest.hTestThread != NULL );

	//1.改变线程的优先级 :NULL
	byPrioritySet = OSA_THREAD_PRI_BELOW_NORMAL;
	dwRet = OsaThreadSetPriority( NULL, byPrioritySet );
	CU_ASSERT( OSA_OK != dwRet );

	//2.改变线程的优先级 :无效参数
	byPrioritySet = 11;
	dwRet = OsaThreadSetPriority( tThreadTest.hTestThread, byPrioritySet );
	CU_ASSERT( OSA_OK != dwRet );

	//1.获得线程的优先级 :NULL
	dwRet = OsaThreadGetPriority( NULL, &byPriorityGet );
	CU_ASSERT( OSA_OK != dwRet );

	//正常强制线程退出
	dwRet = OsaThreadKill( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );

	//等待1秒
	OsaSleep( 1000 );
	//5.验证线程已经退出
	bRet = iIsTestThreadActive( tThreadTest.hTestThread );
	CU_ASSERT( FALSE == bRet );
	dwRet = OsaThreadHandleClose( tThreadTest.hTestThread );
	CU_ASSERT( OSA_OK == dwRet );
}
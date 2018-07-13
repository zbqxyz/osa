#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//功能测试


void utestprt()
{
	OsaLabPrt( GENERAL_PRTLAB_OCC, "call TestCmd [utestprt] success!\n" );
}

void utestprt2()
{
	OsaLabPrt( GENERAL_PRTLAB_OCC, "call TestCmd [utestprt2] success!\n" );
}

void utestprt3()
{
	OsaLabPrt( GENERAL_PRTLAB_OCC, "call TestCmd [utestprt3] success!\n" );
}

//注册调试命令
void FunTest_CmdReg( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaCmdReg( "utestprt", (void*)utestprt, "utestprt:单元测试调试函数" );
	CU_ASSERT( OSA_OK == dwRet );

	dwRet = FastOsaCmdReg( utestprt2, "utestprt2:单元测试调试函数2" );
	CU_ASSERT( OSA_OK == dwRet );

}

//分类标签名称与OCC值的转换
void FunTest_Str2Occ( void )
{
	BOOL bRet = 0;
	INT32 nRet = 0;
	OCC qwTestOcc = 0;
	const char* szTestOccName = "UTestOcc";
	char szTestOccName2[10] = { '\0' };
	qwTestOcc = OsaStr2Occ( szTestOccName );
	CU_ASSERT( qwTestOcc > 0 );

	bRet = OsaOcc2Str( qwTestOcc, szTestOccName2, 10 );
	CU_ASSERT( TRUE == bRet );

	//验证转换回来字符相同
	nRet = strncmp( szTestOccName, szTestOccName2, 10 );
	CU_ASSERT( 0 == nRet );
}

//打印标签操作
void FunTest_PrtLab( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bExist = FALSE;
	UINT32 i = 0;
	char szTestOccName[10] = { '\0' };
	OCC qwTestOcc = OsaStr2Occ( "TestLab" );

	//增加
	dwRet = OsaPrtLabAdd( qwTestOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestOcc );
	CU_ASSERT( TRUE == bExist );

	//删除
	dwRet = OsaPrtLabDel( qwTestOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestOcc );
	CU_ASSERT( FALSE == bExist );

	//增加多个
	for( i = 0; i < 10; i++ )
	{
		sprintf( szTestOccName, "TestLab%d", i+1 );
		qwTestOcc = OsaStr2Occ( "szTestOccName" );
		dwRet = OsaPrtLabAdd( qwTestOcc );
		CU_ASSERT( OSA_OK == dwRet );
		bExist = OsaIsPrtLabExist( qwTestOcc );
		CU_ASSERT( TRUE == bExist );
	}

	//全部清空
	dwRet = OsaPrtLabClearAll( );	
	CU_ASSERT( OSA_OK == dwRet );

	OsaPrtLabAdd( GENERAL_PRTLAB_OCC ); //为了打印显示，必须增加通用打印标签，否则telnet什么也显示了
	OsaPrtLabAdd( OsaStr2Occ("OsaErr") );//不要关了错误打印，要不所有错误就看不到了
	OsaPrtLabAdd( OsaStr2Occ("OSA_MSGOCC") );
	OsaPrtLabAdd( TEST_ERROCC );

	for( i = 0; i < 10; i++ )
	{
		sprintf( szTestOccName, "TestLab%d", i+1 );
		qwTestOcc = OsaStr2Occ( "szTestOccName" );
		bExist = OsaIsPrtLabExist( qwTestOcc );
		CU_ASSERT( FALSE == bExist );
	}
}

//文件日志
void FunTest_FileLog( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bExist = FALSE;
	UINT32 i = 0;
	char szTestLogInfo[64] = { '\0' };
	OCC qwTestErrOcc = TEST_ERROCC;
	OCC qwTestLogOcc = OsaStr2Occ( "TestLog" );

	//增加
// 	dwRet = OsaPrtLabAdd( qwTestErrOcc );
// 	CU_ASSERT( OSA_OK == dwRet );
	dwRet = OsaPrtLabAdd( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestErrOcc );
	CU_ASSERT( TRUE == bExist );
	bExist = OsaIsPrtLabExist( qwTestLogOcc );
	CU_ASSERT( TRUE == bExist );
	//增加多个
	for( i = 0; i < 10; i++ )
	{
		sprintf( szTestLogInfo, "!!!Test Err Print To File,Idx:%d\n", i+1 );
		OsaLabPrt(qwTestErrOcc, szTestLogInfo );
		sprintf( szTestLogInfo, "!!!Test Log Print To File,Idx:%d\n", i+1 );
		OsaLabPrt(qwTestLogOcc, szTestLogInfo );
	}	

	//删除
	dwRet = OsaPrtLabDel( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestLogOcc );
	CU_ASSERT( FALSE == bExist );
	
}

//////////////////////////////////////////////////////////////////////////
//异常测试
//注册调试命令
void ExcptTest_CmdReg( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaCmdReg( NULL, (void*)utestprt3, "utestprt3:单元测试调试函数" );
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = OsaCmdReg( "utestprt3", NULL, "utestprt3:单元测试调试函数" );
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = OsaCmdReg( "utestprt3", (void*)utestprt3, NULL);
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = FastOsaCmdReg( NULL, "utestprt:单元测试调试函数2" );
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = FastOsaCmdReg( utestprt, NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

//分类标签名称与OCC值的转换
void ExcptTest_Str2Occ( void )
{
	BOOL bRet = 0;
	OCC qwTestOcc = 0;
	char szTestOccNameBuf[10] = { '\0' };
	qwTestOcc = OsaStr2Occ( NULL );
	CU_ASSERT( qwTestOcc == 0 );

	bRet = OsaOcc2Str( qwTestOcc, NULL, 10 );
	CU_ASSERT( FALSE == bRet );

	bRet = OsaOcc2Str( qwTestOcc, szTestOccNameBuf, 7 );
	CU_ASSERT( FALSE == bRet );

}

//////////////////////////////////////////////////////////////////////////


//测试线程函数
static void sPerLogTestThreadPrc( void* pParam )
{
	UINT32 dwRet = 0;
#ifdef _MSC_VER
	UINT32 dwID = (UINT32)pParam;
#else
	UINT64 dwID = (UINT64)pParam;
#endif
	UINT32 dwCount=0;
	OCC qwTestLogOcc = OsaStr2Occ( "PersLog" );
	char szTestLogInfo[64] = { '\0' };
	while( 1 )
	{
		dwCount++;
#ifdef _MSC_VER
		sprintf( szTestLogInfo, "!!!PerTest Thread Idx：%d, Print To File,Log Count:%d\n", dwID+1, dwCount );
#else
		sprintf( szTestLogInfo, "!!!PerTest Thread Idx：%llu, Print To File,Log Count:%d\n", dwID+1, dwCount );
#endif
		
		OsaLabPrt(qwTestLogOcc, szTestLogInfo );
		OsaSleep( 10 );
	}
	return;
}


void ppp()
{
	UINT32 i = 0;
	UINT32 dwLineNum = 1000;
	OsaLabPrt( GENERAL_PRTLAB_OCC, "perssure telnet print %d line to test:\n", dwLineNum );
	for( i = 0; i < dwLineNum; i++)
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, " line:%d, call TestCmd [ppp] success!\n",i+ 1 );
	}
}

//压力测试
void PersTest_TelnetAndLog(void )
{
	UINT32 dwRet = OSA_OK;
	UINT32 i = 0;
	TASKHANDLE hTestThread = NULL;
	OCC qwTestLogOcc = OsaStr2Occ( "PersLog" );
	
	//注册打印很多信息的调试命令
	dwRet = FastOsaCmdReg( (void*)ppp, "ppp:狂打印压力测试" );
	CU_ASSERT( OSA_OK == dwRet );

	//增加
	dwRet = OsaPrtLabAdd( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );

	//开启多个线程同时进行日志写入
	for( i = 0; i < 10; i++ )
	{
		//1.创建线程
		dwRet = OsaThreadCreate( sPerLogTestThreadPrc, "sPerLogTestThreadPrc", OSA_THREAD_PRI_NORMAL,
			OSA_DFT_STACKSIZE, (void*)i, FALSE, &hTestThread );
		CU_ASSERT( OSA_OK == dwRet );
	}	

	//循环不退出
	while( 1 )
	{
		OsaSleep( 100 );
	}

	//删除
	dwRet = OsaPrtLabDel( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );
}



//线程创建函数
static void sPerTestThreadCreatePrc( void* pParam )
{
	UINT32 dwRet = 0;
#ifdef _MSC_VER
	UINT32 dwID = (UINT32)pParam;
#else
	UINT64 dwID = (UINT64)pParam;
#endif
	char szTestLogInfo[64] = { '\0' };
	
	do
	{
#ifdef _MSC_VER
		sprintf( szTestLogInfo, "!!!PerTest Thread Create Idx：%d, Print To File.\n", dwID+1 );
#else
		sprintf( szTestLogInfo, "!!!PerTest Thread Create Idx：%llu, Print To File.\n", dwID+1 );
#endif

		OsaLabPrt(GENERAL_PRTLAB_OCC, szTestLogInfo );
		OsaSleep( 10 );
	}while( 0 );
	return;
}

void PersTest_ThreadCreateClose(void )
{
	UINT32 dwRet = OSA_OK;
	UINT32 i = 0;
	UINT32 dwThreadCount = 1000;
	TASKHANDLE ahTestThread[1000];
	//开启多个线程同时进行日志写入
	for( i = 0; i < dwThreadCount; i++ )
	{
		ahTestThread[i] = NULL;
		//创建线程
		dwRet = OsaThreadCreate( sPerTestThreadCreatePrc, "sPerTestThreadCreatePrc", OSA_THREAD_PRI_NORMAL,
			OSA_DFT_STACKSIZE, (void*)i, FALSE, &ahTestThread[i] );
		CU_ASSERT( OSA_OK == dwRet );
		OsaSleep( 10 );
	}	

	for( i = 0; i < dwThreadCount; i++ )
	{
		//关闭线程句柄
		dwRet = OsaThreadHandleClose( ahTestThread[i] );
		CU_ASSERT( OSA_OK == dwRet );
		OsaSleep( 10 );
	}	
}

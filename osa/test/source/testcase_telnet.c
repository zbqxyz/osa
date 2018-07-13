#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//���ܲ���


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

//ע���������
void FunTest_CmdReg( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaCmdReg( "utestprt", (void*)utestprt, "utestprt:��Ԫ���Ե��Ժ���" );
	CU_ASSERT( OSA_OK == dwRet );

	dwRet = FastOsaCmdReg( utestprt2, "utestprt2:��Ԫ���Ե��Ժ���2" );
	CU_ASSERT( OSA_OK == dwRet );

}

//�����ǩ������OCCֵ��ת��
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

	//��֤ת�������ַ���ͬ
	nRet = strncmp( szTestOccName, szTestOccName2, 10 );
	CU_ASSERT( 0 == nRet );
}

//��ӡ��ǩ����
void FunTest_PrtLab( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bExist = FALSE;
	UINT32 i = 0;
	char szTestOccName[10] = { '\0' };
	OCC qwTestOcc = OsaStr2Occ( "TestLab" );

	//����
	dwRet = OsaPrtLabAdd( qwTestOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestOcc );
	CU_ASSERT( TRUE == bExist );

	//ɾ��
	dwRet = OsaPrtLabDel( qwTestOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestOcc );
	CU_ASSERT( FALSE == bExist );

	//���Ӷ��
	for( i = 0; i < 10; i++ )
	{
		sprintf( szTestOccName, "TestLab%d", i+1 );
		qwTestOcc = OsaStr2Occ( "szTestOccName" );
		dwRet = OsaPrtLabAdd( qwTestOcc );
		CU_ASSERT( OSA_OK == dwRet );
		bExist = OsaIsPrtLabExist( qwTestOcc );
		CU_ASSERT( TRUE == bExist );
	}

	//ȫ�����
	dwRet = OsaPrtLabClearAll( );	
	CU_ASSERT( OSA_OK == dwRet );

	OsaPrtLabAdd( GENERAL_PRTLAB_OCC ); //Ϊ�˴�ӡ��ʾ����������ͨ�ô�ӡ��ǩ������telnetʲôҲ��ʾ��
	OsaPrtLabAdd( OsaStr2Occ("OsaErr") );//��Ҫ���˴����ӡ��Ҫ�����д���Ϳ�������
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

//�ļ���־
void FunTest_FileLog( void )
{
	UINT32 dwRet = OSA_OK;
	BOOL bExist = FALSE;
	UINT32 i = 0;
	char szTestLogInfo[64] = { '\0' };
	OCC qwTestErrOcc = TEST_ERROCC;
	OCC qwTestLogOcc = OsaStr2Occ( "TestLog" );

	//����
// 	dwRet = OsaPrtLabAdd( qwTestErrOcc );
// 	CU_ASSERT( OSA_OK == dwRet );
	dwRet = OsaPrtLabAdd( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestErrOcc );
	CU_ASSERT( TRUE == bExist );
	bExist = OsaIsPrtLabExist( qwTestLogOcc );
	CU_ASSERT( TRUE == bExist );
	//���Ӷ��
	for( i = 0; i < 10; i++ )
	{
		sprintf( szTestLogInfo, "!!!Test Err Print To File,Idx:%d\n", i+1 );
		OsaLabPrt(qwTestErrOcc, szTestLogInfo );
		sprintf( szTestLogInfo, "!!!Test Log Print To File,Idx:%d\n", i+1 );
		OsaLabPrt(qwTestLogOcc, szTestLogInfo );
	}	

	//ɾ��
	dwRet = OsaPrtLabDel( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );

	bExist = OsaIsPrtLabExist( qwTestLogOcc );
	CU_ASSERT( FALSE == bExist );
	
}

//////////////////////////////////////////////////////////////////////////
//�쳣����
//ע���������
void ExcptTest_CmdReg( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaCmdReg( NULL, (void*)utestprt3, "utestprt3:��Ԫ���Ե��Ժ���" );
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = OsaCmdReg( "utestprt3", NULL, "utestprt3:��Ԫ���Ե��Ժ���" );
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = OsaCmdReg( "utestprt3", (void*)utestprt3, NULL);
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = FastOsaCmdReg( NULL, "utestprt:��Ԫ���Ե��Ժ���2" );
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = FastOsaCmdReg( utestprt, NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

//�����ǩ������OCCֵ��ת��
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


//�����̺߳���
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
		sprintf( szTestLogInfo, "!!!PerTest Thread Idx��%d, Print To File,Log Count:%d\n", dwID+1, dwCount );
#else
		sprintf( szTestLogInfo, "!!!PerTest Thread Idx��%llu, Print To File,Log Count:%d\n", dwID+1, dwCount );
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

//ѹ������
void PersTest_TelnetAndLog(void )
{
	UINT32 dwRet = OSA_OK;
	UINT32 i = 0;
	TASKHANDLE hTestThread = NULL;
	OCC qwTestLogOcc = OsaStr2Occ( "PersLog" );
	
	//ע���ӡ�ܶ���Ϣ�ĵ�������
	dwRet = FastOsaCmdReg( (void*)ppp, "ppp:���ӡѹ������" );
	CU_ASSERT( OSA_OK == dwRet );

	//����
	dwRet = OsaPrtLabAdd( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );

	//��������߳�ͬʱ������־д��
	for( i = 0; i < 10; i++ )
	{
		//1.�����߳�
		dwRet = OsaThreadCreate( sPerLogTestThreadPrc, "sPerLogTestThreadPrc", OSA_THREAD_PRI_NORMAL,
			OSA_DFT_STACKSIZE, (void*)i, FALSE, &hTestThread );
		CU_ASSERT( OSA_OK == dwRet );
	}	

	//ѭ�����˳�
	while( 1 )
	{
		OsaSleep( 100 );
	}

	//ɾ��
	dwRet = OsaPrtLabDel( qwTestLogOcc );
	CU_ASSERT( OSA_OK == dwRet );
}



//�̴߳�������
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
		sprintf( szTestLogInfo, "!!!PerTest Thread Create Idx��%d, Print To File.\n", dwID+1 );
#else
		sprintf( szTestLogInfo, "!!!PerTest Thread Create Idx��%llu, Print To File.\n", dwID+1 );
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
	//��������߳�ͬʱ������־д��
	for( i = 0; i < dwThreadCount; i++ )
	{
		ahTestThread[i] = NULL;
		//�����߳�
		dwRet = OsaThreadCreate( sPerTestThreadCreatePrc, "sPerTestThreadCreatePrc", OSA_THREAD_PRI_NORMAL,
			OSA_DFT_STACKSIZE, (void*)i, FALSE, &ahTestThread[i] );
		CU_ASSERT( OSA_OK == dwRet );
		OsaSleep( 10 );
	}	

	for( i = 0; i < dwThreadCount; i++ )
	{
		//�ر��߳̾��
		dwRet = OsaThreadHandleClose( ahTestThread[i] );
		CU_ASSERT( OSA_OK == dwRet );
		OsaSleep( 10 );
	}	
}

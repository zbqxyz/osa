#include "stdafx.h"

TOsaTestMgr g_tTestMgr = { 0 };

//��ʼ������ģ��
BOOL TestModuleInit( const char* szCurWorkPath  )
{
	UINT32 dwRet = 0;
	UINT32 dwRsrcTag = 0;
	TOsaInitParam tInitParam = { 0 };
	tInitParam.wTelnetPort = 1234;
	strncpy( tInitParam.szLogFilePrefixName, "UnitTestPreFix", 32 ); 
	//strncpy( tInitParam.szModuleVer, "vda3.0.3.3", 32 ); 

	if( NULL == szCurWorkPath )
	{
		return FALSE;
	}

	//��ʼ��
	dwRet = OsaInit( tInitParam );
	if( dwRet != OSA_OK  )
	{
		return FALSE;
	}

	//����ͨ�ô�ӡ
	dwRet = OsaPrtLabAdd( TEST_ERROCC );
	if( dwRet != OSA_OK  )
	{
		return FALSE;
	}

	//������Դ��ǩ
	dwRet = OsaMRsrcTagAlloc( OSA_TEST_MRSRC_TAG, strlen(OSA_TEST_MRSRC_TAG), &dwRsrcTag);
	if( dwRet != OSA_OK  )
	{
		return FALSE;
	}
	
	g_tTestMgr.m_bIsInit = TRUE;
	g_tTestMgr.m_tInitParam = tInitParam;
	g_tTestMgr.m_dwRsrcTag = dwRsrcTag;
	strncpy( g_tTestMgr.m_szCurWorkPath, szCurWorkPath, 260 );
	return TRUE;
}

BOOL TestModuleUnInit()
{
	UINT32 dwRet = OsaExit();
	if( dwRet != OSA_OK)
	{
		return FALSE;
	}
	return TRUE;
}

TOsaTestMgr* iTestMgrGet( )
{
	return &g_tTestMgr;
}

UINT32 iTestRsrcTagGet( )
{
	return g_tTestMgr.m_dwRsrcTag;
}

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//common test
//反复初始化
void FunTest_MultiInit( void )
{
	BOOL bRet = FALSE;
	char szCurPath[260] = { 0 };
	UINT32 i = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	strncpy( szCurPath, ptTestMgr->m_szCurWorkPath, 260 );

	for( i = 0; i < 20; i++ )
	{
		bRet = TestModuleUnInit();
		CU_ASSERT( TRUE == bRet );
		printf( "->FunTest_MultiInit: Uninit times:%d\n", i + 1 );

		//OsaSleep( 10 );
		bRet = TestModuleInit( szCurPath );
		CU_ASSERT( TRUE == bRet );
		printf( "->FunTest_MultiInit: init times:%d\n", i + 1 );
	}
}

//////////////////////////////////////////////////////////////////////////
//功能测试
//得到所有CPU的逻辑总核数
void FunTest_CpuCoreTotalNumGet( void )
{
	UINT32 dwCpuCoreNumWish = 0;
	UINT32 dwCpuCoreNumGet = 0;
	dwCpuCoreNumGet = OsaCpuCoreTotalNumGet();
	CU_ASSERT( dwCpuCoreNumGet > 0 );

#ifdef _MSC_VER
	{
		SYSTEM_INFO tSysInfo;
		GetSystemInfo(&tSysInfo);
		dwCpuCoreNumWish = tSysInfo.dwNumberOfProcessors;
		CU_ASSERT( dwCpuCoreNumWish == dwCpuCoreNumGet);
	}
#endif

}

//得到ip列表
void FunTest_IpListGet( void )
{
	UINT32 dwRet = 0;
	UINT32 adwIpList[16] = { 0 };
	UINT32 dwRealIpNum = 0;
	dwRet = OsaIpListGet( adwIpList, 16, &dwRealIpNum );
	CU_ASSERT( OSA_OK == dwRet );

	CU_ASSERT( dwRealIpNum > 0 );

	//检查IP地址的有效性
	//略
}

//////////////////////////////////////////////////////////////////////////
//异常测试


//得到ip列表
void ExcptTest_IpListGet( void )
{
	UINT32 dwRet = 0;
	UINT32 adwIpList[16] = { 0 };
	UINT32 dwRealIpNum = 0;
	//1.NULL Pointer
	dwRet = OsaIpListGet( NULL, 16, &dwRealIpNum );
	CU_ASSERT( OSA_OK != dwRet );

	dwRet = OsaIpListGet( adwIpList, 16, NULL );
	CU_ASSERT( OSA_OK != dwRet );


	//2.无效参数
	dwRet = OsaIpListGet( adwIpList, 0, &dwRealIpNum );
	CU_ASSERT( OSA_OK != dwRet );

}
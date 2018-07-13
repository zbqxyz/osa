#ifndef _H_TESTENV_
#define _H_TESTENV_

#define TEST_MRSRC_TAG "OsaTest"	//内存分配标签
#define TEST_ERROCC OsaStr2Occ("TestErr")     

#define OSA_TEST_MRSRC_TAG "OsaTest"	//内存分配标签

typedef struct tagOsaTestMgr
{
	BOOL m_bIsInit;		//是否初始化
	char m_szCurWorkPath[260];
	TOsaInitParam m_tInitParam; //初始化参数信息
 	HANDLE m_hMemAlloc; //Osa模块内部的内存分配器	
	UINT32 m_dwRsrcTag;
}TOsaTestMgr;

//初始化测试模块
BOOL TestModuleInit( const char* szCurWorkPath );
BOOL TestModuleUnInit();

//获取测试管理者
TOsaTestMgr* iTestMgrGet( );
//获取测试内存资源标签
UINT32 iTestRsrcTagGet( );

#endif
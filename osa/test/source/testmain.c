#include "stdafx.h"

/* 通用功能测试集合 */
CU_TestInfo FuncTestInfo_Common[] = {
	TCF( FunTest_MultiInit ),
	CU_TEST_INFO_NULL
};

/* Telnet打印功能测试集合 */
CU_TestInfo FuncTestInfo_Telnet[] = {
	TCF( FunTest_CmdReg ),
	TCF( FunTest_Str2Occ ),
	TCF( FunTest_PrtLab ),
	TCF( FunTest_FileLog ),
  CU_TEST_INFO_NULL
};

/* Telnet打印异常测试集合 */
CU_TestInfo ExcptTestInfo_Telnet[] = {
	TCF( ExcptTest_CmdReg ),
	TCF( ExcptTest_Str2Occ ),
	CU_TEST_INFO_NULL
};

/* Thread功能测试集合 */
CU_TestInfo FuncTestInfo_Thread[] = {
 	TCF( FunTest_ThreadCreate ),
 	TCF( FunTest_ThreadSelfExit ),
	TCF( FunTest_ThreadExitWait ),
 	TCF( FunTest_ThreadKill ),
 	TCF( FunTest_ThreadSuspendResume ),
  	TCF( FunTest_SetPriority ),
	CU_TEST_INFO_NULL
};

/* Thread异常测试集合 */
CU_TestInfo ExcptTestInfo_Thread[] = {
	TCF( ExcptTest_ThreadCreate ),
	TCF( ExcptTest_ThreadKill ),
	TCF( ExcptTest_ThreadSelfHandleGet ),
	TCF( ExcptTest_ThreadSuspendResume ),
	TCF( ExcptTest_SetPriority ),
	CU_TEST_INFO_NULL
};

/* Lock异常测试集合 */
CU_TestInfo ExcptTestInfo_Lock[] = {
	TCF( ExcptTest_LightLockCreate ),
	TCF( ExcptTest_LightLockDelete ),
	TCF( ExcptTest_LightLockLock ),
	TCF( ExcptTest_LightLockUnLock ),

	TCF( ExcptTest_SemBCreate ),
	TCF( ExcptTest_SemDelete ),
	TCF( ExcptTest_SemTake ),
	TCF( ExcptTest_SemTakeByTime ),
	TCF( ExcptTest_SemGive ),

	CU_TEST_INFO_NULL
};



/* MemLib功能测试集合 */
CU_TestInfo FuncTestInfo_MemLib[] = {
	TCF( FunTest_MAllocCreate ), //这个用例是后续内存功能用例的前提，必须运行
 	TCF( FunTest_MallocFree ),
	TCF( FunTest_PoolMallocFree ),
	TCF( FunTest_MAllocDestory ),
	CU_TEST_INFO_NULL
};

/* MemLib异常测试集合 */
CU_TestInfo ExcptTestInfo_MemLib[] = {
	TCF( FunTest_MAllocCreate ), //这个用例是后续内存功能用例的前提，必须运行
	TCF( ExcptTest_MRsrcTagAlloc ),
	TCF( ExcptTest_MAllocCreate ),
	TCF( ExcptTest_MAllocDestroy ),
	TCF( ExcptTest_Malloc ),
	TCF( ExcptTest_PoolMalloc ),
	TCF( ExcptTest_MFree ),
	TCF( ExcptTest_MSizeGet ),
	TCF( ExcptTest_MUserMemGet ),
	TCF( ExcptTest_MSysMemGet ),
	TCF( ExcptTest_MTypeGet ),
	TCF( FunTest_MAllocDestory ),
	CU_TEST_INFO_NULL
};


/* 目录及文件操作功能测试集合 */
CU_TestInfo FuncTestInfo_DirFile[] = {
	TCF( FunTest_DirCreateDel ),
 	TCF( FunTest_FileDelete ),
 	TCF( FunTest_FileLengthGet),
 	TCF( FunTest_DelFilesInDir ),
	CU_TEST_INFO_NULL
};

/* 目录及文件操作异常测试集合 */
CU_TestInfo ExcptTestInfo_DirFile[] = {
	TCF( ExcptTest_DirCreate ), 
	TCF( ExcptTest_DirDelete ),
	TCF( ExcptTest_IsDirExist ),
	TCF( ExcptTest_IsPathExist ),
	TCF( ExcptTest_FileLengthGet ),
	TCF( ExcptTest_FileDelete ),
	TCF( ExcptTest_DelFilesInDir ),
	CU_TEST_INFO_NULL
};


/* 系统信息功能测试集合 */
CU_TestInfo FuncTestInfo_SysInfo[] = {
	TCF( FunTest_CpuCoreTotalNumGet ),
	TCF( FunTest_IpListGet ),
	CU_TEST_INFO_NULL
};

/* 系统信息异常测试集合 */
CU_TestInfo ExcptTestInfo_SysInfo[] = {
	TCF( ExcptTest_IpListGet ), 
	CU_TEST_INFO_NULL
};

/* telnet日志测试集合 */
CU_TestInfo PersTestInfo_TelnetInfo[] = {
	TCF( PersTest_TelnetAndLog ), 
	CU_TEST_INFO_NULL
};

/* 线程句柄释放测试集合 */
CU_TestInfo PersTestInfo_Thread[] = {
	TCF( PersTest_ThreadCreateClose ), 
	CU_TEST_INFO_NULL
};

/* 套件集合 */
CU_SuiteInfo tMySuites[] = 
{
	{"FuncTestInfo_Common", NULL, NULL, FuncTestInfo_Common},
	{"FuncTestInfo_Telnet", NULL, NULL, FuncTestInfo_Telnet},
	{"FuncTestInfo_Thread", NULL, NULL, FuncTestInfo_Thread},
	{"FuncTestInfo_MemLib", NULL, NULL, FuncTestInfo_MemLib},
	{"FuncTestInfo_SysInfo", NULL, NULL, FuncTestInfo_SysInfo},
	{"FuncTestInfo_DirFile", NULL, NULL, FuncTestInfo_DirFile},

	{"ExcptTestInfo_Telnet", NULL, NULL, ExcptTestInfo_Telnet},
	{"ExcptTestInfo_Thread", NULL, NULL, ExcptTestInfo_Thread},
	{"ExcptTestInfo_Lock", NULL, NULL, ExcptTestInfo_Lock},
	{"ExcptTestInfo_MemLib", NULL, NULL, ExcptTestInfo_MemLib},
	{"ExcptTestInfo_SysInfo", NULL, NULL, ExcptTestInfo_SysInfo},
	{"ExcptTestInfo_DirFile", NULL, NULL, ExcptTestInfo_DirFile},
	{"PersTestInfo_Thread", NULL, NULL, PersTestInfo_Thread},
//	{"PersTestInfo_TelnetInfo", NULL, NULL, PersTestInfo_TelnetInfo},
	CU_SUITE_INFO_NULL
};

int main(int argc,char *argv[])
{
	char *pt = NULL;
	char achCurrPath[256+1] = {0};
    BOOL bRet = FALSE;

	printf("argv: %s\n", argv[0]);
	//得到RM的绝对路径
	strncpy(achCurrPath, argv[0], 256);

#ifdef _MSC_VER
	pt = strrchr(achCurrPath, '\\');
#else
	pt = strrchr(achCurrPath, '/');
#endif
	if(NULL != pt)
	{
		*pt = 0;

		//设置工作路径
		chdir(achCurrPath);
	}

    //初始化测试各模块
    bRet = TestModuleInit( achCurrPath );
	if( TRUE != bRet )
	{
		printf("TestModuleInit err!\n");
		return -1;
	}

	/* 自动测试输出文档 */
	CU_set_output_filename("Osa-Test");

    //注册测试套件
    bRet = RegisterMySuites();
    if( TRUE != bRet )
    {
        printf("RegisterMySuites err!\n");
        return -1;
    }
	
	/* 运行测试 */
	//CU_basic_run_tests();
	/* 自动测试 */
	CU_automated_run_tests();
	CU_cleanup_registry();

	getchar();

	/* 运行测试等待 */
	bRet = TestModuleUnInit();
	if( TRUE != bRet )
	{
		printf("TestModuleUnInit err!\n");
		return -1;
	}

//     exit(0);

	return 0;
}


//注册测试套件
BOOL RegisterMySuites( )
{
    /* 初始化 */
    if(CUE_SUCCESS != CU_initialize_registry())
    {
        printf("cunit err, %s\n", CU_get_error_msg());
        return FALSE;
    }

    /*注册测试套件*/
    if(CUE_SUCCESS != CU_register_suites(tMySuites))
    {
        printf("cunit err, %s\n", CU_get_error_msg());
        return FALSE;
    }
    return TRUE;
}

#ifndef _H_TESTCASE_SYSINFO_
#define _H_TESTCASE_SYSINFO_
//////////////////////////////////////////////////////////////////////////
//common test
//反复初始化
void FunTest_MultiInit( void );

//////////////////////////////////////////////////////////////////////////
//功能测试
//得到所有CPU的逻辑总核数
void FunTest_CpuCoreTotalNumGet( void );
//得到ip列表
void FunTest_IpListGet( void );
//////////////////////////////////////////////////////////////////////////
//异常测试

//得到ip列表
void ExcptTest_IpListGet( void );

#endif
#ifndef _H_TESTCASE_TELNET_
#define _H_TESTCASE_TELNET_

//////////////////////////////////////////////////////////////////////////
//功能测试
//注册调试命令
void FunTest_CmdReg( void );

//分类标签名称与OCC值的转换
void FunTest_Str2Occ( void );

//打印标签操作
void FunTest_PrtLab( void );

//文件日志
void FunTest_FileLog( void );


//////////////////////////////////////////////////////////////////////////
//异常测试
//注册调试命令
void ExcptTest_CmdReg( void );
//分类标签名称与OCC值的转换
void ExcptTest_Str2Occ( void );

//////////////////////////////////////////////////////////////////////////
//压力测试
void PersTest_TelnetAndLog(void );

void PersTest_ThreadCreateClose(void );
#endif
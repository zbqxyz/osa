#ifndef _H_TESTCASE_THREAD_
#define _H_TESTCASE_THREAD_

//////////////////////////////////////////////////////////////////////////
//功能测试
//创建并激活一个线程
void FunTest_ThreadCreate( void );
//线程函数内部自己退出
void FunTest_ThreadSelfExit( void );
//结束线程完成退出
void FunTest_ThreadExitWait( void );
//结束线程完成退出
void FunTest_ThreadKill( void );
//线程挂起继续
void FunTest_ThreadSuspendResume( void );
//改变线程的优先级
void FunTest_SetPriority( void );

//////////////////////////////////////////////////////////////////////////
//异常测试
//创建并激活一个线程
void ExcptTest_ThreadCreate( void );
//结束线程完成退出
void ExcptTest_ThreadKill( void );
//获取当前运行线程的句柄
void ExcptTest_ThreadSelfHandleGet( void );
//线程挂起继续
void ExcptTest_ThreadSuspendResume( void );
//改变线程的优先级
void ExcptTest_SetPriority( void );

#endif
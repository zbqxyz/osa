#ifndef _H_TESTCASE_LOCK_
#define _H_TESTCASE_LOCK_

//////////////////////////////////////////////////////////////////////////
//功能测试
//Lock的功能在Osa内部已经在使用，其他功能正常已经确保Lock功能Ok了
//Sem的功能在线程的测试中已经涵盖

//////////////////////////////////////////////////////////////////////////
//异常测试

//创建轻量级锁
void ExcptTest_LightLockCreate( void );
//删除轻量级锁
void ExcptTest_LightLockDelete( void );
//锁轻量级锁
void ExcptTest_LightLockLock( void );
//解锁轻量级锁
void ExcptTest_LightLockUnLock( void );

//创建一个二元信号量
void ExcptTest_SemBCreate( void );
//删除信号量
void ExcptTest_SemDelete( void );
//信号量Take
void ExcptTest_SemTake( void );
//带超时的信号量Take
void ExcptTest_SemTakeByTime( void );
//信号量Give
void ExcptTest_SemGive( void );

#endif
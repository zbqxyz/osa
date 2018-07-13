#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//功能测试
//无

//////////////////////////////////////////////////////////////////////////
//异常测试

//创建轻量级锁
void ExcptTest_LightLockCreate( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockCreate( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//删除轻量级锁
void ExcptTest_LightLockDelete( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockDelete( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

//锁轻量级锁
void ExcptTest_LightLockLock( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockLock( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//解锁轻量级锁
void ExcptTest_LightLockUnLock( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockUnLock( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

//创建一个二元信号量
void ExcptTest_SemBCreate( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemBCreate( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//删除信号量
void ExcptTest_SemDelete( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemDelete( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//信号量Take
void ExcptTest_SemTake( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemTake( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//带超时的信号量Take
void ExcptTest_SemTakeByTime( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemTakeByTime( NULL, 2000 );
	CU_ASSERT( OSA_OK != dwRet );
}
//信号量Give
void ExcptTest_SemGive( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemGive( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//���ܲ���
//��

//////////////////////////////////////////////////////////////////////////
//�쳣����

//������������
void ExcptTest_LightLockCreate( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockCreate( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//ɾ����������
void ExcptTest_LightLockDelete( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockDelete( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

//����������
void ExcptTest_LightLockLock( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockLock( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//������������
void ExcptTest_LightLockUnLock( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaLightLockUnLock( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

//����һ����Ԫ�ź���
void ExcptTest_SemBCreate( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemBCreate( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//ɾ���ź���
void ExcptTest_SemDelete( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemDelete( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//�ź���Take
void ExcptTest_SemTake( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemTake( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//����ʱ���ź���Take
void ExcptTest_SemTakeByTime( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemTakeByTime( NULL, 2000 );
	CU_ASSERT( OSA_OK != dwRet );
}
//�ź���Give
void ExcptTest_SemGive( void )
{
	UINT32 dwRet = OSA_OK;
	dwRet = OsaSemGive( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//���ܲ���
//������Դ��ǩ
void FunTest_MRsrcTagAlloc( void )
{
	//��ʼ���Ѿ������ù��ܣ���
}

//�����ڴ������
void FunTest_MAllocCreate( void )
{
	//��ʼ���Ѿ������ù��ܣ���
	UINT32 dwRet = OSA_OK;
	HANDLE hMemAlloc = NULL;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//�����ڴ������
	dwRet = OsaMAllocCreate( 0, 0, &hMemAlloc );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( hMemAlloc != NULL );
	if( dwRet != OSA_OK || NULL == hMemAlloc )
	{
		return;
	}

	ptTestMgr->m_hMemAlloc = hMemAlloc;
}

//�����ڴ���ͷ�
void FunTest_MallocFree( void )
{
	UINT32 dwRet = OSA_OK;
	UINT64 qwOldUserMSize = 0;
	UINT64 qwNewUserMSize = 0;
	UINT64 qwSysMSize = 0;
	void* pMalloc = NULL;
	UINT32 dwMallocSize = 1024;
	UINT32 dwMallocSizeGet = 0;
	UINT32 dwMType = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	if( NULL == hMemAlloc )
	{
		CU_ASSERT( 0 );
		return;
	}

	//1.��ȡ�û�������ڴ��С
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwOldUserMSize );
	CU_ASSERT( OSA_OK == dwRet );

	//2.�����ڴ�
	pMalloc = OsaMalloc( hMemAlloc, dwMallocSize, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( pMalloc != 0 );

	//3.�ٻ�ȡ�û�������ڴ��С
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	//��֤�ڴ������
	CU_ASSERT( (UINT32)(qwNewUserMSize - qwOldUserMSize) == dwMallocSize );

	//4.�õ��ڴ�����������ϵͳ������ڴ�����
	dwRet = OsaMSysMemGet( hMemAlloc, &qwSysMSize );
	CU_ASSERT( OSA_OK == dwRet );
	//ϵͳ�ڴ���󣬰��������û��ڴ�Ķ����ڴ�
	CU_ASSERT( qwSysMSize > qwNewUserMSize );
	
	//5.ͨ���ڴ�ָ�룬��ȡ�����ڴ�Ĵ�С
	dwRet = OsaMSizeGet( pMalloc, &dwMallocSizeGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( dwMallocSizeGet == dwMallocSize );

	//6.��ȡ�ڴ������
	dwRet = OsaMTypeGet( hMemAlloc, pMalloc, &dwMType );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( BLOCKTYPE_DIRECT == dwMType );

	//7.�ͷ��ڴ�
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );

	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( qwOldUserMSize == qwNewUserMSize );

}
//�ڴ���з����ڴ���ͷ�
void FunTest_PoolMallocFree( void )
{
	UINT32 dwRet = OSA_OK;
	UINT64 qwOldUserMSize = 0;
	UINT64 qwNewUserMSize = 0;
	void* pMalloc = NULL;
	UINT32 dwMallocSize = 2048;
	UINT32 dwMallocSizeGet = 0;
	UINT32 dwMType = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	if( NULL == hMemAlloc )
	{
		CU_ASSERT( 0 );
		return;
	}

	//1.��ȡ�û�������ڴ��С
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwOldUserMSize );
	CU_ASSERT( OSA_OK == dwRet );

	//2.�����ڴ�
	pMalloc = OsaPoolMalloc( hMemAlloc, dwMallocSize, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( pMalloc != 0 );

	//3.�ٻ�ȡ�û�������ڴ��С
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( (UINT32)(qwNewUserMSize - qwOldUserMSize) == dwMallocSize );

	//4.��ȡ�ڴ������
	dwRet = OsaMTypeGet( hMemAlloc, pMalloc, &dwMType );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( BLOCKTYPE_EXPONENT == dwMType );

	//5.ͨ���ڴ�ָ�룬��ȡ�����ڴ�Ĵ�С
	dwRet = OsaMSizeGet( pMalloc, &dwMallocSizeGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( dwMallocSizeGet == dwMallocSize );

	//6.�ͷ��ڴ�
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );

	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( qwOldUserMSize == qwNewUserMSize );

}

//�����ڴ������
void FunTest_MAllocDestory( void )
{
	//��ʼ���Ѿ������ù��ܣ���
	UINT32 dwRet = OSA_OK;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//�ͷ��ڴ������
	dwRet = OsaMAllocDestroy( ptTestMgr->m_hMemAlloc );
	CU_ASSERT( OSA_OK == dwRet );

	ptTestMgr->m_hMemAlloc = NULL;
}
//////////////////////////////////////////////////////////////////////////
//�쳣����


//������Դ��ǩ
void ExcptTest_MRsrcTagAlloc( void )
{
	UINT32 dwRet = OSA_OK;
	UINT32 dwRsrcTag = 0;
    const char* szNormalTag =  "tagOk";	//�ڴ�����ǩ
    const char* szLongTag =  "OsaTest1234567890";	//�ڴ�����ǩ

	//1.NULL Pointer 
	dwRet = OsaMRsrcTagAlloc( NULL, strlen(OSA_TEST_MRSRC_TAG), &dwRsrcTag );
	CU_ASSERT( OSA_OK != dwRet );

	//2.too long tag string len 
	dwRet = OsaMRsrcTagAlloc( szLongTag, strlen(szLongTag), &dwRsrcTag );
	CU_ASSERT( OSA_OK != dwRet );

	//3.NULL Pointer
	dwRet = OsaMRsrcTagAlloc( szNormalTag, strlen(szNormalTag), NULL );
	CU_ASSERT( OSA_OK != dwRet );
}

//�����ڴ������
void ExcptTest_MAllocCreate( void )
{
	UINT32 dwRet = OSA_OK;
	//1.NULL Pointer 
	dwRet = OsaMAllocCreate( 0, 0, NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//�ͷ��ڴ������
void ExcptTest_MAllocDestroy( void )
{
	UINT32 dwRet = OSA_OK;
	//1.NULL Pointer 
	dwRet = OsaMAllocDestroy( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//�����ڴ�
void ExcptTest_Malloc( void )
{
	UINT32 dwRet = OSA_OK;
	void* pMalloc = NULL;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//1.NULL Pointer 
	pMalloc = OsaMalloc( NULL, 123, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( NULL == pMalloc );

	//2.Malloc 0 size
	pMalloc = OsaMalloc( hMemAlloc, 0, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( NULL == pMalloc );
}
//�ڴ���з����ڴ�
void ExcptTest_PoolMalloc( void )
{
	UINT32 dwRet = OSA_OK;
	void* pMalloc = NULL;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//1.NULL Pointer 
	pMalloc = OsaPoolMalloc( NULL, 123, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( NULL == pMalloc );

	//2.Malloc 0 size
	pMalloc = OsaPoolMalloc( hMemAlloc, 0, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( NULL == pMalloc );
}

//�ͷ��ڴ�
void ExcptTest_MFree( void )
{
	UINT32 dwRet = OSA_OK;
	void* pMalloc = NULL;
	UINT32 dwMallocSize = 123;

	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//���������ڴ�
	pMalloc = OsaMalloc( hMemAlloc, dwMallocSize, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( pMalloc != NULL );

	//1.NULL Pointer 
	dwRet = OsaMFree( NULL, pMalloc );
	CU_ASSERT( OSA_OK != dwRet );

	//2.NULL Mem Pointer 
	dwRet = OsaMFree( hMemAlloc, NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//�����ͷ��ڴ�
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );

}
//�õ�ָ��ָ���ڴ����Ĵ�С
void ExcptTest_MSizeGet( void )
{
	UINT32 dwRet = OSA_OK;
	UINT32 dwSizeGet = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//1.NULL Pointer 
	dwRet = OsaMSizeGet( NULL, &dwSizeGet );
	CU_ASSERT( OSA_OK != dwRet );

	//2.NULL Mem Pointer 
	dwRet = OsaMSizeGet( hMemAlloc, NULL );
	CU_ASSERT( OSA_OK != dwRet );

}

//�õ�ָ����ǩ���û��ڴ�����
void ExcptTest_MUserMemGet( void )
{
	UINT32 dwRet = OSA_OK;
	UINT64 qwMUserSizeGet = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//1.NULL Pointer 
	dwRet = OsaMUserMemGet( NULL, ptTestMgr->m_dwRsrcTag, &qwMUserSizeGet );
	CU_ASSERT( OSA_OK != dwRet );

	//2.NULL Mem Pointer 
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, NULL );
	CU_ASSERT( OSA_OK != dwRet );

}
//�õ��ڴ�����������ϵͳ������ڴ�����
void ExcptTest_MSysMemGet( void )
{
	UINT32 dwRet = OSA_OK;
	UINT64 qwMSysSizeGet = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//1.NULL Pointer 
	dwRet = OsaMSysMemGet( NULL, &qwMSysSizeGet );
	CU_ASSERT( OSA_OK != dwRet );

	//2.NULL Mem Pointer 
	dwRet = OsaMSysMemGet( hMemAlloc, NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//�õ��ڴ������
void ExcptTest_MTypeGet( void )
{
	UINT32 dwRet = OSA_OK;
	void* pMalloc = NULL;
	UINT32 dwMallocSize = 256;
	UINT32 dwType = 0;

	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//���������ڴ�
	pMalloc = OsaMalloc( hMemAlloc, dwMallocSize, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( pMalloc != 0 );

	//1.NULL Pointer 
	dwRet = OsaMTypeGet( NULL, pMalloc, &dwType );
	CU_ASSERT( OSA_OK != dwRet );

	//2.NULL Mem Pointer 
	dwRet = OsaMTypeGet( hMemAlloc, NULL, &dwType );
	CU_ASSERT( OSA_OK != dwRet );

	//3.NULL Mem Pointer 
	dwRet = OsaMTypeGet( hMemAlloc, pMalloc, NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//�����ͷ��ڴ�
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );
}
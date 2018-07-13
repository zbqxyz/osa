#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//功能测试
//分配资源标签
void FunTest_MRsrcTagAlloc( void )
{
	//初始化已经包括该功能，略
}

//生成内存分配器
void FunTest_MAllocCreate( void )
{
	//初始化已经包括该功能，略
	UINT32 dwRet = OSA_OK;
	HANDLE hMemAlloc = NULL;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//创建内存分配器
	dwRet = OsaMAllocCreate( 0, 0, &hMemAlloc );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( hMemAlloc != NULL );
	if( dwRet != OSA_OK || NULL == hMemAlloc )
	{
		return;
	}

	ptTestMgr->m_hMemAlloc = hMemAlloc;
}

//分配内存和释放
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

	//1.获取用户分配的内存大小
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwOldUserMSize );
	CU_ASSERT( OSA_OK == dwRet );

	//2.分配内存
	pMalloc = OsaMalloc( hMemAlloc, dwMallocSize, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( pMalloc != 0 );

	//3.再获取用户分配的内存大小
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	//验证内存的增长
	CU_ASSERT( (UINT32)(qwNewUserMSize - qwOldUserMSize) == dwMallocSize );

	//4.得到内存分配器向操作系统申请的内存总量
	dwRet = OsaMSysMemGet( hMemAlloc, &qwSysMSize );
	CU_ASSERT( OSA_OK == dwRet );
	//系统内存更大，包含管理用户内存的额外内存
	CU_ASSERT( qwSysMSize > qwNewUserMSize );
	
	//5.通过内存指针，获取分配内存的大小
	dwRet = OsaMSizeGet( pMalloc, &dwMallocSizeGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( dwMallocSizeGet == dwMallocSize );

	//6.获取内存块类型
	dwRet = OsaMTypeGet( hMemAlloc, pMalloc, &dwMType );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( BLOCKTYPE_DIRECT == dwMType );

	//7.释放内存
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );

	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( qwOldUserMSize == qwNewUserMSize );

}
//内存池中分配内存和释放
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

	//1.获取用户分配的内存大小
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwOldUserMSize );
	CU_ASSERT( OSA_OK == dwRet );

	//2.分配内存
	pMalloc = OsaPoolMalloc( hMemAlloc, dwMallocSize, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( pMalloc != 0 );

	//3.再获取用户分配的内存大小
	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( (UINT32)(qwNewUserMSize - qwOldUserMSize) == dwMallocSize );

	//4.获取内存块类型
	dwRet = OsaMTypeGet( hMemAlloc, pMalloc, &dwMType );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( BLOCKTYPE_EXPONENT == dwMType );

	//5.通过内存指针，获取分配内存的大小
	dwRet = OsaMSizeGet( pMalloc, &dwMallocSizeGet );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( dwMallocSizeGet == dwMallocSize );

	//6.释放内存
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );

	dwRet = OsaMUserMemGet( hMemAlloc, ptTestMgr->m_dwRsrcTag, &qwNewUserMSize );
	CU_ASSERT( OSA_OK == dwRet );
	CU_ASSERT( qwOldUserMSize == qwNewUserMSize );

}

//销毁内存分配器
void FunTest_MAllocDestory( void )
{
	//初始化已经包括该功能，略
	UINT32 dwRet = OSA_OK;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//释放内存分配器
	dwRet = OsaMAllocDestroy( ptTestMgr->m_hMemAlloc );
	CU_ASSERT( OSA_OK == dwRet );

	ptTestMgr->m_hMemAlloc = NULL;
}
//////////////////////////////////////////////////////////////////////////
//异常测试


//分配资源标签
void ExcptTest_MRsrcTagAlloc( void )
{
	UINT32 dwRet = OSA_OK;
	UINT32 dwRsrcTag = 0;
    const char* szNormalTag =  "tagOk";	//内存分配标签
    const char* szLongTag =  "OsaTest1234567890";	//内存分配标签

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

//生成内存分配器
void ExcptTest_MAllocCreate( void )
{
	UINT32 dwRet = OSA_OK;
	//1.NULL Pointer 
	dwRet = OsaMAllocCreate( 0, 0, NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//释放内存分配器
void ExcptTest_MAllocDestroy( void )
{
	UINT32 dwRet = OSA_OK;
	//1.NULL Pointer 
	dwRet = OsaMAllocDestroy( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//分配内存
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
//内存池中分配内存
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

//释放内存
void ExcptTest_MFree( void )
{
	UINT32 dwRet = OSA_OK;
	void* pMalloc = NULL;
	UINT32 dwMallocSize = 123;

	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//正常分配内存
	pMalloc = OsaMalloc( hMemAlloc, dwMallocSize, ptTestMgr->m_dwRsrcTag );
	CU_ASSERT( pMalloc != NULL );

	//1.NULL Pointer 
	dwRet = OsaMFree( NULL, pMalloc );
	CU_ASSERT( OSA_OK != dwRet );

	//2.NULL Mem Pointer 
	dwRet = OsaMFree( hMemAlloc, NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//正常释放内存
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );

}
//得到指针指向内存区的大小
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

//得到指定标签的用户内存总量
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
//得到内存分配器向操作系统申请的内存总量
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
//得到内存块类型
void ExcptTest_MTypeGet( void )
{
	UINT32 dwRet = OSA_OK;
	void* pMalloc = NULL;
	UINT32 dwMallocSize = 256;
	UINT32 dwType = 0;

	TOsaTestMgr* ptTestMgr = iTestMgrGet( );
	HANDLE hMemAlloc = ptTestMgr->m_hMemAlloc;

	//正常分配内存
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

	//正常释放内存
	dwRet = OsaMFree( hMemAlloc, pMalloc );
	CU_ASSERT( OSA_OK == dwRet );
}
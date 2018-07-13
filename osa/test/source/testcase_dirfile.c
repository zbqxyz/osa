#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//功能测试
//创建和删除目录
void FunTest_DirCreateDel( void )
{
	UINT32 dwRet = 0;
	BOOL bIsExist = FALSE;
	char szTestDirFullPathName[256] = {0};
	char* szTestDirName = "TestDirName";
	char* szTestDirName2 = "TestDirName2";
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//1.绝度路径目录的创建和删除
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( bIsExist )
	{
		dwRet = OsaDirDelete( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	dwRet = OsaDirCreate( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	CU_ASSERT( TRUE == bIsExist );

	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	CU_ASSERT( FALSE == bIsExist );

	//2.相对路径目录的创建和删除
	memset( szTestDirFullPathName, 0, sizeof( szTestDirFullPathName) );
#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, ".\\%s", szTestDirName );
#else
	sprintf( szTestDirFullPathName, "./%s", szTestDirName );
#endif

	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( bIsExist )
	{
		dwRet = OsaDirDelete( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	dwRet = OsaDirCreate( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	CU_ASSERT( TRUE == bIsExist );

	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	CU_ASSERT( FALSE == bIsExist );
}


static BOOL sCreateTestFile( const char* szFileFullPathName )
{
	FILE* pTmpfile = NULL;
	if( !szFileFullPathName )
	{
		return FALSE;
	}

	//打开文件
	pTmpfile = fopen(szFileFullPathName, "ab+");
	if ( NULL == pTmpfile)//文件打开失败
	{
		return FALSE;
	}

	fclose( pTmpfile );
	return TRUE;
}

//创建指定大小的文件
BOOL sCreateTestFileBySize(const char* szFileFullPathName, UINT64 qwSize)
{
	char c = 0x00;
	FILE* pTmpfile = NULL;
	if( !szFileFullPathName )
	{
		return FALSE;
	}

	if( qwSize <= 0 )
	{
		return FALSE;
	}

	//打开文件
	pTmpfile = fopen(szFileFullPathName, "w+b");
	if ( NULL == pTmpfile)//文件打开失败
	{
		return FALSE;
	}

// 	_fseeki64(pTmpfile, 0, SEEK_SET);
// 	fwrite(&c, 1, 1, pTmpfile);
#ifdef _MSC_VER
	_fseeki64(pTmpfile, qwSize - 1, SEEK_SET);
#else
	fseeko(pTmpfile, qwSize - 1, SEEK_SET);
#endif
	fwrite(&c, 1, 1, pTmpfile);

	fclose( pTmpfile );
	return TRUE;
}

//删除文件
void FunTest_FileDelete( void )
{
	UINT32 dwRet = 0;
	BOOL bRet = FALSE;
	BOOL bIsExist = FALSE;
	char szTestDirFullPathName[256] = {0};
	char szTestFileFullPathName[512] = {0};
	char* szTestDirName = "TestDirName";
	char* szTestFileName = "TestFileName";
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//1.如果测试目录不存在，先创建测试目录
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( FALSE == bIsExist )
	{
		dwRet = OsaDirCreate( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );

		bIsExist = OsaIsDirExist( szTestDirFullPathName );
		CU_ASSERT( TRUE == bIsExist );
	}

	//2.绝度路径文件的创建和删除
#ifdef _MSC_VER
	sprintf( szTestFileFullPathName, "%s\\%s", szTestDirFullPathName, szTestFileName );
#else
	sprintf( szTestFileFullPathName, "%s/%s", szTestDirFullPathName, szTestFileName );
#endif

	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	if( bIsExist )
	{
		dwRet = OsaFileDelete( szTestFileFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	bRet = sCreateTestFile( szTestFileFullPathName );
	CU_ASSERT( TRUE == bRet );

	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( TRUE == bIsExist );

	//3.删除文件
	dwRet = OsaFileDelete( szTestFileFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	OsaSleep(10 ); //经常测试不过，增加等待验证
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( FALSE == bIsExist );

	//4.删除测试目录
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	bIsExist = OsaIsPathExist( szTestDirFullPathName );
	CU_ASSERT( FALSE == bIsExist );
}

//获取文件长度
void FunTest_FileLengthGet( void )
{
	UINT32 dwRet = 0;
	BOOL bRet = FALSE;
	BOOL bIsExist = FALSE;
	char szTestDirFullPathName[256] = {0};
	char szTestFileFullPathName[512] = {0};
	char* szTestDirName = "TestDirName";
	char* szTestFileName = "TestFileName";
	UINT64 qwTestFileLen1 = 4 << 10; //4K
	UINT64 qwTestFileLen2 = 4 << 20; //4M
	UINT64 qwTestFileLenGet = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//1.如果测试目录不存在，先创建测试目录
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( FALSE == bIsExist )
	{
		dwRet = OsaDirCreate( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

#ifdef _MSC_VER
	sprintf( szTestFileFullPathName, "%s\\%s", szTestDirFullPathName, szTestFileName );
#else
	sprintf( szTestFileFullPathName, "%s/%s", szTestDirFullPathName, szTestFileName );
#endif

	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	if( bIsExist )
	{
		dwRet = OsaFileDelete( szTestFileFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	//2.创建指定大小的文件
	bRet = sCreateTestFileBySize( szTestFileFullPathName, qwTestFileLen1 );
	CU_ASSERT( TRUE == bRet );
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( TRUE == bIsExist );

	//3.测试获取文件大小
	qwTestFileLenGet = OsaFileLengthGet( szTestFileFullPathName );
	CU_ASSERT( qwTestFileLenGet == qwTestFileLen1 );

	dwRet = OsaFileDelete( szTestFileFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( FALSE == bIsExist );


	//4.创建指定大小的文件
	bRet = sCreateTestFileBySize( szTestFileFullPathName, qwTestFileLen2 );
	CU_ASSERT( TRUE == bRet );

	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( TRUE == bIsExist );

	//5.测试获取文件大小
	qwTestFileLenGet = OsaFileLengthGet( szTestFileFullPathName );
	CU_ASSERT( qwTestFileLenGet == qwTestFileLen2 );

	dwRet = OsaFileDelete( szTestFileFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( FALSE == bIsExist );

	//6.删除测试目录
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
}

//删除目录下的所有文件
void FunTest_DelFilesInDir( void )
{
	UINT32 dwRet = 0;
	UINT32 i = 0;
	BOOL bRet = FALSE;
	BOOL bIsExist = FALSE;
	char szTestDirFullPathName[256] = {0};
	char szTestFileFullPathName[512] = {0};
	char* szTestDirName = "TestDirName";
	char* szTestFileName = "TestFileName";
	UINT32 dwTestFileLenGet = 0;
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//1.如果测试目录不存在，先创建测试目录
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( FALSE == bIsExist )
	{
		dwRet = OsaDirCreate( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	//2.在指定目录下创建多个文件
	for( i = 0; i < 10; i++ )
	{
#ifdef _MSC_VER
		sprintf( szTestFileFullPathName, "%s\\%s%d", szTestDirFullPathName, szTestFileName, i+1 );
#else
		sprintf( szTestFileFullPathName, "%s/%s%d", szTestDirFullPathName, szTestFileName, i+1 );
#endif

		bIsExist = OsaIsPathExist( szTestFileFullPathName );
		if( bIsExist )
		{
			dwRet = OsaFileDelete( szTestFileFullPathName );
			CU_ASSERT( OSA_OK == dwRet );
		}

		bRet = sCreateTestFile( szTestFileFullPathName );
		CU_ASSERT( TRUE == bRet );
		bIsExist = OsaIsPathExist( szTestFileFullPathName );
		CU_ASSERT( TRUE == bIsExist );
	}

	//3.删除指定目录下的所有文件
	dwRet = OsaDelFilesInDir( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	//4.验证所有文件已经删除
	for( i = 0; i < 10; i++ )
	{
#ifdef _MSC_VER
		sprintf( szTestFileFullPathName, "%s\\%s%d", szTestDirFullPathName, szTestFileName, i+1 );
#else
		sprintf( szTestFileFullPathName, "%s/%s%d", szTestDirFullPathName, szTestFileName, i+1 );
#endif

		bIsExist = OsaIsPathExist( szTestFileFullPathName );
		CU_ASSERT( FALSE == bIsExist );
	}

	//5.删除测试目录
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
}

//////////////////////////////////////////////////////////////////////////
//异常测试


//创建目录
void ExcptTest_DirCreate( void )
{
	UINT32 dwRet = 0;
	//1.NULL Pointer
	dwRet = OsaDirCreate( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//删除目录
void ExcptTest_DirDelete( void )
{
	UINT32 dwRet = 0;
	BOOL bIsExist = FALSE;
	char szTestDirFullPathName[256] = {0};
	char* szTestDirName = "TestEmptyName";
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//1.NULL Pointer
	dwRet = OsaDirDelete( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//2.不存在的目录
#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//1.绝度路径目录的创建和删除
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( bIsExist )
	{
		dwRet = OsaDirDelete( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK != dwRet );
}

//目录是否存在
void ExcptTest_IsDirExist( void )
{
	BOOL bRet = FALSE;
	//1.NULL Pointer
	bRet = OsaIsDirExist( NULL );
	CU_ASSERT( FALSE == bRet );

	//3.空内存的路径
	{
		char szEmptyPath[256];
		memset( szEmptyPath, 0, 256 );
		bRet = OsaIsDirExist(szEmptyPath);
		CU_ASSERT( FALSE == bRet );
	}
}
//路径是否存在
void ExcptTest_IsPathExist( void )
{
	BOOL bRet = FALSE;
	char szTestDirFullPathName[256] = {0};
	char* szTestDirName = "NoExistEmptyDir";
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//1.NULL Pointer
	bRet = OsaIsPathExist( NULL );
	CU_ASSERT( FALSE == bRet );

#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//2.不存在的路径
	bRet = OsaIsPathExist( szTestDirFullPathName );
	CU_ASSERT( FALSE == bRet );

	//3.空内存的路径
	{
		char szEmptyPath[256];
		memset( szEmptyPath, 0, 256 );
		bRet = OsaIsPathExist(szEmptyPath);
		CU_ASSERT( FALSE == bRet );
	}
}

//获取文件长度函数
void ExcptTest_FileLengthGet( void )
{
	UINT64 qwLen = 0;
	//1.NULL Pointer
	qwLen = OsaFileLengthGet( NULL );
	CU_ASSERT( 0 == qwLen );
}
//删除文件
void ExcptTest_FileDelete( void )
{
	UINT32 dwRet = 0;
	BOOL bIsExist = FALSE;
	char szTestDirFullPathName[256] = {0};
	char szTestFileFullPathName[512] = {0};
	char* szTestDirName = "TestDirName";
	char* szTestFileName = "TestUnknowFileName";
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//1.NULL Pointer
	dwRet = OsaFileDelete( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//2.不存在的文件
#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//如果测试目录不存在，先创建测试目录
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( FALSE == bIsExist )
	{
		dwRet = OsaDirCreate( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
		bIsExist = OsaIsDirExist( szTestDirFullPathName );
		CU_ASSERT( TRUE == bIsExist );
	}

#ifdef _MSC_VER
	sprintf( szTestFileFullPathName, "%s\\%s", szTestDirFullPathName, szTestFileName );
#else
	sprintf( szTestFileFullPathName, "%s/%s", szTestDirFullPathName, szTestFileName );
#endif

	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	if( bIsExist )
	{
		dwRet = OsaFileDelete( szTestFileFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	dwRet = OsaFileDelete( szTestFileFullPathName );
	CU_ASSERT( OSA_OK != dwRet );

	//3.删除测试目录
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

}
//删除目录下的所有文件
void ExcptTest_DelFilesInDir( void )
{
	UINT32 dwRet = 0;
	BOOL bIsExist = FALSE;
	char szTestDirFullPathName[256] = {0};
	char* szTestDirName = "TestEmptyName";
	TOsaTestMgr* ptTestMgr = iTestMgrGet( );

	//1.NULL Pointer
	dwRet = OsaDelFilesInDir( NULL );
	CU_ASSERT( OSA_OK != dwRet );

	//2.不存在的目录
#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//如果测试目录不存在，先创建测试目录
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( bIsExist )
	{
		dwRet = OsaDirDelete( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	dwRet = OsaDelFilesInDir( szTestDirFullPathName );
	CU_ASSERT( OSA_OK != dwRet );
}
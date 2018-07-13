#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//���ܲ���
//������ɾ��Ŀ¼
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

	//1.����·��Ŀ¼�Ĵ�����ɾ��
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

	//2.���·��Ŀ¼�Ĵ�����ɾ��
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

	//���ļ�
	pTmpfile = fopen(szFileFullPathName, "ab+");
	if ( NULL == pTmpfile)//�ļ���ʧ��
	{
		return FALSE;
	}

	fclose( pTmpfile );
	return TRUE;
}

//����ָ����С���ļ�
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

	//���ļ�
	pTmpfile = fopen(szFileFullPathName, "w+b");
	if ( NULL == pTmpfile)//�ļ���ʧ��
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

//ɾ���ļ�
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

	//1.�������Ŀ¼�����ڣ��ȴ�������Ŀ¼
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( FALSE == bIsExist )
	{
		dwRet = OsaDirCreate( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );

		bIsExist = OsaIsDirExist( szTestDirFullPathName );
		CU_ASSERT( TRUE == bIsExist );
	}

	//2.����·���ļ��Ĵ�����ɾ��
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

	//3.ɾ���ļ�
	dwRet = OsaFileDelete( szTestFileFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	OsaSleep(10 ); //�������Բ��������ӵȴ���֤
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( FALSE == bIsExist );

	//4.ɾ������Ŀ¼
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	bIsExist = OsaIsPathExist( szTestDirFullPathName );
	CU_ASSERT( FALSE == bIsExist );
}

//��ȡ�ļ�����
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

	//1.�������Ŀ¼�����ڣ��ȴ�������Ŀ¼
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

	//2.����ָ����С���ļ�
	bRet = sCreateTestFileBySize( szTestFileFullPathName, qwTestFileLen1 );
	CU_ASSERT( TRUE == bRet );
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( TRUE == bIsExist );

	//3.���Ի�ȡ�ļ���С
	qwTestFileLenGet = OsaFileLengthGet( szTestFileFullPathName );
	CU_ASSERT( qwTestFileLenGet == qwTestFileLen1 );

	dwRet = OsaFileDelete( szTestFileFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( FALSE == bIsExist );


	//4.����ָ����С���ļ�
	bRet = sCreateTestFileBySize( szTestFileFullPathName, qwTestFileLen2 );
	CU_ASSERT( TRUE == bRet );

	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( TRUE == bIsExist );

	//5.���Ի�ȡ�ļ���С
	qwTestFileLenGet = OsaFileLengthGet( szTestFileFullPathName );
	CU_ASSERT( qwTestFileLenGet == qwTestFileLen2 );

	dwRet = OsaFileDelete( szTestFileFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
	bIsExist = OsaIsPathExist( szTestFileFullPathName );
	CU_ASSERT( FALSE == bIsExist );

	//6.ɾ������Ŀ¼
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
}

//ɾ��Ŀ¼�µ������ļ�
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

	//1.�������Ŀ¼�����ڣ��ȴ�������Ŀ¼
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( FALSE == bIsExist )
	{
		dwRet = OsaDirCreate( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	//2.��ָ��Ŀ¼�´�������ļ�
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

	//3.ɾ��ָ��Ŀ¼�µ������ļ�
	dwRet = OsaDelFilesInDir( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

	//4.��֤�����ļ��Ѿ�ɾ��
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

	//5.ɾ������Ŀ¼
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );
}

//////////////////////////////////////////////////////////////////////////
//�쳣����


//����Ŀ¼
void ExcptTest_DirCreate( void )
{
	UINT32 dwRet = 0;
	//1.NULL Pointer
	dwRet = OsaDirCreate( NULL );
	CU_ASSERT( OSA_OK != dwRet );
}
//ɾ��Ŀ¼
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

	//2.�����ڵ�Ŀ¼
#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//1.����·��Ŀ¼�Ĵ�����ɾ��
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( bIsExist )
	{
		dwRet = OsaDirDelete( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK != dwRet );
}

//Ŀ¼�Ƿ����
void ExcptTest_IsDirExist( void )
{
	BOOL bRet = FALSE;
	//1.NULL Pointer
	bRet = OsaIsDirExist( NULL );
	CU_ASSERT( FALSE == bRet );

	//3.���ڴ��·��
	{
		char szEmptyPath[256];
		memset( szEmptyPath, 0, 256 );
		bRet = OsaIsDirExist(szEmptyPath);
		CU_ASSERT( FALSE == bRet );
	}
}
//·���Ƿ����
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

	//2.�����ڵ�·��
	bRet = OsaIsPathExist( szTestDirFullPathName );
	CU_ASSERT( FALSE == bRet );

	//3.���ڴ��·��
	{
		char szEmptyPath[256];
		memset( szEmptyPath, 0, 256 );
		bRet = OsaIsPathExist(szEmptyPath);
		CU_ASSERT( FALSE == bRet );
	}
}

//��ȡ�ļ����Ⱥ���
void ExcptTest_FileLengthGet( void )
{
	UINT64 qwLen = 0;
	//1.NULL Pointer
	qwLen = OsaFileLengthGet( NULL );
	CU_ASSERT( 0 == qwLen );
}
//ɾ���ļ�
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

	//2.�����ڵ��ļ�
#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//�������Ŀ¼�����ڣ��ȴ�������Ŀ¼
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

	//3.ɾ������Ŀ¼
	dwRet = OsaDirDelete( szTestDirFullPathName );
	CU_ASSERT( OSA_OK == dwRet );

}
//ɾ��Ŀ¼�µ������ļ�
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

	//2.�����ڵ�Ŀ¼
#ifdef _MSC_VER
	sprintf( szTestDirFullPathName, "%s\\%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#else
	sprintf( szTestDirFullPathName, "%s/%s", ptTestMgr->m_szCurWorkPath, szTestDirName );
#endif

	//�������Ŀ¼�����ڣ��ȴ�������Ŀ¼
	bIsExist = OsaIsDirExist( szTestDirFullPathName );
	if( bIsExist )
	{
		dwRet = OsaDirDelete( szTestDirFullPathName );
		CU_ASSERT( OSA_OK == dwRet );
	}

	dwRet = OsaDelFilesInDir( szTestDirFullPathName );
	CU_ASSERT( OSA_OK != dwRet );
}
#ifndef _H_TESTENV_
#define _H_TESTENV_

#define TEST_MRSRC_TAG "OsaTest"	//�ڴ�����ǩ
#define TEST_ERROCC OsaStr2Occ("TestErr")     

#define OSA_TEST_MRSRC_TAG "OsaTest"	//�ڴ�����ǩ

typedef struct tagOsaTestMgr
{
	BOOL m_bIsInit;		//�Ƿ��ʼ��
	char m_szCurWorkPath[260];
	TOsaInitParam m_tInitParam; //��ʼ��������Ϣ
 	HANDLE m_hMemAlloc; //Osaģ���ڲ����ڴ������	
	UINT32 m_dwRsrcTag;
}TOsaTestMgr;

//��ʼ������ģ��
BOOL TestModuleInit( const char* szCurWorkPath );
BOOL TestModuleUnInit();

//��ȡ���Թ�����
TOsaTestMgr* iTestMgrGet( );
//��ȡ�����ڴ���Դ��ǩ
UINT32 iTestRsrcTagGet( );

#endif
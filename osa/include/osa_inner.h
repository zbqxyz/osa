#ifndef _CDS_INNER_H
#define _CDS_INNER_H


#define TASKNAME_MAXLEN	32
typedef struct tagOsaThreadInfo 
{
	ENode m_tNode;
	TASKHANDLE m_hTaskHandle;
	TfuncThreadEntry m_pfTaskEntry;
	char m_achName[TASKNAME_MAXLEN];
	UINT8 m_byPri;
	ULONG m_ulTaskId;
	UINT32 m_dwStackSize;
	// 	UINT32 m_dwPID;
	// 	UINT32 m_dwStackBaseAddr;
	// 	BOOL m_bStackGrowLow;
	UINT32 m_dwCurHeartBeat;
	UINT32 m_dwLastHeartBeat;
	UINT32 m_dwHBRate;
	UINT32 m_dwHBChkItvl;
	UINT64 m_qwLastHBChkMs;
	UINT32 m_dwHBStopCount;
}TOsaThreadInfo;


#ifdef _MSC_VER
typedef	CRITICAL_SECTION InnerLockHandle;
#else
typedef	pthread_mutex_t	InnerLockHandle;
#endif

//��Դ��ǩ��Ϣ��Ŀǰ�������ڴ��ǩ
typedef struct tagRsrcTagInfo 
{
	UINT32	m_dwTag;
	char	m_achTagName[RSRCTAGNAME_MAXLEN+1];
}TRsrcTagInfo;

//�ڴ��
typedef struct tagOsaMemLib
{
	UINT32 m_dwCurMemAllocNum;						//��ǰ�Ѿ�������ڴ����������
	HANDLE m_ahMemAlloc[MAX_MEM_ALLOC_NUM];			//�ڴ����������
	LockHandle m_ahMemAllocLock[MAX_MEM_ALLOC_NUM]; //���ڱ����ڴ������

	TRsrcTagInfo *m_ptMemRsrcTagList;	//�ڴ���Դ��ǩ�б���̬����
	LockHandle m_hMemRsrcTagListLock;	//���ڱ����ڴ���Դ��ǩ�б���� 
	UINT32 m_dwMaxTagNum;				//�ڴ���Դ��ǩ����
	UINT32 m_dwAllocTagNum;				//�Ѿ����������Դ��ǩ����	
}TOsaMemLib;


#ifdef _MSC_VER
typedef HANDLE	TOsaSem;
#else
typedef struct tagLinuxSem
{
	pthread_mutex_t		mutex;
	pthread_cond_t		condition;
	UINT32				semCount;
	UINT32				semMaxCount;
}TLinuxSem;
typedef TLinuxSem TOsaSem;
#endif

#define OSA_SEMNAME_MAXLEN	32
#define OSA_FILENAME_MAXLEN	32

//�ź�������Ϣ�������������������ź���ͳһ��أ�
typedef struct tagOsaSemInfo
{
	ENode m_tNode;					//Ƕ��ڵ�
	TOsaSem m_tOsaSem;					//�ź���
	InnerLockHandle m_hInnerLightLock;	//��������
	BOOL m_bLightLock;				//�Ƿ���������
	UINT32 m_dwSemCount;			//����ֵ, ���������Ƿ�������ź���
	char m_szSemName[OSA_SEMNAME_MAXLEN + 1];		//�ź�����
	char m_szTakeFileName[OSA_FILENAME_MAXLEN + 1];	//take�������ڵ��ļ�
	UINT32 m_dwTakeLine;			//take�������ڵ�����
	UINT32 m_dwTakeCount;			//take����
	UINT32 m_dwGiveCount;			//give����
	// 	UINT64 m_qwCurTakeMs;			//��ǰtakeʱ�ĺ�����
	UINT32 m_dwCurTakeThreadID;		//��ǰtake�����߳�ID
	UINT32 m_dwCurGiveThreadID;		//��ǰgive�����߳�ID
	UINT32 m_dwSemTakeSeq;			//��ǰtake�����к�
}TOsaSemInfo;

//����
typedef struct tagOsaMgr
{
	BOOL m_bIsInit;		//�Ƿ��ʼ��
	TOsaInitParam m_tInitParam; //��ʼ��������Ϣ

	HANDLE m_hMemAlloc; //Osaģ���ڲ����ڴ������
	TelnetHandle m_hTelHdl;  //
	UINT32 m_dwRsrcTag;

	EList m_tSemWatchList; //�ź����������
	InnerLockHandle m_hSemWatchListLock; //�����ź����������(LockHandle��Ҫ����أ�������ⲻ���ٱ���أ�������InnerLockHandle )

	EList m_tTaskList; //�����б�
	LockHandle m_hTaskListLock; //���ڱ��������б���� 

	TOsaMemLib m_tMemLib; //�ڴ��
}TOsaMgr;


#define OSA_MRSRC_TAG "Osa"	//�ڴ�����ǩ

#define OSA_ERROCC OsaStr2Occ("OsaErr")     
#define OSA_MSGOCC OsaStr2Occ("OsaMsg")  
#define OSA_LOGOCC OsaStr2Occ("OsaLog") 


#ifdef _MSC_VER
#define TASK_NULL NULL 
#else
#define TASK_NULL 0   //��ֹlinux�¸澯
#endif

#ifdef _LINUX_
#define __FUNCDNAME__ __FUNCTION__  
#endif

#ifndef COMERR_UNINIT
#define COMERR_UNINIT COMERR_NOT_INIT
#endif

#define OSA_CHECK_NULL_TASKHANDLE_RETURN_U32( p, paramName )\
	if (TASK_NULL == (p) )\
{\
	dwRet = COMERR_INVALID_PARAM;\
	OsaLabPrt( OSA_ERROCC, "%s(), %s is TASK_NULL\n", __FUNCDNAME__, paramName );\
	return dwRet;\
}\

#define OSA_CHECK_NULL_POINTER_RETURN_U32( p, paramName )\
	if (NULL == (p) )\
	{\
		dwRet = COMERR_NULL_POINTER;\
		OsaLabPrt( OSA_ERROCC, "%s(), %s is null\n", __FUNCDNAME__, paramName );\
		return dwRet;\
	}\

#define OSA_CHECK_NULL_POINTER_RETURN_POINT( p,  paramName  )\
	if (NULL == (p) )\
	{\
		OsaLabPrt( OSA_ERROCC, "%s(), %s is null\n", __FUNCDNAME__, paramName);\
		return NULL;\
	}\

#define OSA_CHECK_NULL_POINTER_RETURN_VOID( p, paramName )\
	if (NULL == (p) )\
	{\
		OsaLabPrt( OSA_ERROCC, "%s(), %s is null\n", __FUNCDNAME__, paramName);\
		return ;\
	}\

#define OSA_CHECK_NULL_POINTER_RETURN_BOOL( p, paramName  )\
	if (NULL == (p) )\
	{\
		OsaLabPrt( OSA_ERROCC, "%s(), %s is null\n", __FUNCDNAME__, paramName);\
		return FALSE;\
	}\

#define OSA_CHECK_INIT_RETURN_U32( )\
	if (FALSE == iIsOsaInit() )\
	{\
		dwRet = COMERR_UNINIT;\
		OsaLabPrt( OSA_ERROCC, "%s(), Osa is not init\n", __FUNCDNAME__ );\
		return dwRet;\
	}\

#define OSA_CHECK_INIT_RETURN_POINT( )\
	if (FALSE == iIsOsaInit() )\
	{\
		OsaLabPrt( OSA_ERROCC, "%s(), Osa is not init\n", __FUNCDNAME__ );\
		return NULL;\
	}\


//����ʱ�������ȴ�������ͬ������
#define OSA_COND_WAIT(cdtn, timewait) \
{ \
	int dwWaitSum = 0; \
	while(!cdtn) \
	{ \
		OsaSleep(10); \
		dwWaitSum += 10; \
		if(dwWaitSum > timewait) \
		{ \
			break; \
		} \
	} \
}

//////////////////////////////////////////////////////////////////////////
// Common
TOsaMgr* iOsaMgrGet();
BOOL iIsOsaInit();

HANDLE iOsaMemAllocGet();
UINT32 iOsaMRsrcTagGet();

TOsaMemLib* iOsaMemLibGet();

//ȡ�ַ����ĺ���ָ�����ȵ��Ӵ�
const char* iOsaReverseTrim(const char *str,UINT32 dwLen);
//////////////////////////////////////////////////////////////////////////
// Thread
#ifdef _MSC_VER
//�ж�ָ���߳��Ƿ���
BOOL iWin_IsThreadActive( TASKHANDLE hThread );
#endif 

//��õ����̵߳�ID
ULONG iOsaThreadSelfID(void);


//////////////////////////////////////////////////////////////////////////
// lock��Sem
//�ڲ�������
void iInnerLightLockCreate( InnerLockHandle *phInnerLock );
//�ڲ�ɾ����
void iInnerLightLockDelete( InnerLockHandle *phInnerLock );
//���ü�ص�����������
void iInnerLightLockLockNoWatch( InnerLockHandle *phInnerLock );
//���ü�صĽ�����������
void iInnerLightLockUnLockNoWatch( InnerLockHandle *phInnerLock );

//////////////////////////////////////////////////////////////////////////
//memlib
//��ʼ���ڴ��
UINT32 iMemLibInit( );
//�˳��ڴ��
void iMemLibExit( );

//��ȡ����ڴ���Դ����
UINT32 iMaxMemTagNumGet( );

//////////////////////////////////////////////////////////////////////////
//telnet

//�õ�Osaģ���ʼ��������telnet������
TelnetHandle OsaTelnetHdlGet(void);

//��ʼ��telnet������
UINT32 InnerOsaTelnetSvrInit( UINT16 wStartPort, char* szLogFilePrefixName, OUT TelnetHandle *phTelHdl ); 

//�˳�telnet������
UINT32 InnerOsaTelnetSvrExit( TelnetHandle hTelHdl ); 

//��ʾ��ע��ĸ�ģ���������
void cmdshow( );
void help( );

void iOsaTelnetInit( );
void iOsaLog(OCC occName, const char* pszformat,...);

//������еĴ�ӡ��ǩ
//void labprtclear( );
void lc( );
//�����ļ���ӡ
void fpon( );
//�ر��ļ���ӡ
void fpoff( );

//����
void osahelp( );
//��ʾmgr
void osamgr( );
//��ʾ������Ϣ
void taskdump( );
//��ʾ���źŵ��ź���������������������Ϣ
void semdump( );
//��ʾ�ź��������Ϣ
void semdumpall( );
//��ʾ��������ڴ���Դ��ǩ�б�
void memrstaglist( );
//��ʾ�ڴ�������б�
void memalclist( );
//��ʾָ���ڴ������ͳ����Ϣ
void memalcinfo( UINT32 dwIdx );
//��ʾ�ڴ�������ڴ����Ϣ
void mempoolinfo( UINT32 dwIdx );
//��ʾָ���ڴ��������ָ����ǩ��δ�ͷŵ��ڴ��б�
void memnofree( UINT32 dwAlcIdx, UINT32 dwTagIdx );

void osaerron();
void osaerroff();
void osamsgon();
void osamsgoff();

#endif //_CDS_INNER_H



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

//资源标签信息，目前仅用于内存标签
typedef struct tagRsrcTagInfo 
{
	UINT32	m_dwTag;
	char	m_achTagName[RSRCTAGNAME_MAXLEN+1];
}TRsrcTagInfo;

//内存库
typedef struct tagOsaMemLib
{
	UINT32 m_dwCurMemAllocNum;						//当前已经申请的内存分配器数量
	HANDLE m_ahMemAlloc[MAX_MEM_ALLOC_NUM];			//内存分配器数组
	LockHandle m_ahMemAllocLock[MAX_MEM_ALLOC_NUM]; //用于保护内存分配器

	TRsrcTagInfo *m_ptMemRsrcTagList;	//内存资源标签列表，动态创建
	LockHandle m_hMemRsrcTagListLock;	//用于保护内存资源标签列表的锁 
	UINT32 m_dwMaxTagNum;				//内存资源标签数量
	UINT32 m_dwAllocTagNum;				//已经被申请的资源标签数量	
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

//信号量的信息（加入轻量级锁，和信号量统一监控）
typedef struct tagOsaSemInfo
{
	ENode m_tNode;					//嵌入节点
	TOsaSem m_tOsaSem;					//信号量
	InnerLockHandle m_hInnerLightLock;	//轻量级锁
	BOOL m_bLightLock;				//是否轻量级锁
	UINT32 m_dwSemCount;			//计数值, 用于区分是否二进制信号量
	char m_szSemName[OSA_SEMNAME_MAXLEN + 1];		//信号量名
	char m_szTakeFileName[OSA_FILENAME_MAXLEN + 1];	//take发生所在的文件
	UINT32 m_dwTakeLine;			//take发生所在的行数
	UINT32 m_dwTakeCount;			//take次数
	UINT32 m_dwGiveCount;			//give次数
	// 	UINT64 m_qwCurTakeMs;			//当前take时的毫秒数
	UINT32 m_dwCurTakeThreadID;		//当前take所属线程ID
	UINT32 m_dwCurGiveThreadID;		//当前give所属线程ID
	UINT32 m_dwSemTakeSeq;			//当前take的序列号
}TOsaSemInfo;

//定义
typedef struct tagOsaMgr
{
	BOOL m_bIsInit;		//是否初始化
	TOsaInitParam m_tInitParam; //初始化参数信息

	HANDLE m_hMemAlloc; //Osa模块内部的内存分配器
	TelnetHandle m_hTelHdl;  //
	UINT32 m_dwRsrcTag;

	EList m_tSemWatchList; //信号量监控链表
	InnerLockHandle m_hSemWatchListLock; //保护信号量监控链表(LockHandle需要被监控，而监控这不行再被监控，这里用InnerLockHandle )

	EList m_tTaskList; //任务列表
	LockHandle m_hTaskListLock; //用于保护任务列表的锁 

	TOsaMemLib m_tMemLib; //内存库
}TOsaMgr;


#define OSA_MRSRC_TAG "Osa"	//内存分配标签

#define OSA_ERROCC OsaStr2Occ("OsaErr")     
#define OSA_MSGOCC OsaStr2Occ("OsaMsg")  
#define OSA_LOGOCC OsaStr2Occ("OsaLog") 


#ifdef _MSC_VER
#define TASK_NULL NULL 
#else
#define TASK_NULL 0   //防止linux下告警
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


//带超时的条件等待，用于同步测试
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

//取字符串的后面指定长度的子串
const char* iOsaReverseTrim(const char *str,UINT32 dwLen);
//////////////////////////////////////////////////////////////////////////
// Thread
#ifdef _MSC_VER
//判断指定线程是否存活
BOOL iWin_IsThreadActive( TASKHANDLE hThread );
#endif 

//获得调用线程的ID
ULONG iOsaThreadSelfID(void);


//////////////////////////////////////////////////////////////////////////
// lock和Sem
//内部创建锁
void iInnerLightLockCreate( InnerLockHandle *phInnerLock );
//内部删除锁
void iInnerLightLockDelete( InnerLockHandle *phInnerLock );
//不用监控的锁轻量级锁
void iInnerLightLockLockNoWatch( InnerLockHandle *phInnerLock );
//不用监控的解锁轻量级锁
void iInnerLightLockUnLockNoWatch( InnerLockHandle *phInnerLock );

//////////////////////////////////////////////////////////////////////////
//memlib
//初始化内存库
UINT32 iMemLibInit( );
//退出内存库
void iMemLibExit( );

//获取最大内存资源数量
UINT32 iMaxMemTagNumGet( );

//////////////////////////////////////////////////////////////////////////
//telnet

//得到Osa模块初始化创建的telnet服务器
TelnetHandle OsaTelnetHdlGet(void);

//初始化telnet服务器
UINT32 InnerOsaTelnetSvrInit( UINT16 wStartPort, char* szLogFilePrefixName, OUT TelnetHandle *phTelHdl ); 

//退出telnet服务器
UINT32 InnerOsaTelnetSvrExit( TelnetHandle hTelHdl ); 

//显示已注册的各模块帮助命令
void cmdshow( );
void help( );

void iOsaTelnetInit( );
void iOsaLog(OCC occName, const char* pszformat,...);

//清除所有的打印标签
//void labprtclear( );
void lc( );
//开启文件打印
void fpon( );
//关闭文件打印
void fpoff( );

//帮助
void osahelp( );
//显示mgr
void osamgr( );
//显示任务信息
void taskdump( );
//显示有信号的信号量或上锁的轻量级锁信息
void semdump( );
//显示信号量监控信息
void semdumpall( );
//显示已申请的内存资源标签列表
void memrstaglist( );
//显示内存分配器列表
void memalclist( );
//显示指定内存分配器统计信息
void memalcinfo( UINT32 dwIdx );
//显示内存分配器内存池信息
void mempoolinfo( UINT32 dwIdx );
//显示指定内存分配器的指定标签项未释放的内存列表
void memnofree( UINT32 dwAlcIdx, UINT32 dwTagIdx );

void osaerron();
void osaerroff();
void osamsgon();
void osamsgoff();

#endif //_CDS_INNER_H



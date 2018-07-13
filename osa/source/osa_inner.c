#include "osa_common.h"
#include "osa_mempool.h"

TOsaMgr g_tOsaMgr; 

TOsaMgr* iOsaMgrGet()
{
	return &g_tOsaMgr;
}

BOOL iIsOsaInit()
{
	return g_tOsaMgr.m_bIsInit;
}

HANDLE iOsaMemAllocGet()
{
	return g_tOsaMgr.m_hMemAlloc;
}

UINT32 iOsaMRsrcTagGet()
{
	return g_tOsaMgr.m_dwRsrcTag;
}

TOsaMemLib* iOsaMemLibGet()
{
	return &g_tOsaMgr.m_tMemLib;
}

const char* iOsaReverseTrim(const char *str,UINT32 dwLen)
{
	const char *strRlt=NULL;
	UINT32 dwStrLen=0;

	if (NULL == str)
	{
		return NULL;
	}

	dwStrLen=(UINT32)strlen(str);
	if (dwStrLen>dwLen)
	{
		strRlt=str+(dwStrLen-dwLen);
	}
	else
	{
		strRlt=str;
	}

	return strRlt;
}

//////////////////////////////////////////////////////////////////////////
//telnet
// static __inline char * ENDSTR_FIXLEN(char *str, UINT32 fixlen) 
// {
// 	if(str==NULL)
// 	{
// 		return NULL;
// 	}
// 	return (strlen(str) <= fixlen) ? str : str + strlen(str) - fixlen;
// }

void osahelp( )
{
	TOsaMgr* ptMgr = iOsaMgrGet();
	OsaLabPrt( GENERAL_PRTLAB_OCC, "module ver: %s, osa compiletime: %s %s\n", ptMgr->m_tInitParam.szModuleVer,__DATE__, __TIME__);
	OsaLabPrt( GENERAL_PRTLAB_OCC, "osahelp: 显示操作系统适配模块帮助命令\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "osamgr: 显示操作系统适配模块Mgr\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "taskdump: 显示任务(线程)信息\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "semdump: 显示有信号的信号量或上锁的轻量级锁信息\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "semdumpall: 显示信号量监控信息\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "memrstaglist: 显示已申请的内存资源标签列表\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "memalclist: 显示内存分配器列表\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "memalcinfo: 显示指定内存分配器统计信息, 参数1：分配器索引(0~15)\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "mempoolinfo: 显示内存分配器内存池信息, 参数1：分配器索引(0~15)\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "memnofree: 显示指定内存分配器的指定标签项未释放的内存列表, 参数1：分配器索引(0~15),参数2：标签项索引(0~%d)\n", iMaxMemTagNumGet( ) );
	

	OsaLabPrt( GENERAL_PRTLAB_OCC, "osaerron: 开启错误打印\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "osaerroff: 关闭错误打印\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "osamsgon: 开启消息打印\n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "osamsgoff: 关闭消息打印\n");
}

//显示mgr
void osamgr( )
{
	TOsaMgr* ptMgr = iOsaMgrGet();
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	OsaLabPrt( GENERAL_PRTLAB_OCC, "IsInit: %d\n", ptMgr->m_bIsInit );
	if( ptMgr->m_bIsInit )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "初始化参数: \n");
		OsaLabPrt( GENERAL_PRTLAB_OCC, "  TelnetPort:        %d\n", ptMgr->m_tInitParam.wTelnetPort );
		OsaLabPrt( GENERAL_PRTLAB_OCC, "  LogFilePrefixName: %s\n", ptMgr->m_tInitParam.szLogFilePrefixName );
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "内部参数: \n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "  hMemAlloc: 0x%x\n", ptMgr->m_hMemAlloc);
	OsaLabPrt( GENERAL_PRTLAB_OCC, "  hTelHdl:   0x%x\n", ptMgr->m_hTelHdl);
	OsaLabPrt( GENERAL_PRTLAB_OCC, "  RsrcTag:   %d\n", ptMgr->m_dwRsrcTag);

	if( NULL != ptMemLib )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "内部库参数: \n");
		OsaLabPrt( GENERAL_PRTLAB_OCC, "  MaxTagNum:       %d\n", ptMemLib->m_dwMaxTagNum );
		OsaLabPrt( GENERAL_PRTLAB_OCC, "  AllocTagNum:     %d\n", ptMemLib->m_dwAllocTagNum );
		OsaLabPrt( GENERAL_PRTLAB_OCC, "  MaxMemAllocNum:  %d\n", MAX_MEM_ALLOC_NUM );
		OsaLabPrt( GENERAL_PRTLAB_OCC, "  CurMemAllocNum:  %d\n", ptMemLib->m_dwCurMemAllocNum );
	}
}

//显示任务信息
void taskdump( )
{
	UINT32 dwMaxNum = 0;
	ENode *ptNode = NULL;
	UINT32 dwCount = 0;
	TOsaThreadInfo *ptTaskInfo = NULL;
//	UINT32 dwFreeStackSize = 0;
	TOsaMgr* ptMgr = iOsaMgrGet();

	OsaLabPrt( GENERAL_PRTLAB_OCC, "%6s %20s %10s %8s %6s %5s %10s \n",
		"线程号", 
		"线程名称",
		"检测间隔",
		"心跳",
		"频率", 
		"优先级",
		"堆栈");

	OsaLightLockLock (ptMgr->m_hTaskListLock );

	dwMaxNum = EListSize(&ptMgr->m_tTaskList);
	ptNode = EListGetFirst(&ptMgr->m_tTaskList);

	while(ptNode != NULL && dwCount < dwMaxNum )
	{
		ptTaskInfo = HOST_ENTRY(ptNode, TOsaThreadInfo, m_tNode);

		OsaLabPrt( GENERAL_PRTLAB_OCC, "%6lu %20s %10d %8x %6d %5u %10u \n",
			ptTaskInfo->m_ulTaskId, 
			iOsaReverseTrim(ptTaskInfo->m_achName, 20),
			ptTaskInfo->m_dwHBChkItvl,
			ptTaskInfo->m_dwCurHeartBeat,
			ptTaskInfo->m_dwHBRate,
			ptTaskInfo->m_byPri, 
			ptTaskInfo->m_dwStackSize);				

		ptNode = EListNext(&ptMgr->m_tTaskList, ptNode);	
		dwCount++;
	}
	OsaLabPrt( GENERAL_PRTLAB_OCC, "total task num %d\n", dwMaxNum);
	OsaLightLockUnLock( ptMgr->m_hTaskListLock );
}

//显示信号量监控信息
void semdump( )
{
	TOsaMgr* ptMgr = iOsaMgrGet();

	UINT32 dwCount = 0;
	UINT32 dwMaxNum = 0;
	ENode *ptNode = NULL;
	TOsaSemInfo *ptSemInfo = NULL;

	OsaLabPrt( GENERAL_PRTLAB_OCC,  
		"%10s %8s %4s %8s %12s %12s %6s %6s %5s %s\n",
		"SemName",
		"Islight",
		"SCnt",
		"PSeq",
		"PCnt",
		"VCnt", 
		"PThd", 
		"VThd", 
		"PLine",
		"PFile");

	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);

	dwMaxNum = EListSize(&ptMgr->m_tSemWatchList);
	ptNode = EListGetFirst(&ptMgr->m_tSemWatchList);

	while(ptNode != NULL && dwCount < dwMaxNum )
	{
		ptSemInfo = HOST_ENTRY(ptNode, TOsaSemInfo, m_tNode);
		if(ptSemInfo->m_dwTakeCount >= (ptSemInfo->m_dwGiveCount + ptSemInfo->m_dwSemCount))
		{
			OsaLabPrt( GENERAL_PRTLAB_OCC,  
				"%10s %8d %4u %8u %12u %12u %6u %6u %5u %s\n",
				iOsaReverseTrim(ptSemInfo->m_szSemName, 10),
				ptSemInfo->m_bLightLock, 
				ptSemInfo->m_dwSemCount, 
				ptSemInfo->m_dwSemTakeSeq,
				ptSemInfo->m_dwTakeCount, 
				ptSemInfo->m_dwGiveCount,
				ptSemInfo->m_dwCurTakeThreadID,
				ptSemInfo->m_dwCurGiveThreadID, 
				ptSemInfo->m_dwTakeLine,				
				iOsaReverseTrim(ptSemInfo->m_szTakeFileName, OSA_FILENAME_MAXLEN) );
		}

		ptNode = EListNext(&ptMgr->m_tSemWatchList, ptNode);	
		dwCount++;
	}

	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	OsaLabPrt( GENERAL_PRTLAB_OCC, "\n");
}

//显示信号量监控信息
void semdumpall( )
{
	TOsaMgr* ptMgr = iOsaMgrGet();

	UINT32 dwCount = 0;
	UINT32 dwMaxNum = 0;
	ENode *ptNode = NULL;
	TOsaSemInfo *ptSemInfo = NULL;

	OsaLabPrt( GENERAL_PRTLAB_OCC,  
		"%10s %8s %4s %8s %12s %12s %6s %6s %5s %s\n",
		"SemName",
		"Islight",
		"SCnt",
		"PSeq",
		"PCnt",
		"VCnt", 
		"PThd", 
		"VThd", 
		"PLine",
		"PFile");

	iInnerLightLockLockNoWatch(&ptMgr->m_hSemWatchListLock);

	dwMaxNum = EListSize(&ptMgr->m_tSemWatchList);
	ptNode = EListGetFirst(&ptMgr->m_tSemWatchList);

	while(ptNode != NULL && dwCount < dwMaxNum )
	{
		int nLen = 0;
		ptSemInfo = HOST_ENTRY(ptNode, TOsaSemInfo, m_tNode);
		nLen = (int)strlen(ptSemInfo->m_szSemName);
		if( nLen > 0 )
		{
			OsaLabPrt( GENERAL_PRTLAB_OCC,  
				"%10s %8d %4u %8u %12u %12u %6u %6u %5u %s\n",
				iOsaReverseTrim(ptSemInfo->m_szSemName, 10),
				ptSemInfo->m_bLightLock, 
				ptSemInfo->m_dwSemCount, 
				ptSemInfo->m_dwSemTakeSeq,
				ptSemInfo->m_dwTakeCount, 
				ptSemInfo->m_dwGiveCount,
				ptSemInfo->m_dwCurTakeThreadID,
				ptSemInfo->m_dwCurGiveThreadID, 
				ptSemInfo->m_dwTakeLine,				
				iOsaReverseTrim(ptSemInfo->m_szTakeFileName, OSA_FILENAME_MAXLEN) );
		}

		ptNode = EListNext(&ptMgr->m_tSemWatchList, ptNode);	
		dwCount++;
	}

	iInnerLightLockUnLockNoWatch(&ptMgr->m_hSemWatchListLock);

	OsaLabPrt( GENERAL_PRTLAB_OCC, "\n");
}

//显示已申请的内存资源标签列表
void memrstaglist( )
{
	UINT32 i = 0;
	UINT32 dwMaxTagNum = 0;
	UINT32 dwAllocTagNum = 0;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	dwMaxTagNum = ptMemLib->m_dwMaxTagNum;
	dwAllocTagNum = ptMemLib->m_dwAllocTagNum;
	OsaLabPrt( GENERAL_PRTLAB_OCC, "MemRsrsTag MaxNum:%d, CurAllocNum:%d \n", dwMaxTagNum, dwAllocTagNum);
	OsaLabPrt( GENERAL_PRTLAB_OCC, "%8s %8s\n", "TagId", "TagName");

	for( i = 0; i < dwAllocTagNum; i++ )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "%8d %8s\n", ptMemLib->m_ptMemRsrcTagList[i].m_dwTag, ptMemLib->m_ptMemRsrcTagList[i].m_achTagName);
	}
}

//显示内存分配器列表
void memalclist( )
{
	UINT32 i = 0;
	UINT32 dwCurMemAllocNum = 0;
	UINT32 dwCount = 0;
	HANDLE hMemAlloc = NULL;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	dwCurMemAllocNum = ptMemLib->m_dwCurMemAllocNum;
	OsaLabPrt( GENERAL_PRTLAB_OCC, "MemAllocator MaxNum:%d, CurAllocNum:%d \n", MAX_MEM_ALLOC_NUM, dwCurMemAllocNum);
	OsaLabPrt( GENERAL_PRTLAB_OCC, " %-12s %-12s %-12s %-12s %-12s\n", 
		"AlcIdx", "SysMem", "UsrMem", "OsMlc", "PoolMlc");

	for( i = 0; i < MAX_MEM_ALLOC_NUM && dwCount < dwCurMemAllocNum; i++ )
	{
		OsaLightLockLock( ptMemLib->m_ahMemAllocLock[i] );
		hMemAlloc = ptMemLib->m_ahMemAlloc[i];
		if(  hMemAlloc != NULL )
		{
			MemAllocator *pAlloc=(MemAllocator *)hMemAlloc;
			OsaLabPrt( GENERAL_PRTLAB_OCC, " %-12d %-12d %-12d %-12d %-12d\n", 
				pAlloc->dwMemAllocIdx, 
				pAlloc->qwTotalSysMem, 
				pAlloc->qwTotalTagUserMem, 
				pAlloc->qwMemViaMalloc, 
				pAlloc->qwMemViaPoolMalloc);

			dwCount++;
		}
		OsaLightLockUnLock( ptMemLib->m_ahMemAllocLock[i] );
	}
}

//显示指定内存分配器统计信息
void memalcinfo( UINT32 dwIdx )
{
	UINT32 i = 0;
	UINT32 dwCount = 0;
	HANDLE hMemAlloc = NULL;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );
	if( dwIdx >= MAX_MEM_ALLOC_NUM )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "Param error! Idx limit( 0 ~ %d), your input Idx:%d \n", MAX_MEM_ALLOC_NUM, dwIdx);
		return;
	}

	hMemAlloc = ptMemLib->m_ahMemAlloc[dwIdx];
	if( NULL == hMemAlloc )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "ptMemLib->m_ahMemAlloc[%s] is NULL!\n", dwIdx);
		return;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "\n%-8s %-10s %-12s %-12s %-12s %-12s %-12s\n", 
		"TagIdx", "TagName", "UserMemBytes", "MallocCnt", "PoolMallocCnt", "MlcAlignCnt", "FreeCnt"); 


	OsaLightLockLock( ptMemLib->m_ahMemAllocLock[dwIdx] );
	for( i = 0; i < dwMaxTagNum && dwCount < ptMemLib->m_dwAllocTagNum; i++ )
	{
		MemAllocator *pAlloc=(MemAllocator *)hMemAlloc;
		if(  pAlloc->pqwTagUserMem[i] > 0 )
		{
			OsaLabPrt( GENERAL_PRTLAB_OCC, "%-8d %-10s %-12d %-12d %-12d %-12d\n",
				dwIdx, 
				ptMemLib->m_ptMemRsrcTagList[dwIdx].m_achTagName,
				pAlloc->pqwTagUserMem[dwIdx], 
				pAlloc->pqwTagMallocNum[dwIdx], 
				pAlloc->pqwTagPoolMallocNum[dwIdx], 
				pAlloc->pqwTagMemAlignNum[dwIdx], 
				pAlloc->pqwTagFreeNum[dwIdx]);

			dwCount++;
		}
	}
	OsaLightLockUnLock( ptMemLib->m_ahMemAllocLock[dwIdx] );
}

//显示内存分配器内存池信息
void mempoolinfo( UINT32 dwIdx )
{
	UINT32 i = 0;
//	UINT32 dwCount = 0;
	HANDLE hMemAlloc = NULL;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
//	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );
	if( dwIdx >= MAX_MEM_ALLOC_NUM )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "Param error! Idx limit( 0 ~ %d), your input Idx:%d \n", MAX_MEM_ALLOC_NUM, dwIdx);
		return;
	}

	hMemAlloc = ptMemLib->m_ahMemAlloc[dwIdx];
	if( NULL == hMemAlloc )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "ptMemLib->m_ahMemAlloc[%s] is NULL!\n", dwIdx);
		return;
	}

	OsaLightLockLock( ptMemLib->m_ahMemAllocLock[dwIdx] );
	{
		MemAllocator *pAlloc=(MemAllocator *)hMemAlloc;
		MemPoolShow( pAlloc->hPool );
	}
	OsaLightLockUnLock( ptMemLib->m_ahMemAllocLock[i] );
}

//显示指定内存分配器的指定标签项未释放的内存列表
void memnofree( UINT32 dwAlcIdx, UINT32 dwTagIdx )
{
//	UINT32 i = 0;
	ENode *ptNode = NULL;
	UINT32 dwCount = 0;
	HANDLE hMemAlloc = NULL;
	MemAllocator *pAlloc = NULL;
	MemAllocHeader* ptAllocHeader = NULL;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );
	if( dwAlcIdx >= MAX_MEM_ALLOC_NUM )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "Param error! dwAlcIdx limit( 0 ~ %d), your input Idx:%d \n", MAX_MEM_ALLOC_NUM, dwAlcIdx);
		return;
	}

	hMemAlloc = ptMemLib->m_ahMemAlloc[dwAlcIdx];
	if( NULL == hMemAlloc )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "ptMemLib->m_ahMemAlloc[%s] is NULL!\n", dwAlcIdx);
		return;
	}

	if( dwTagIdx >= dwMaxTagNum )
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "Param error! dwTagIdx limit( 0 ~ %d), your input dwTagIdx:%d \n", dwMaxTagNum, dwTagIdx);
		return;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "\n%-8s %-8s %-8s %-12s %-6s %-12s %-12s\n", 
		"Size", "AlgnSize", "OsSize", "MagicNum", "RefNum", "MallocLine", "MallocFile"); 


	OsaLightLockLock( ptMemLib->m_ahMemAllocLock[dwAlcIdx] );
	pAlloc = (MemAllocator *)hMemAlloc;
	ptNode = EListGetFirst(&pAlloc->tAllocInfoList);
	while(ptNode != NULL && dwCount < 5120 ) //最多打印5120块内存
	{
		ptAllocHeader = HOST_ENTRY(ptNode, MemAllocHeader, tNode);
		if( dwTagIdx == ptAllocHeader->dwTag )
		{

			OsaLabPrt( GENERAL_PRTLAB_OCC,  
				"%-8d %-8d %-8d 0x%-10x %-6d %-12d %s\n",
				ptAllocHeader->dwSize, 
				ptAllocHeader->dwAlignSize, 
				ptAllocHeader->dwOsSize, 
				ptAllocHeader->dwMagicNum,				
				ptAllocHeader->sdwRef,				
				ptAllocHeader->sdwLine,				
				iOsaReverseTrim((char*)ptAllocHeader->sFileName, OSA_FILENAME_MAXLEN) );
		}

		ptNode = EListNext(&pAlloc->tAllocInfoList, ptNode);	
		dwCount++;
	}
	OsaLightLockUnLock( ptMemLib->m_ahMemAllocLock[dwAlcIdx] );
}


//清除所有的打印标签
void lc( )
{
	OsaPrtLabClearAll();
	OsaPrtLabAdd( GENERAL_PRTLAB_OCC );
}

//开启文件打印
void fpon( )
{
	OsaLabPrt2FileSet( TRUE );
}

//关闭文件打印
void fpoff( )
{
	OsaLabPrt2FileSet( FALSE );
}


//打开错误打印
void osaerron( )
{
	OsaPrtLabAdd( OSA_ERROCC);
}

//关闭错误打印
void osaerroff( )
{
	OsaPrtLabDel( OSA_ERROCC);
}

//打开消息打印
void osamsgon( )
{
	OsaPrtLabAdd( OSA_MSGOCC);
}

//关闭消息打印
void osamsgoff( )
{
	OsaPrtLabDel( OSA_MSGOCC );
}


UINT32 sOsaAllCmdReg( )
{
	//通用命令
	OsaCmdReg( "cmdshow", (void*)cmdshow, "cmdshow(), 显示所有模块注册的帮助命令\n");
	OsaCmdReg( "help", (void*)help, "同cmdshow，显示所有模块注册的帮助命令\n");
	OsaCmdReg( "lc", (void*)lc, "lc(), 清除所有的打印标签\n");
	OsaCmdReg( "fpon", (void*)fpon, "fpon(), 开启文件打印\n");
	OsaCmdReg( "fpoff", (void*)fpoff, "fpoff(), 关闭文件打印\n");

	//本模块命令
	FastOsaCmdReg( osahelp,		"显示操作系统适配模块帮助命令\n");
	FastOsaCmdReg( osamgr,		"显示操作系统适配模块mgr\n");
	FastOsaCmdReg( taskdump,	"显示任务信息\n");
	FastOsaCmdReg( semdump,		"显示有信号的信号量或上锁的轻量级锁信息\n");
	FastOsaCmdReg( semdumpall,	"显示信号量监控信息\n");
	FastOsaCmdReg( memrstaglist,"显示已申请的内存资源标签列表\n");
	FastOsaCmdReg( memalclist,	"显示内存分配器列表\n");
	FastOsaCmdReg( memalcinfo,	"显示指定内存分配器统计信息\n");
	FastOsaCmdReg( mempoolinfo , "显示内存分配器内存池信息\n");
	FastOsaCmdReg( memnofree , "显示指定内存分配器的指定标签项未释放的内存列表\n");

	FastOsaCmdReg( osaerron, "开启错误打印\n");
	FastOsaCmdReg( osaerroff, "关闭错误打印\n");
	FastOsaCmdReg( osamsgon, "开启消息打印\n");
	FastOsaCmdReg( osamsgoff, "关闭消息打印\n");
	return 0;
}

void iOsaTelnetInit( )
{
	sOsaAllCmdReg( );
	//开启通用打印
	OsaPrtLabAdd( GENERAL_PRTLAB_OCC );

	//默认开启错误打印
	OsaPrtLabAdd( OSA_ERROCC);
	OsaPrtLabAdd( OSA_LOGOCC);

	//调试阶段
#ifdef COMM_DEBUG_TAG
	OsaPrtLabAdd( OSA_MSGOCC);
#endif
}

void iOsaLog(OCC occName, const char* pszformat,...)
{
	char achBuffer[1024];
	va_list args;
	int nLen;
	TelnetHandle hTelHdl = OsaTelnetHdlGet();

	va_start(args, pszformat);
	nLen = vsnprintf(achBuffer, sizeof(achBuffer)-1, pszformat, args);
	va_end(args);

	if (nLen <= 0)
	{
		return;
	}

	achBuffer[1024 - 1] = '\0';

	if( hTelHdl != NULL  )
	{
		OsaLabPrt( occName, "%s", achBuffer);
	}
	else
	{
		printf( "%s", achBuffer);
	}
	return;
}
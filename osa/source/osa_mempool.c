#include <stdlib.h>
#include <memory.h>
#include "osa_common.h"
#include "osa_mempool.h"


/*内存块链表*/ 
typedef struct tagMemBlockList
{
	UINT32 dwBlockSize;//内存块的大小
	UINT32 dwBlockType;//内存块类型
	EList tFreeList;//自由块链表
//	UINT32 dwTotalMem; //malloc分配的内存总数
	void *phAlloc;		//内存分配器句柄，统计实际内存用
	UINT32 dwUsedBlockNum; //用户用到的内存块数
}MemBlockList;

/*内存池*/
typedef struct tagMemPool
{
	MemBlockList *pPriBlockList;//按参数分配时初始设定的优先内存块链表数组(优先分配)
	UINT32 dwPriBlockCount;
	MemBlockList taBlockList[MEM_GRANULARITY_LEVEL];//2的整数幂内存块链表数组
	MemBlockList taMemAlignBlockList[MEM_GRANULARITY_LEVEL];	//memalign内存块链表数组
	MemBlockList tDrtOsList;//直接操作系统分配内存块链表（大小未必是统一的）
	MemBlockList tDrtOsMemAlignList;//直接操作系统memalign分配内存块链表（大小未必是统一的）
//	UINT64 qwTotalSysMemInMemPool; // MemPool句柄用掉的系统内存
	void *phAlloc;		//内存分配器句柄，统计实际内存用
}MemPool;


/************************************************************************/
/* 内部函数                                                              */
/************************************************************************/

BOOL InitCustomBlockList(MemPool *pPool, void *phAlloc, UINT32 *pdwCustomMem, UINT32 dwArrayNum);
BOOL InitExponentBlockList(MemPool *pPool, void *phAlloc);
BOOL InitDrtOsList(MemPool *pPool, void *phAlloc);

BOOL BlockListDestroy(MemBlockList *pBlockList);
void* BlockListAlloc(MemBlockList *pBlockList, UINT32 dwBlockType);
BOOL BlockListFree(MemBlockList *pBlockList,void *pMem);
BOOL DrtOsListFree(MemBlockList *pBlockList,void *pMem);
void* DrtOsListAlloc(MemBlockList *pBlockList,UINT32 dwBytes);
UINT32  Clip2Exponent(UINT32 x);
BOOL MemBlockValidate(void *pMem,UINT32 dwBlockType);


/*求一个数是2的几次幂（向上对齐）, 对于2G以上的内存, 有问题 !!!*/
UINT32  Clip2Exponent(UINT32 x)
{
	UINT32 u=0;
	const UINT32 dwMaxTimes=64;
	
	/*求整数的最接近2的幂*/
	x = x - 1;
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >>16);
//	x = x | (x >>32); //???
	x = x + 1;
	
	/*判断是否是2的幂*/
	if(0 != (x&(x-1)))
	{
		OsaLabPrt( OSA_ERROCC, "Clip2Exponent(), %d is not 2 exponent !!!\n");	
	}

	/*求一个数是2的几次幂*/
	while (u<dwMaxTimes && !(x&1))
	{
		x=x>>1;
		u++;
	}
	return u;

}

/*===========================================================
功能： 初始化定制内存块链表数组
输入参数说明：  MemPool *pPool － 内存池的指针
				void *phAlloc － 内存分配器指针
				UINT32 *pdwCustomMem - 传入定制内存信息, 必须大于1M，且数组中的内存大小不能相同。单位为 byte
				UINT32 dwArrayNum － 数组元素个数，即定制内存有dwArrayNum个不同尺寸
返回值说明： 成功，返回TRUE; 失败，返回FALSE
===========================================================*/
BOOL InitCustomBlockList(MemPool *pPool, void *phAlloc, UINT32 *pdwCustomMem, UINT32 dwArrayNum)
{
	UINT32 dwIdx = 0;
	MemBlockList *pPriBlockList = NULL;
	
	if( (NULL == pPool) || (NULL == phAlloc) || (NULL == pdwCustomMem) || (0 == dwArrayNum) )
	{
		printf("InitCustomBlockList(), pPool=0x%x, phAlloc=0x%x, pdwCustomMem=0x%x, dwArrayNum=%d\n", 
			(LONG)pPool, (LONG)phAlloc, (LONG)pdwCustomMem, dwArrayNum);

		return FALSE;
	}

	/*按照初始参数分配, 设置定制内存块链表数组*/
	pPriBlockList = (MemBlockList *)MallocByStatis(phAlloc, sizeof(MemBlockList)*dwArrayNum);
	if (NULL == pPriBlockList)
	{
		printf("InitCustomBlockList(), pPriBlockList malloc %d bytes failed\n", sizeof(MemBlockList)*dwArrayNum);		
		return FALSE;
	}

	memset(pPriBlockList,0,sizeof(MemBlockList)*dwArrayNum);
	
	pPool->pPriBlockList = pPriBlockList;
	pPool->dwPriBlockCount = dwArrayNum;
	
	/*按照参数中设定的大小(增加存链接信息和幻数的部分)和块数分配内存*/
	for (dwIdx=0; dwIdx < dwArrayNum; dwIdx++)
	{		
		memset(&pPriBlockList[dwIdx], 0, sizeof(MemBlockList));
		EListInit(&pPriBlockList[dwIdx].tFreeList);
		pPriBlockList[dwIdx].dwBlockType = BLOCKTYPE_CUSTOM;
		pPriBlockList[dwIdx].phAlloc = phAlloc;			
		pPriBlockList[dwIdx].dwBlockSize = pdwCustomMem[dwIdx];
	}

	return TRUE;
}

/*===========================================================
功能： 按照64,128,256,...,512K,1M初始化内存块链表数组
输入参数说明：  MemPool *pPool － 内存池的指针
				void *phAlloc － 内存分配器指针
返回值说明： 成功，返回TRUE; 失败，返回FALSE
===========================================================*/
BOOL InitExponentBlockList(MemPool *pPool, void *phAlloc)
{
	UINT32 dwIdx=0;

	if( (NULL == pPool) || (NULL == phAlloc) )
	{
		printf("InitExponentBlockList(), pPool=0x%x, phAlloc=0x%x\n", pPool, phAlloc);
		return FALSE;
	}
	
	for (dwIdx=0;dwIdx<MEM_GRANULARITY_LEVEL;dwIdx++)
	{
		memset(&(pPool->taBlockList[dwIdx]),0,sizeof(pPool->taBlockList[dwIdx]));
		pPool->taBlockList[dwIdx].dwBlockSize = (1 << (MEM_GRANULARITY_MIN+dwIdx));
		pPool->taBlockList[dwIdx].dwBlockType = BLOCKTYPE_EXPONENT;
		pPool->taBlockList[dwIdx].phAlloc = phAlloc;
		EListInit(&pPool->taBlockList[dwIdx].tFreeList);
	}
	
	return TRUE;
}

/*===========================================================
功能： 按照64,128,256,...,512K,1M初始化align内存块链表数组
输入参数说明：  MemPool *pPool － 内存池的指针
void *phAlloc － 内存分配器指针
返回值说明： 成功，返回TRUE; 失败，返回FALSE
===========================================================*/
BOOL InitAlignBlockList(MemPool *pPool, void *phAlloc)
{
	UINT32 dwIdx=0;

	if( (NULL == pPool) || (NULL == phAlloc) )
	{
		printf("InitAlignBlockList(), pPool=0x%x, phAlloc=0x%x\n", pPool, phAlloc);
		return FALSE;
	}

	for (dwIdx=0;dwIdx<MEM_GRANULARITY_LEVEL;dwIdx++)
	{
		memset(&(pPool->taMemAlignBlockList[dwIdx]),0,sizeof(pPool->taMemAlignBlockList[dwIdx]));
		pPool->taMemAlignBlockList[dwIdx].dwBlockSize = (1 << (MEM_GRANULARITY_MIN+dwIdx));
		pPool->taMemAlignBlockList[dwIdx].dwBlockType = BLOCKTYPE_POOLALIGN;
		pPool->taMemAlignBlockList[dwIdx].phAlloc = phAlloc;
		EListInit(&pPool->taMemAlignBlockList[dwIdx].tFreeList);
	}

	return TRUE;
}

/*===========================================================
功能： 初始化大内存块链表
输入参数说明：  MemPool *pPool － 内存池的指针
				void *phAlloc － 内存分配器指针
返回值说明： 成功，返回TRUE; 失败，返回FALSE
===========================================================*/
BOOL InitDrtOsList(MemPool *pPool, void *phAlloc)
{
	if( (NULL == pPool) || (NULL == phAlloc) )
	{
		printf("InitDrtOsList(), pPool=0x%x, phAlloc=0x%x\n", pPool, phAlloc);
		return FALSE;
	}

	memset(&(pPool->tDrtOsList),0,sizeof(pPool->tDrtOsList));
	pPool->tDrtOsList.dwBlockType = BLOCKTYPE_DIRECT;
	pPool->tDrtOsList.phAlloc = phAlloc;
	EListInit(&pPool->tDrtOsList.tFreeList);
	
	return TRUE;
}

/*===========================================================
功能： 初始化大内存块链表
输入参数说明：  MemPool *pPool － 内存池的指针
void *phAlloc － 内存分配器指针
返回值说明： 成功，返回TRUE; 失败，返回FALSE
===========================================================*/
BOOL InitDrtOsMemAlignList(MemPool *pPool, void *phAlloc)
{
	if( (NULL == pPool) || (NULL == phAlloc) )
	{
		printf("InitDrtOsMemAlignList(), pPool=0x%x, phAlloc=0x%x\n", pPool, phAlloc);
		return FALSE;
	}

	memset(&(pPool->tDrtOsMemAlignList),0,sizeof(pPool->tDrtOsMemAlignList));
	pPool->tDrtOsMemAlignList.dwBlockType = BLOCKTYPE_ALIGN;
	pPool->tDrtOsMemAlignList.phAlloc = phAlloc;
	EListInit(&pPool->tDrtOsMemAlignList.tFreeList);

	return TRUE;
}

/*===========================================================
功能： 判断内存块是否有效
输入参数说明：  void *pMem 内存块的指针
				UINT32 dwBlockType block类型
回值说明： 有效则返回真，否则假
===========================================================*/
BOOL MemBlockValidate(void *pMem, UINT32 dwBlockType)
{
//	UINT32 dwSize=0;
	MemAllocHeader *ptMemAllocHeader = NULL;
	
	if (NULL == pMem)
	{
		OsaLabPrt( OSA_ERROCC, "MemBlockValidate(), pMem is null\n");
		return FALSE;
	}

	ptMemAllocHeader = MEMHEADER_ADDRESS(pMem);

	if( ptMemAllocHeader->dwBlockType != dwBlockType )
	{
	//	printf("MemBlockValidate(), head magic 0x%x != 0x%x\n", ptMemBlockHead->dwMagicNum, dwMagicNum);
		return FALSE;
	}


	return TRUE;
}


/*===========================================================
功能： 释放内存块链表
输入参数说明：  MemBlockList *pBlockList 内存块链表
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL BlockListDestroy(MemBlockList *pBlockList)
{
	BOOL bDestroy=FALSE;
	UINT32 dwRet = 0;
	ENode *pNode=NULL;
	MemAllocHeader *pMemAlloc = NULL;
//	void *pMem = NULL;

	if (NULL == pBlockList)
	{
		printf("BlockListDestroy(), pBlockList is null\n");
		return FALSE;
	}
	
	//pBlockList->tFreeList.dwLength ??? set while count
	
	pNode = EListGetFirst(&pBlockList->tFreeList);
	
	while (NULL != pNode)
	{
		dwRet=EListRemoveFirst(&pBlockList->tFreeList);
		if( dwRet != 0)
		{
			printf("BlockListDestroy(), EListRemoveFirst() failed, blocksize=%d!!!\n", pBlockList->dwBlockSize);
			return FALSE;
		}

		pMemAlloc = MEMHEADER_ENTRY_BLOCKNODE(pNode);	
		if( pMemAlloc->dwMagicNum != _MEMALLOC_MAGIC_NUME )
		{
			printf("BlockListDestroy(), dwMagicNum:(0x%x) error!!!\n", pMemAlloc->dwMagicNum );		
		}
		else
		{
			pMemAlloc->dwBlockType = BLOCKTYPE_NONE;
			FreeByStatis(pBlockList->phAlloc, pMemAlloc, pMemAlloc->dwSize); //???
		}
		
		pNode = NULL;
		if( EListIsEmpty(&pBlockList->tFreeList ))
		{
			break;
		}
		pNode = EListGetFirst(&pBlockList->tFreeList);
	}
	bDestroy=TRUE;

	return bDestroy;
}

/*===========================================================
功能： 释放align内存块链表
输入参数说明：  MemBlockList *pBlockList 内存块链表
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL AlignBlockListDestroy(MemBlockList *pBlockList)
{
	BOOL bDestroy=FALSE;
	UINT32 dwRet = 0;
	ENode *pNode=NULL;
	MemAllocHeader *pMemAlloc = NULL;
//	void *pMem = NULL;
	void *pSysAddr = NULL;

	if (NULL == pBlockList)
	{
		printf("AlignBlockListDestroy(), pBlockList is null\n");
		return FALSE;
	}

	//pBlockList->tFreeList.dwLength ??? set while count

	pNode = EListGetFirst(&pBlockList->tFreeList);

	while (NULL != pNode)
	{
		dwRet=EListRemoveFirst(&pBlockList->tFreeList);
		if(dwRet != 0 )
		{
			printf("AlignBlockListDestroy(), EListRemoveFirst() failed, blocksize=%d!!!\n", pBlockList->dwBlockSize);
			return FALSE;
		}

		pMemAlloc = MEMHEADER_ENTRY_BLOCKNODE(pNode);	
		pMemAlloc->dwBlockType = BLOCKTYPE_NONE;

		pSysAddr = (void *)pMemAlloc->lSysAddr; //得到真实的系统内存地址
		if(pSysAddr == NULL)
		{
			printf("AlignBlockListDestroy(), pSysAddr is null\n");
			return FALSE;
		}

		FreeByStatis(pBlockList->phAlloc, pSysAddr, pMemAlloc->dwSize); //???

		pNode = NULL;
		if( EListIsEmpty(&pBlockList->tFreeList ))
		{
			break;
		}
		pNode = EListGetFirst(&pBlockList->tFreeList);
	}
	bDestroy=TRUE;

	return bDestroy;
}

/*===========================================================
功能： 在某个内存块链表中分配内存
算法实现：若链表头节点为free，则表头节点就是所求节点，摘链，置使用标志，并加到链表尾部。
否则向操作系统请求分配，，置使用标志，并加到链表尾部。
输入参数说明：  MemBlockList *pBlockList － 内存块链表
				UINT32 dwBlockType - 内存块类型
返回值说明： 所分配的用户内存地址
===========================================================*/
void* BlockListAlloc(MemBlockList *pBlockList, UINT32 dwBlockType)
{
	void *pMem=NULL;
	ENode *pNode=NULL;
	void *pBuf = NULL;
//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	UINT32 dwRet = 0;
	UINT32 dwRealBlockSize=0;

	if (NULL == pBlockList)
	{
		OsaLabPrt( OSA_ERROCC, "BlockListAlloc(), pBlockList is null\n");
		return NULL;
	}

	pFreeList = &(pBlockList->tFreeList);
	pNode = (EListGetFirst(pFreeList));

	if (NULL != pNode)
	{
		pMemAlloc = MEMHEADER_ENTRY_BLOCKNODE(pNode);

		if(pMemAlloc->sdwRef == 0)
		{
			pMem = MEM_ADDRESS(pMemAlloc);
			
			/*从链首摘下，置于链尾*/
			dwRet=EListRemoveFirst(pFreeList);
			if(dwRet != 0)
			{
				OsaLabPrt( OSA_ERROCC, "BlockListAlloc(), EListRemoveFirst() failed, blocksize=%d !!!\n", pMemAlloc->dwSize);
				return NULL;
			}
			
			dwRet=EListInsertLast(pFreeList,pNode);
			if(dwRet != 0)
			{
				OsaLabPrt( OSA_ERROCC, "BlockListAlloc(), EListInsertLast() failed, blocksize=%d !!!\n", pMemAlloc->dwSize);
				return NULL;
			}
			
			pBlockList->dwUsedBlockNum++;
		}
	}
	
// 	// 若分配，则返回; 如果是定制内存，即使拿不到，也返回，不再从系统分配
// 	if((dwBlockType == BLOCKTYPE_CUSTOM) || (NULL != pMem))
// 	{
// 		return pMem;
// 	
//	}

	if(NULL != pMem)
	{
		return pMem;	
	}

	//请求操作系统分配
	dwRealBlockSize = pBlockList->dwBlockSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail);

	pBuf = MallocByStatis(pBlockList->phAlloc, dwRealBlockSize);
	if (NULL == pBuf)
	{
		OsaLabPrt( OSA_ERROCC, "BlockListAlloc(), malloc size %d, failed\n", pBlockList->dwBlockSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail));
		return NULL;
	}

	pMemAlloc = (MemAllocHeader *)pBuf;

	pMemAlloc->dwBlockType = pBlockList->dwBlockType;
	pMemAlloc->dwOsSize = dwRealBlockSize;

	pNode = BLOCKNODE_ENTRY(pMemAlloc);
	memset(pNode, 0, sizeof(*pNode));

	dwRet=EListInsertLast(pFreeList, pNode);
	if(dwRet != 0)
	{
		OsaLabPrt( OSA_ERROCC, "BlockListAlloc(), EListInsertLast() failed, blocksize=%d ##2 !!!\n", pMemAlloc->dwSize);
		return NULL;
	}
	
	pMem = MEM_ADDRESS(pMemAlloc);

	pBlockList->dwUsedBlockNum++;

	return pMem;
}



/*===========================================================
功能： 从aling内存块链表中分配内存
算法实现：若链表头节点为free，则表头节点就是所求节点，摘链，置使用标志，并加到链表尾部。
否则向操作系统请求分配，，置使用标志，并加到链表尾部。
输入参数说明：  MemBlockList *pBlockList － 内存块链表
, UINT32 dwBlockType - 内存块类型
返回值说明： 所分配的用户内存地址
===========================================================*/
void* AlignBlockListAlloc(MemBlockList *pBlockList, UINT32 dwAlign)
{
	void *pMem=NULL;
	ENode *pNode=NULL;
	void *pBuf = NULL;

	void* lSysUsrAddr = 0;
	void* lSysUsrAlignAddr = 0;
	void* lSysAddr = 0;	

	void *pSysAddr = NULL;
	//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	UINT32 dwRet = 0;
	UINT32 dwRealBlockSize=0;
	void* lShift = 0;
	UINT32 dwMaxNum = 0;


	if (NULL == pBlockList)
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListAlloc(), pBlockList is null\n");
		return NULL;
	}

	
	pFreeList = &(pBlockList->tFreeList);
	pNode = (EListGetFirst(pFreeList));
	dwMaxNum = pFreeList->dwSize;

	while(NULL != pNode )
	{
		//内存头
		pMemAlloc = MEMHEADER_ENTRY_BLOCKNODE(pNode);

		if((pMemAlloc->sdwRef == 0) && (pMemAlloc->dwAlign == dwAlign))
		{
			//用户内存
			pMem = MEM_ADDRESS(pMemAlloc);

#if 0
			/*从链首摘下，置于链尾*/
			dwRet=EListRemoveFirst(pFreeList);
			if(dwRet != 0)
			{
				OsaLabPrt( OSA_ERROCC, "AlignBlockListAlloc(), EListRemoveFirst() failed, blocksize=%d !!!\n", pMemAlloc->dwSize);
				return NULL;
			}

			dwRet=EListInsertLast(pFreeList,pNode);
			if(dwRet != 0)
			{
				OsaLabPrt( OSA_ERROCC, "AlignBlockListAlloc(), EListInsertLast() failed, blocksize=%d !!!\n", pMemAlloc->dwSize);
				return NULL;
			}
#endif

			pBlockList->dwUsedBlockNum++;

			break;
		}
		
		pNode = EListNext(pFreeList, pNode);
	}


	if(NULL != pMem)
	{
		return pMem;	
	}

	//请求操作系统分配, 加上对齐的字节数
	dwRealBlockSize = pBlockList->dwBlockSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail) + dwAlign - 1;
		
	//从操作系统分配内存
	pSysAddr = MallocByStatis(pBlockList->phAlloc, dwRealBlockSize);
	if (NULL == pSysAddr)
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListAlloc(), malloc size %d, failed\n", pBlockList->dwBlockSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail));
		return NULL;
	}

	lSysAddr = pSysAddr;

	//计算偏移量
	lSysUsrAddr = (void*)((UINT64)lSysAddr + sizeof(MemAllocHeader));
	lSysUsrAlignAddr = (void*)(((UINT64)lSysUsrAddr + dwAlign - 1)/dwAlign * dwAlign);

	//不可能出现的情况 !!!
	if(lSysUsrAlignAddr < lSysUsrAddr)
	{
		FreeByStatis(pBlockList->phAlloc, pSysAddr, dwRealBlockSize);
		OsaLabPrt( OSA_ERROCC, "!!!!!! AlignBlockListAlloc(), dwSysUsrAlignAddr(0x%x) < dwSysUsrAddr(0x%x)\n",
			lSysUsrAlignAddr, lSysUsrAddr);
		return NULL;
	}
	
	lShift = (void*)((UINT64)lSysUsrAlignAddr - (UINT64)lSysUsrAddr);

	//对齐后的头信息
	pBuf = (void *)((UINT64)lSysAddr + (UINT64)lShift);
	pMemAlloc = (MemAllocHeader *)pBuf;

	pMemAlloc->dwBlockType = BLOCKTYPE_POOLALIGN;
	pMemAlloc->dwOsSize = dwRealBlockSize;
	pMemAlloc->lSysAddr = lSysAddr;
	pMemAlloc->dwAlign = dwAlign;
	//pMemAlloc->dwSize


	pNode = BLOCKNODE_ENTRY(pMemAlloc);
	memset(pNode, 0, sizeof(*pNode));

	dwRet=EListInsertLast(pFreeList, pNode);
	if(dwRet != 0)
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListAlloc(), EListInsertLast() failed, blocksize=%d ##2 !!!\n", pMemAlloc->dwSize);
		return NULL;
	}

	pMem = MEM_ADDRESS(pMemAlloc);

	pBlockList->dwUsedBlockNum++;

	return pMem;
};

/*===========================================================
功能： 在某个内存块链表中释放内存（不真正释放）
输入参数说明：  MemBlockList *pBlockList 内存块链表
				void *pMem 内存地址
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL BlockListFree(MemBlockList *pBlockList,void *pMem)
{
//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	ENode *pNode=NULL;
	UINT32 dwRet = 0;

	if ( (NULL == pBlockList) || (NULL == pMem) )
	{
		OsaLabPrt( OSA_ERROCC, "BlockListFree(), pBlockList=0x%x, pMem=0x%x\n", pBlockList, pMem);
		return FALSE;		
	}	
	
	if(FALSE == MemBlockValidate(pMem, pBlockList->dwBlockType))
	{
		OsaLabPrt( OSA_ERROCC, "BlockListFree(), MemBlockValidate() failed\n");
		return FALSE;
	}

	pFreeList = &pBlockList->tFreeList;	

	pMemAlloc = MEMHEADER_ADDRESS(pMem);	
	

	/*摘链，并置于链首*/
	pNode = BLOCKNODE_ENTRY(pMemAlloc);
	
	dwRet=EListRemove(pFreeList,pNode);
	if( dwRet != 0 )
	{
		OsaLabPrt( OSA_ERROCC, "BlockListFree(), EListRemove failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return FALSE;
	}
	
	dwRet=EListInsertFirst(pFreeList,pNode);
	if( dwRet != 0 )
	{
		OsaLabPrt( OSA_ERROCC, "BlockListFree(), EListInsertFirst failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return FALSE;
	}

	pBlockList->dwUsedBlockNum--;

	return TRUE;
}


/*===========================================================
功能： 在align内存块链表中释放内存（不真正释放）
输入参数说明：  MemBlockList *pBlockList 内存块链表
void *pMem 内存地址
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL AlignBlockListFree(MemBlockList *pBlockList,void *pMem)
{
	//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	ENode *pNode=NULL;
	UINT32 dwRet = 0;

	if ( (NULL == pBlockList) || (NULL == pMem) )
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListFree(), pBlockList=0x%x, pMem=0x%x\n", pBlockList, pMem);
		return FALSE;		
	}	

	if(FALSE == MemBlockValidate(pMem, pBlockList->dwBlockType))
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListFree(), MemBlockValidate() failed\n");
		return FALSE;
	}

	pFreeList = &pBlockList->tFreeList;	

	pMemAlloc = MEMHEADER_ADDRESS(pMem);	


	/*摘链，并置于链首*/
	pNode = BLOCKNODE_ENTRY(pMemAlloc);

	dwRet=EListRemove(pFreeList,pNode);
	if( dwRet != 0 )
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListFree(), EListRemove failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return FALSE;
	}

	dwRet=EListInsertFirst(pFreeList,pNode);
	if( dwRet != 0 )
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListFree(), EListInsertFirst failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return FALSE;
	}

	pBlockList->dwUsedBlockNum--;

	return TRUE;
}

/*===========================================================
功能： 在大内存块链表中释放内存
输入参数说明：  MemBlockList *pDrtOsList 直通os内存块链表
				void *pMem 内存地址
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL DrtOsListFree(MemBlockList *pDrtOsList,void *pMem)
{
//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	ENode *pNode=NULL;
	UINT32 dwRet = 0;

	BOOL bValid = FALSE;

	if ( (NULL == pDrtOsList) || (NULL == pMem) )
	{
		OsaLabPrt( OSA_ERROCC, "DrtOsListFree(), pDrtOsList=0x%x, pMem=0x%x\n", pDrtOsList, pMem);
		return FALSE;
	}
	
	bValid = MemBlockValidate(pMem,pDrtOsList->dwBlockType);
	if(bValid == FALSE)
	{		
		OsaLabPrt( OSA_ERROCC, "DrtOsListFree(), MemBlockValidate() failed\n");
		return FALSE;
	}


	pFreeList=&pDrtOsList->tFreeList;
	
//	pMemBlock=MEMBLOCK_ADDRESS(pMem);
	pMemAlloc = MEMHEADER_ADDRESS(pMem);

	/*摘链，并释放内存*/
	pNode=BLOCKNODE_ENTRY(pMemAlloc);
	
	dwRet=EListRemove(pFreeList,pNode);
	if(dwRet != 0 )
	{
		OsaLabPrt( OSA_ERROCC, "BlockListFree(), EListRemove failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return FALSE;
	}
	
	pMemAlloc->dwBlockType = BLOCKTYPE_NONE;
	
	FreeByStatis(pDrtOsList->phAlloc, pMemAlloc, REALSIZE(pMemAlloc));

	return TRUE;
}



/*===========================================================
功能： 在大内存块链表中释放内存
输入参数说明：  MemBlockList *pDrtOsList 直通os内存块链表
void *pMem 内存地址
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL DrtOsMemAlignListFree(MemBlockList *pDrtOsList,void *pMem)
{
	//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	ENode *pNode=NULL;
	UINT32 dwRet = 0;
	BOOL bValid = FALSE;
	void *pbySys = NULL;

	if ( (NULL == pDrtOsList) || (NULL == pMem) )
	{
		OsaLabPrt( OSA_ERROCC, "DrtOsMemAlignListFree(), pDrtOsList=0x%x, pMem=0x%x\n", pDrtOsList, pMem);
		return FALSE;
	}

	bValid = MemBlockValidate(pMem,pDrtOsList->dwBlockType);
	if(bValid == FALSE)
	{		
		OsaLabPrt( OSA_ERROCC, "DrtOsMemAlignListFree(), MemBlockValidate() failed\n");
		return FALSE;
	}


	pFreeList=&pDrtOsList->tFreeList;

	//	pMemBlock=MEMBLOCK_ADDRESS(pMem);
	pMemAlloc = MEMHEADER_ADDRESS(pMem);

	/*摘链，并释放内存*/
	pNode=BLOCKNODE_ENTRY(pMemAlloc);

	dwRet=EListRemove(pFreeList,pNode);
	if(dwRet != 0)
	{
		OsaLabPrt( OSA_ERROCC, "BlockListFree(), EListRemove failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return FALSE;
	}

	pMemAlloc->dwBlockType = BLOCKTYPE_NONE;
	
	pbySys = (void *)(pMemAlloc->lSysAddr);

	FreeByStatis(pDrtOsList->phAlloc, pbySys, REALSIZE(pMemAlloc));

	return TRUE;
}




/*===========================================================
功能： 在大内存块链表中分配内存,直接向操作系统请求分配
输入参数说明：  MemBlockList *pDrtOsList 大内存块链表
				UINT32 dwBytes 待分配的大小
返回值说明： 所分配的内存地址
===========================================================*/
void* DrtOsListAlloc(MemBlockList *pDrtOsList,UINT32 dwBytes)
{
	void *pMem=NULL;
	ENode *pNode=NULL;
//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	UINT32 dwRet=0;
	UINT32 dwBlockSize=0;
	void *pBuf = NULL;
	
	if (NULL == pDrtOsList)
	{
		OsaLabPrt( OSA_ERROCC, "DrtOsListAlloc(), pDrtOsList is null\n");
		return NULL;
	}
	
	pFreeList=&pDrtOsList->tFreeList;
	
	/*请求操作系统分配*/
	dwBlockSize = dwBytes + sizeof(MemAllocHeader) + sizeof(MemAllocTail);

	pBuf = MallocByStatis(pDrtOsList->phAlloc, dwBlockSize);
	if(NULL == pBuf)
	{
		OsaLabPrt( OSA_ERROCC, "DrtOsListAlloc(), malloc %d bytes failed\n", dwBlockSize);
		return NULL;
	}

//	pMemBlock = BLOCKADDR_BY_ALLOCHEAD(pBuf);
	pMemAlloc = (MemAllocHeader *)pBuf;

	pMemAlloc->dwBlockType = BLOCKTYPE_DIRECT;	
	pMemAlloc->dwOsSize = dwBlockSize;

	pNode = BLOCKNODE_ENTRY(pMemAlloc);
		
	memset(pNode, 0, sizeof(*pNode));

	dwRet=EListInsertLast(pFreeList, pNode);
	if(dwRet != 0)
	{
		FreeByStatis(pDrtOsList->phAlloc, pMemAlloc, REALSIZE(pMemAlloc));
		OsaLabPrt( OSA_ERROCC, "DrtOsListAlloc(), EListInsertLast failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return NULL;
	}	
	
	pMem = MEM_ADDRESS(pMemAlloc);

	return pMem;
}




/*===========================================================
功能： 在大内存块链表中分配内存,直接向操作系统 memalign
输入参数说明：  MemBlockList *pDrtOsList 大内存块链表
UINT32 dwBytes 待分配的大小
返回值说明： 所分配的内存地址
===========================================================*/
void* DrtOsListMemAlign(MemBlockList *pDrtOsMemAlignList, UINT32 dwAlign, UINT32 dwBytes)
{
	void *pMem=NULL;
	ENode *pNode=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	EList *pFreeList=NULL;
	UINT32 dwRet=0;
	UINT32 dwBlockSize=0;
	void *pBuf = NULL;
	void *pSysAddr = NULL;

	void* lSysAddr = 0;
	void* lSysUsrAddr = 0;
	void* lSysUsrAlignAddr = 0;

	void* lShift = 0;

	if (NULL == pDrtOsMemAlignList)
	{
		OsaLabPrt( OSA_ERROCC, "DrtOsListMemAlign(), pDrtOsMemAlignList is null\n");
		return NULL;
	}

	pFreeList=&pDrtOsMemAlignList->tFreeList;

	/*请求操作系统分配, 加上对齐的字节数*/
	dwBlockSize = dwBytes + sizeof(MemAllocHeader) + sizeof(MemAllocTail) + dwAlign - 1;

	pSysAddr = MallocByStatis(pDrtOsMemAlignList->phAlloc, dwBlockSize);
	if(NULL == pSysAddr)
	{
		OsaLabPrt( OSA_ERROCC, "DrtOsListMemAlign(), malloc %d bytes failed\n", dwBlockSize);
		return NULL;
	}

	lSysAddr = pSysAddr;

	//计算偏移量
	lSysUsrAddr = (void*)((UINT64)lSysAddr + sizeof(MemAllocHeader));
	lSysUsrAlignAddr = (void*)(((UINT64)lSysUsrAddr + dwAlign - 1)/dwAlign * dwAlign);

	lShift = (void*)((UINT64)lSysUsrAlignAddr - (UINT64)lSysUsrAddr);

	//对齐后的头信息
	pBuf = (void *)((UINT64)lSysAddr + (UINT64)lShift);
	pMemAlloc = (MemAllocHeader *)pBuf;


	pMemAlloc->dwBlockType = BLOCKTYPE_ALIGN;	
	pMemAlloc->lSysAddr = lSysAddr;
	pMemAlloc->dwAlign = dwAlign;
	pMemAlloc->dwOsSize = dwBlockSize;

	pNode = BLOCKNODE_ENTRY(pMemAlloc);

	memset(pNode, 0, sizeof(*pNode));

	dwRet=EListInsertLast(pFreeList, pNode);
	if( dwRet != 0)
	{
		FreeByStatis(pDrtOsMemAlignList->phAlloc, pMemAlloc, REALSIZE(pMemAlloc));
		OsaLabPrt( OSA_ERROCC, "DrtOsListMemAlign(), EListInsertLast failed, size=%d, type=0x%x !!!\n", pMemAlloc->dwSize, pMemAlloc->dwBlockType);
		return NULL;
	}	

	pMem = MEM_ADDRESS(pMemAlloc);

	return pMem;
}


/************************************************************************/
/* 接口函数                                                              */
/************************************************************************/
/*===========================================================
功能： 生成内存池
输入参数说明：  
				UINT32 *pdwCustomMem - 传入定制内存信息, 必须大于1M，且数组中的内存大小不能相同。单位为 byte
				UINT32 dwArrayNum － 数组元素个数，即定制内存有dwArrayNum个不同尺寸
				void *phAlloc － 内存分配器句柄
返回值说明： 内存池的句柄，失败返回NULL
===========================================================*/
HANDLE MemPoolCreate(UINT32 *pdwCustomMem, UINT32 dwArrayNum, void *phAlloc)
{
	MemPool *pPool=NULL;
//	UINT32 dwIdx=0,dwIdxBlock=0;
//	UINT32 dwRealBlockSize=0;
//	UINT32 dwBlockNum=0;
//	MemBlockHeader *pMemBlock=NULL;
//	MemAllocHeader *pMemAlloc = NULL;
//	void *pBuf = NULL;
//	MemBlockList *pPriBlockList=NULL;
	BOOL bSuc = FALSE;
//	ENode *pNode = NULL;

	if(phAlloc == NULL)
	{
		printf("MemPoolCreate(), phAlloc is null\n");
		return NULL;
	}

//	printf("sizeof(MemPool)=%d\n", sizeof(MemPool));
	pPool = (MemPool *)MallocByStatis(phAlloc, sizeof(MemPool));
	if (NULL == pPool)
	{
		return NULL;
	}

	memset(pPool,0,sizeof(MemPool));

	//记录phAlloc
	pPool->phAlloc = phAlloc;
	
	/*初始化2的整数幂内存块链表数组*/
	bSuc = InitExponentBlockList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitExponentBlockList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}

	/*初始化pool align内存块链表数组*/
	bSuc = InitAlignBlockList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitAlignBlockList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}
	
	/*初始化align内存块链表数组*/
	bSuc = InitDrtOsMemAlignList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitDrtOsMemAlignList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}
		
	/*初始化直通内存块链表*/
	bSuc = InitDrtOsList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitDrtOsList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}

	//如果不定制内存，返回句柄
	if( (pdwCustomMem == NULL) || (dwArrayNum == 0) )
	{
		return (HANDLE)pPool;	
	}

	//初始化定制内存块
	bSuc = InitCustomBlockList(pPool, phAlloc, pdwCustomMem, dwArrayNum);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitCustomBlockList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}
	return (HANDLE)pPool;
}

/*===========================================================
功能： 释放内存池
输入参数说明：  HANDLE hPool 内存池的句柄
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL MemPoolDestroy(HANDLE hPool)
{
	BOOL bDestory=TRUE;
	MemPool *pPool=(MemPool *)hPool;
	UINT32 dwIdx=0;

	if (NULL == pPool)
	{
		printf("MemPoolDestroy(), pPool is null\n");
		return FALSE;
	}

	/*清除优先内存块链表*/		
	if (NULL != pPool->pPriBlockList)
	{
		for (dwIdx=0;dwIdx<pPool->dwPriBlockCount;dwIdx++)
		{
			bDestory &= BlockListDestroy(&pPool->pPriBlockList[dwIdx]);
		}

		FreeByStatis(pPool->phAlloc, pPool->pPriBlockList, sizeof(MemBlockList)*(pPool->dwPriBlockCount)); //???
	}
	
	/*清除pool align内存块链表*/
	for (dwIdx=0;dwIdx<MEM_GRANULARITY_LEVEL;dwIdx++)
	{
		bDestory &= AlignBlockListDestroy(&pPool->taMemAlignBlockList[dwIdx]);
	}

	/*清除2的整数幂内存块链表*/
	for (dwIdx=0;dwIdx<MEM_GRANULARITY_LEVEL;dwIdx++)
	{
		bDestory &= BlockListDestroy(&pPool->taBlockList[dwIdx]);
	}
	
	/*清除直通内存块链表*/
	bDestory &= BlockListDestroy(&pPool->tDrtOsList);
	
	/*清除align内存块链表*/
	bDestory &= BlockListDestroy(&pPool->tDrtOsMemAlignList);

	/*释放内存池句柄*/
	FreeByStatis(pPool->phAlloc, pPool, sizeof(MemPool));
	pPool=NULL;

	return bDestory;
}



/*===========================================================
功能： 从内存池分配内存
输入参数说明：  HANDLE hPool 内存池的句柄
				UINT32 dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* MemPoolAlloc(HANDLE hPool, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;
	UINT32 dwIdx=0;
	UINT32 dwBinaPower=0;//dwBytes在二进制下的量级

	if(NULL == pPool)
	{
		OsaLabPrt( OSA_ERROCC, "MemPoolAlloc(), pPool=0x%x\n", pPool);
		return NULL;
	}


	/*有定制内存*/
	if(NULL != pPool->pPriBlockList)
	{	
		for(dwIdx = 0; dwIdx < pPool->dwPriBlockCount; dwIdx++)
		{
			if(pPool->pPriBlockList[dwIdx].dwBlockSize == dwBytes)
			{
				pMem = BlockListAlloc(&pPool->pPriBlockList[dwIdx], BLOCKTYPE_CUSTOM); //???
				return pMem;
			}
		}
	}
	

	/*在2的整数幂内存块链表中分配*/
	dwBinaPower = Clip2Exponent(dwBytes);
	dwIdx = (dwBinaPower < MEM_GRANULARITY_MIN) ? 0: (dwBinaPower-MEM_GRANULARITY_MIN);
	
	if (dwIdx < MEM_GRANULARITY_LEVEL)
	{
		/*按照默认分配方式*/
		pMem=BlockListAlloc(&pPool->taBlockList[dwIdx], BLOCKTYPE_EXPONENT);
	}

	return pMem;
}


/*===========================================================
功能： 从align内存池分配内存
输入参数说明：  HANDLE hPool 内存池的句柄
UINT32 dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* MemAlignAlloc(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;
	UINT32 dwIdx=0;
	UINT32 dwBinaPower=0;//dwBytes在二进制下的量级

	if(NULL == pPool)
	{
		OsaLabPrt( OSA_ERROCC, "MemAlignAlloc(), pPool=0x%x\n", pPool);
		return NULL;
	}


	/*在2的整数幂内存块链表中分配*/
	dwBinaPower = Clip2Exponent(dwBytes);
	dwIdx = (dwBinaPower < MEM_GRANULARITY_MIN) ? 0: (dwBinaPower-MEM_GRANULARITY_MIN);

	if (dwIdx < MEM_GRANULARITY_LEVEL)
	{		
		pMem = AlignBlockListAlloc(&pPool->taMemAlignBlockList[dwIdx], dwAlign);
	}
	else
	{
		return NULL;
	}

	return pMem;
}

/*===========================================================
功能： 直接从os分配内存
输入参数说明：  HANDLE hPool 内存池的句柄
				UINT32 dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* MemOsAlloc(HANDLE hPool, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;

	pMem = DrtOsListAlloc(&pPool->tDrtOsList,dwBytes);

	return pMem;
}

/*===========================================================
功能： 直接从os memalign
输入参数说明：  HANDLE hPool 内存池的句柄
UINT32 dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* OsMemAlign(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;

	pMem = DrtOsListMemAlign(&pPool->tDrtOsMemAlignList, dwAlign, dwBytes);

	return pMem;
}

/*===========================================================
功能： 释放内存，将被释放的内存从链表中删除
输入参数说明：  HANDLE hPool 内存池的句柄
				void *pMem 待释放的用户内存指针
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL MemPoolFree(HANDLE hPool,void *pMem)
{
	BOOL bFree=FALSE;
	MemPool *pPool=(MemPool *)hPool;
	UINT32 dwSize=0;
	UINT32 dwBinaPower=0;//dwSize在二进制下的量级
	UINT32 dwIdx=0;
	MemAllocHeader *pMemAlloc = NULL;
	
	if ( (NULL == pPool) || (NULL == pMem) )
	{
		OsaLabPrt( OSA_ERROCC, "MemPoolFree(), pPool=0x%x, pMem=0x%x\n", pPool, pMem);
		return FALSE;
	}	
	
	pMemAlloc = MEMHEADER_ADDRESS(pMem);
	dwSize = pMemAlloc->dwSize;

	if((NULL != pPool->pPriBlockList) && MemBlockValidate(pMem,BLOCKTYPE_CUSTOM))
	{
		/*若优先内存块链表存在，则先在优先链表中释放*/
		for (dwIdx=0;dwIdx<pPool->dwPriBlockCount;dwIdx++)
		{
			if (pPool->pPriBlockList[dwIdx].dwBlockSize == dwSize)
			{
				bFree=BlockListFree(&pPool->pPriBlockList[dwIdx],pMem);
			}
		}
	}
	
	if (TRUE == bFree)
	{
		return TRUE;		
	}

	/*若还没有释放，则在默认分配表中释放*/
	dwBinaPower=Clip2Exponent(dwSize);
	dwIdx=(dwBinaPower < MEM_GRANULARITY_MIN)?0:(dwBinaPower-MEM_GRANULARITY_MIN);
	
	if (dwIdx < MEM_GRANULARITY_LEVEL && MemBlockValidate(pMem,BLOCKTYPE_EXPONENT))
	{
		/*按照2的整数幂释放方式*/
		bFree=BlockListFree(&pPool->taBlockList[dwIdx],pMem);
	}
	else if (dwIdx < MEM_GRANULARITY_LEVEL && MemBlockValidate(pMem,BLOCKTYPE_POOLALIGN))
	{
		/*按照pool ALIGN释放方式*/
		bFree=AlignBlockListFree(&pPool->taMemAlignBlockList[dwIdx],pMem);
	}
	else if(MemBlockValidate(pMem,BLOCKTYPE_DIRECT))
	{
		/*释放操作系统内存块*/
		bFree=DrtOsListFree(&pPool->tDrtOsList,pMem);
	}
	else if(MemBlockValidate(pMem,BLOCKTYPE_ALIGN))
	{
		/*释放操作系统align内存块*/
		bFree=DrtOsMemAlignListFree(&pPool->tDrtOsMemAlignList,pMem);
	}
	else
	{
		OsaLabPrt( OSA_ERROCC, "MemPoolFree(), mem 0x%x is not in any block !!!\n", pMem);
	}

	return bFree;
}



/*===========================================================
功能： 添加内存引用计数
输入参数说明：  HANDLE hPool 内存池的句柄
				void *pMem 用户内存块的指针
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL MemPoolMDup(HANDLE hPool,void *pMem)
{
	BOOL bDup=FALSE;
	MemPool *pPool=(MemPool *)hPool;
//	MemBlockHeader *pMemBlock=NULL;
	
	if ( (NULL != pPool) && (NULL != pMem) )
	{
		if ( MemBlockValidate(pMem,BLOCKTYPE_CUSTOM) ||
			 MemBlockValidate(pMem,BLOCKTYPE_EXPONENT) ||
			 MemBlockValidate(pMem,BLOCKTYPE_DIRECT) 
			)
		{
		//	pMemBlock=MEMBLOCK_ADDRESS(pMem);
		//	ADD_BLOCK_REF(pMemBlock);
			bDup=TRUE;
		}
	}

	return bDup;
}

/*===========================================================
功能：得到指定大小的定制内存块的个数
输入参数说明：  HANDLE hPool - 内存池的句柄
				UINT32 dwSize - 块大小
返回值说明： 
===========================================================*/
UINT32 CustomBlockNumGet(HANDLE hPool, UINT32 dwSize)
{
	UINT32 dwListNum = 0;
	UINT32 dwIdx = 0;
	UINT32 dwBlockNum = 0;
	MemPool *pPool = NULL;
	MemBlockList *pList = NULL;

	if((hPool == NULL) || (dwSize == 0))
	{
		OsaLabPrt( OSA_ERROCC, "CustomBlockNumGet(), hPool=0x%x, size=%d\n", hPool, dwSize);
		return 0;
	}

	pPool = (MemPool *)hPool;
	dwListNum = pPool->dwPriBlockCount;

	if((dwListNum == 0) || (pPool->pPriBlockList == NULL))
	{
		return 0;
	}

	for(dwIdx = 0; dwIdx < dwListNum; dwIdx++)
	{
		pList = pPool->pPriBlockList + dwIdx;	
		if(pList->dwBlockSize == dwSize)
		{
			dwBlockNum = pList->tFreeList.dwSize;
			return dwBlockNum;
		}
	}

	return 0;
}

/*===========================================================
功能：得到指定大小的2的n次方内存块的个数
输入参数说明：  HANDLE hPool - 内存池的句柄
				UINT32 dwSize - 块大小
返回值说明： 
===========================================================*/
UINT32 ExponentBlockNumGet(HANDLE hPool, UINT32 dwSize)
{
	UINT32 dwIdx = 0;
	UINT32 dwBlockNum = 0;
	MemPool *pPool = NULL;
	MemBlockList *pList = NULL;

	if((hPool == NULL) || (dwSize == 0))
	{
		OsaLabPrt( OSA_ERROCC, "ExponentBlockNumGet(), hPool=0x%x, size=%d\n", hPool, dwSize);
		return 0;
	}

	pPool = (MemPool *)hPool;


	for(dwIdx = 0; dwIdx < MEM_GRANULARITY_LEVEL; dwIdx++)
	{
		pList = &(pPool->taBlockList[dwIdx]);

		if(pList->dwBlockSize == dwSize)
		{
			dwBlockNum = pList->tFreeList.dwSize;
			return dwBlockNum;
		}
	}

	return 0;
}

/*===========================================================
功能：得到直通内存块的个数
输入参数说明：  HANDLE hPool - 内存池的句柄
返回值说明： 
===========================================================*/
UINT32 DirectBlockNumGet(HANDLE hPool)
{
// 	UINT32 dwListNum = 0;
// 	UINT32 dwIdx = 0;
// 	UINT32 dwBlockNum = 0;
	MemPool *pPool = NULL;
	MemBlockList *pList = NULL;

	if(hPool == NULL) 
	{
		OsaLabPrt( OSA_ERROCC, "DirectBlockNumGet(), hPool=0x%x\n", hPool);
		return 0;
	}

	pPool = (MemPool *)hPool;
	pList = &(pPool->tDrtOsList);

	return pList->tFreeList.dwSize;
}

/*===========================================================
功能： 带统计的内存分配
输入参数说明：  phAlloc － 内存分配器句柄
				dwBytes － 要分配的字节数
返回值说明： 分配的内存指针
===========================================================*/								 
void* MallocByStatis(void *phAlloc, UINT32 dwBytes )
{
	void *p = NULL;	
	
	p = malloc(dwBytes);	
	if((p != NULL) && (phAlloc != NULL))
	{
		((MemAllocator *)phAlloc)->qwTotalSysMem += dwBytes;
	}	
	
	return p;
}

	
/*===========================================================
功能： 带统计的内存分配
输入参数说明：  phAlloc － 内存分配器句柄
				p － 要释放的指针
				dwBytes － 要释放的字节数
返回值说明： 
===========================================================*/						
void FreeByStatis(void *phAlloc, void *p, UINT32 dwBytes )
{
	if(p == NULL)
	{
		OsaLabPrt( OSA_ERROCC, "FreeByStatis(), p is NULL\n");
		return;
	}		
	
	free(p);

	if(phAlloc == NULL)
	{
		return;
	}

	if( ((MemAllocator *)phAlloc)->qwTotalSysMem >= dwBytes )
	{
		((MemAllocator *)phAlloc)->qwTotalSysMem -= dwBytes;
	}
	else
	{
		OsaLabPrt( OSA_ERROCC, "FreeByStatis() err, sysmem=%d, freeBytes=%d\n",
			((MemAllocator *)phAlloc)->qwTotalSysMem, dwBytes);
	}
	
	return;
}

/*===========================================================
功能： 显示内存池信息
输入参数说明： HANDLE hPool 内存池的句柄
返回值说明： 
===========================================================*/
void MemPoolShow( HANDLE hPool)
{
	UINT32 dwIdx = 0;
//	UINT32 dwCount = 0;
	UINT32 dwMaxNum = 0;
	MemPool *pPool=(MemPool *)hPool;
	MemBlockList *pMemBlockList = NULL;
//	MemBlockHeader *pMemBlock=NULL;
	MemAllocHeader *pMemAlloc = NULL;
	ENode *pNode = NULL;

	if(hPool == NULL)
	{
	//	printf("MemPoolShow(), hPool is null\n");
		return;
	}

	OsaLabPrt( GENERAL_PRTLAB_OCC, "\nmempool info:\n");

	//输出直通os内存块
	pMemBlockList = &(pPool->tDrtOsList);
	dwMaxNum = pMemBlockList->tFreeList.dwSize;

	if(dwMaxNum > 0)
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "\n-------- direct os block, bocknum=%d -------- \n", dwMaxNum);
		OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10s %-10s %-10s\n", "idx", "blockSize", "ref");
		
		pNode = EListGetFirst(&pMemBlockList->tFreeList);
		
		dwIdx = 0;
		while(pNode != NULL)
		{
			//pMemBlock = MEMBLOCK_ENTRY(pNode);
			pMemAlloc = MEMHEADER_ENTRY_BLOCKNODE(pNode);
			
			OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10d %-10d %-10d\n", 
				dwIdx, pMemAlloc->dwSize, pMemAlloc->sdwRef);
			
			pNode=EListNext(&(pMemBlockList->tFreeList),pNode);
			
			dwIdx++;
			if(dwIdx > dwMaxNum + 10)
			{
				OsaLabPrt( GENERAL_PRTLAB_OCC, "MemPoolShow(), drtos block show err, idx=%d, maxnum=%d\n", dwIdx, dwMaxNum);
				break;
			}
		}
	}


	//输出定制内存块
	if((pPool->pPriBlockList != NULL) && (pPool->dwPriBlockCount > 0))
	{
		OsaLabPrt( GENERAL_PRTLAB_OCC, "\n---------- user custom blocklist, num=%d ---------- \n", pPool->dwPriBlockCount);
		OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10s %-10s %-10s %-10s\n", "idx", "blockSize", "blocknum", "usedblocknum");

		for(dwIdx = 0; dwIdx < pPool->dwPriBlockCount; dwIdx++)
		{
			pMemBlockList = pPool->pPriBlockList + dwIdx;
			OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10d %-10d %-10d %-10d\n", 
				dwIdx, pMemBlockList->dwBlockSize, pMemBlockList->tFreeList.dwSize, pMemBlockList->dwUsedBlockNum);
		}
	}

	
	//输出2的整数幂内存块
	OsaLabPrt( GENERAL_PRTLAB_OCC, "\n---------- 2 exponent blocklist ---------- \n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10s %-10s %-10s %-10s\n", "exponent", "blockSize", "blocknum", "usedblocknum");

	for(dwIdx = 0; dwIdx < MEM_GRANULARITY_LEVEL; dwIdx++)
	{
		pMemBlockList = &(pPool->taBlockList[dwIdx]);

		if(pMemBlockList->tFreeList.dwSize == 0)
		{
			continue;
		}

		OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10d %-10d %-10d %-10d\n", 
			dwIdx + MEM_GRANULARITY_MIN, pMemBlockList->dwBlockSize, pMemBlockList->tFreeList.dwSize, pMemBlockList->dwUsedBlockNum);
	}


	//输出align内存块
	OsaLabPrt( GENERAL_PRTLAB_OCC, "\n---------- align blocklist ---------- \n");
	OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10s %-10s %-10s %-10s\n", "exponent", "blockSize", "blocknum", "usedblocknum");

	for(dwIdx = 0; dwIdx < MEM_GRANULARITY_LEVEL; dwIdx++)
	{
		pMemBlockList = &(pPool->taMemAlignBlockList[dwIdx]);

		if(pMemBlockList->tFreeList.dwSize == 0)
		{
			continue;
		}

		OsaLabPrt( GENERAL_PRTLAB_OCC, "%-10d %-10d %-10d %-10d\n", 
			dwIdx + MEM_GRANULARITY_MIN, pMemBlockList->dwBlockSize, pMemBlockList->tFreeList.dwSize, pMemBlockList->dwUsedBlockNum);
	}


	OsaLabPrt( GENERAL_PRTLAB_OCC, "\n");
	
	return;
}

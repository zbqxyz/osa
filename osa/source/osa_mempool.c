#include <stdlib.h>
#include <memory.h>
#include "osa_common.h"
#include "osa_mempool.h"


/*�ڴ������*/ 
typedef struct tagMemBlockList
{
	UINT32 dwBlockSize;//�ڴ��Ĵ�С
	UINT32 dwBlockType;//�ڴ������
	EList tFreeList;//���ɿ�����
//	UINT32 dwTotalMem; //malloc������ڴ�����
	void *phAlloc;		//�ڴ�����������ͳ��ʵ���ڴ���
	UINT32 dwUsedBlockNum; //�û��õ����ڴ����
}MemBlockList;

/*�ڴ��*/
typedef struct tagMemPool
{
	MemBlockList *pPriBlockList;//����������ʱ��ʼ�趨�������ڴ����������(���ȷ���)
	UINT32 dwPriBlockCount;
	MemBlockList taBlockList[MEM_GRANULARITY_LEVEL];//2���������ڴ����������
	MemBlockList taMemAlignBlockList[MEM_GRANULARITY_LEVEL];	//memalign�ڴ����������
	MemBlockList tDrtOsList;//ֱ�Ӳ���ϵͳ�����ڴ��������Сδ����ͳһ�ģ�
	MemBlockList tDrtOsMemAlignList;//ֱ�Ӳ���ϵͳmemalign�����ڴ��������Сδ����ͳһ�ģ�
//	UINT64 qwTotalSysMemInMemPool; // MemPool����õ���ϵͳ�ڴ�
	void *phAlloc;		//�ڴ�����������ͳ��ʵ���ڴ���
}MemPool;


/************************************************************************/
/* �ڲ�����                                                              */
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


/*��һ������2�ļ����ݣ����϶��룩, ����2G���ϵ��ڴ�, ������ !!!*/
UINT32  Clip2Exponent(UINT32 x)
{
	UINT32 u=0;
	const UINT32 dwMaxTimes=64;
	
	/*����������ӽ�2����*/
	x = x - 1;
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >>16);
//	x = x | (x >>32); //???
	x = x + 1;
	
	/*�ж��Ƿ���2����*/
	if(0 != (x&(x-1)))
	{
		OsaLabPrt( OSA_ERROCC, "Clip2Exponent(), %d is not 2 exponent !!!\n");	
	}

	/*��һ������2�ļ�����*/
	while (u<dwMaxTimes && !(x&1))
	{
		x=x>>1;
		u++;
	}
	return u;

}

/*===========================================================
���ܣ� ��ʼ�������ڴ����������
�������˵����  MemPool *pPool �� �ڴ�ص�ָ��
				void *phAlloc �� �ڴ������ָ��
				UINT32 *pdwCustomMem - ���붨���ڴ���Ϣ, �������1M���������е��ڴ��С������ͬ����λΪ byte
				UINT32 dwArrayNum �� ����Ԫ�ظ������������ڴ���dwArrayNum����ͬ�ߴ�
����ֵ˵���� �ɹ�������TRUE; ʧ�ܣ�����FALSE
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

	/*���ճ�ʼ��������, ���ö����ڴ����������*/
	pPriBlockList = (MemBlockList *)MallocByStatis(phAlloc, sizeof(MemBlockList)*dwArrayNum);
	if (NULL == pPriBlockList)
	{
		printf("InitCustomBlockList(), pPriBlockList malloc %d bytes failed\n", sizeof(MemBlockList)*dwArrayNum);		
		return FALSE;
	}

	memset(pPriBlockList,0,sizeof(MemBlockList)*dwArrayNum);
	
	pPool->pPriBlockList = pPriBlockList;
	pPool->dwPriBlockCount = dwArrayNum;
	
	/*���ղ������趨�Ĵ�С(���Ӵ�������Ϣ�ͻ����Ĳ���)�Ϳ��������ڴ�*/
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
���ܣ� ����64,128,256,...,512K,1M��ʼ���ڴ����������
�������˵����  MemPool *pPool �� �ڴ�ص�ָ��
				void *phAlloc �� �ڴ������ָ��
����ֵ˵���� �ɹ�������TRUE; ʧ�ܣ�����FALSE
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
���ܣ� ����64,128,256,...,512K,1M��ʼ��align�ڴ����������
�������˵����  MemPool *pPool �� �ڴ�ص�ָ��
void *phAlloc �� �ڴ������ָ��
����ֵ˵���� �ɹ�������TRUE; ʧ�ܣ�����FALSE
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
���ܣ� ��ʼ�����ڴ������
�������˵����  MemPool *pPool �� �ڴ�ص�ָ��
				void *phAlloc �� �ڴ������ָ��
����ֵ˵���� �ɹ�������TRUE; ʧ�ܣ�����FALSE
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
���ܣ� ��ʼ�����ڴ������
�������˵����  MemPool *pPool �� �ڴ�ص�ָ��
void *phAlloc �� �ڴ������ָ��
����ֵ˵���� �ɹ�������TRUE; ʧ�ܣ�����FALSE
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
���ܣ� �ж��ڴ���Ƿ���Ч
�������˵����  void *pMem �ڴ���ָ��
				UINT32 dwBlockType block����
��ֵ˵���� ��Ч�򷵻��棬�����
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
���ܣ� �ͷ��ڴ������
�������˵����  MemBlockList *pBlockList �ڴ������
����ֵ˵���� �ɹ������棬�����
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
���ܣ� �ͷ�align�ڴ������
�������˵����  MemBlockList *pBlockList �ڴ������
����ֵ˵���� �ɹ������棬�����
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

		pSysAddr = (void *)pMemAlloc->lSysAddr; //�õ���ʵ��ϵͳ�ڴ��ַ
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
���ܣ� ��ĳ���ڴ�������з����ڴ�
�㷨ʵ�֣�������ͷ�ڵ�Ϊfree�����ͷ�ڵ��������ڵ㣬ժ������ʹ�ñ�־�����ӵ�����β����
���������ϵͳ������䣬����ʹ�ñ�־�����ӵ�����β����
�������˵����  MemBlockList *pBlockList �� �ڴ������
				UINT32 dwBlockType - �ڴ������
����ֵ˵���� ��������û��ڴ��ַ
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
			
			/*������ժ�£�������β*/
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
	
// 	// �����䣬�򷵻�; ����Ƕ����ڴ棬��ʹ�ò�����Ҳ���أ����ٴ�ϵͳ����
// 	if((dwBlockType == BLOCKTYPE_CUSTOM) || (NULL != pMem))
// 	{
// 		return pMem;
// 	
//	}

	if(NULL != pMem)
	{
		return pMem;	
	}

	//�������ϵͳ����
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
���ܣ� ��aling�ڴ�������з����ڴ�
�㷨ʵ�֣�������ͷ�ڵ�Ϊfree�����ͷ�ڵ��������ڵ㣬ժ������ʹ�ñ�־�����ӵ�����β����
���������ϵͳ������䣬����ʹ�ñ�־�����ӵ�����β����
�������˵����  MemBlockList *pBlockList �� �ڴ������
, UINT32 dwBlockType - �ڴ������
����ֵ˵���� ��������û��ڴ��ַ
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
		//�ڴ�ͷ
		pMemAlloc = MEMHEADER_ENTRY_BLOCKNODE(pNode);

		if((pMemAlloc->sdwRef == 0) && (pMemAlloc->dwAlign == dwAlign))
		{
			//�û��ڴ�
			pMem = MEM_ADDRESS(pMemAlloc);

#if 0
			/*������ժ�£�������β*/
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

	//�������ϵͳ����, ���϶�����ֽ���
	dwRealBlockSize = pBlockList->dwBlockSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail) + dwAlign - 1;
		
	//�Ӳ���ϵͳ�����ڴ�
	pSysAddr = MallocByStatis(pBlockList->phAlloc, dwRealBlockSize);
	if (NULL == pSysAddr)
	{
		OsaLabPrt( OSA_ERROCC, "AlignBlockListAlloc(), malloc size %d, failed\n", pBlockList->dwBlockSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail));
		return NULL;
	}

	lSysAddr = pSysAddr;

	//����ƫ����
	lSysUsrAddr = (void*)((UINT64)lSysAddr + sizeof(MemAllocHeader));
	lSysUsrAlignAddr = (void*)(((UINT64)lSysUsrAddr + dwAlign - 1)/dwAlign * dwAlign);

	//�����ܳ��ֵ���� !!!
	if(lSysUsrAlignAddr < lSysUsrAddr)
	{
		FreeByStatis(pBlockList->phAlloc, pSysAddr, dwRealBlockSize);
		OsaLabPrt( OSA_ERROCC, "!!!!!! AlignBlockListAlloc(), dwSysUsrAlignAddr(0x%x) < dwSysUsrAddr(0x%x)\n",
			lSysUsrAlignAddr, lSysUsrAddr);
		return NULL;
	}
	
	lShift = (void*)((UINT64)lSysUsrAlignAddr - (UINT64)lSysUsrAddr);

	//������ͷ��Ϣ
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
���ܣ� ��ĳ���ڴ���������ͷ��ڴ棨�������ͷţ�
�������˵����  MemBlockList *pBlockList �ڴ������
				void *pMem �ڴ��ַ
����ֵ˵���� �ɹ������棬�����
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
	

	/*ժ��������������*/
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
���ܣ� ��align�ڴ���������ͷ��ڴ棨�������ͷţ�
�������˵����  MemBlockList *pBlockList �ڴ������
void *pMem �ڴ��ַ
����ֵ˵���� �ɹ������棬�����
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


	/*ժ��������������*/
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
���ܣ� �ڴ��ڴ���������ͷ��ڴ�
�������˵����  MemBlockList *pDrtOsList ֱͨos�ڴ������
				void *pMem �ڴ��ַ
����ֵ˵���� �ɹ������棬�����
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

	/*ժ�������ͷ��ڴ�*/
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
���ܣ� �ڴ��ڴ���������ͷ��ڴ�
�������˵����  MemBlockList *pDrtOsList ֱͨos�ڴ������
void *pMem �ڴ��ַ
����ֵ˵���� �ɹ������棬�����
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

	/*ժ�������ͷ��ڴ�*/
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
���ܣ� �ڴ��ڴ�������з����ڴ�,ֱ�������ϵͳ�������
�������˵����  MemBlockList *pDrtOsList ���ڴ������
				UINT32 dwBytes ������Ĵ�С
����ֵ˵���� ��������ڴ��ַ
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
	
	/*�������ϵͳ����*/
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
���ܣ� �ڴ��ڴ�������з����ڴ�,ֱ�������ϵͳ memalign
�������˵����  MemBlockList *pDrtOsList ���ڴ������
UINT32 dwBytes ������Ĵ�С
����ֵ˵���� ��������ڴ��ַ
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

	/*�������ϵͳ����, ���϶�����ֽ���*/
	dwBlockSize = dwBytes + sizeof(MemAllocHeader) + sizeof(MemAllocTail) + dwAlign - 1;

	pSysAddr = MallocByStatis(pDrtOsMemAlignList->phAlloc, dwBlockSize);
	if(NULL == pSysAddr)
	{
		OsaLabPrt( OSA_ERROCC, "DrtOsListMemAlign(), malloc %d bytes failed\n", dwBlockSize);
		return NULL;
	}

	lSysAddr = pSysAddr;

	//����ƫ����
	lSysUsrAddr = (void*)((UINT64)lSysAddr + sizeof(MemAllocHeader));
	lSysUsrAlignAddr = (void*)(((UINT64)lSysUsrAddr + dwAlign - 1)/dwAlign * dwAlign);

	lShift = (void*)((UINT64)lSysUsrAlignAddr - (UINT64)lSysUsrAddr);

	//������ͷ��Ϣ
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
/* �ӿں���                                                              */
/************************************************************************/
/*===========================================================
���ܣ� �����ڴ��
�������˵����  
				UINT32 *pdwCustomMem - ���붨���ڴ���Ϣ, �������1M���������е��ڴ��С������ͬ����λΪ byte
				UINT32 dwArrayNum �� ����Ԫ�ظ������������ڴ���dwArrayNum����ͬ�ߴ�
				void *phAlloc �� �ڴ���������
����ֵ˵���� �ڴ�صľ����ʧ�ܷ���NULL
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

	//��¼phAlloc
	pPool->phAlloc = phAlloc;
	
	/*��ʼ��2���������ڴ����������*/
	bSuc = InitExponentBlockList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitExponentBlockList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}

	/*��ʼ��pool align�ڴ����������*/
	bSuc = InitAlignBlockList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitAlignBlockList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}
	
	/*��ʼ��align�ڴ����������*/
	bSuc = InitDrtOsMemAlignList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitDrtOsMemAlignList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}
		
	/*��ʼ��ֱͨ�ڴ������*/
	bSuc = InitDrtOsList(pPool, phAlloc);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitDrtOsList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}

	//����������ڴ棬���ؾ��
	if( (pdwCustomMem == NULL) || (dwArrayNum == 0) )
	{
		return (HANDLE)pPool;	
	}

	//��ʼ�������ڴ��
	bSuc = InitCustomBlockList(pPool, phAlloc, pdwCustomMem, dwArrayNum);
	if(bSuc == FALSE)
	{
		printf("MemPoolCreate(), InitCustomBlockList() failed\n");
		MemPoolDestroy((HANDLE)pPool);
	}
	return (HANDLE)pPool;
}

/*===========================================================
���ܣ� �ͷ��ڴ��
�������˵����  HANDLE hPool �ڴ�صľ��
����ֵ˵���� �ɹ������棬�����
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

	/*��������ڴ������*/		
	if (NULL != pPool->pPriBlockList)
	{
		for (dwIdx=0;dwIdx<pPool->dwPriBlockCount;dwIdx++)
		{
			bDestory &= BlockListDestroy(&pPool->pPriBlockList[dwIdx]);
		}

		FreeByStatis(pPool->phAlloc, pPool->pPriBlockList, sizeof(MemBlockList)*(pPool->dwPriBlockCount)); //???
	}
	
	/*���pool align�ڴ������*/
	for (dwIdx=0;dwIdx<MEM_GRANULARITY_LEVEL;dwIdx++)
	{
		bDestory &= AlignBlockListDestroy(&pPool->taMemAlignBlockList[dwIdx]);
	}

	/*���2���������ڴ������*/
	for (dwIdx=0;dwIdx<MEM_GRANULARITY_LEVEL;dwIdx++)
	{
		bDestory &= BlockListDestroy(&pPool->taBlockList[dwIdx]);
	}
	
	/*���ֱͨ�ڴ������*/
	bDestory &= BlockListDestroy(&pPool->tDrtOsList);
	
	/*���align�ڴ������*/
	bDestory &= BlockListDestroy(&pPool->tDrtOsMemAlignList);

	/*�ͷ��ڴ�ؾ��*/
	FreeByStatis(pPool->phAlloc, pPool, sizeof(MemPool));
	pPool=NULL;

	return bDestory;
}



/*===========================================================
���ܣ� ���ڴ�ط����ڴ�
�������˵����  HANDLE hPool �ڴ�صľ��
				UINT32 dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* MemPoolAlloc(HANDLE hPool, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;
	UINT32 dwIdx=0;
	UINT32 dwBinaPower=0;//dwBytes�ڶ������µ�����

	if(NULL == pPool)
	{
		OsaLabPrt( OSA_ERROCC, "MemPoolAlloc(), pPool=0x%x\n", pPool);
		return NULL;
	}


	/*�ж����ڴ�*/
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
	

	/*��2���������ڴ�������з���*/
	dwBinaPower = Clip2Exponent(dwBytes);
	dwIdx = (dwBinaPower < MEM_GRANULARITY_MIN) ? 0: (dwBinaPower-MEM_GRANULARITY_MIN);
	
	if (dwIdx < MEM_GRANULARITY_LEVEL)
	{
		/*����Ĭ�Ϸ��䷽ʽ*/
		pMem=BlockListAlloc(&pPool->taBlockList[dwIdx], BLOCKTYPE_EXPONENT);
	}

	return pMem;
}


/*===========================================================
���ܣ� ��align�ڴ�ط����ڴ�
�������˵����  HANDLE hPool �ڴ�صľ��
UINT32 dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* MemAlignAlloc(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;
	UINT32 dwIdx=0;
	UINT32 dwBinaPower=0;//dwBytes�ڶ������µ�����

	if(NULL == pPool)
	{
		OsaLabPrt( OSA_ERROCC, "MemAlignAlloc(), pPool=0x%x\n", pPool);
		return NULL;
	}


	/*��2���������ڴ�������з���*/
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
���ܣ� ֱ�Ӵ�os�����ڴ�
�������˵����  HANDLE hPool �ڴ�صľ��
				UINT32 dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* MemOsAlloc(HANDLE hPool, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;

	pMem = DrtOsListAlloc(&pPool->tDrtOsList,dwBytes);

	return pMem;
}

/*===========================================================
���ܣ� ֱ�Ӵ�os memalign
�������˵����  HANDLE hPool �ڴ�صľ��
UINT32 dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* OsMemAlign(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes)
{
	void *pMem=NULL;
	MemPool *pPool=(MemPool *)hPool;

	pMem = DrtOsListMemAlign(&pPool->tDrtOsMemAlignList, dwAlign, dwBytes);

	return pMem;
}

/*===========================================================
���ܣ� �ͷ��ڴ棬�����ͷŵ��ڴ��������ɾ��
�������˵����  HANDLE hPool �ڴ�صľ��
				void *pMem ���ͷŵ��û��ڴ�ָ��
����ֵ˵���� �ɹ������棬�����
===========================================================*/
BOOL MemPoolFree(HANDLE hPool,void *pMem)
{
	BOOL bFree=FALSE;
	MemPool *pPool=(MemPool *)hPool;
	UINT32 dwSize=0;
	UINT32 dwBinaPower=0;//dwSize�ڶ������µ�����
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
		/*�������ڴ��������ڣ������������������ͷ�*/
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

	/*����û���ͷţ�����Ĭ�Ϸ�������ͷ�*/
	dwBinaPower=Clip2Exponent(dwSize);
	dwIdx=(dwBinaPower < MEM_GRANULARITY_MIN)?0:(dwBinaPower-MEM_GRANULARITY_MIN);
	
	if (dwIdx < MEM_GRANULARITY_LEVEL && MemBlockValidate(pMem,BLOCKTYPE_EXPONENT))
	{
		/*����2���������ͷŷ�ʽ*/
		bFree=BlockListFree(&pPool->taBlockList[dwIdx],pMem);
	}
	else if (dwIdx < MEM_GRANULARITY_LEVEL && MemBlockValidate(pMem,BLOCKTYPE_POOLALIGN))
	{
		/*����pool ALIGN�ͷŷ�ʽ*/
		bFree=AlignBlockListFree(&pPool->taMemAlignBlockList[dwIdx],pMem);
	}
	else if(MemBlockValidate(pMem,BLOCKTYPE_DIRECT))
	{
		/*�ͷŲ���ϵͳ�ڴ��*/
		bFree=DrtOsListFree(&pPool->tDrtOsList,pMem);
	}
	else if(MemBlockValidate(pMem,BLOCKTYPE_ALIGN))
	{
		/*�ͷŲ���ϵͳalign�ڴ��*/
		bFree=DrtOsMemAlignListFree(&pPool->tDrtOsMemAlignList,pMem);
	}
	else
	{
		OsaLabPrt( OSA_ERROCC, "MemPoolFree(), mem 0x%x is not in any block !!!\n", pMem);
	}

	return bFree;
}



/*===========================================================
���ܣ� ����ڴ����ü���
�������˵����  HANDLE hPool �ڴ�صľ��
				void *pMem �û��ڴ���ָ��
����ֵ˵���� �ɹ������棬�����
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
���ܣ��õ�ָ����С�Ķ����ڴ��ĸ���
�������˵����  HANDLE hPool - �ڴ�صľ��
				UINT32 dwSize - ���С
����ֵ˵���� 
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
���ܣ��õ�ָ����С��2��n�η��ڴ��ĸ���
�������˵����  HANDLE hPool - �ڴ�صľ��
				UINT32 dwSize - ���С
����ֵ˵���� 
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
���ܣ��õ�ֱͨ�ڴ��ĸ���
�������˵����  HANDLE hPool - �ڴ�صľ��
����ֵ˵���� 
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
���ܣ� ��ͳ�Ƶ��ڴ����
�������˵����  phAlloc �� �ڴ���������
				dwBytes �� Ҫ������ֽ���
����ֵ˵���� ������ڴ�ָ��
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
���ܣ� ��ͳ�Ƶ��ڴ����
�������˵����  phAlloc �� �ڴ���������
				p �� Ҫ�ͷŵ�ָ��
				dwBytes �� Ҫ�ͷŵ��ֽ���
����ֵ˵���� 
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
���ܣ� ��ʾ�ڴ����Ϣ
�������˵���� HANDLE hPool �ڴ�صľ��
����ֵ˵���� 
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

	//���ֱͨos�ڴ��
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


	//��������ڴ��
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

	
	//���2���������ڴ��
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


	//���align�ڴ��
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

#include "osa_common.h"
#include "osa_mempool.h"

#define OSA_DFT_MAXTAG_NUM (UINT32)256

#define ALIGN_BYTES		4   //�����ֽ�����arm��Ҫ�����
#define SIZE_ALIGN(size, align) ( (size % align == 0) ? size : ((size/align) + 1) * align )


//��ʼ���ڴ��
UINT32 iMemLibInit(  )
{
	UINT32 dwRet = 0;
	UINT32 dwIdx = 0;
	UINT32 dwMaxTagNum = OSA_DFT_MAXTAG_NUM;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	if( NULL == ptMemLib )
	{
		return COMERR_NULL_POINTER;
	}
	memset( ptMemLib, 0, sizeof(TOsaMemLib) );

	//��ʱ��֧�ֶ���
	ptMemLib->m_dwMaxTagNum = dwMaxTagNum;
	ptMemLib->m_ptMemRsrcTagList = (TRsrcTagInfo *)malloc(sizeof(TRsrcTagInfo) * dwMaxTagNum );
	if( NULL == ptMemLib )
	{
		return COMERR_NULL_POINTER;
	}
	memset(ptMemLib->m_ptMemRsrcTagList, 0, sizeof(TRsrcTagInfo) * dwMaxTagNum );

	//���ڱ����ڴ���Դ��ǩ�б����
	dwRet = OsaLightLockCreate( &ptMemLib->m_hMemRsrcTagListLock);
	if( dwRet != 0 )
	{
		iMemLibExit( );
		return dwRet;
	}

	//�����ڴ��������
	for(dwIdx = 0; dwIdx < MAX_MEM_ALLOC_NUM; dwIdx++)
	{
		dwRet = OsaLightLockCreate( &ptMemLib->m_ahMemAllocLock[dwIdx] );
		if( dwRet != 0 )
		{
			break;
		}
	}

	if( dwRet != OSA_OK )
	{
		iMemLibExit( );
	}
	return dwRet;
}

//�˳��ڴ��
void iMemLibExit( )
{
	UINT32 dwIdx = 0;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	if( NULL == ptMemLib )
	{
		return;
	}

	//ɾ���ڴ��������
	for(dwIdx = 0; dwIdx < MAX_MEM_ALLOC_NUM; dwIdx++)
	{
		if( ptMemLib->m_ahMemAllocLock[dwIdx] != NULL )
		{
			OsaLightLockDelete( ptMemLib->m_ahMemAllocLock[dwIdx] );
		}
	}

	//ɾ�����ڱ����ڴ���Դ��ǩ�б����
	if( ptMemLib->m_hMemRsrcTagListLock != NULL )
	{
		OsaLightLockDelete( ptMemLib->m_hMemRsrcTagListLock );
	}

	free ( ptMemLib->m_ptMemRsrcTagList );
	memset( ptMemLib, 0, sizeof(TOsaMemLib) );
}


//��ȡ����ڴ���Դ����
UINT32 iMaxMemTagNumGet( )
{
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	if( ptMemLib != NULL )
	{
		return ptMemLib->m_dwMaxTagNum;
	}
	return 0;
}

static BOOL sSerachRsrcTag( const char *pchName, UINT32 dwNameLen, UINT32 dwCurAllocTagNum, OUT UINT32 *pdwRsrcTag )
{
	UINT32 dwIdx = 0;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	for(dwIdx = 0; dwIdx < dwCurAllocTagNum; dwIdx++)
	{
		if(strncmp(ptMemLib->m_ptMemRsrcTagList[dwIdx].m_achTagName, pchName, dwNameLen) == 0)
		{
			*pdwRsrcTag = ptMemLib->m_ptMemRsrcTagList[dwIdx].m_dwTag;
			return TRUE;
		}
	}
	return FALSE;
}

//��鶨���ڴ�
static BOOL sCustomMemCheck( UINT32 *pdwCustomMem, IN UINT32 dwArrayNum)
{
	UINT32 i = 0;
	UINT32 j = 0;

	if((pdwCustomMem == NULL) || (dwArrayNum == 0))
	{
		return TRUE;
	}

	for(i = 0; i < dwArrayNum-1; i++)
	{
		for(j = i+1; j < dwArrayNum; j++)
		{
			if(pdwCustomMem[i] == pdwCustomMem[j])
			{
				iOsaLog(OSA_ERROCC, "CustomMemCheck(), idx %d and %d, have same size %d\n", i, j, pdwCustomMem[i]);
				return FALSE;
			}
		}
	}

	return TRUE;
}


static BOOL sMemAllocatorInit( MemAllocator *pAlloc, UINT32 dwMaxTagNum )
{
	UINT32 dwBytes = sizeof(UINT64) * (dwMaxTagNum);

	//�ڴ�����
	pAlloc->pqwTagUserMem = (UINT64 *)MallocByStatis(NULL, dwBytes );
	if(NULL == pAlloc->pqwTagUserMem) 
	{
		iOsaLog(OSA_ERROCC, "sMemAllocatorInit(), pAlloc->pqwTagUserMem, malloc %d bytes failed\n", dwBytes);
		return FALSE;
	}
	memset(pAlloc->pqwTagUserMem, 0, dwBytes );	

	//Malloc ����
	pAlloc->pqwTagMallocNum = (UINT64 *)MallocByStatis(NULL, dwBytes);
	if(NULL == pAlloc->pqwTagMallocNum) 
	{
		iOsaLog(OSA_ERROCC, "sMemAllocatorInit(), pAlloc->pqwTagMallocNum, malloc %d bytes failed\n", dwBytes);
		return FALSE;
	}
	memset(pAlloc->pqwTagMallocNum, 0, dwBytes);	


	//PoolMalloc ����
	pAlloc->pqwTagPoolMallocNum = (UINT64 *)MallocByStatis(NULL, dwBytes);
	if(NULL == pAlloc->pqwTagPoolMallocNum) 
	{
		iOsaLog(OSA_ERROCC, "sMemAllocatorInit(), pAlloc->pqwTagPoolMallocNum, malloc %d bytes failed\n", dwBytes);
		return FALSE;
	}
	memset(pAlloc->pqwTagPoolMallocNum, 0, dwBytes);	


// 	//PoolMemAlign ����
// 	pAlloc->pqwTagPoolMemalignNum = (UINT64 *)MallocByStatis(NULL, dwBytes);
// 	if(NULL == pAlloc->pdwTagPoolMemalignNum) 
// 	{
// 		iOsaLog(OSA_ERROCC, "sMemAllocatorInit(), pAlloc->pdwTagPoolMemalignNum, malloc %d bytes failed\n", dwBytes);
// 		return FALSE;
// 	}
// 	memset(pAlloc->pdwTagPoolMemalignNum, 0, dwBytes);	
// 
// 
	//MemAlign ����
	pAlloc->pqwTagMemAlignNum = (UINT64 *)MallocByStatis(NULL, dwBytes);
	if(NULL == pAlloc->pqwTagMemAlignNum) 
	{
		iOsaLog(OSA_ERROCC, "sMemAllocatorInit(), pAlloc->pqwTagMemAlignNum, malloc %d bytes failed\n", dwBytes);
		return FALSE;
	}
	memset(pAlloc->pqwTagMemAlignNum, 0, dwBytes);	

// 
// 	//PoolRealloc ����
// 	pAlloc->pqwTagReallocNum = (UINT64 *)MallocByStatis(NULL, dwBytes);
// 	if(NULL == pAlloc->pdwTagReallocNum) 
// 	{
// 		iOsaLog(OSA_ERROCC, "sMemAllocatorInit(), pAlloc->pdwTagReallocNum, malloc %d bytes failed\n", dwBytes);
// 		return FALSE;
// 	}
// 	memset(pAlloc->pdwTagReallocNum, 0, dwBytes);	


	//MFree ����
	pAlloc->pqwTagFreeNum = (UINT64 *)MallocByStatis(NULL, dwBytes);
	if(NULL == pAlloc->pqwTagFreeNum) 
	{
		iOsaLog(OSA_ERROCC, "sMemAllocatorInit(), pAlloc->pqwTagFreeNum, malloc %d bytes failed\n", dwBytes);
		return FALSE;
	}
	memset(pAlloc->pqwTagFreeNum, 0, dwBytes);
	return TRUE;
}

static void sMemAllocatorExit(MemAllocator *pAlloc, UINT32 dwMaxTagNum )
{
	UINT32 dwBytes = sizeof(UINT64) * (dwMaxTagNum);

	FreeByStatis(NULL, pAlloc->pqwTagFreeNum, dwBytes);
	pAlloc->pqwTagFreeNum = NULL;

// 	FreeByStatis(NULL, pAlloc->pdwTagReallocNum, dwBytes);
// 	pAlloc->pdwTagReallocNum = NULL;
// 
	FreeByStatis(NULL, pAlloc->pqwTagMemAlignNum, dwBytes);
	pAlloc->pqwTagMemAlignNum = NULL;
// 
// 	FreeByStatis(NULL, pAlloc->pdwTagPoolMemalignNum, dwBytes);
// 	pAlloc->pdwTagPoolMemalignNum = NULL;

	FreeByStatis(NULL, pAlloc->pqwTagPoolMallocNum, dwBytes);
	pAlloc->pqwTagPoolMallocNum = NULL;

	FreeByStatis(NULL, pAlloc->pqwTagMallocNum, dwBytes);
	pAlloc->pqwTagMallocNum = NULL;

	//�ڴ�����
	FreeByStatis(NULL, pAlloc->pqwTagUserMem, dwBytes);
	pAlloc->pqwTagUserMem = NULL;
	return;
}

//���ڴ������
void sOsaMemAllocLock( UINT32 dwMemAllocIdx)
{	
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	if( NULL == ptMemLib )
	{
		return;
	}

	if(dwMemAllocIdx >= MAX_MEM_ALLOC_NUM)
	{
		iOsaLog(OSA_ERROCC, "sOsaMemAllocLock, dwMemAllocIdx(%d) >= MAX_MEM_ALLOC_NUM(%d)\n",
			dwMemAllocIdx, MAX_MEM_ALLOC_NUM);
		return;
	}

	OsaLightLockLock( ptMemLib->m_ahMemAllocLock[dwMemAllocIdx] );
}

//�����ڴ������
void sOsaMemAllocUnLock( UINT32 dwMemAllocIdx)
{	
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	if( NULL == ptMemLib )
	{
		return;
	}

	if(dwMemAllocIdx >= MAX_MEM_ALLOC_NUM)
	{
		iOsaLog(OSA_ERROCC, "sOsaMemAllocUnLock, dwMemAllocIdx(%d) >= MAX_MEM_ALLOC_NUM(%d)\n",
			dwMemAllocIdx, MAX_MEM_ALLOC_NUM);
		return;
	}

	OsaLightLockUnLock( ptMemLib->m_ahMemAllocLock[dwMemAllocIdx] );
}

static void sZeroMemHead( MemAllocHeader *pMemHead )
{
	if(pMemHead == NULL)
	{
		return;
	}

	pMemHead->dwMagicNum = 0;
	pMemHead->dwSize = 0;
	pMemHead->dwTag = 0;
	pMemHead->sdwLine = 0;
	pMemHead->sdwRef = 0;

	memset(pMemHead->sFileName, 0, sizeof(pMemHead->sFileName));
	memset(&(pMemHead->tNode), 0, sizeof(ENode)); 
}

static BOOL sMemValidate(void *pMem)
{
	BOOL bValidate=FALSE;
	MemAllocHeader *pMemHeader = NULL;
	MemAllocTail *pMemTail = NULL;

	if (NULL == pMem)
	{
		return FALSE;
	}

	pMemHeader = MEMHEADER_ADDRESS(pMem);
	pMemTail = (MemAllocTail*)((UINT8*)pMem + pMemHeader->dwAlignSize);

	if( (pMemHeader->dwMagicNum == _MEMALLOC_MAGIC_NUME) &&
		( pMemTail->dwMagicNum == _MEMALLOC_MAGIC_NUME ) )
	{
		bValidate = TRUE;
	}
	else
	{
		iOsaLog(OSA_ERROCC, "sMemValidate() failed, addr 0x%x, headmagic=0x%x, tailmagic=0x%x\n",
			pMem, pMemHeader->dwMagicNum, pMemTail->dwMagicNum);

		bValidate = FALSE;
	}
	return bValidate;
}


/*===========================================================
���ܣ� ������Դ��ǩ��Ŀǰ�����ڴ�ӿ��У����������ź������̵߳���Դ
�������˵����  char *pchName - ��ǩ��
				UINT32 dwNameLen - ��ǩ�����ȣ���������β��'\0', ���Ϊ8
				UINT32 *pdwRsrcTag - ���ر�ǩ
����ֵ˵���� �ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMRsrcTagAlloc( const char *pchName, UINT32 dwNameLen, OUT UINT32 *pdwRsrcTag )
{
	UINT32 dwRet = 0;
	UINT32 dwTag = 0;
	BOOL bTagNameExist = FALSE;
	TOsaMemLib* ptMemLib = iOsaMemLibGet();
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );
	UINT32 dwCurAllocTagNum = 0;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pchName, "pchName" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pdwRsrcTag, "pdwRsrcTag" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptMemLib, "ptMemLib" );

	if((dwNameLen == 0) || (dwNameLen > RSRCTAGNAME_MAXLEN) )
	{
		iOsaLog(OSA_ERROCC, "OsaMRsrcTagAlloc(), para err, dwNameLen=%d \n", dwNameLen );
		return COMERR_INVALID_PARAM;
	}

	OsaLightLockLock(ptMemLib->m_hMemRsrcTagListLock);
	do 
	{
		dwCurAllocTagNum = ptMemLib->m_dwAllocTagNum;
		//�ж��Ƿ����
		if(dwCurAllocTagNum >= dwMaxTagNum )
		{		
			dwRet = COMERR_ABILITY_LIMIT;
			iOsaLog(OSA_ERROCC, "OsaMRsrcTagAlloc(), tag reach maxno %d\n", dwMaxTagNum); 
			break;
		}

		//������еı�ǩ����ͬ���������еı�ǩ
		bTagNameExist = sSerachRsrcTag( pchName, dwNameLen, dwCurAllocTagNum, pdwRsrcTag );
// 		if( TRUE == bTagNameExist )
// 		{
// 			break;
// 		}
	} while (0);

	if( 0 == dwRet && FALSE == bTagNameExist )
	{
		dwTag = dwCurAllocTagNum;
		dwCurAllocTagNum++;
		ptMemLib->m_dwAllocTagNum = dwCurAllocTagNum;
		strncpy(ptMemLib->m_ptMemRsrcTagList[dwTag].m_achTagName, pchName, RSRCTAGNAME_MAXLEN);
		ptMemLib->m_ptMemRsrcTagList[dwTag].m_achTagName[RSRCTAGNAME_MAXLEN] = '\0';
		ptMemLib->m_ptMemRsrcTagList[dwTag].m_dwTag = dwTag;
		*pdwRsrcTag = dwTag;
	}

	OsaLightLockUnLock(ptMemLib->m_hMemRsrcTagListLock);
	return dwRet;
}


/*===========================================================
���ܣ� �����ڴ������������һ��ʵ������󴴽�����MAX_MEM_ALLOC_NUM��
����˵����  pdwCustomMem - ���붨���ڴ���Ϣ(����NULL��ʾ������), �ڴ��С���ޣ������е��ڴ��С������ͬ����λΪ byte
			dwArrayNum �� ����Ԫ�ظ������������ڴ���dwArrayNum����ͬ�ߴ磬����0��ʾ�ڴ治����
			phMemAlloc - �ڴ�������ľ�� 
����ֵ˵���� �ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMAllocCreate(UINT32 *pdwCustomMem, UINT32 dwArrayNum, OUT HANDLE *phMemAlloc )
{
	UINT32 dwRet = 0;
	BOOL bSuc;
	MemAllocator *pAlloc=NULL;
	UINT32 dwCurMemAllocNum = 0;
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );
	TOsaMemLib* ptMemLib = iOsaMemLibGet();

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( phMemAlloc, "phMemAlloc" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( ptMemLib, "ptMemLib" );

	dwCurMemAllocNum = ptMemLib->m_dwCurMemAllocNum;
	do 
	{
		if(dwCurMemAllocNum >= MAX_MEM_ALLOC_NUM)
		{
			dwRet = COMERR_INST_LIMIT;
			iOsaLog(OSA_ERROCC, "OsaMAllocCreate(), dwCurMemAllocNum(%d) >= MAX_MEM_ALLOC_NUM(%d)\n", dwCurMemAllocNum, MAX_MEM_ALLOC_NUM);
			break;
		}

		//��鶨���ڴ�
		bSuc = sCustomMemCheck(pdwCustomMem, dwArrayNum);
		if(bSuc == FALSE)
		{
			dwRet = COMERR_INVALID_PARAM;
			iOsaLog(OSA_ERROCC, "OsaMAllocCreate(), CustomMemCheck() failed\n");
			break;
		}

		//����
		pAlloc = (MemAllocator *)MallocByStatis(NULL, sizeof(MemAllocator));
		if(NULL == pAlloc) 
		{
			dwRet = COMERR_MEMERR;
			iOsaLog(OSA_ERROCC, "OsaMAllocCreate(), pAlloc is 0x%x\n", pAlloc);
			break;
		}
		memset(pAlloc,0,sizeof(*pAlloc));
		
		//��ʼ���ڴ�������ṹ����̬�����ǩ��ӡ����ڴ�
		bSuc = sMemAllocatorInit(pAlloc, dwMaxTagNum );
		if(FALSE == bSuc) 
		{
			dwRet = COMERR_MEMERR;
			iOsaLog(OSA_ERROCC, "OsaMAllocCreate(), MallocFreeCountInit failed\n");			
			break;
		}

		EListInit(&pAlloc->tAllocInfoList);

		pAlloc->hPool = MemPoolCreate(pdwCustomMem, dwArrayNum, pAlloc);
		if (NULL == pAlloc->hPool)
		{
			dwRet = COMERR_MEMERR;
			iOsaLog(OSA_ERROCC, "OsaMAllocCreate(), MemPoolCreate failed\n");			
			break;
		}
	} while (0);
	
	if( dwRet != 0 )
	{
		OsaMAllocDestroy( pAlloc );
	}
	else
	{
		UINT32 dwIndx = 0;
		dwCurMemAllocNum++;
		for(dwIndx = 0; dwIndx < MAX_MEM_ALLOC_NUM; dwIndx++)
		{
			if(ptMemLib->m_ahMemAlloc[dwIndx] == 0)
			{
				ptMemLib->m_ahMemAlloc[dwIndx] = (HANDLE)pAlloc;
				break;
			}
		}
		pAlloc->dwMemAllocIdx = dwIndx;
		ptMemLib->m_dwCurMemAllocNum = dwCurMemAllocNum;
		*phMemAlloc = (HANDLE)pAlloc;
	}

	return dwRet;
}

/*===========================================================
���ܣ� �ͷ��ڴ������
����˵����  hMemAlloc �ڴ�������ľ��            
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMAllocDestroy(HANDLE hMemAlloc)
{
	UINT32 dwRet = 0;
	MemAllocator *pAlloc=(MemAllocator *)hMemAlloc;
	UINT32 dwMemAllocIdx = 0;

//	UINT32 dwCurMemAllocNum = 0;
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );
	TOsaMemLib* ptMemLib = iOsaMemLibGet();

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pAlloc, "pAlloc" );

	dwMemAllocIdx = pAlloc->dwMemAllocIdx;
	if(dwMemAllocIdx >= MAX_MEM_ALLOC_NUM)
	{
		iOsaLog(OSA_ERROCC, "OsaMAllocDestroy(), dwMemAllocIdx(%d) >= MAX_MEM_ALLOC_NUM(%d)\n", dwMemAllocIdx, MAX_MEM_ALLOC_NUM);
		return COMERR_MEMERR;
	}

	/*���й¶��Ϣ*/
	//OalMemInfoShow(hAlloc);

	/*�ͷ��ڴ��*/
	if (NULL != pAlloc->hPool)
	{
		BOOL bDestory = MemPoolDestroy(pAlloc->hPool);
		if( !bDestory )
		{
			dwRet = COMERR_MEMERR;
			iOsaLog(OSA_ERROCC, "OsaMAllocDestroy(), MemPoolDestroy return FALSE\n" );
		}
	}	

	//���սṹ�ڲ���̬������ڴ�
	sMemAllocatorExit(pAlloc, dwMaxTagNum );

	//�ڴ����
	FreeByStatis(NULL, pAlloc, 0);
	pAlloc = NULL;

	ptMemLib->m_ahMemAlloc[dwMemAllocIdx] = 0;
	ptMemLib->m_dwCurMemAllocNum--;

	return dwRet;
}

/*===========================================================
���ܣ� ��ȡOsa�ڲ����ڴ������
����˵����  ��         
����ֵ˵�����ɹ������ڴ�������ľ����ʧ�ܷ���NULL
===========================================================*/
HANDLE OsaMemAllocGet()
{
	return iOsaMemAllocGet();
}


//�ͷ��ڴ棬���Ӹ���������ɾ��
static BOOL sOsaMFreeNotDiscTrackList( HANDLE hAlloc,IN void *pMem)
{
	MemAllocator *pAlloc=(MemAllocator *)hAlloc;
	BOOL bFree = FALSE;
	MemAllocHeader *pMemHeader; //MEMHEADER_ADDRESS(pMem);
	void *pFreeMem = pMem;
	UINT32 dwBytes = 0;
	UINT32 dwTag = 0;
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );

	if( (NULL == pAlloc) || (NULL == pMem) )
	{
		iOsaLog(OSA_ERROCC, "sOsaMFreeNotDiscTrackList(), pAlloc=0x%x, pMem=0x%x\n", pAlloc, pMem);
		return FALSE;			
	}


	pMemHeader = MEMHEADER_ADDRESS(pMem);
	pFreeMem = pMem;

	dwBytes = pMemHeader->dwSize;
	dwTag = pMemHeader->dwTag;

	if(/*(dwTag == 0) || */(dwTag >= dwMaxTagNum))
	{
		iOsaLog(OSA_ERROCC, "sOsaMFreeNotDiscTrackList(), dwTag=%d, g_dwMaxTagNum=%d\n", dwTag, dwMaxTagNum );
		return FALSE;
	}

	bFree = MemPoolFree(pAlloc->hPool,pFreeMem);

	if(bFree == FALSE)
	{
		return FALSE;		
	}

	pFreeMem = NULL; 

	//ͳ���û�ʹ�õ��ڴ�����
	if(pAlloc->qwTotalTagUserMem >= dwBytes)
	{
		pAlloc->qwTotalTagUserMem -= dwBytes;
	}
	else
	{
		iOsaLog(OSA_ERROCC, "!!!!!! sOsaMFreeNotDiscTrackList(), usermem=%d, freeBytes=%d\n",
			pAlloc->qwTotalTagUserMem, dwBytes);
	}

	if(pAlloc->pqwTagUserMem[dwTag] >= dwBytes)
	{
		pAlloc->pqwTagUserMem[dwTag] -= dwBytes;
	}
	else
	{
		iOsaLog(OSA_ERROCC, "!!!!!! sOsaMFreeNotDiscTrackList(), tag=%d, usermem=%d, freeBytes=%d\n", dwTag, pAlloc->pqwTagUserMem[dwTag], dwBytes);
	}	

	pAlloc->pqwTagFreeNum[dwTag]++;

	return TRUE;
}

/*===========================================================
���ܣ� �����ڴ棬����������Ϣ����������(�ڲ��������ⲿ����CtlMAlloc)
����˵����  hMemAlloc �ڴ�������ľ��
            dwBytes ������Ĵ�С
			dwTag ��ǩ����Osa��̬���� 
			bPool - �Ƿ񾭹��ڴ��
					FALSE: ֱͨ����ϵͳ���ڴ���䣬���ͷţ����䶼���ɲ���ϵͳ����ѡ���ʺϴ������ڴ�ķ��䣬�Ҹ��ڴ�����Ͳ��ͷš�
					TRUE:  �����ͷŶ�ͨ���ڴ�أ�����ϵͳ���õĴΣ���ѡ���ʺ�С�����ڴ棨�����ڴ棩�ķ������䣬�ͷš�
����ֵ˵���� ������ڴ��ָ��
˵����   �û���Ҫֱ�ӵ��ú��� ������������������
===========================================================*/
void* InnerOsaMAlloc( HANDLE hMemAlloc, UINT32 dwBytes, UINT32  dwTag, BOOL bPool, const char *szFile, UINT32 dwLine)
{
	UINT32 dwRet=0;	
	MemAllocator *pAlloc=(MemAllocator *)hMemAlloc;
	MemAllocHeader *pMemHeader=NULL;
	UINT32 dwAlignSize=0;
//	ENode *ptNode = NULL;
	void *pUserMem = NULL;
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );

	//�������
	OSA_CHECK_NULL_POINTER_RETURN_POINT( pAlloc, "pAlloc" );
	OSA_CHECK_NULL_POINTER_RETURN_POINT( szFile, "szFile" );
	
	//��ʼ�����
	OSA_CHECK_INIT_RETURN_POINT();

	if((dwBytes == 0) /*|| (dwTag == 0)*/ || (dwTag >= dwMaxTagNum))
	{
		iOsaLog(OSA_ERROCC, "InnerOsaMAlloc(), bytes=%d, tag=%d, dwMaxTagNum=%d, szFile=%s, line=%d\n", 
			dwBytes, dwTag, dwMaxTagNum, szFile, dwLine );
		return NULL;
	}

	{
		//������Ϣ����Telnet��
		TelnetHandle hTelHdl = OsaTelnetHdlGet();
		if( hTelHdl != NULL )
		{
			//���Դ�ӡ
			iOsaLog(OSA_MSGOCC, "InnerOsaMAlloc(), bytes=%d, tag=%d, szFile=%s, line=%d\n", 
				dwBytes, dwTag, szFile, dwLine );
		}
	}

	sOsaMemAllocLock(pAlloc->dwMemAllocIdx);

	do 
	{
		dwAlignSize = SIZE_ALIGN(dwBytes, ALIGN_BYTES);	
		if(bPool && dwAlignSize <= MAXBLOCK_MEM)
		{
			//С�ڵ���8M�����ڴ�ط���
			pUserMem = MemPoolAlloc(pAlloc->hPool, dwAlignSize);			
		}
		else
		{
			pUserMem = MemOsAlloc(pAlloc->hPool, dwAlignSize);	
		}

		if(NULL == pUserMem)
		{	
			iOsaLog(OSA_ERROCC, "InnerOsaMAlloc failed, bPool=%d, bytes=%d, tag=%d, file=%s, line=%d\n", 
				bPool, dwBytes, dwTag,szFile, dwLine );	
			break;
		}

		pMemHeader = MEMHEADER_ADDRESS(pUserMem); //(MemAllocHeader *)(pUserMem - sizeof(MemAllocHeader));

		sZeroMemHead(pMemHeader);
		
		//��ʼ���ڴ�ͷ����Ϣ
		sprintf(pMemHeader->sFileName,iOsaReverseTrim(szFile,_PART_FNAME));
		pMemHeader->sFileName[_PART_FNAME] = '\0';

		pMemHeader->sdwLine=dwLine;
		pMemHeader->dwSize=dwBytes;
		pMemHeader->dwAlignSize = dwAlignSize;
		pMemHeader->sdwRef=1;
		pMemHeader->dwMagicNum=_MEMALLOC_MAGIC_NUME;
		pMemHeader->dwTag = dwTag;

		((MemAllocTail*)((UINT8*)pUserMem + dwAlignSize))->dwMagicNum= _MEMALLOC_MAGIC_NUME;


		/*�������з�����Ϣ���ڴ����������*/
		dwRet=EListInsertLast(&pAlloc->tAllocInfoList, ALLOCNODE_ENTRY(pMemHeader));
		if(dwRet != 0)
		{
			iOsaLog(OSA_ERROCC, "InnerOsaMAlloc(), EListInsertLast() failed, file=%s, line=%d !!!\n", szFile, dwLine );
			sOsaMFreeNotDiscTrackList(hMemAlloc, pUserMem);
			break;
		}	


		//ͳ��
		pAlloc->qwTotalTagUserMem += dwBytes;
		pAlloc->pqwTagUserMem[dwTag] += dwBytes;

		if(bPool)
		{
			pAlloc->pqwTagPoolMallocNum[dwTag]++;
			pAlloc->qwMemViaPoolMalloc += dwBytes;
		}
		else
		{
			pAlloc->pqwTagMallocNum[dwTag]++;
			pAlloc->qwMemViaMalloc += dwBytes;
		}
	} while (0);

	sOsaMemAllocUnLock( pAlloc->dwMemAllocIdx );		
	return pUserMem;
}

void* InnerOsaMAlignAlloc( HANDLE hMemAlloc, UINT32 dwAlign, UINT32 dwBytes, UINT32 dwTag, const char *szFile, UINT32 dwLine)
{
	UINT32 dwRet=0;	
	MemAllocator *pAlloc=(MemAllocator *)hMemAlloc;
	MemAllocHeader *pMemHeader=NULL;
	UINT32 dwAlignSize=0;
	//	ENode *ptNode = NULL;
	void *pUserMem = NULL;
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );

	//�������
	OSA_CHECK_NULL_POINTER_RETURN_POINT( pAlloc, "pAlloc" );
	OSA_CHECK_NULL_POINTER_RETURN_POINT( szFile, "szFile" );

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_POINT();

	if((dwBytes == 0) /*|| (dwTag == 0)*/ || (dwTag >= dwMaxTagNum))
	{
		iOsaLog(OSA_ERROCC, "%s(), bytes=%d, tag=%d, dwMaxTagNum=%d, szFile=%s, line=%d\n", 
			__FUNCTION__, dwBytes, dwTag, dwMaxTagNum, szFile, dwLine );
		return NULL;
	}

	{
		//������Ϣ����Telnet��
		TelnetHandle hTelHdl = OsaTelnetHdlGet();
		if( hTelHdl != NULL )
		{
			//���Դ�ӡ
			iOsaLog(OSA_MSGOCC, "%s(), bytes=%d, tag=%d, szFile=%s, line=%d\n", 
				__FUNCTION__, dwBytes, dwTag, szFile, dwLine );
		}
	}

	sOsaMemAllocLock(pAlloc->dwMemAllocIdx);

	do 
	{
		dwAlignSize = SIZE_ALIGN(dwBytes, ALIGN_BYTES);	
		pUserMem = OsMemAlign(pAlloc->hPool, dwAlign, dwAlignSize);	

		if(NULL == pUserMem)
		{	
			iOsaLog(OSA_ERROCC, "%s() failed, bytes=%d, tag=%d, file=%s, line=%d\n", 
				 __FUNCTION__, dwBytes, dwTag, szFile, dwLine );	
			break;
		}

		pMemHeader = MEMHEADER_ADDRESS(pUserMem); //(MemAllocHeader *)(pUserMem - sizeof(MemAllocHeader));

		sZeroMemHead(pMemHeader);

		//��ʼ���ڴ�ͷ����Ϣ
		sprintf(pMemHeader->sFileName,iOsaReverseTrim(szFile,_PART_FNAME));
		pMemHeader->sFileName[_PART_FNAME] = '\0';

		pMemHeader->sdwLine=dwLine;
		pMemHeader->dwSize=dwBytes;
		pMemHeader->dwAlignSize = dwAlignSize;
		pMemHeader->sdwRef=1;
		pMemHeader->dwMagicNum=_MEMALLOC_MAGIC_NUME;
		pMemHeader->dwTag = dwTag;

		((MemAllocTail*)((UINT8*)pUserMem + dwAlignSize))->dwMagicNum= _MEMALLOC_MAGIC_NUME;


		/*�������з�����Ϣ���ڴ����������*/
		dwRet=EListInsertLast(&pAlloc->tAllocInfoList, ALLOCNODE_ENTRY(pMemHeader));
		if(dwRet != 0)
		{
			iOsaLog(OSA_ERROCC, "InnerOsaMAlloc(), EListInsertLast() failed, file=%s, line=%d !!!\n", szFile, dwLine );
			sOsaMFreeNotDiscTrackList(hMemAlloc, pUserMem);
			break;
		}	


		//ͳ��
		pAlloc->qwTotalTagUserMem += dwBytes;
		pAlloc->pqwTagUserMem[dwTag] += dwBytes;
		pAlloc->pqwTagMemAlignNum[dwTag]++;
		pAlloc->qwMemViaMemAlign += dwBytes;
		
	} while (0);

	sOsaMemAllocUnLock( pAlloc->dwMemAllocIdx );		
	return pUserMem;
}

/*===========================================================
���ܣ� �ͷ��ڴ棬�����ͷŵ��ڴ��������ɾ��
����˵���� hMemAlloc �ڴ�������ľ�� 
           pMem ���ͷŵ��ڴ���ָ��
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMFree(HANDLE hMemAlloc, void *pMem)
{
	UINT32 dwRet = 0;
	BOOL bFree=FALSE;
	MemAllocator *pAlloc=(MemAllocator *)hMemAlloc;
	MemAllocHeader *pMemHeader=NULL;
	ENode *pNode=NULL;
	UINT32 dwBytes = 0;
	BOOL bSuc=FALSE;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pAlloc, "pAlloc" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pMem, "pMem" );

	sOsaMemAllocLock(pAlloc->dwMemAllocIdx);

	do 
	{
		/*�����ڴ���Ƿ���Ч*/
		bSuc = sMemValidate(pMem);
		if(bSuc == FALSE)
		{
			dwRet = COMERR_MEMERR;
			iOsaLog(OSA_ERROCC, "OsaMFree(), sMemValidate() failed\n");
			break;
		}

		pMemHeader = MEMHEADER_ADDRESS(pMem);


		{
			//������Ϣ����Telnet��
			TelnetHandle hTelHdl = OsaTelnetHdlGet();
			if( hTelHdl != NULL )
			{
				//���Դ�ӡ
				iOsaLog(OSA_MSGOCC, "OsaMFree(), type %d, bytes=%d, tag=%d, malloc in %s, line=%d\n", 
					pMemHeader->dwBlockType, 
					pMemHeader->dwSize,
					pMemHeader->dwTag, 
					pMemHeader->sFileName, 
					pMemHeader->sdwLine);
			}
		}

		dwBytes = pMemHeader->dwSize;
		if(pMemHeader->sdwRef <= 0)
		{
			dwRet = COMERR_MEMERR;
			iOsaLog(OSA_ERROCC, "OsaMFree(), ref(%d) <= 0, file=%s, line=%d !!!\n", pMemHeader->sdwRef, pMemHeader->sFileName, pMemHeader->sdwLine);
			break;
		}

		pMemHeader->sdwRef--;

		if (pMemHeader->sdwRef > 0) //��ʱ����֧��
		{
			break;		
		}

		/*�Ӽ��ط�����Ϣ��������ɾ��*/
		pNode=ALLOCNODE_ENTRY(pMemHeader);
		dwRet=EListRemove(&pAlloc->tAllocInfoList,pNode);
		if(dwRet != 0)
		{		
			dwRet = COMERR_NOT_FOUND;
			iOsaLog(OSA_ERROCC, "OsaMFree(), EListRemove() failed, file=%s, line=%d !!!\n", pMemHeader->sFileName, pMemHeader->sdwLine);
			break;	
		}

		if(pMemHeader->dwBlockType == BLOCKTYPE_DIRECT)
		{
			pAlloc->qwMemViaMalloc -= dwBytes;
		}
		else if(pMemHeader->dwBlockType == BLOCKTYPE_EXPONENT)
		{
			pAlloc->qwMemViaPoolMalloc -= dwBytes;
		}
// 		else if(pMemHeader->dwBlockType == BLOCKTYPE_POOLALIGN)
// 		{
// 			pAlloc->m_dwMemViaMemAlign -= dwBytes;
// 		}

		bFree = sOsaMFreeNotDiscTrackList(hMemAlloc, pMem);	
		if(bFree == FALSE)
		{		
			dwRet = COMERR_NOT_FOUND;
			break;	
		}

	} while (0);

	sOsaMemAllocUnLock( pAlloc->dwMemAllocIdx );
	return dwRet;
}
/*===========================================================
���ܣ� �õ�ָ��ָ���ڴ����Ĵ�С
����˵���� 
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMSizeGet(void *pMem, OUT UINT32  *pdwSize)
{
	UINT32 dwRet = 0;
	MemAllocHeader *pMemHeader=NULL;
	BOOL bSuc = FALSE;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pMem, "pMem" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pdwSize, "pdwSize" );

	/*�����ڴ���Ƿ���Ч*/
	bSuc = sMemValidate(pMem);
	if(bSuc == FALSE)
	{
		iOsaLog(OSA_ERROCC, "OsaMSizeGet(), sMemValidate() failed\n");
		return COMERR_MEMERR;
	}

	pMemHeader = MEMHEADER_ADDRESS(pMem);
	*pdwSize = pMemHeader->dwSize;
	return dwRet;
}


/*===========================================================
���ܣ� �õ�ָ����ǩ���û��ڴ�����
����˵���� hMemAlloc �� �ڴ�������ľ��  
           dwTag ��ǩ �����Ϊ0���õ����б�ǩ���û��ڴ�����
		   pdwSize �� �û��ڴ�����
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMUserMemGet(HANDLE hMemAlloc, UINT32 dwTag, OUT UINT64 *pqwSize )
{
	UINT32 dwRet = 0;
	MemAllocator *pAlloc = (MemAllocator *)hMemAlloc;
	UINT32 dwMaxTagNum = iMaxMemTagNumGet( );

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pAlloc, "pAlloc" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pqwSize, "pqwSize" );

	if( dwTag > dwMaxTagNum)
	{
		iOsaLog(OSA_ERROCC, "OsaMUserMemGet(), tag=%d error, dwMaxTagNum=%d\n", dwTag, dwMaxTagNum);
		return FALSE;
	}

	sOsaMemAllocLock(pAlloc->dwMemAllocIdx);

	*pqwSize = pAlloc->pqwTagUserMem[dwTag];

	sOsaMemAllocUnLock( pAlloc->dwMemAllocIdx );

	return dwRet;
}

/*===========================================================
���ܣ� �õ��ڴ�����������ϵͳ������ڴ�����
����˵���� hMemAlloc �ڴ�������ľ��  
           pdwSize �� ϵͳ�ڴ�����
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMSysMemGet(HANDLE hMemAlloc, OUT UINT64 *pqwSize )
{
	UINT32 dwRet = 0;
	MemAllocator *pAlloc = (MemAllocator *)hMemAlloc;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pAlloc, "pAlloc" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pqwSize, "pqwSize" );

	sOsaMemAllocLock(pAlloc->dwMemAllocIdx);

	*pqwSize = pAlloc->qwTotalSysMem;

	sOsaMemAllocUnLock( pAlloc->dwMemAllocIdx );
	return dwRet;
}

/*===========================================================
���ܣ� �õ��ڴ������
����˵���� hMemAlloc �ڴ�������ľ��  
           pMem �ڴ���ָ��
		   pdwType - �ڴ�����ͣ���BLOCKTYPE_CUSTOM
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaMTypeGet(HANDLE hMemAlloc, void *pMem, OUT UINT32 *pdwType)
{
	UINT32 dwRet = 0;
	BOOL bSuc = FALSE;
	MemAllocator *pAlloc = (MemAllocator *)hMemAlloc;
	MemAllocHeader *pMemHeader = NULL;

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pAlloc, "pAlloc" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pMem, "pAllocpMem" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pdwType, "pdwType" );

	sOsaMemAllocLock(pAlloc->dwMemAllocIdx);

	bSuc = sMemValidate(pMem);
	if(bSuc == FALSE)
	{
		dwRet = COMERR_MEMERR;
		iOsaLog(OSA_ERROCC, "OsaMTypeGet(), sMemValidate() failed\n");
		*pdwType = BLOCKTYPE_NONE;
	}
	else
	{
		pMemHeader = MEMHEADER_ADDRESS(pMem);
		*pdwType = pMemHeader->dwBlockType;
	}

	sOsaMemAllocUnLock( pAlloc->dwMemAllocIdx );
	return dwRet;
}


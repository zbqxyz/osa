#ifndef _MEMPOOL_H
#define _MEMPOOL_H

//�ڴ��
#define MEM_GRANULARITY_MIN 6 /*2��6�η�=64*/
#define MEM_GRANULARITY_MAX 23 /*2��23�η�=8M*/
#define MEM_GRANULARITY_LEVEL (MEM_GRANULARITY_MAX - MEM_GRANULARITY_MIN + 1) /*64,128,256,...,512K,1M��15��*/

#define MAXBLOCK_MEM (1<<MEM_GRANULARITY_MAX)


#define _PART_FNAME 32 //ȡ�����ļ����ĳ���
#define _MEMALLOC_MAGIC_NUME 0x2A2A2A2A //�ڴ��Ļ���(****)��������ڴ���ͷβ

#define CUSTOM_MINSIZE	(UINT32)(1<<20) //�����ڴ����Сֵ��������ڸ�ֵ

/*�ڴ�������������¼�ڴ�������Ϣ*/
typedef struct tagMemAllocator
{
	HANDLE hPool;//�ڴ�صľ��
	UINT32 dwMemAllocIdx; //�ڴ��Ƶ�����������������õ�
// 	UINT32 dwOptions;// ��ʼ��־λ
	EList tAllocInfoList;//������Ϣ����
	UINT64 qwTotalSysMem;	//ʵ���õ���ϵͳ�ڴ�, ������malloc������ֽ���
	UINT64 qwTotalTagUserMem; //���е��û��ڴ�
	UINT64 qwMemViaMalloc;
	UINT64 qwMemViaPoolMalloc;
 	UINT64 qwMemViaMemAlign;	
// 	UINT64 m_qwMemViaPoolMemAlign;

	UINT64 *pqwTagUserMem;	  //��ǩΪtag���ڴ�
	UINT64 *pqwTagMallocNum;	//��ǩΪtag���ڴ�, Malloc ����
	UINT64 *pqwTagPoolMallocNum;	//��ǩΪtag���ڴ�, PoolMalloc ����
// 	UINT64 *pqwTagPoolMemalignNum;	//��ǩΪtag���ڴ�, PoolMemAlign ����
 	UINT64 *pqwTagMemAlignNum;	//��ǩΪtag���ڴ�, MemAlign ����
// 	UINT64 *pqwTagReallocNum;	//��ǩΪtag���ڴ�, PoolRealloc ����
	UINT64 *pqwTagFreeNum;		//��ǩΪtag���ڴ�, MFree ����
}MemAllocator;


/*�ڴ���������Ϣ��λ����ʵ������ڴ��ǰ��*/

/*�ڴ��Ļ�����λ����ʵ������ڴ��β��*/
// typedef struct tagMemBlockTail
// { 
// 	UINT32 dwMagicNum;//�ڴ��Ļ���
// } MemBlockTail; 
// 

/*��¼�ڴ������Ϣ�Ľṹ��λ����ʵ������ڴ�֮ǰ*/
typedef struct tagMemAllocHeader
{ 
	ENode tNode;//�û������ڴ������ڵ�
	INT32 sdwRef;//���ü���
	INT8	sFileName[_PART_FNAME+1];	//�����������ڵ��ļ���
	INT32	sdwLine;//�����������ڵ���
	UINT32	dwSize;//�ڴ��С
	UINT32 dwAlignSize; //�����Ĵ�С
	UINT32 dwOsSize; //ʵ��malloc�Ĵ�С
	UINT32 dwTag;		//��ǩ�����ڶ���ͳ�Ƶ���ģ����ڴ�

	/*=== �ڴ����Ϣ ===*/
	ENode tBlockNode; //�ڴ�������ڵ�
	UINT32 dwBlockType;//�ڴ�������
	/*=== �ڴ����Ϣ ===*/

	/*=== align��Ϣ ===*/
	UINT32 dwAlign;	//�����ֽ�
	void* lSysAddr;	//����ϵͳ���ڴ���ʼ��ַ
	UINT32 dwMagicNum;//����

} MemAllocHeader; 

/*������ڴ���β����������֤�ڴ���һ����*/
typedef struct tagMemAllocTail
{
	UINT32 dwMagicNum;//�ڴ��Ļ���
}MemAllocTail;


#define  TAGINFO_MAXLEN	 20

typedef struct tagMemTagInfo
{
	UINT32 dwTag;
	INT8  achTagInfo[TAGINFO_MAXLEN];
}TMemTagInfo;		


/*��MemAllocHeaderָ��õ�����ڵ�ָ��*/
#define ALLOCNODE_ENTRY(pMemHeader) ( &(((MemAllocHeader *)pMemHeader)->tNode) )

/*��ALLOC�ڵ�ָ��õ�MemAllocHeaderָ��*/
#define	MEMHEADER_ENTRY_ALLOCNODE(pNode) HOST_ENTRY(pNode, MemAllocHeader, tNode)

/*��MemAllocHeaderָ��õ���ڵ�ָ��*/
#define BLOCKNODE_ENTRY(pMemHeader) ( &(((MemAllocHeader *)pMemHeader)->tBlockNode) )

/*�ɿ�ڵ�ָ��õ�MemAllocHeaderָ��*/
#define	MEMHEADER_ENTRY_BLOCKNODE(pBlockNode) HOST_ENTRY(pBlockNode, MemAllocHeader, tBlockNode)


/*��MemAllocHeaderָ��õ��û��ڴ�ĵ�ַ*/
#define	MEM_ADDRESS(pMemHeader)  (void*)((UINT8*)(pMemHeader)+sizeof(MemAllocHeader))

/*���û��ڴ��ַ��MemAllocHeader�ĵ�ַ*/
#define	MEMHEADER_ADDRESS(pMem)  (MemAllocHeader*)((UINT8*)(pMem)-sizeof(MemAllocHeader))

//�õ�ʵ�ʵ��ڴ��С
#define REALSIZE(pMemAlloc) (pMemAlloc->dwOsSize)
//( pMemAlloc->dwSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail) )

void* MallocByStatis(void *phAlloc, UINT32 dwBytes);
void FreeByStatis(void *phAlloc, void *p, UINT32 dwBytes );


/*===========================================================
���ܣ� �����ڴ��
�������˵����  
		pdwCustomMem - ���붨���ڴ���Ϣ, �������1M���������е��ڴ��С������ͬ����λΪ byte
		dwArrayNum �� ����Ԫ�ظ������������ڴ���dwArrayNum����ͬ�ߴ�
		phAlloc �� �ڴ���������            
����ֵ˵���� �ڴ�صľ����ʧ�ܷ���NULL
===========================================================*/
HANDLE MemPoolCreate(UINT32 *pdwCustomMem, UINT32 dwArrayNum, void *phAlloc);

/*===========================================================
���ܣ� �ͷ��ڴ��
�������˵����  hPool �ڴ�صľ��
����ֵ˵���� �ɹ�����TRUE������FALSE
===========================================================*/
BOOL MemPoolDestroy(HANDLE hPool);

/*===========================================================
���ܣ� ���ڴ�ط����ڴ�
�������˵����  hPool �ڴ�صľ��
				dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* MemPoolAlloc(HANDLE hPool,UINT32 dwBytes);

/*===========================================================
���ܣ� ֱ�Ӵ�os�����ڴ�
�������˵����  hPool �ڴ�صľ��
				dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* MemOsAlloc(HANDLE hPool, UINT32 dwBytes);

/*===========================================================
���ܣ� ֱ�Ӵ�os memalign
�������˵����  hPool �ڴ�صľ��
				dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* OsMemAlign(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes);

/*===========================================================
���ܣ� ��align�ڴ�ط����ڴ�
�������˵����  hPool �ڴ�صľ��
				dwBytes ������Ĵ�С
����ֵ˵���� ������ڴ��ָ��
===========================================================*/
void* MemAlignAlloc(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes);

/*===========================================================
���ܣ� �ͷ��ڴ棬�����ͷŵ��ڴ��������ɾ��
�������˵����  hPool �ڴ�صľ��
				pMem ���ͷŵ��ڴ���ָ��
����ֵ˵���� �ɹ������棬�����
===========================================================*/
BOOL MemPoolFree(HANDLE hPool,void *pMem);

/*===========================================================
���ܣ� ����ڴ����ü���
�������˵����  hPool �ڴ�صľ��
				pMem �ڴ���ָ��
����ֵ˵���� �ɹ������棬�����
===========================================================*/
BOOL MemPoolMDup(HANDLE hPool,void *pMem);

/*===========================================================
���ܣ��õ�ָ����С�Ķ����ڴ��ĸ���
�������˵����  hPool - �ڴ�صľ��
				dwSize - ���С
����ֵ˵���� 
===========================================================*/
UINT32 CustomBlockNumGet(HANDLE hPool, UINT32 dwSize);

/*===========================================================
���ܣ��õ�ָ����С��2��n�η��ڴ��ĸ���
�������˵����  hPool - �ڴ�صľ��
				dwSize - ���С
����ֵ˵���� 
===========================================================*/
UINT32 ExponentBlockNumGet(HANDLE hPool, UINT32 dwSize);

/*===========================================================
���ܣ��õ�ֱͨ�ڴ��ĸ���
�������˵����  HANDLE hPool - �ڴ�صľ��
����ֵ˵���� 
===========================================================*/
UINT32 DirectBlockNumGet(HANDLE hPool);

/*===========================================================
���ܣ� ��ʾ�ڴ����Ϣ
�������˵����  hPool �ڴ�صľ��
����ֵ˵���� 
===========================================================*/
void MemPoolShow( HANDLE hPool);

#endif


#ifndef _MEMPOOL_H
#define _MEMPOOL_H

//内存块
#define MEM_GRANULARITY_MIN 6 /*2的6次方=64*/
#define MEM_GRANULARITY_MAX 23 /*2的23次方=8M*/
#define MEM_GRANULARITY_LEVEL (MEM_GRANULARITY_MAX - MEM_GRANULARITY_MIN + 1) /*64,128,256,...,512K,1M共15级*/

#define MAXBLOCK_MEM (1<<MEM_GRANULARITY_MAX)


#define _PART_FNAME 32 //取出的文件名的长度
#define _MEMALLOC_MAGIC_NUME 0x2A2A2A2A //内存块的幻数(****)，添加于内存块的头尾

#define CUSTOM_MINSIZE	(UINT32)(1<<20) //定制内存的最小值，必须大于该值

/*内存分配器，负责记录内存分配的信息*/
typedef struct tagMemAllocator
{
	HANDLE hPool;//内存池的句柄
	UINT32 dwMemAllocIdx; //内存分频器索引，调试命令用到
// 	UINT32 dwOptions;// 初始标志位
	EList tAllocInfoList;//分配信息链表
	UINT64 qwTotalSysMem;	//实际用掉的系统内存, 即调用malloc分配的字节数
	UINT64 qwTotalTagUserMem; //所有的用户内存
	UINT64 qwMemViaMalloc;
	UINT64 qwMemViaPoolMalloc;
 	UINT64 qwMemViaMemAlign;	
// 	UINT64 m_qwMemViaPoolMemAlign;

	UINT64 *pqwTagUserMem;	  //标签为tag的内存
	UINT64 *pqwTagMallocNum;	//标签为tag的内存, Malloc 次数
	UINT64 *pqwTagPoolMallocNum;	//标签为tag的内存, PoolMalloc 次数
// 	UINT64 *pqwTagPoolMemalignNum;	//标签为tag的内存, PoolMemAlign 次数
 	UINT64 *pqwTagMemAlignNum;	//标签为tag的内存, MemAlign 次数
// 	UINT64 *pqwTagReallocNum;	//标签为tag的内存, PoolRealloc 次数
	UINT64 *pqwTagFreeNum;		//标签为tag的内存, MFree 次数
}MemAllocator;


/*内存块的链接信息，位于真实分配的内存的前面*/

/*内存块的幻数，位于真实分配的内存的尾部*/
// typedef struct tagMemBlockTail
// { 
// 	UINT32 dwMagicNum;//内存块的幻数
// } MemBlockTail; 
// 

/*记录内存分配信息的结构，位于真实分配的内存之前*/
typedef struct tagMemAllocHeader
{ 
	ENode tNode;//用户分配内存的链表节点
	INT32 sdwRef;//引用计数
	INT8	sFileName[_PART_FNAME+1];	//函数调用所在的文件名
	INT32	sdwLine;//函数调用所在的行
	UINT32	dwSize;//内存大小
	UINT32 dwAlignSize; //对齐后的大小
	UINT32 dwOsSize; //实际malloc的大小
	UINT32 dwTag;		//标签，用于独立统计单个模块的内存

	/*=== 内存块信息 ===*/
	ENode tBlockNode; //内存块的链表节点
	UINT32 dwBlockType;//内存块的类型
	/*=== 内存块信息 ===*/

	/*=== align信息 ===*/
	UINT32 dwAlign;	//对齐字节
	void* lSysAddr;	//操作系统的内存起始地址
	UINT32 dwMagicNum;//幻数

} MemAllocHeader; 

/*添加于内存块的尾部，用于验证内存块的一致性*/
typedef struct tagMemAllocTail
{
	UINT32 dwMagicNum;//内存块的幻数
}MemAllocTail;


#define  TAGINFO_MAXLEN	 20

typedef struct tagMemTagInfo
{
	UINT32 dwTag;
	INT8  achTagInfo[TAGINFO_MAXLEN];
}TMemTagInfo;		


/*由MemAllocHeader指针得到链表节点指针*/
#define ALLOCNODE_ENTRY(pMemHeader) ( &(((MemAllocHeader *)pMemHeader)->tNode) )

/*由ALLOC节点指针得到MemAllocHeader指针*/
#define	MEMHEADER_ENTRY_ALLOCNODE(pNode) HOST_ENTRY(pNode, MemAllocHeader, tNode)

/*由MemAllocHeader指针得到块节点指针*/
#define BLOCKNODE_ENTRY(pMemHeader) ( &(((MemAllocHeader *)pMemHeader)->tBlockNode) )

/*由块节点指针得到MemAllocHeader指针*/
#define	MEMHEADER_ENTRY_BLOCKNODE(pBlockNode) HOST_ENTRY(pBlockNode, MemAllocHeader, tBlockNode)


/*由MemAllocHeader指针得到用户内存的地址*/
#define	MEM_ADDRESS(pMemHeader)  (void*)((UINT8*)(pMemHeader)+sizeof(MemAllocHeader))

/*由用户内存地址得MemAllocHeader的地址*/
#define	MEMHEADER_ADDRESS(pMem)  (MemAllocHeader*)((UINT8*)(pMem)-sizeof(MemAllocHeader))

//得到实际的内存大小
#define REALSIZE(pMemAlloc) (pMemAlloc->dwOsSize)
//( pMemAlloc->dwSize + sizeof(MemAllocHeader) + sizeof(MemAllocTail) )

void* MallocByStatis(void *phAlloc, UINT32 dwBytes);
void FreeByStatis(void *phAlloc, void *p, UINT32 dwBytes );


/*===========================================================
功能： 生成内存池
输入参数说明：  
		pdwCustomMem - 传入定制内存信息, 必须大于1M，且数组中的内存大小不能相同。单位为 byte
		dwArrayNum － 数组元素个数，即定制内存有dwArrayNum个不同尺寸
		phAlloc － 内存分配器句柄            
返回值说明： 内存池的句柄，失败返回NULL
===========================================================*/
HANDLE MemPoolCreate(UINT32 *pdwCustomMem, UINT32 dwArrayNum, void *phAlloc);

/*===========================================================
功能： 释放内存池
输入参数说明：  hPool 内存池的句柄
返回值说明： 成功返回TRUE，否则FALSE
===========================================================*/
BOOL MemPoolDestroy(HANDLE hPool);

/*===========================================================
功能： 从内存池分配内存
输入参数说明：  hPool 内存池的句柄
				dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* MemPoolAlloc(HANDLE hPool,UINT32 dwBytes);

/*===========================================================
功能： 直接从os分配内存
输入参数说明：  hPool 内存池的句柄
				dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* MemOsAlloc(HANDLE hPool, UINT32 dwBytes);

/*===========================================================
功能： 直接从os memalign
输入参数说明：  hPool 内存池的句柄
				dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* OsMemAlign(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes);

/*===========================================================
功能： 从align内存池分配内存
输入参数说明：  hPool 内存池的句柄
				dwBytes 待分配的大小
返回值说明： 分配的内存的指针
===========================================================*/
void* MemAlignAlloc(HANDLE hPool, UINT32 dwAlign, UINT32 dwBytes);

/*===========================================================
功能： 释放内存，将被释放的内存从链表中删除
输入参数说明：  hPool 内存池的句柄
				pMem 待释放的内存块的指针
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL MemPoolFree(HANDLE hPool,void *pMem);

/*===========================================================
功能： 添加内存引用计数
输入参数说明：  hPool 内存池的句柄
				pMem 内存块的指针
返回值说明： 成功返回真，否则假
===========================================================*/
BOOL MemPoolMDup(HANDLE hPool,void *pMem);

/*===========================================================
功能：得到指定大小的定制内存块的个数
输入参数说明：  hPool - 内存池的句柄
				dwSize - 块大小
返回值说明： 
===========================================================*/
UINT32 CustomBlockNumGet(HANDLE hPool, UINT32 dwSize);

/*===========================================================
功能：得到指定大小的2的n次方内存块的个数
输入参数说明：  hPool - 内存池的句柄
				dwSize - 块大小
返回值说明： 
===========================================================*/
UINT32 ExponentBlockNumGet(HANDLE hPool, UINT32 dwSize);

/*===========================================================
功能：得到直通内存块的个数
输入参数说明：  HANDLE hPool - 内存池的句柄
返回值说明： 
===========================================================*/
UINT32 DirectBlockNumGet(HANDLE hPool);

/*===========================================================
功能： 显示内存池信息
输入参数说明：  hPool 内存池的句柄
返回值说明： 
===========================================================*/
void MemPoolShow( HANDLE hPool);

#endif


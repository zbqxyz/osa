#ifndef _H_TESTCASE_MEMLIB_
#define _H_TESTCASE_MEMLIB_

//////////////////////////////////////////////////////////////////////////
//功能测试

//分配资源标签
void FunTest_MRsrcTagAlloc( void );

//生成内存分配器
void FunTest_MAllocCreate( void );
//分配内存和释放
void FunTest_MallocFree( void );
//内存池中分配内存和释放
void FunTest_PoolMallocFree( void );
//销毁内存分配器
void FunTest_MAllocDestory( void );

//////////////////////////////////////////////////////////////////////////
//异常测试

//分配资源标签
void ExcptTest_MRsrcTagAlloc( void );
//生成内存分配器
void ExcptTest_MAllocCreate( void );
//释放内存分配器
void ExcptTest_MAllocDestroy( void );
//分配内存
void ExcptTest_Malloc( void );
//内存池中分配内存
void ExcptTest_PoolMalloc( void );
//释放内存
void ExcptTest_MFree( void );
//得到指针指向内存区的大小
void ExcptTest_MSizeGet( void );
//得到指定标签的用户内存总量
void ExcptTest_MUserMemGet( void );
//得到内存分配器向操作系统申请的内存总量
void ExcptTest_MSysMemGet( void );
//得到内存块类型
void ExcptTest_MTypeGet( void );

#endif
#ifndef _H_TESTCASE_MEMLIB_
#define _H_TESTCASE_MEMLIB_

//////////////////////////////////////////////////////////////////////////
//���ܲ���

//������Դ��ǩ
void FunTest_MRsrcTagAlloc( void );

//�����ڴ������
void FunTest_MAllocCreate( void );
//�����ڴ���ͷ�
void FunTest_MallocFree( void );
//�ڴ���з����ڴ���ͷ�
void FunTest_PoolMallocFree( void );
//�����ڴ������
void FunTest_MAllocDestory( void );

//////////////////////////////////////////////////////////////////////////
//�쳣����

//������Դ��ǩ
void ExcptTest_MRsrcTagAlloc( void );
//�����ڴ������
void ExcptTest_MAllocCreate( void );
//�ͷ��ڴ������
void ExcptTest_MAllocDestroy( void );
//�����ڴ�
void ExcptTest_Malloc( void );
//�ڴ���з����ڴ�
void ExcptTest_PoolMalloc( void );
//�ͷ��ڴ�
void ExcptTest_MFree( void );
//�õ�ָ��ָ���ڴ����Ĵ�С
void ExcptTest_MSizeGet( void );
//�õ�ָ����ǩ���û��ڴ�����
void ExcptTest_MUserMemGet( void );
//�õ��ڴ�����������ϵͳ������ڴ�����
void ExcptTest_MSysMemGet( void );
//�õ��ڴ������
void ExcptTest_MTypeGet( void );

#endif
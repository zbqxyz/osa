#ifndef _H_TESTCASE_THREAD_
#define _H_TESTCASE_THREAD_

//////////////////////////////////////////////////////////////////////////
//���ܲ���
//����������һ���߳�
void FunTest_ThreadCreate( void );
//�̺߳����ڲ��Լ��˳�
void FunTest_ThreadSelfExit( void );
//�����߳�����˳�
void FunTest_ThreadExitWait( void );
//�����߳�����˳�
void FunTest_ThreadKill( void );
//�̹߳������
void FunTest_ThreadSuspendResume( void );
//�ı��̵߳����ȼ�
void FunTest_SetPriority( void );

//////////////////////////////////////////////////////////////////////////
//�쳣����
//����������һ���߳�
void ExcptTest_ThreadCreate( void );
//�����߳�����˳�
void ExcptTest_ThreadKill( void );
//��ȡ��ǰ�����̵߳ľ��
void ExcptTest_ThreadSelfHandleGet( void );
//�̹߳������
void ExcptTest_ThreadSuspendResume( void );
//�ı��̵߳����ȼ�
void ExcptTest_SetPriority( void );

#endif
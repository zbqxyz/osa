#ifndef _H_TESTCASE_LOCK_
#define _H_TESTCASE_LOCK_

//////////////////////////////////////////////////////////////////////////
//���ܲ���
//Lock�Ĺ�����Osa�ڲ��Ѿ���ʹ�ã��������������Ѿ�ȷ��Lock����Ok��
//Sem�Ĺ������̵߳Ĳ������Ѿ�����

//////////////////////////////////////////////////////////////////////////
//�쳣����

//������������
void ExcptTest_LightLockCreate( void );
//ɾ����������
void ExcptTest_LightLockDelete( void );
//����������
void ExcptTest_LightLockLock( void );
//������������
void ExcptTest_LightLockUnLock( void );

//����һ����Ԫ�ź���
void ExcptTest_SemBCreate( void );
//ɾ���ź���
void ExcptTest_SemDelete( void );
//�ź���Take
void ExcptTest_SemTake( void );
//����ʱ���ź���Take
void ExcptTest_SemTakeByTime( void );
//�ź���Give
void ExcptTest_SemGive( void );

#endif
#ifndef _H_TESTCASE_TELNET_
#define _H_TESTCASE_TELNET_

//////////////////////////////////////////////////////////////////////////
//���ܲ���
//ע���������
void FunTest_CmdReg( void );

//�����ǩ������OCCֵ��ת��
void FunTest_Str2Occ( void );

//��ӡ��ǩ����
void FunTest_PrtLab( void );

//�ļ���־
void FunTest_FileLog( void );


//////////////////////////////////////////////////////////////////////////
//�쳣����
//ע���������
void ExcptTest_CmdReg( void );
//�����ǩ������OCCֵ��ת��
void ExcptTest_Str2Occ( void );

//////////////////////////////////////////////////////////////////////////
//ѹ������
void PersTest_TelnetAndLog(void );

void PersTest_ThreadCreateClose(void );
#endif
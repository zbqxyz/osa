#ifndef _H_TESTCASE_DIRFILE_
#define _H_TESTCASE_DIRFILE_

//////////////////////////////////////////////////////////////////////////
//���ܲ���
//������ɾ��Ŀ¼
void FunTest_DirCreateDel( void );
//ɾ���ļ�
void FunTest_FileDelete( void );
//��ȡ�ļ�����
void FunTest_FileLengthGet( void );
//ɾ��Ŀ¼�µ������ļ�
void FunTest_DelFilesInDir( void );

//////////////////////////////////////////////////////////////////////////
//�쳣����

//����Ŀ¼
void ExcptTest_DirCreate( void );
//ɾ��Ŀ¼
void ExcptTest_DirDelete( void );
//Ŀ¼�Ƿ����
void ExcptTest_IsDirExist( void );
//·���Ƿ����
void ExcptTest_IsPathExist( void );

//��ȡ�ļ����Ⱥ���
void ExcptTest_FileLengthGet( void );
//ɾ���ļ�
void ExcptTest_FileDelete( void );
//ɾ��Ŀ¼�µ������ļ�
void ExcptTest_DelFilesInDir( void );

#endif
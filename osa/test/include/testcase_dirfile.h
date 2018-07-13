#ifndef _H_TESTCASE_DIRFILE_
#define _H_TESTCASE_DIRFILE_

//////////////////////////////////////////////////////////////////////////
//功能测试
//创建和删除目录
void FunTest_DirCreateDel( void );
//删除文件
void FunTest_FileDelete( void );
//获取文件长度
void FunTest_FileLengthGet( void );
//删除目录下的所有文件
void FunTest_DelFilesInDir( void );

//////////////////////////////////////////////////////////////////////////
//异常测试

//创建目录
void ExcptTest_DirCreate( void );
//删除目录
void ExcptTest_DirDelete( void );
//目录是否存在
void ExcptTest_IsDirExist( void );
//路径是否存在
void ExcptTest_IsPathExist( void );

//获取文件长度函数
void ExcptTest_FileLengthGet( void );
//删除文件
void ExcptTest_FileDelete( void );
//删除目录下的所有文件
void ExcptTest_DelFilesInDir( void );

#endif
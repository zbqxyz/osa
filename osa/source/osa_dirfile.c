#include "osa_common.h"

#ifdef _LINUX_
//支持O_DIRECT
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

//支持大文件
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#endif

#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>
#include <errno.h>
#include <io.h>
//#include <shlwapi.h>
#endif

#ifdef _LINUX_
#include <dirent.h>
#endif


#ifdef _PPC_
#define DIRMODE		0777
#else
#define DIRMODE		0
#endif

#ifdef _MSC_VER
BOOL sWin_IsDirExistEx( const char *szDirPath )
{
	int nRet = 0;
	char achPath[OSA_MAX_DIRPATH_LEN+1] = {0};	

	if(!szDirPath)
		return FALSE;

	strncpy(achPath, szDirPath, OSA_MAX_DIRPATH_LEN);
	achPath[OSA_MAX_DIRPATH_LEN] = '\0';
	nRet = _access( (const char* )achPath, 0 );
	if( -1 == nRet )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
	//return PathFileExists( (const char* )achPath );
}
#else
BOOL sLinux_IsDirExistEx( const char *szDirPath )
{
	struct stat tStat;
	int ret;
	char achPath[OSA_MAX_DIRPATH_LEN+1];	
	char *pchJdgPath = NULL;

	pchJdgPath = (char *)szDirPath;
	UINT32 dwPathLen = strlen(szDirPath);

	//去掉结尾的‘/’
	if( (szDirPath[dwPathLen - 1] == '/') || (szDirPath[dwPathLen - 1] == '\\')) 
	{
		strncpy(achPath, szDirPath, OSA_MAX_DIRPATH_LEN);
		achPath[dwPathLen - 1] = '\0';

		pchJdgPath = achPath;
	}

	//判断目录是否存在
	ret = stat(pchJdgPath, &tStat);
	if(ret != 0)
	{
		if(errno != ENOENT)
		{
			iOsaLog(OSA_ERROCC, "sLinux_IsDirExistEx(), stat failed, dir=%s, ret=%d, errno=%d, %s\n", pchJdgPath, ret, errno, strerror(errno));
		}		
	}

	return (ret == 0) ? TRUE : FALSE;
}
#endif

/*====================================================================
功能:	创建目录
参数说明:	szDirPath - 目录路径
返回值说明：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaDirCreate(const char* szDirPath)
{
	UINT32 dwRet = 0;
	int ret;
	UINT32 dwPathLen;
	
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( szDirPath, "szDirPath" );

	dwPathLen = (UINT32)strlen(szDirPath);
	if(dwPathLen > OSA_MAX_DIRPATH_LEN )
	{
		iOsaLog(OSA_ERROCC, "OsaDirCreate(), path len is too long\n");
		return COMERR_INVALID_PARAM;
	}

#ifdef _MSC_VER
	ret = mkdir(szDirPath);
#endif

#ifdef _LINUX_
	ret = mkdir(szDirPath, DIRMODE);
#endif

	if(ret != 0)
	{
		iOsaLog(OSA_ERROCC, "OsaDirCreate err, dir is %s, errno is %d, %s\n", szDirPath, errno, strerror(errno));
		return COMERR_SYSCALL;
	}

	return dwRet;
}

/*====================================================================
功能:	删除目录，仅支持删除空目录
参数说明:	szDirPath - 目录路径				
返回值说明：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaDirDelete(const char* szDirPath)
{
	UINT32 dwRet = 0;
	int ret;
	UINT32 dwPathLen;

	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( szDirPath, "szDirPath" );

	dwPathLen = strlen(szDirPath);
	if(dwPathLen > OSA_MAX_DIRPATH_LEN)
	{
		iOsaLog(OSA_ERROCC, "OsaDirDelete(), path len is too long\n");
		return COMERR_INVALID_PARAM;
	}

	ret = rmdir(szDirPath);
	if(ret != 0)
	{
		iOsaLog(OSA_ERROCC, "OsaDirDelete err, dir is %s, errno is %d, %s\n", szDirPath, errno, strerror(errno));
		return COMERR_SYSCALL;
	}
	return dwRet;
}
 
/*====================================================================
功能:	目录是否存在
参数说明:	szDirPath - 目录路径
返回值说明：存在，TRUE；不存在，FALSE
====================================================================*/
BOOL OsaIsDirExist(const char* szDirPath)
{
	UINT32 dwPathLen;
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_BOOL( szDirPath, "szDirPath" );

	dwPathLen = (UINT32)strlen(szDirPath);
	if(dwPathLen > OSA_MAX_DIRPATH_LEN)
	{
		iOsaLog(OSA_ERROCC, "OsaIsDirExist(), path len is too long\n");
		return FALSE;
	}
	if(dwPathLen <= 1 )
	{
		iOsaLog(OSA_ERROCC, "OsaIsDirExist(), path len is too short\n");
		return FALSE;
	}

#ifdef _MSC_VER
	return sWin_IsDirExistEx( szDirPath );
#else
	return sLinux_IsDirExistEx( szDirPath );
#endif	
}

/*====================================================================
功能:	路径是否存在，可以是目录，也可以是文件
参数说明:	szDirPath - 目录路径
返回值说明：存在，TRUE；不存在，FALSE
====================================================================*/
BOOL OsaIsPathExist(const char* szDirPath)
{
#ifdef _MSC_VER
	struct __stat64 tStat;
#else
	struct stat64 tStat;
#endif

	int ret;
	UINT32 dwPathLen;
	char achPath[OSA_MAX_DIRPATH_LEN+1];	
	char *pchJdgPath = NULL;

	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_BOOL( szDirPath, "szDirPath" );

	dwPathLen = (UINT32)strlen(szDirPath);
	if(dwPathLen > OSA_MAX_DIRPATH_LEN)
	{
		iOsaLog(OSA_ERROCC, "OalIsPathExist(), path len is too long\n");
		return FALSE;
	}

	if(dwPathLen <= 1 )
	{
		iOsaLog(OSA_ERROCC, "OalIsPathExist(), path len is too short\n");
		return FALSE;
	}

	pchJdgPath = (char *)szDirPath;

	//去掉结尾的‘/’
	if( (szDirPath[dwPathLen - 1] == '/') || (szDirPath[dwPathLen - 1] == '\\')) 
	{
		strncpy(achPath, szDirPath, OSA_MAX_DIRPATH_LEN);
		achPath[dwPathLen - 1] = '\0';

		pchJdgPath = achPath;
	}

	//判断路径是否存在
#ifdef _MSC_VER
	ret = _stat64(pchJdgPath, &tStat);
#else
	ret = stat64(pchJdgPath, &tStat);
#endif
	if(ret != 0)
	{
		if( errno != 2 )
		{
			char achCWD[512];
			iOsaLog(OSA_ERROCC, "OsaIsPathExist(), stat failed, szDirPath=%s, dir=%s, ret=%d, errno=%d, %s, cwd=%s\n",
				szDirPath, pchJdgPath, ret, errno, strerror(errno), getcwd(achCWD, 512));
		}
	}

	return (ret == 0) ? TRUE : FALSE;
}

/*====================================================================
功能:	64位获取文件长度函数
参数说明:	szFilePathName - 文件全路径
返回值说明：成功返回文件长度，失败返回0
====================================================================*/
UINT64 OsaFileLengthGet(const char* szFilePathName )
{
	int ret;
#ifdef _MSC_VER
	struct __stat64 tStat;
#else
	struct stat64 tStat;
#endif
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_BOOL( szFilePathName, "szFilePathName" );

	//判断路径是否存在
#ifdef _MSC_VER
	ret = _stat64(szFilePathName, &tStat);
#else
	ret = stat64(szFilePathName, &tStat);
#endif
	if(ret != 0)
	{
		return 0;
	}

	return (UINT64)tStat.st_size;
}

/*====================================================================
功  能:	获取文件最后修改时间
参  数:	szFilePathName - 文件全路径
返回值：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaFileMdfTimeGet(const char* szFilePathName, UINT64* pqwMdfTime )
{
	UINT32 dwRet = 0;
	int ret;
#ifdef _MSC_VER
	struct __stat64 tStat;
#else
	struct stat64 tStat;
#endif
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( szFilePathName, "szFilePathName" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pqwMdfTime, "pqwMdfTime" );

	//判断路径是否存在
#ifdef _MSC_VER
	ret = _stat64(szFilePathName, &tStat);
#else
	ret = stat64(szFilePathName, &tStat);
#endif
	if(ret != 0)
	{
		iOsaLog(OSA_ERROCC, "OsaFileMdfTimeGet call stat64 err, file is %s, errno is %d, %s\n", szFilePathName, errno, strerror(errno));
		return COMERR_SYSCALL;
	}

	*pqwMdfTime = tStat.st_mtime;
	return OSA_OK;
}

/*====================================================================
功能:	删除文件
参数说明:	szFilePath - 文件路径
返回值说明：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaFileDelete(const char* szFilePathName)
{
	UINT32 dwRet = 0;
	int ret;

	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( szFilePathName, "szFilePathName" );

	ret = remove( szFilePathName );
	if(ret != 0)
	{
		iOsaLog(OSA_ERROCC, "OalFileDelete err, file is %s, errno is %d, %s\n", szFilePathName, errno, strerror(errno));
		return COMERR_SYSCALL;
	}

	return dwRet;
}

#ifdef _MSC_VER
static BOOL sWin_DelFilesInDir( const char* pchDir )
{
	WIN32_FIND_DATA tFindData;
	BOOL bFind = FALSE;
	BOOL bSuc = FALSE;
	char sTmp[512] = {0};
	HANDLE hFind;
	//int ret;
	UINT32 dwFileAttributes;

	_snprintf(sTmp, sizeof(sTmp)-1, "%s/*", pchDir);

	hFind = FindFirstFile(sTmp, &tFindData);
	if(INVALID_HANDLE_VALUE == hFind)
	{
		return TRUE;
	}
	
	while(TRUE)
	{
		//目录, 跳过
		if(tFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			bFind = FindNextFile(hFind, &tFindData);
			if(FALSE == bFind)
			{
				bSuc = TRUE;
				break;
			}
			continue;
		}

		memset(sTmp, 0, sizeof(sTmp));	
		_snprintf(sTmp, sizeof(sTmp)-1, "%s\\%s", pchDir, tFindData.cFileName);	
		
		//去除只读权限
		dwFileAttributes = tFindData.dwFileAttributes & (~FILE_ATTRIBUTE_READONLY);
		SetFileAttributes(sTmp, dwFileAttributes);

		//删除
		bSuc = DeleteFile(sTmp);
		if(!bSuc)
		{			
			iOsaLog(OSA_ERROCC, "OalDelFilesInDir  file %s failed, errno is %d\n", sTmp, GetLastError());
			
			FindClose(hFind);
			return FALSE;
		}
		
		bFind = FindNextFile(hFind, &tFindData);
		if(FALSE == bFind)
		{
			bSuc = TRUE;
			break;
		}
	}
	
	FindClose(hFind);
	return bSuc;
}

#else
static BOOL sLinux_DelFilesInDir( const char* pchDir )
{
//	BOOL bSuc;
	DIR  *pdir = NULL;
	struct dirent *dp = NULL;
	char sTmp[512] = {0};
	struct stat tStat;
	int ret;
	
	pdir = opendir(pchDir);
	if(pdir == NULL)
	{
		iOsaLog(OSA_ERROCC,"OalDelFilesInDir, opendir %s failed, errno=%d, %s\n",
			pchDir, errno, strerror(errno));
		return FALSE;
	}

	while((dp = readdir(pdir)) != NULL)
	{
		memset(sTmp, 0, sizeof(sTmp));
		snprintf(sTmp, sizeof(sTmp)-1, "%s/%s", pchDir, dp->d_name);	

		if(lstat(sTmp,  &tStat)  <  0)
		{   
			closedir(pdir);
			iOsaLog(OSA_ERROCC, "OalDelFilesInDir, lstat %s error, %d, %s\n", sTmp, errno, strerror(errno));   
			return FALSE;
		}

		//如果是目录，跳过
		if(S_ISDIR(tStat.st_mode))
		{
			continue;
		}

		ret = remove(sTmp);
		if(ret != 0)
		{
			closedir(pdir);
			iOsaLog(OSA_ERROCC, "OalDelFilesInDir err, file is %s, errno is %d, %s\n", sTmp, errno, strerror(errno));
			return FALSE;
		}
	}

	closedir(pdir);

	return TRUE;
}

#endif	

/*====================================================================
功能:	删除目录下的所有文件(不递归删除)
参数说明:	szDirPath - 目录(绝对，相对路径都可以, 不要以 "/", "\\" 结尾)
返回值说明：成功返回OSA_OK，失败返回错误码. 
====================================================================*/
UINT32 OsaDelFilesInDir(const char* szDirPath)
{
	UINT32 dwRet = 0;
	BOOL bSuc = FALSE;
	//参数检查
	OSA_CHECK_NULL_POINTER_RETURN_U32( szDirPath, "szDirPath" );

	bSuc = OsaIsPathExist( szDirPath );
	if(FALSE == bSuc)
	{
		iOsaLog(OSA_ERROCC, "OalDelFilesInDir, %s not exist\n", szDirPath);
		return COMERR_INVALID_PARAM;
	}
#ifdef _MSC_VER
	bSuc = sWin_DelFilesInDir( szDirPath );
#else
	bSuc = sLinux_DelFilesInDir( szDirPath );
#endif	

	if( !bSuc )
	{
		dwRet = COMERR_SYSCALL; 
	}
	return dwRet;
}

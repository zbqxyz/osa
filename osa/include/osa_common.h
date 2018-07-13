#ifndef _CDS_COMMON_H
#define _CDS_COMMON_H

//#pragma warning (disable: 4996)
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <sys/types.h>
//#include <sys/stat.h>
#include <winsock2.h>
#else
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#endif //_MSC_VER

#include <time.h>


#include "00common/vssbasetype.h"
#include "00common/vssbaseerr.h"

#include "10os/cds.h"
#include "10os/osa.h"

//��ģ��ʵ��ͷ�ļ�
#include "osa_inner.h"


#ifdef _MSC_VER
//ֱ�����������Ŀ��ļ���ʡ��ÿ�����̶���������Щ��
//#pragma comment(lib,"ShLwApi.Lib")
#pragma comment(lib,"Ws2_32.Lib")
#endif //_MSC_VER

#endif //_CDS_COMMON_H



#include "osa_common.h"


/*===========================================================
���ܣ��õ�����CPU���߼��ܺ���
����˵����	
����ֵ˵���� ����cpu����
===========================================================*/
UINT32 OsaCpuCoreTotalNumGet(void)
{
	UINT32 dwNum = 0;
#ifdef _MSC_VER
	SYSTEM_INFO tSysInfo;
	GetSystemInfo(&tSysInfo);
	dwNum = tSysInfo.dwNumberOfProcessors;
#endif

#ifdef _LINUX_
	dwNum = sysconf(_SC_NPROCESSORS_CONF );

#endif

	return dwNum;
}

#ifdef _MSC_VER
UINT32 sWin_IpListGet(IN OUT UINT32 *pdwIpList,  UINT32 dwListEleNum, OUT UINT32 *pdwRealIpNum)
{
	INT32 nRet = 0;
	char szName[512] = {0};
	char *pLocalIP = NULL;
	struct hostent *hp = NULL;
	UINT32 dwNum;
	UINT32 dwIdx;
	char **ppAddrList;
	char *pchIpStr;

	//��ȡ����������
	nRet = gethostname( szName, sizeof(szName) );
	if ( nRet == -1)
	{
		iOsaLog(OSA_ERROCC, "sWin_IpListGet gethostname error, error info:%s\n", strerror(errno));
		return COMERR_SYSCALL;
	}

	//��ip�б�
	hp = gethostbyname(szName);
	if (NULL == hp)
	{
		iOsaLog(OSA_ERROCC, "sWin_IpListGet gethostbyname error, error info:%s\n", strerror(errno));
		return COMERR_SYSCALL;
	}

	ppAddrList = hp->h_addr_list;

	//�ø������ж�
	dwIdx = 0;
	while(ppAddrList[dwIdx] != NULL && dwIdx < 32 )
	{
		dwIdx++;
	}

	dwNum = dwIdx;
	if(dwListEleNum < dwNum)
	{
		dwNum = dwListEleNum;
	}
	*pdwRealIpNum = dwNum;

	//��ֵ
	for(dwIdx = 0; dwIdx < dwNum; dwIdx++)
	{
		pchIpStr = inet_ntoa(*((struct in_addr *)ppAddrList[dwIdx]));
		pdwIpList[dwIdx] = inet_addr(pchIpStr);
	}	

	return OSA_OK;
}
#endif

#ifdef _LINUX_
#define MACIP_MAXNUM	5  //���Mac��ַ����
static const char* g_aStrNetCardName[MACIP_MAXNUM] = {"eth0", "eth1", "eth2", "eth3", "ppp0"};
UINT32 sLinux_IpListGet(IN OUT UINT32 *pdwIpList,  UINT32 dwListEleNum, OUT UINT32 *pdwRealIpNum)
{
	int sock; 
	struct sockaddr_in sin; 
	struct ifreq ifr;
	UINT32 dwIdx;
	UINT32 dwRealIpNum = 0;

	sock = socket(AF_INET, SOCK_DGRAM, 0); 
	if(sock<0)
	{
		iOsaLog(OSA_ERROCC, "IpListGet(), socket err\n");			
		return COMERR_SYSCALL;
	}

	dwRealIpNum = 0;
	for(dwIdx = 0; dwIdx < MACIP_MAXNUM; dwIdx++)
	{
		strcpy(ifr.ifr_name, g_aStrNetCardName[dwIdx]);

		if(ioctl(sock, SIOCGIFADDR, &ifr) < 0) 
		{
			continue;
		}		

		if(dwRealIpNum < dwListEleNum)
		{
			memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
			pdwIpList[dwRealIpNum]=sin.sin_addr.s_addr;
		}	

		dwRealIpNum++;
	}

	close(sock);

	*pdwRealIpNum = dwRealIpNum;

	return OSA_OK;	
}
#endif

/*===========================================================
���ܣ��õ�ip�б�
����˵����	pdwIpList �� ָ��ip��ַ��UINT32, �����򣩵�ָ��
			dwListEleNum - �����б�Ԫ�صĸ����������Դ�ż���ip��ַ	
			pdwRealIpNum - ʵ�ʵ�ip��ַ����
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
===========================================================*/
UINT32 OsaIpListGet(IN OUT UINT32 *pdwIpList,  UINT32 dwListEleNum, OUT UINT32 *pdwRealIpNum)
{
	UINT32 dwRet = 0;
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( pdwIpList, "pdwIpList" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( pdwRealIpNum, "pdwRealIpNum" );

	if( dwListEleNum == 0 )
	{		
		iOsaLog(OSA_ERROCC, "OsaIpListGet para err, dwListEleNum=%d\n", dwListEleNum );
		return COMERR_INVALID_PARAM;
	}

#ifdef _MSC_VER
	dwRet = sWin_IpListGet( pdwIpList, dwListEleNum, pdwRealIpNum );
#endif

#ifdef _LINUX_
	dwRet = sLinux_IpListGet( pdwIpList, dwListEleNum, pdwRealIpNum );
#endif
	return dwRet;
}

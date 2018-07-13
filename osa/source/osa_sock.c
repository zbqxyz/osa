#include "osa_common.h"

/*====================================================================
���ܣ��׽ӿڿ��ʼ������װwindows��WSAStartup
�������˵����
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
====================================================================*/
UINT32 OsaSocketInit(void)
{
	UINT32 dwRet = 0;
#ifdef _MSC_VER
	int nRet = 0;
	WSADATA wsaData;

	nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(nRet != 0)
	{
		iOsaLog(OSA_ERROCC, "WSAStartup failed, errno=%d\n", WSAGetLastError());
		dwRet = COMERR_SYSCALL;
	}
#endif

	return dwRet;
}

/*====================================================================
���ܣ��׽ӿڿ��˳�����װwindows��WSACleanup
�������˵����
����ֵ˵�����ɹ�����TRUE��ʧ�ܷ���FALSE
====================================================================*/
UINT32 OsaSocketExit(void)
{
	UINT32 dwRet = 0;
#ifdef _MSC_VER
	int nRet = 0;
	nRet = WSACleanup();
	if(nRet != 0)
	{
		iOsaLog(OSA_ERROCC, "WSACleanup failed, errno=%d\n", WSAGetLastError());
		dwRet = COMERR_SYSCALL;
	}
#endif

	return dwRet;
}

/*====================================================================
���ܣ�����Tcp�׽���
�������˵����
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
 ====================================================================*/
UINT32 OsaTcpSocketCreate( SOCKHANDLE *phSock )
{
	UINT32 dwRet = 0;
	SOCKHANDLE hListenSock = 0;
	//�������
	OSA_CHECK_NULL_POINTER_RETURN_U32( phSock, "phSock" );

	hListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if(hListenSock == INVALID_SOCKET)
	{
		iOsaLog(OSA_ERROCC, "OsaTcpSocketCreate(), listen socket() failed\n");
		dwRet = COMERR_SYSCALL;
	}

	*phSock = hListenSock;
	return dwRet;
}

/*====================================================================
��  �ܣ������׽����Ƿ�����
��  ����
����ֵ���ɹ�����OSA_OK��ʧ�ܷ��ش�����
 ====================================================================*/
UINT32 OsaTcpSocketBlockSet(SOCKHANDLE hSock, BOOL bIsBlock)
{
	UINT32 dwRet = 0;
	int nRet = 0;
	unsigned long dwFlag;
	//�������
	if( INVALID_SOCKET == hSock )
	{
		iOsaLog(OSA_ERROCC, "OsaSocketClose error param hSock=0x%x\n", hSock );
		return COMERR_INVALID_PARAM;
	}
	dwFlag = (bIsBlock == TRUE ? 0 : 1);
#ifdef _MSC_VER
	nRet = ioctlsocket(hSock ,FIONBIO, &dwFlag);
	if(nRet != 0)
	{
		iOsaLog(OSA_ERROCC, "ioctlsocket failed, errno=%d\n", WSAGetLastError());
		dwRet = COMERR_SYSCALL;
	}
#else
	nRet = ioctl(hSock, FIONBIO, &dwFlag);
	if(nRet != 0)
	{
		iOsaLog(OSA_ERROCC, "ioctl failed, errno=%d, %s\n", nRet, strerror(nRet) );
		dwRet = COMERR_SYSCALL;
	}
#endif

	return dwRet;
}


/*====================================================================
��  �ܣ����ô���ʱ���׽����Ƿ�����
��  ����
����ֵ���ɹ�����OSA_OK��ʧ�ܷ��ش�����
 ====================================================================*/
UINT32 OsaTcpSocketBlockSetByTimeout(SOCKHANDLE hSock, BOOL bIsBlock, UINT32 dwNetTimeout)
{
	UINT32 dwRet = 0;
	int nRet = 0;
	unsigned long dwFlag;
	int nNetTimeout = dwNetTimeout;	//5000; //Ĭ�ϳ�ʱʱ��5��
	//�������
	if( INVALID_SOCKET == hSock )
	{
		iOsaLog(OSA_ERROCC, "OsaSocketClose error param hSock=0x%x\n", hSock );
		return COMERR_INVALID_PARAM;
	}
	dwFlag = (bIsBlock == TRUE ? 0 : 1);
#ifdef _MSC_VER
	nRet = ioctlsocket(hSock ,FIONBIO, &dwFlag);
	if(nRet != 0)
	{
		iOsaLog(OSA_ERROCC, "ioctlsocket failed, errno=%d\n", WSAGetLastError());
		dwRet = COMERR_SYSCALL;
	}
	//���÷��ͳ�ʱʱ�䣬��ֹsend���� add by yhq 20160302
	if(dwFlag)
	{
		nRet = setsockopt(hSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&nNetTimeout, sizeof(int));
		//iOsaLog(OSA_ERROCC, "setsockopt, nRet=%d\n", nRet);
		if(nRet != 0)
		{
			iOsaLog(OSA_ERROCC, "setsockopt failed, errno=%d\n", WSAGetLastError());
			dwRet = COMERR_SYSCALL;
		}
	}
	
#else
	nRet = ioctl(hSock, FIONBIO, &dwFlag);
	if(nRet != 0)
	{
		iOsaLog(OSA_ERROCC, "ioctl failed, errno=%d, %s\n", nRet, strerror(nRet) );
		dwRet = COMERR_SYSCALL;
	}
#endif

	return dwRet;
}

/*====================================================================
���ܣ��ر��׽���
�������˵����
����ֵ˵�����ɹ�����TRUE��ʧ�ܷ���FALSE
====================================================================*/
UINT32 OsaSocketClose(SOCKHANDLE hSock)
{
	UINT32 dwRet = 0;
	int nRet = 0;
	//�������
	if( INVALID_SOCKET == hSock )
	{
		iOsaLog(OSA_ERROCC, "OsaSocketClose error param hSock=0x%x\n", hSock );
		return COMERR_INVALID_PARAM;
	}

#ifdef _MSC_VER
	nRet = closesocket(hSock);
	if(nRet != 0)
	{
		iOsaLog(OSA_ERROCC, "closesocket failed, errno=%d\n", WSAGetLastError());
		dwRet = COMERR_SYSCALL;
	}
#endif

#ifdef _LINUX_

	// 0,1,2 Ϊ STDIN, STDOUT, STDERR
	if(hSock < 3)
	{
		dwRet = COMERR_INVALID_PARAM;
	}
	else
	{
		INT32 nError = 0;
		nRet = shutdown(hSock, 2 );
// 		if( nRet != 0) 
// 		{ 
// 			nError = errno;
// 			iOsaLog(OSA_ERROCC, "shutdown(hSock) failed, errno=%d, %s, line:%d.\n", nError, strerror(nError), __LINE__ );
// 		} 

		nRet = close(hSock);
		if(nRet != 0)
		{
			nError = errno;
			iOsaLog(OSA_ERROCC, "close(hSock) failed, errno=%d, %s, line:%d.\n", nError, strerror(nError), __LINE__ );
			dwRet = COMERR_SYSCALL;
		}
	}
#endif

	return dwRet;
}


/*====================================================================
���ܣ����׽��ֶ˿�
�������˵����
����ֵ˵�����ɹ�����TRUE��ʧ�ܷ���FALSE
====================================================================*/
UINT32 OsaSocketBind(SOCKHANDLE hSock, UINT16 wPort )
{
	UINT32 dwRet = 0;
	struct sockaddr_in tSvrAddr;
	INT32 nRet = 0;

	//�������
	if( INVALID_SOCKET == hSock )
	{
		iOsaLog(OSA_ERROCC, "OsaSocketBind error param hSock=0x%x\n", hSock );
		return COMERR_INVALID_PARAM;
	}

	memset( &tSvrAddr, 0, sizeof(tSvrAddr) );    
	tSvrAddr.sin_family = AF_INET; 
	tSvrAddr.sin_port = htons(wPort);
	tSvrAddr.sin_addr.s_addr = INADDR_ANY;

	nRet = bind(hSock, (struct sockaddr *)&tSvrAddr, sizeof(tSvrAddr)); 
	if( nRet != 0 )
	{
		INT32 nError = 0;
#ifdef _MSC_VER		
		nError = WSAGetLastError();
		iOsaLog(OSA_ERROCC, "bind failed, wPort=%d, errno=%d\n", wPort, nError );
		dwRet = COMERR_SYSCALL;
#else
		nError = errno;
		iOsaLog(OSA_ERROCC, "bind failed, wPort=%d, errno=%d, %s\n", wPort, nError, strerror(nError) );
		dwRet = COMERR_SYSCALL;
#endif
	}
	return dwRet;
}

/*====================================================================
���ܣ������׽���
�������˵����
����ֵ˵�����ɹ�����TRUE��ʧ�ܷ���FALSE
====================================================================*/
UINT32 OsaSocketListen(SOCKHANDLE hSock, INT32 nBacklog )
{
	UINT32 dwRet = 0;
	INT32 nRet = 0;

	nRet = listen(hSock, nBacklog); 
	if( nRet != 0 )
	{
		INT32 nError = 0;
#ifdef _MSC_VER		
		nError = WSAGetLastError();
		iOsaLog(OSA_ERROCC, "listen failed, nBacklog=%d errno=%d\n", nBacklog, nError );
		dwRet = COMERR_SYSCALL;
#else
		nError = errno;
		iOsaLog(OSA_ERROCC, "listen failed, nBacklog=%d errno=%d, %s\n", nBacklog, nError, strerror(nError) );
		dwRet = COMERR_SYSCALL;
#endif
	}
	return dwRet;
}

/*====================================================================
���ܣ���������
�������˵����
����ֵ˵�����ɹ�����TRUE��ʧ�ܷ���FALSE
====================================================================*/
UINT32 OsaSocketAccept(SOCKHANDLE hListenSock, SOCKHANDLE *phAcceptSock, OUT struct sockaddr *addr, IN OUT int *addrlen )
{
	UINT32 dwRet = 0;
	SOCKHANDLE hAcceptSock = 0;
	//�������
	if( INVALID_SOCKET == hListenSock )
	{
		iOsaLog(OSA_ERROCC, "OsaSocketAccept error param hListenSock=0x%x\n", hListenSock );
		return COMERR_INVALID_PARAM;
	}

	OSA_CHECK_NULL_POINTER_RETURN_U32( phAcceptSock, "phAcceptSock" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( addr, "addr" );
	OSA_CHECK_NULL_POINTER_RETURN_U32( addrlen, "addrlen" );
#ifdef _MSC_VER
	hAcceptSock = accept(hListenSock, addr, addrlen);
#else
	hAcceptSock = accept(hListenSock, addr, (socklen_t*)addrlen);
#endif

	if(INVALID_SOCKET == hAcceptSock )
	{
		iOsaLog(OSA_ERROCC, "OsaSocketAccept failed, hListenSock=0x%x\n", hListenSock );
		return COMERR_SYSCALL;
	}
	*phAcceptSock = hAcceptSock;
	return dwRet;
}

/*====================================================================
���ܣ���socket������Ϣ
�������˵����tSock -- �׽���
		   pchBuf -- Ҫ���͵Ļ���
		   dwLen -- �����С
		   nFlag -- ���ͱ�־	
����ֵ˵�����������ݳ���
====================================================================*/
INT32 OsaSocketSend(SOCKHANDLE hSock, const char *pchBuf, UINT32 dwLen, INT32 nFlag )
{
	INT32 ret = 0;

	//�������
	if((hSock == INVALID_SOCKET) || (pchBuf == NULL) || (dwLen == 0))
	{
		iOsaLog(OSA_ERROCC, "OsaSocketSend error param\n" );
		return 0;
	}

	ret = send(hSock, pchBuf, dwLen, nFlag);
	if(ret <= 0)	
	{
		INT32 nError;
#ifdef _MSC_VER		
		nError = WSAGetLastError();
		if( nError != 0 && nError != 10035 )
		{
			//��������־��ӡ�����ô����ӡ������Ƕ����ѭ��
			iOsaLog(OSA_LOGOCC, "send(hSock) len:%d, errno=%d\n", ret, nError );
		}
#endif
#ifdef _LINUX_
		nError = errno;
		iOsaLog(OSA_LOGOCC, "send(hSock) len:%d, errno=%d, %s\n", ret, nError, strerror(nError) );
#endif
		return ret;
	}

	return ret;
}

/*====================================================================
���ܣ���socket������Ϣ
�������˵����tSock-- �׽���
		   pchBuf -- �������ݵĻ���
		   dwLen -- �����С
		   pdwErrNo -- ������ 
����ֵ˵�������ؽ��ܵ����ֽ���
====================================================================*/
INT32 OsaSocketRecv(SOCKHANDLE hSock, char *pchBuf, UINT32 dwLen, UINT32 *pdwErrNo)
{
	INT32 nError = 0;
	INT32 ret = SOCKET_ERROR;	

	//�������
	if((hSock == INVALID_SOCKET) || (pchBuf == NULL) || (dwLen == 0) || (pdwErrNo == NULL))
	{		
		iOsaLog(OSA_ERROCC, "OsaSocketRecv para err in SockRecv(), hSock=0x%x, pchBuf=0x%x, buflen=0x%x, pdwErrNo=0x%x\n",
			hSock, pchBuf, dwLen, pdwErrNo);
		return 0;
	}

	*pdwErrNo = 0;

	ret = recv(hSock, pchBuf, dwLen, 0);

	//if(SOCKET_ERROR == ret || 0 == ret)	
	if((ret < 0 )||( 0 == ret))	//����ֵ<0����  =0���ӹر� >0���յ����ֽ��� add by yhq 20160226
	{
#ifdef _MSC_VER		
		nError = WSAGetLastError();
		*pdwErrNo = nError;
		if( nError != 0 && nError != 10035 )
		{
			iOsaLog(OSA_ERROCC, "recv(hSock) failed, errno=%d\n", nError );
		}
		else
		{
			iOsaLog(OSA_MSGOCC, "recv(hSock) len:%d return 0, socket disconnect!\n", dwLen );
		}
#endif

#ifdef _LINUX_
		nError = errno;
		*pdwErrNo = nError;
		if((ret < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR)) //ֻ��recv����ֵ<0�Ҵ����벻�������ֲ�˵��socket���쳣��add by yhq 20160226		
		{
			iOsaLog(OSA_ERROCC, "recv(hSock) failed, errno=%d, %s\n", nError, strerror(nError) );
		}		
#endif
		return ret;
	}

	return ret;
}

/*====================================================================
��  �ܣ����ӷ�����
��  ����tSock-- �׽���
	   szIp -- ������IP��ַ
	   wPort -- �����������˿�
����ֵ���ɹ�����OSA_OK��ʧ�ܷ��ش�����
====================================================================*/
UINT32 OsaSocketConnect(SOCKHANDLE hSock, const char* szIp, UINT16 wPort )
{
	UINT32 dwRet = 0;
	INT32 nError = 0;
	struct sockaddr_in tSvrAddr;
	INT32 nRet = 0;

	memset( &tSvrAddr, 0, sizeof(tSvrAddr) );    
	tSvrAddr.sin_family = AF_INET; 
	tSvrAddr.sin_port = htons(wPort);
	tSvrAddr.sin_addr.s_addr = inet_addr( szIp );

	//�������
	if((hSock == INVALID_SOCKET) || (szIp == NULL) )
	{		
		iOsaLog(OSA_ERROCC, "OsaSocketConnect para err , hSock=0x%x, szIp=0x%x\n",hSock, szIp );
		return COMERR_INVALID_PARAM;
	}

	nRet = connect(hSock, (struct sockaddr *)&tSvrAddr, sizeof(tSvrAddr)); 
	if( nRet != 0 )
	{
#ifdef _MSC_VER		
		nError = WSAGetLastError();
		if( 10035 != nError )
		{
			iOsaLog(OSA_ERROCC, "connect failed, serverIp:%s, port:%d��errno=%d, line:%d.\n", szIp, wPort, nError, __LINE__ );
			dwRet = COMERR_SYSCALL;
		}
#endif
		
#ifdef _LINUX_
		nError = errno;
		if( EINPROGRESS != nError)
		{
			iOsaLog(OSA_ERROCC, "connect failed, serverIp:%s, port:%d��errno=%d, %s\n", szIp, wPort, nError, strerror(nError) );
			dwRet = COMERR_SYSCALL;
		}
#endif
	}
	return dwRet;
}
#include "osa_common.h"

/*====================================================================
���ܣ�Osa��ʼ��
����˵������
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
====================================================================*/
UINT32 OsaInit( TOsaInitParam tInitParam )
{
	UINT32 dwRet = 0;
	UINT16 wTelnetPort = OSA_DEFAULT_TELNET_PORT;
	TOsaMgr* ptMgr = iOsaMgrGet();
	if( ptMgr->m_bIsInit )
	{
		return COMERR_ALREADY_INIT;
	}

	memset( ptMgr, 0, sizeof(TOsaMgr) );

	if( tInitParam.wTelnetPort != 0 )
	{
		wTelnetPort = tInitParam.wTelnetPort;
	}

	do 
	{
		//��ʼ���ź����������
		EListInit(&ptMgr->m_tSemWatchList);
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), EListInit failed\n");
			break;
		}

		//�������ڱ����ź����������(LockHandle��Ҫ����أ�������ⲻ���ٱ���أ�������InnerLockHandle )
		iInnerLightLockCreate( &ptMgr->m_hSemWatchListLock );

		//��ʼ�������б�
		dwRet = EListInit( &ptMgr->m_tTaskList );
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), EListInit failed\n");
			break;
		}
		//�������ڱ��������б����
		dwRet = OsaLightLockCreate( &ptMgr->m_hTaskListLock );
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), OsaLightLockCreate failed, ret=0x%x\n", dwRet);
			break;
		}

		//��ʼ���ڴ��
		dwRet = iMemLibInit( );
		if( dwRet != OSA_OK )
		{
			printf("OsaInit(), iMemLibInit failed, ret=0x%x\n", dwRet);
			break;
		}

		//��ʼ���׽ӿڿ⣬����Telnet
		dwRet = OsaSocketInit();
		if( dwRet != OSA_OK )
		{
			break;
		}

	} while (0);

	if( OSA_OK == dwRet )
	{
		//���е�����ͱ�־��ʼ���ɹ��ˣ���������Tel������õĶ���ڲ����ܣ������ʼ����־
		ptMgr->m_bIsInit = TRUE;
		ptMgr->m_tInitParam = tInitParam;

		do 
		{
			//����Osa�ڴ���Դ��ǩ��
			dwRet = OsaMRsrcTagAlloc(OSA_MRSRC_TAG, (UINT32)strlen(OSA_MRSRC_TAG), &ptMgr->m_dwRsrcTag);
			if( dwRet != OSA_OK )
			{
				printf("OsaInit(), OsaMRsrcTagAlloc failed, ret=0x%x\n", dwRet);
				break;
			}	
			//�����ڴ����������Osa�ڲ�ʹ��
			dwRet = OsaMAllocCreate( 0, 0, &ptMgr->m_hMemAlloc );
			if( dwRet != OSA_OK || NULL == ptMgr->m_hMemAlloc )
			{
				printf("OsaInit(), OsaMAllocCreate failed, ret=0x%x\n", dwRet);
				break;
			}

			//����telnet����, ��ע���������
			dwRet = InnerOsaTelnetSvrInit( wTelnetPort, tInitParam.szLogFilePrefixName, &ptMgr->m_hTelHdl );
			if( dwRet != OSA_OK )
			{
				printf("OsaInit(), OsaTelnetSvrInit failed, port is %d, ret=0x%x\n", wTelnetPort, dwRet );
			}
			else
			{
				//��ģ�����Telnet��ʼ��
				OsaTelnetHdlGet();
				iOsaTelnetInit();
			}
		} while (0);
	}

	if( dwRet != 0 )
	{
		ptMgr->m_bIsInit = FALSE;
	}
	else
	{
		//������־��ӡ
		fpon();
		//���Դ�ӡ
		iOsaLog(OSA_MSGOCC, "OsaInit(), success.\n");
	}

	return dwRet;
}

/*====================================================================
���ܣ�Osa�˳�
����˵������
����ֵ˵�����ɹ�����OSA_OK��ʧ�ܷ��ش�����
====================================================================*/
UINT32 OsaExit(void)
{
	UINT32 dwRet = 0;
	TelnetHandle hTelHdl = NULL;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//��ʼ�����
	OSA_CHECK_INIT_RETURN_U32();
	//�ر���־��ӡ
	fpoff();
	hTelHdl = ptMgr->m_hTelHdl;

	//�˳�Telnet�󣬾������Ч�ˣ�������ΪNULL���ڲ���ӡ���Զ��л���printf��
	//ͬʱȷ���˳�Telnet�˳�����ʹ��telnet��������ܻ����쳣
	ptMgr->m_hTelHdl = NULL; 
	//�˳�telnet������
	InnerOsaTelnetSvrExit( hTelHdl );
	OsaSocketExit( );

	//�ͷ��ڴ������
	OsaMAllocDestroy( ptMgr->m_hMemAlloc );
	//�˳��ڴ��
	iMemLibExit( );

	//ɾ�����ڱ��������б����
	OsaLightLockDelete( ptMgr->m_hTaskListLock );
	//ɾ�����ڱ��������б����
	iInnerLightLockDelete( &ptMgr->m_hSemWatchListLock );

	//�����������
	memset( ptMgr, 0, sizeof(TOsaMgr) );
	return OSA_OK;
}


char *FCC2STR(FCC tFccVal)
{
	UINT64 qwVal = tFccVal;
	UINT64 qwQot; //����
	UINT8 byRsd; //����
	UINT32 dwIdx;
	static char achFcc[5];


	dwIdx = 0;
	do
	{
		byRsd = (UINT8)(qwVal%ONEBYTE_BASE);
		qwQot =	qwVal/ONEBYTE_BASE;

		achFcc[dwIdx] = (char)byRsd;
		dwIdx++;
		qwVal = qwQot;

	}while(qwQot > 0);

	achFcc[4] = '\0';
	return achFcc;	
}

/*=================================================================
��    ��: �õ�Osaģ���ʼ��������telnet������
����˵��: 
����ֵ:	 �ɹ�����telnet�����������ʧ�ܷ���NULL
=================================================================*/
TelnetHandle OsaTelnetHdlGet(void)
{
//	UINT32 dwRet = 0;
	TOsaMgr* ptMgr = iOsaMgrGet();

	//��ʼ�����
	if (FALSE == iIsOsaInit() )
	{
		return NULL;
	}

	return ptMgr->m_hTelHdl;
}

#ifndef _H_STDAFX_
#define _H_STDAFX_

#pragma warning (disable: 4996)

#include <stdio.h>
#include <direct.h>
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

#include "test_include/cunit/CUnit.h"
#include "test_include/cunit/Automated.h"
#include "test_include/cunit/Basic.h"

#include "00common/vssbasetype.h"
#include "00common/vssbaseerr.h"
#include "10os/cds.h"
#include "10os/osa.h"


//本模块头文件
#include "testenv.h"
#include "testmain.h"
#include "testcase_telnet.h"
#include "testcase_thread.h"
#include "testcase_lock.h"
#include "testcase_memlib.h"
#include "testcase_dirfile.h"
#include "testcase_sysinfo.h"


#ifdef _MSC_VER
#include "00common/vsslinklib.h"
#ifdef _WIN64
#pragma comment(lib,LIB_TEST_CUNIT_IMPORT("libcunit64.lib"))
#ifdef _DEBUG
#pragma comment(lib,LIB_TEST_DEBUG_IMPORT("cds64_d.lib"))
#pragma comment(lib,LIB_TEST_DEBUG_IMPORT("osa64_d.lib"))
#else
#pragma comment(lib,LIB_TEST_RELEASE_IMPORT("cds64.lib"))
#pragma comment(lib,LIB_TEST_RELEASE_IMPORT("osa64.lib"))
#endif
#else //_WIN32
#pragma comment(lib,LIB_TEST_CUNIT_IMPORT("libcunit.lib"))
#ifdef _DEBUG
#pragma comment(lib,LIB_TEST_DEBUG_IMPORT("cds_d.lib"))
#pragma comment(lib,LIB_TEST_DEBUG_IMPORT("osa_d.lib"))
#else
#pragma comment(lib,LIB_TEST_RELEASE_IMPORT("cds.lib"))
#pragma comment(lib,LIB_TEST_RELEASE_IMPORT("osa.lib"))
#endif
#endif
#endif

#endif



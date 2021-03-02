// service_use.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <process.h>

//#include <rpcdce.h>

#include <strsafe.h>

#include <tchar.h>
#include <Accctrl.h>
#include <Aclapi.h>
#include <tlhelp32.h>
#include "Wtsapi32.h"

#include <Wtsapi32.h>
#pragma comment( lib, "Wtsapi32.lib" )
#pragma warning( disable : 4996 )
#pragma warning( disable : 4995 )
#define SVCNAME TEXT("serviceuse")

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR*);
VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR*);
VOID SvcReportEvent(LPTSTR);

VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}


VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.
		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
		return;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR* lpszArgv)
{
	// Register the handler function for the service

	gSvcStatusHandle = RegisterServiceCtrlHandler(SVCNAME, SvcCtrlHandler);

	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	//MessageBox(0,L"service", L"service process is running",0);
	// Report initial status to the SCM
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	// Perform service-specific initialization and work.
	SvcInit(dwArgc, lpszArgv);
}

VOID SvcInit(DWORD dwArgc, LPTSTR* lpszArgv)
{
	//做事阿
	// TO_DO: Declare and set any required variables.
	//   Be sure to periodically call ReportSvcStatus() with 
	//   SERVICE_START_PENDING. If initialization fails, call
	//   ReportSvcStatus with SERVICE_STOPPED.

	// Create an event. The control handler function, SvcCtrlHandler,
	// signals this event when it receives the stop control code.

	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
	// Report running status when initialization is complete.
	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);
	// TO_DO: Perform work until service stops.


	//WaitForSingleObject(ghTrdFirewall, INFINITE);
	while (1)
	{
		// Check whether to stop the service.
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
}

BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.

	typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	auto ret = GetModuleHandle(TEXT("kernel32"));
	if (ret != NULL)
	{
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(ret, "IsWow64Process");
		if (NULL != fnIsWow64Process)
			fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	}
	return bIsWow64;
}

void write_ros_reg()
{
	HKEY hKey;
	TCHAR ros_reg[MAX_PATH] = { 0 };

	TCHAR zsInstallLocation[MAX_PATH] = { 0 };
	TCHAR zsUninstallString[MAX_PATH] = { 0 };
	//DisplayName DisplayVersion InstallLocation Publisher UninstallString
	if (IsWow64())
	{
		_tcscpy(ros_reg, TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\uninstall\\Serviceuse"));
		_tcscpy(zsInstallLocation, TEXT("D:\\PersonalPractice\\service_use\\Debug"));
	}
	else
	{
		_tcscpy(ros_reg, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Serviceuse"));
		_tcscpy(zsInstallLocation, TEXT("D:\\PersonalPractice\\service_use\\Debug"));
	}

	if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, ros_reg, 0L, KEY_ALL_ACCESS, &hKey))
		RegCreateKey(HKEY_LOCAL_MACHINE, ros_reg, &hKey);

	RegSetValueEx(hKey, TEXT("DisplayName"), NULL, REG_SZ, (LPBYTE)TEXT("Serviceuse"), sizeof(TCHAR) * (_tcslen(TEXT("Serviceuse")) + 1));
	RegSetValueEx(hKey, TEXT("DisplayVersion"), NULL, REG_SZ, (LPBYTE)TEXT("1.0.0.0"), sizeof(TCHAR) * (_tcslen(TEXT("1.0.0.0")) + 1));
	RegSetValueEx(hKey, TEXT("InstallLocation"), NULL, REG_SZ, (LPBYTE)zsInstallLocation, sizeof(TCHAR) * (_tcslen(zsInstallLocation) + 1));
	RegSetValueEx(hKey, TEXT("UninstallString"), NULL, REG_SZ, (LPBYTE)zsUninstallString, sizeof(TCHAR) * (_tcslen(zsUninstallString) + 1));

	RegCloseKey(hKey);
}


void UninstallService(PWSTR pszServiceName)
{
	SC_HANDLE schSCManager = NULL;
	SC_HANDLE schService = NULL;
	SERVICE_STATUS ssSvcStatus = {};

	HKEY hKey;
	TCHAR ros_reg[MAX_PATH] = { 0 };
	if (IsWow64())
		_tcscpy(ros_reg, TEXT("SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\uninstall"));
	else
		_tcscpy(ros_reg, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Serviceuse"));
	if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, ros_reg, 0L, KEY_ALL_ACCESS, &hKey))
	{
		if (ERROR_SUCCESS != RegDeleteKey(hKey, _T("Serviceuse")))
		{
			TCHAR errcode[128] = { 0 };
			_stprintf(errcode, TEXT("SvcDeleteReg(c),errcode = %d"), GetLastError());
			SvcReportEvent(errcode);
			SvcReportEvent(TEXT("SvcDeleteReg(c)"));
		}
	}
	RegCloseKey(hKey);

	// Open the local default service control manager database
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (schSCManager == NULL)
	{
		wprintf(L"OpenSCManager failed w/err 0x%08lx\n", GetLastError());
		goto Cleanup;
	}

	// Open the service with delete, stop, and query status permissions
	schService = OpenService(schSCManager, pszServiceName, SERVICE_STOP |
		SERVICE_QUERY_STATUS | DELETE);
	if (schService == NULL)
	{
		wprintf(L"OpenService failed w/err 0x%08lx\n", GetLastError());
		goto Cleanup;
	}

	// Try to stop the service
	if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
	{
		wprintf(L"Stopping %s.", pszServiceName);
		Sleep(1000);

		while (QueryServiceStatus(schService, &ssSvcStatus))
		{
			if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
			{
				wprintf(L".");
				Sleep(1000);
			}
			else break;
		}

		if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
			wprintf(L"\n%s is stopped.\n", pszServiceName);
		else
			wprintf(L"\n%s failed to stop.\n", pszServiceName);
	}

	// Now remove the service by calling DeleteService.
	if (!DeleteService(schService))
	{
		wprintf(L"DeleteService failed w/err 0x%08lx\n", GetLastError());
		goto Cleanup;
	}
	wprintf(L"%s is removed.\n", pszServiceName);

Cleanup:
	// Centralized cleanup for all allocated resources.
	if (schSCManager)
	{
		CloseServiceHandle(schSCManager);
		schSCManager = NULL;
	}
	if (schService)
	{
		CloseServiceHandle(schService);
		schService = NULL;
	}
}

VOID SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			0,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}

VOID SvcInstall()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		SvcReportEvent(TEXT("SvcInstall(a)"));
		return;
	}

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		SvcReportEvent(TEXT("SvcInstall(b)"));
		return;
	}

	// Create the service

	schService = CreateService(
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
		SVCNAME,                   // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_AUTO_START,      // start type SERVICE_DEMAND_START | SERVICE_BOOT_START | SERVICE_SYSTEM_START |
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL)
	{
		CloseServiceHandle(schSCManager);
		UninstallService(SVCNAME);
		schSCManager = OpenSCManager(
			NULL,                    // local computer
			NULL,                    // ServicesActive database 
			SC_MANAGER_ALL_ACCESS);  // full access rights 
		schService = CreateService(
			schSCManager,              // SCM database 
			SVCNAME,                   // name of service 
			SVCNAME,                   // service name to display 
			SERVICE_ALL_ACCESS,        // desired access 
			SERVICE_WIN32_OWN_PROCESS, // service type 
			SERVICE_AUTO_START,      // start type SERVICE_DEMAND_START | SERVICE_BOOT_START | SERVICE_SYSTEM_START |
			SERVICE_ERROR_NORMAL,      // error control type 
			szPath,                    // path to service's binary 
			NULL,                      // no load ordering group 
			NULL,                      // no tag identifier 
			NULL,                      // no dependencies 
			NULL,                      // LocalSystem account 
			NULL);                     // no password 
		if (schService == NULL)
		{
			TCHAR errcode[128] = { 0 };
			_stprintf(errcode, TEXT("SvcInstall(c),errcode = %d"), GetLastError());
			SvcReportEvent(errcode);
			SvcReportEvent(TEXT("SvcInstall(c)"));
			CloseServiceHandle(schSCManager);
			return;
		}
	}

	write_ros_reg();
	
	if (StartService(schService, 0, NULL) != TRUE)
	{
		TCHAR errcode[128] = { 0 };
		_stprintf(errcode, TEXT("SvcStartService(c),errcode = %d"), GetLastError());
		SvcReportEvent(errcode);
		SvcReportEvent(TEXT("SvcStartService(c)"));
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}


int _tmain(int argc, WCHAR* argv[])
{
	if (lstrcmpi(argv[1], TEXT("remove")) == 0)
	{
		UninstallService(SVCNAME);
		return 0 ;
	}
	if (lstrcmpi(argv[1], TEXT("install")) == 0)
	{
		SvcInstall();
		return 0;
	}
	// TO_DO: Add any additional services for the process to this table.
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
		{ NULL, NULL }
	};

	if (!StartServiceCtrlDispatcher(DispatchTable))
		SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));

	return 0;
}


// CrashHandler.cpp : Defines the exported functions for the DLL application.
//

#include <windows.h>
#include <dbghelp.h>
#include "CrashHandler.h"

#pragma comment ( lib, "dbghelp.lib" )

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

WCHAR g_fileName[MAX_PATH + 1];
LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter;
bool g_isDotNet;
HANDLE g_hProcess;
DWORD g_processId;

LONG WINAPI ExceptionHandler(_In_ struct _EXCEPTION_POINTERS *ep)
{
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = GetCurrentThreadId();
	mei.ExceptionPointers = ep;
	mei.ClientPointers = FALSE;

	auto hFile = CreateFile(
		g_fileName,
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		EXCEPTION_CONTINUE_SEARCH;

	DWORD dumpType = MiniDumpNormal;
	if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW)
	{
		dumpType = MiniDumpWithFullMemory
			| MiniDumpWithHandleData
			| MiniDumpWithThreadInfo
			| MiniDumpWithProcessThreadData
			| MiniDumpWithFullMemoryInfo
			| MiniDumpWithUnloadedModules
			| MiniDumpWithFullAuxiliaryState
			| MiniDumpIgnoreInaccessibleMemory
			| MiniDumpWithTokenInformation;
	}

	MiniDumpWriteDump(
		g_hProcess,
		g_processId,
		hFile,
		(MINIDUMP_TYPE)dumpType,
		&mei,
		nullptr,
		nullptr);

	CloseHandle(hFile);
	TerminateProcess(GetCurrentProcess(), 1);
	return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI VectoredExceptionHandler(_In_ struct _EXCEPTION_POINTERS *ep)
{
	if (g_isDotNet)
		return ExceptionHandler(ep);
	auto ec = ep->ExceptionRecord->ExceptionCode;
	switch (ec)
	{
	case EXCEPTION_STACK_OVERFLOW:
	case EXCEPTION_ACCESS_VIOLATION:
		ExceptionHandler(ep);
		return EXCEPTION_CONTINUE_SEARCH;
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

CRASHHANDLER_API void ConfigureUnhandledExceptionHandler(LPCWSTR dumpFileName, bool isDotNet)
{
	memset(g_fileName, 0, sizeof(WCHAR)*(MAX_PATH + 1));
	wcsncpy_s(g_fileName, dumpFileName, MAX_PATH);
	g_isDotNet = isDotNet;
	g_hProcess = GetCurrentProcess();
	g_processId = GetCurrentProcessId();
	SetErrorMode(SEM_NOGPFAULTERRORBOX);
	AddVectoredExceptionHandler(1, VectoredExceptionHandler);
	g_prevFilter = SetUnhandledExceptionFilter(ExceptionHandler);
}

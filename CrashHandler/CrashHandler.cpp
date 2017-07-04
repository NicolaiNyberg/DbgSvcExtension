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

const DWORD DefaultDumpType = MiniDumpWithFullMemory
							| MiniDumpWithHandleData
							| MiniDumpWithThreadInfo
							| MiniDumpWithProcessThreadData
							| MiniDumpWithFullMemoryInfo
							| MiniDumpWithUnloadedModules
							| MiniDumpWithFullAuxiliaryState
							| MiniDumpIgnoreInaccessibleMemory
							| MiniDumpWithTokenInformation;

WCHAR g_fileName[MAX_PATH + 1];
LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter;
bool g_failInVectoredHandler;
HANDLE g_hProcess;
DWORD g_processId;
HANDLE g_dumpEvent;
HANDLE g_dumpThread;
DWORD g_dumpThreadId;
volatile bool g_nothingToDo = false;
MINIDUMP_EXCEPTION_INFORMATION g_mei;
HANDLE g_vectoredHandlerHandle;
pfnExceptionCallback g_exceptionCallback;
DWORD g_dumpType;

LONG WINAPI ExceptionHandler(_In_ struct _EXCEPTION_POINTERS *ep)
{
	g_mei.ThreadId = GetCurrentThreadId();
	g_mei.ExceptionPointers = ep;
	g_mei.ClientPointers = FALSE;
	SetEvent(g_dumpEvent);
	WaitForSingleObject(g_dumpThread, INFINITE);
	TerminateProcess(g_hProcess, 1);
	return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI VectoredExceptionHandler(_In_ struct _EXCEPTION_POINTERS *ep)
{
	auto ec = ep->ExceptionRecord->ExceptionCode;
	if (g_failInVectoredHandler && (ec==EXCEPTION_STACK_OVERFLOW || ec==EXCEPTION_ACCESS_VIOLATION))
		return ExceptionHandler(ep);
	return EXCEPTION_CONTINUE_SEARCH;
}

HANDLE CreateManualResetEvent(LPCWSTR name, BOOL initialState)
{
	return CreateEvent(nullptr, TRUE, initialState, name);
}

DWORD WINAPI DumpThread(LPVOID param)
{
	WaitForSingleObject(g_dumpEvent, INFINITE);
	if (g_nothingToDo)
		return 0;

	auto hFile = CreateFile(
		g_fileName,
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;

	auto exceptionCode = g_mei.ExceptionPointers->ExceptionRecord->ExceptionCode;

	MiniDumpWriteDump(
		g_hProcess,
		g_processId,
		hFile,
		(MINIDUMP_TYPE)g_dumpType,
		&g_mei,
		nullptr,
		nullptr);
	CloseHandle(hFile);

	if (g_exceptionCallback != nullptr)
		g_exceptionCallback(exceptionCode);
	return 0;
}

CRASHHANDLER_API void ConfigureUnhandledExceptionHandler(LPCWSTR dumpFileName, DWORD dumpType, bool failInVectoredHandler, pfnExceptionCallback exceptionCallback)
{
	memset(g_fileName, 0, sizeof(WCHAR)*(MAX_PATH + 1));
	wcsncpy_s(g_fileName, dumpFileName, MAX_PATH);
	g_failInVectoredHandler = failInVectoredHandler;
	g_hProcess = GetCurrentProcess();
	g_processId = GetCurrentProcessId();
	g_dumpEvent = CreateManualResetEvent(L"dumpEvent", FALSE);
	g_dumpThread = CreateThread(nullptr, 0, DumpThread, nullptr, 0, &g_dumpThreadId);
	g_dumpType = dumpType == 0xffffffff ? DefaultDumpType : dumpType;
	SetErrorMode(SEM_NOGPFAULTERRORBOX);
	g_exceptionCallback = exceptionCallback;
	g_vectoredHandlerHandle = AddVectoredExceptionHandler(1, VectoredExceptionHandler);
	g_prevFilter = SetUnhandledExceptionFilter(ExceptionHandler);
}

CRASHHANDLER_API void RemoveExceptionHandlers()
{
	RemoveVectoredExceptionHandler(g_vectoredHandlerHandle);
	SetUnhandledExceptionFilter(g_prevFilter);
	g_nothingToDo = true;
	SetEvent(g_dumpEvent);
	WaitForSingleObject(g_dumpThread, INFINITE);
	CloseHandle(g_dumpThread);
	CloseHandle(g_dumpEvent);
	g_exceptionCallback = nullptr;
}

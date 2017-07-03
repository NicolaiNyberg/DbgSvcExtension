#include <windows.h>
#include <dbghelp.h>
#include <algorithm>

#pragma comment ( lib, "dbghelp.lib" )

const int SmallSize = 10240;

struct Small
{
	byte Bytes[SmallSize];
};

void StackOverflow()
{
#ifdef DEBUG
	Small b;
	b.Bytes[SmallSize - 1] = 1;
#else
	// in Release build the compiler realizes that the above construct with Small b is not actually used
	// hence we need to trigger a StackoverFlow exception by trying to allocate a too big amount of memory using the runtime
	_alloca(SmallSize);
#endif
	StackOverflow();
}

void AccessViolation()
{
	unsigned long * p = nullptr;
	*p = 1;
}

LONG WINAPI ExceptionHandler(_In_ struct _EXCEPTION_POINTERS *ep)
{
	auto hFile = CreateFile(
		L"Soec.Cseh.exe.dmp",
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		EXCEPTION_CONTINUE_SEARCH;

	auto hProcess = GetCurrentProcess();
	auto dwProcessId = GetCurrentProcessId();
	auto dwThreadId = GetCurrentThreadId();

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
	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = dwThreadId;
	mei.ExceptionPointers = ep;
	mei.ClientPointers = FALSE;

	MiniDumpWriteDump(
		hProcess,
		dwProcessId,
		hFile,
		(MINIDUMP_TYPE)dumpType,
		&mei,
		nullptr,
		nullptr);

	CloseHandle(hFile);
	TerminateProcess(GetCurrentProcess(), 1);
	return EXCEPTION_EXECUTE_HANDLER;
}

int main(WCHAR** args, int argc)
{
	SetUnhandledExceptionFilter(ExceptionHandler);
	if (argc>1)
		StackOverflow();
	else
		AccessViolation();
	return 0;
}

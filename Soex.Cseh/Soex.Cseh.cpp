#include <windows.h>
#include <dbghelp.h>

#pragma comment ( lib, "dbghelp.lib" )

const int SmallSize = 1024;

struct Small
{
	byte Bytes[SmallSize];
};

static int g_check;

void StackOverflow()
{
	g_check = 1;
	Small b;
	b.Bytes[SmallSize - 1] = 1;
	StackOverflow();
}

HANDLE OpenDumpFile()
{
	return CreateFile(
		L"Soec.Cseh.exe.dmp",
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
}

BOOL CALLBACK MyMiniDumpCallback(
	PVOID                            pParam,
	const PMINIDUMP_CALLBACK_INPUT   pInput,
	PMINIDUMP_CALLBACK_OUTPUT        pOutput
)
{
	BOOL bRet = FALSE;
	// Check parameters 
	if (pInput == 0)
		return FALSE;
	if (pOutput == 0)
		return FALSE;

	// Process the callbacks 
	switch (pInput->CallbackType)
	{
		case IncludeModuleCallback:
		case IncludeThreadCallback:
		case ThreadCallback:
		case ThreadExCallback:
		case MemoryCallback:
		case ModuleCallback:
			bRet = TRUE;
			break;

		case CancelCallback:
			break;
	}

	return bRet;

}

BOOL CreateMaxiDump(HANDLE hFile, EXCEPTION_POINTERS* ep)
{
	auto hProcess = GetCurrentProcess();
	auto dwProcessId = GetCurrentProcessId();
	auto dwThreadId = GetCurrentThreadId();

	DWORD maxiDump = MiniDumpWithFullMemory
		| MiniDumpWithFullMemoryInfo
		| MiniDumpWithHandleData
		| MiniDumpWithThreadInfo
		| MiniDumpWithUnloadedModules;

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = dwThreadId;
	mei.ExceptionPointers = ep;
	mei.ClientPointers = FALSE;

	return MiniDumpWriteDump(
		hProcess,
		dwProcessId,
		hFile,
		(MINIDUMP_TYPE)maxiDump,
		&mei,
		nullptr,
		nullptr);
}

BOOL CreateMidiDump(HANDLE hFile, EXCEPTION_POINTERS* ep)
{
	auto hProcess = GetCurrentProcess();
	auto dwProcessId = GetCurrentProcessId();
	auto dwThreadId = GetCurrentThreadId();

	DWORD midiDump = MiniDumpWithPrivateReadWriteMemory
		| MiniDumpWithDataSegs
		| MiniDumpWithHandleData
		| MiniDumpWithFullMemoryInfo
		| MiniDumpWithThreadInfo
		| MiniDumpWithUnloadedModules;

	MINIDUMP_EXCEPTION_INFORMATION mei;
	mei.ThreadId = dwThreadId;
	mei.ExceptionPointers = ep;
	mei.ClientPointers = FALSE;

	MINIDUMP_CALLBACK_INFORMATION mci;

	mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback;
	mci.CallbackParam = 0;

	return MiniDumpWriteDump(
		hProcess,
		dwProcessId,
		hFile,
		(MINIDUMP_TYPE)midiDump,
		&mei,
		nullptr,
		&mci);
}

void CreateMiniDump(EXCEPTION_POINTERS* ep)
{
	auto hFile = OpenDumpFile();
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	CreateMidiDump(hFile, ep);
	CloseHandle(hFile);
}

LONG WINAPI SehHandler(_In_ struct _EXCEPTION_POINTERS *ep)
{
	CreateMiniDump(ep);
	TerminateProcess(GetCurrentProcess(), 1);
	return EXCEPTION_EXECUTE_HANDLER;
}

void SetupSeh()
{
	SetUnhandledExceptionFilter(SehHandler);
}

int main()
{
	SetupSeh();
	g_check = 0;
	StackOverflow();
	return 0;
}

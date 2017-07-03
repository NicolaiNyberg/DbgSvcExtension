#include <windows.h>
#include "..\CrashHandler\CrashHandler.h"
#include <algorithm>

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

void OnException(DWORD code)
{
	printf("Handled exception: %x", code);
}

int main()
{
	ConfigureUnhandledExceptionHandler(L"SoexCh.C.exe.dmp", 0xffffffff, false, OnException);
	AccessViolation();
	//StackOverflow();
    return 0;
}


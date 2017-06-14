#include <windows.h>

const int SmallSize = 10240;

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

int main()
{
	g_check = 0;
	Sleep(1 * 1000); // this is enough to give DbgSvc time to attach to the process
	StackOverflow();
    return 0;
}


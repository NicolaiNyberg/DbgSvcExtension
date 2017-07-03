#include <windows.h>
#include <metahost.h>
#pragma comment(lib, "mscoree.lib")
#include "..\CrashHandler\CrashHandler.h"
#include <cstdio>

struct Context
{
	WCHAR configFile[MAX_PATH];
	STARTUP_FLAGS startupFlags;
	ICLRMetaHost * metaHost;
	ICLRRuntimeInfo * rt;
	ICLRRuntimeHost * host;
	bool isRuntimeStarted;

	Context() :
	    startupFlags(),
		metaHost(nullptr),
		rt(nullptr),
		host(nullptr),
		isRuntimeStarted(false)
	{
		memset(configFile, 0, MAX_PATH * sizeof(WCHAR));
	}

	~Context()
	{
		if (isRuntimeStarted) host->Stop();
		if (host) host->Release();
		if (rt) rt->Release();
		if (metaHost) rt->Release();
	}
};

void DisableFlag(Context * c, STARTUP_FLAGS f)
{
	c->startupFlags = (STARTUP_FLAGS)(c->startupFlags & (~f));
}

void EnableFlag(Context * c, STARTUP_FLAGS f)
{
	c->startupFlags = (STARTUP_FLAGS)(c->startupFlags | f);
}


HRESULT GetMetaHost(Context * c)
{
	return CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&c->metaHost);
}

HRESULT GetRuntime(Context * c)
{
	return c->metaHost->GetRuntime(L"v4.0.30319", IID_ICLRRuntimeInfo, (LPVOID*)&c->rt);
}

HRESULT GetHost(Context * c)
{
	return c->rt->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (LPVOID*)&c->host);
}


HRESULT GetDefaultFlags(Context * c)
{
	DWORD maxSize = MAX_PATH;
	return c->rt->GetDefaultStartupFlags((DWORD*)&c->startupFlags, c->configFile, &maxSize);
}

HRESULT SetDefaultFlags(Context * c)
{
	LPCWSTR pcf = lstrlen(c->configFile) == 0
		? nullptr
		: c->configFile;
	return c->rt->SetDefaultStartupFlags(c->startupFlags, pcf);
}

HRESULT StartRuntime(Context * c)
{
	auto hr = c->host->Start();
	c->isRuntimeStarted = hr == S_OK;
	return hr;
}

HRESULT ExecuteInDefaultAppDomain(
	Context * c,
	LPCWSTR assembly,
	LPCWSTR typeName,
	LPCWSTR methodName,
	LPCWSTR arguments
)
{
	DWORD ret = 0;
	return c->host->ExecuteInDefaultAppDomain(assembly, typeName, methodName, arguments, &ret );
}

void OnException(DWORD code)
{
	printf("Handled exception: %x", code);
}

int main(WCHAR** args, int argc)
{
	{
		Context c;
		auto hr = GetMetaHost(&c);
		if (!hr) hr = GetRuntime(&c);
		if (!hr) hr = GetDefaultFlags(&c);
		if (hr) goto exit;
		EnableFlag(&c, STARTUP_SERVER_GC);
		hr = SetDefaultFlags(&c); 
		if (!hr) hr = GetHost(&c);
		if (!hr) hr = StartRuntime(&c);
		ConfigureUnhandledExceptionHandler(L"HostClr.dmp", 0xffffffff, true, OnException);
		//if (!hr) hr = ExecuteInDefaultAppDomain(&c, L"HwCs.exe", L"HwCs.Program", L"MainWrapper", L"is this working");
		if (!hr) hr = ExecuteInDefaultAppDomain(&c, L"Soex.Csharp.exe", L"Soex.Csharp.Program", L"MainWrapper", L"");
	}
exit:
    return 0;
}

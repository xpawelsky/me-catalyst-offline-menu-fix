#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "hook.h"

#pragma comment(lib, "ws2_32.lib")

static HMODULE g_hThisModule = NULL;
static HMODULE g_hOriginalDll = NULL;
static void* (*g_pfnOriginalGetBuildInfo)(void) = NULL;
static volatile LONG g_HooksInitialized = 0;

/* --- Case-insensitive substring search helpers --- */

static int contains_icase_a(const char* haystack, const char* needle) {
    if (!haystack || !needle) return 0;
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        if (_strnicmp(&haystack[i], needle, nlen) == 0) {
            return 1;
        }
    }
    return 0;
}

static int contains_icase_w(const wchar_t* haystack, const wchar_t* needle) {
    if (!haystack || !needle) return 0;
    size_t hlen = wcslen(haystack);
    size_t nlen = wcslen(needle);
    if (nlen > hlen) return 0;
    for (size_t i = 0; i <= hlen - nlen; i++) {
        if (_wcsnicmp(&haystack[i], needle, nlen) == 0) {
            return 1;
        }
    }
    return 0;
}

/* --- Dynamic Forwarder for Engine.BuildInfo_Win64_retail_original.dll --- */

static void LoadOriginalDll(void) {
    if (g_hOriginalDll) return;

    char dllPath[MAX_PATH];
    if (GetModuleFileNameA(g_hThisModule, dllPath, MAX_PATH)) {
        char* lastSlash = strrchr(dllPath, '\\');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';
            strncat(dllPath, "Engine.BuildInfo_Win64_retail_original.dll", MAX_PATH - strlen(dllPath) - 1);
            g_hOriginalDll = LoadLibraryA(dllPath);
        }
    }

    if (!g_hOriginalDll) {
        g_hOriginalDll = LoadLibraryA("Engine.BuildInfo_Win64_retail_original.dll");
    }

    if (g_hOriginalDll) {
        g_pfnOriginalGetBuildInfo = (void* (*)(void))GetProcAddress(g_hOriginalDll, "getBuildInfo");
    } else {
        OutputDebugStringA("[MEC-Shim] ERROR: Failed to load Engine.BuildInfo_Win64_retail_original.dll\n");
    }
}

__declspec(dllexport) void* getBuildInfo(void) {
    if (!g_pfnOriginalGetBuildInfo) {
        LoadOriginalDll();
    }
    if (g_pfnOriginalGetBuildInfo) {
        return g_pfnOriginalGetBuildInfo();
    }
    return NULL;
}

/* --- Winsock Hook Targets & Detours --- */

typedef int (WSAAPI *pfn_getaddrinfo)(
    PCSTR pNodeName,
    PCSTR pServiceName,
    const ADDRINFOA *pHints,
    PADDRINFOA *ppResult
);
static pfn_getaddrinfo original_getaddrinfo = NULL;

typedef int (WSAAPI *pfn_GetAddrInfoW)(
    PCWSTR pNodeName,
    PCWSTR pServiceName,
    const ADDRINFOW *pHints,
    PADDRINFOW *ppResult
);
static pfn_GetAddrInfoW original_GetAddrInfoW = NULL;

typedef int (WSAAPI *pfn_connect)(
    SOCKET s,
    const struct sockaddr *name,
    int namelen
);
static pfn_connect original_connect = NULL;

typedef int (WSAAPI *pfn_WSAConnect)(
    SOCKET s,
    const struct sockaddr *name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS
);
static pfn_WSAConnect original_WSAConnect = NULL;

static int IsTargetServer(const struct sockaddr *name, int namelen) {
    if (!name || namelen < (int)sizeof(struct sockaddr_in)) {
        return 0;
    }
    if (name->sa_family == AF_INET) {
        const struct sockaddr_in *sin = (const struct sockaddr_in *)name;
        unsigned long target_ip = inet_addr("159.153.51.20");
        unsigned short target_port = htons(42127);

        if (sin->sin_port == target_port || sin->sin_addr.s_addr == target_ip) {
            return 1;
        }
    }
    return 0;
}

int WSAAPI Hooked_getaddrinfo(
    PCSTR pNodeName,
    PCSTR pServiceName,
    const ADDRINFOA *pHints,
    PADDRINFOA *ppResult
) {
    if (pNodeName) {
        if (contains_icase_a(pNodeName, "gosredirector") ||
            contains_icase_a(pNodeName, "winter15") ||
            (contains_icase_a(pNodeName, "gos") && contains_icase_a(pNodeName, "ea.com"))) {
            OutputDebugStringA("[MEC-Shim] Blocked getaddrinfo: ");
            OutputDebugStringA(pNodeName);
            OutputDebugStringA(" -> WSAHOST_NOT_FOUND (fail-fast)\n");
            if (ppResult) *ppResult = NULL;
            WSASetLastError(WSAHOST_NOT_FOUND);
            return EAI_NONAME;
        }
    }
    return original_getaddrinfo(pNodeName, pServiceName, pHints, ppResult);
}

int WSAAPI Hooked_GetAddrInfoW(
    PCWSTR pNodeName,
    PCWSTR pServiceName,
    const ADDRINFOW *pHints,
    PADDRINFOW *ppResult
) {
    if (pNodeName) {
        if (contains_icase_w(pNodeName, L"gosredirector") ||
            contains_icase_w(pNodeName, L"winter15") ||
            (contains_icase_w(pNodeName, L"gos") && contains_icase_w(pNodeName, L"ea.com"))) {
            OutputDebugStringW(L"[MEC-Shim] Blocked GetAddrInfoW: ");
            OutputDebugStringW(pNodeName);
            OutputDebugStringW(L" -> WSAHOST_NOT_FOUND (fail-fast)\n");
            if (ppResult) *ppResult = NULL;
            WSASetLastError(WSAHOST_NOT_FOUND);
            return EAI_NONAME;
        }
    }
    return original_GetAddrInfoW(pNodeName, pServiceName, pHints, ppResult);
}

int WSAAPI Hooked_connect(
    SOCKET s,
    const struct sockaddr *name,
    int namelen
) {
    if (IsTargetServer(name, namelen)) {
        OutputDebugStringA("[MEC-Shim] Blocked connect to GOS server -> WSAECONNREFUSED (fail-fast)\n");
        WSASetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }
    return original_connect(s, name, namelen);
}

int WSAAPI Hooked_WSAConnect(
    SOCKET s,
    const struct sockaddr *name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS
) {
    if (IsTargetServer(name, namelen)) {
        OutputDebugStringA("[MEC-Shim] Blocked WSAConnect to GOS server -> WSAECONNREFUSED (fail-fast)\n");
        WSASetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }
    return original_WSAConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
}

/* --- Hook Initialization --- */

static void InstallHooks(void) {
    if (InterlockedCompareExchange(&g_HooksInitialized, 1, 0) != 0) {
        return;
    }

    HMODULE hWs2 = LoadLibraryA("ws2_32.dll");
    if (!hWs2) {
        OutputDebugStringA("[MEC-Shim] ws2_32.dll not available\n");
        return;
    }

    if (!hook_init()) {
        OutputDebugStringA("[MEC-Shim] hook_init failed\n");
        return;
    }

    void* pGetAddrInfo = (void*)GetProcAddress(hWs2, "getaddrinfo");
    if (pGetAddrInfo) {
        hook_install(pGetAddrInfo, (void*)Hooked_getaddrinfo, (void**)&original_getaddrinfo);
    }

    void* pGetAddrInfoW = (void*)GetProcAddress(hWs2, "GetAddrInfoW");
    if (pGetAddrInfoW) {
        hook_install(pGetAddrInfoW, (void*)Hooked_GetAddrInfoW, (void**)&original_GetAddrInfoW);
    }

    void* pConnect = (void*)GetProcAddress(hWs2, "connect");
    if (pConnect) {
        hook_install(pConnect, (void*)Hooked_connect, (void**)&original_connect);
    }

    void* pWSAConnect = (void*)GetProcAddress(hWs2, "WSAConnect");
    if (pWSAConnect) {
        hook_install(pWSAConnect, (void*)Hooked_WSAConnect, (void**)&original_WSAConnect);
    }

    OutputDebugStringA("[MEC-Shim] Network fail-fast hooks successfully installed\n");
}

static DWORD WINAPI InitHooksThread(LPVOID lpParam) {
    (void)lpParam;
    InstallHooks();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)lpvReserved;
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            g_hThisModule = hinstDLL;
            {
                HANDLE hThread = CreateThread(NULL, 0, InitHooksThread, NULL, 0, NULL);
                if (hThread) {
                    CloseHandle(hThread);
                }
            }
            break;

        case DLL_PROCESS_DETACH:
            hook_cleanup();
            if (g_hOriginalDll) {
                FreeLibrary(g_hOriginalDll);
                g_hOriginalDll = NULL;
            }
            break;
    }
    return TRUE;
}


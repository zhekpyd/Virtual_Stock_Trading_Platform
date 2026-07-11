#include "utils/http_client.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <iostream>

#pragma comment(lib, "winhttp.lib")

std::string http_get(const std::string& url) {
    std::string result;

    // 解析 URL
    std::wstring wurl(url.begin(), url.end());
    URL_COMPONENTS uc = {0};
    uc.dwStructSize = sizeof(uc);

    wchar_t host[256] = {0}, path[1024] = {0};
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = 1024;

    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &uc)) return "";

    HINTERNET session = WinHttpOpen(L"Mozilla/5.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return "";

    HINTERNET connect = WinHttpConnect(session, host, uc.nPort, 0);
    if (!connect) { WinHttpCloseHandle(session); return ""; }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", path, NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) { WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return ""; }

    WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    WinHttpReceiveResponse(request, NULL);

    DWORD size = 0, downloaded = 0;
    do {
        size = 0;
        WinHttpQueryDataAvailable(request, &size);
        if (size == 0) break;
        char* buf = new char[size + 1];
        WinHttpReadData(request, buf, size, &downloaded);
        result.append(buf, downloaded);
        delete[] buf;
    } while (size > 0);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return result;
}
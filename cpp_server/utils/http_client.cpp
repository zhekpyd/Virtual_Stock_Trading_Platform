#include "utils/http_client.h"
#include <string>

#ifdef _WIN32
    // ==================== Windows 版本 (WinInet) ====================
    #include <windows.h>
    #include <wininet.h>
    #pragma comment(lib, "wininet.lib")

    std::string http_get(const std::string& url) {
        std::string result;

        URL_COMPONENTSA uc = { 0 };
        uc.dwStructSize = sizeof(uc);
        char host[256] = { 0 }, path[4096] = { 0 };
        uc.lpszHostName = host;
        uc.dwHostNameLength = 256;
        uc.lpszUrlPath = path;
        uc.dwUrlPathLength = 4096;

        if (!InternetCrackUrlA(url.c_str(), 0, 0, &uc)) return "";

        HINTERNET session = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT,
                                           NULL, NULL, 0);
        if (!session) return "";

        HINTERNET conn = InternetConnectA(session, host, uc.nPort, NULL, NULL,
                                           INTERNET_SERVICE_HTTP, 0, 0);
        if (!conn) { InternetCloseHandle(session); return ""; }

        DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ?
                      INTERNET_FLAG_SECURE : 0;

        HINTERNET req = HttpOpenRequestA(conn, "GET", path, NULL, NULL, NULL,
                                          flags | INTERNET_FLAG_RELOAD, 0);
        if (!req) { InternetCloseHandle(conn); InternetCloseHandle(session); return ""; }

        const char* headers = "Referer: https://finance.sina.com.cn\r\n";
        HttpSendRequestA(req, headers, -1, NULL, 0);

        char buf[4096];
        DWORD read = 0;
        while (InternetReadFile(req, buf, sizeof(buf) - 1, &read) && read > 0) {
            buf[read] = '\0';
            result += buf;
        }

        InternetCloseHandle(req);
        InternetCloseHandle(conn);
        InternetCloseHandle(session);
        return result;
    }

#else
    // ==================== Linux 版本 (libcurl) ====================
    #include <curl/curl.h>

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    std::string http_get(const std::string& url) {
        std::string result;
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Referer: https://finance.sina.com.cn");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return result;
    }
#endif
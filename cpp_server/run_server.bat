@echo off
set "PATH=%~dp0vcpkg_installed\x64-mingw-dynamic\bin;%PATH%"
xcopy dependencies\ build\ /s /y /q

REM 启动 Ollama（新窗口，不阻塞）
start "Ollama" /min "C:\Users\24264\AppData\Local\Programs\Ollama\ollama.exe" serve

REM 等 Ollama 就绪
echo 等待 Ollama 启动...
timeout /t 3 /nobreak >nul

REM 启动服务器
.\build\StockServer.exe

REM 服务器关了，停 Ollama
taskkill /f /im ollama.exe >nul 2>&1
pause
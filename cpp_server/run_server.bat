@echo off
set "PATH=%~dp0vcpkg_installed\x64-mingw-dynamic\bin;%PATH%"
xcopy dependencies\ build\ /s /y /q
.\build\StockServer.exe
pause
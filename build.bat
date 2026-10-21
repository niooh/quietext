@echo off
setlocal
cd /d "%~dp0"

if not exist dist mkdir dist

g++ -O2 -municode -mwindows -static ^
    -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
    -Isrc ^
    src\main.cpp src\editor.cpp src\encoding.cpp src\fileio.cpp ^
    -o dist\quietext.exe ^
    -lcomdlg32 -lshell32

if errorlevel 1 exit /b 1
echo [build] Done: dist\quietext.exe

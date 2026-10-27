@echo off
setlocal
cd /d "%~dp0"

if not exist dist mkdir dist

windres -I src\assets src\assets\icon.rc -O coff -o dist\icon.o
if errorlevel 1 (
    echo [build] windres failed.
    exit /b 1
)

g++ -O2 -municode -mwindows -static ^
    -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
    -Isrc ^
    src\main.cpp src\editor.cpp src\encoding.cpp src\fileio.cpp ^
    dist\icon.o ^
    -o dist\quietext.exe ^
    -lcomdlg32 -lshell32

del dist\icon.o

if errorlevel 1 exit /b 1
echo [build] Done: dist\quietext.exe

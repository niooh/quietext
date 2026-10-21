@echo off
setlocal
cd /d "%~dp0"

set "EXE=%~dp0dist\quietext.exe"
if not exist "%EXE%" (
    echo [install] dist\quietext.exe not found. Run build.bat first.
    exit /b 1
)

set "TARGET=%LOCALAPPDATA%\quietext\bin"
if not exist "%TARGET%" mkdir "%TARGET%"

copy /Y "%EXE%" "%TARGET%\quietext.exe" >nul
if errorlevel 1 (
    echo [install] Copy failed.
    exit /b 1
)
echo [install] Installed to %TARGET%\quietext.exe

rem ---- probe for a usable PowerShell, in order ----
set "PSEXE="
powershell.exe -NoProfile -Command "exit 0" >nul 2>nul
if not errorlevel 1 (
    set "PSEXE=powershell.exe"
    goto :update_path
)
pwsh.exe -NoProfile -Command "exit 0" >nul 2>nul
if not errorlevel 1 (
    set "PSEXE=pwsh.exe"
    goto :update_path
)
if exist "%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" (
    set "PSEXE=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
    goto :update_path
)
goto :no_ps

:update_path
echo [install] Updating user PATH via %PSEXE%
set "TARGET_DIR=%TARGET%"
"%PSEXE%" -NoProfile -ExecutionPolicy Bypass -Command ^
  "$dir=$env:TARGET_DIR;" ^
  "$p=[Environment]::GetEnvironmentVariable('Path','User');" ^
  "if($null -eq $p){$p=''};" ^
  "if($p.Split(';') -contains $dir){" ^
  "  Write-Host '[install] Already in user PATH.'" ^
  "} else {" ^
  "  $np=($p.TrimEnd(';')+';'+$dir).TrimStart(';');" ^
  "  [Environment]::SetEnvironmentVariable('Path',$np,'User');" ^
  "  Write-Host '[install] Added to user PATH. Restart your terminal to apply.'" ^
  "}"
if errorlevel 1 (
    echo [install] Failed to update user PATH automatically.
    goto :manual
)
echo [install] Done. Open a new terminal and run: quietext
exit /b 0

:no_ps
echo [install] No usable PowerShell found (tried powershell.exe, pwsh.exe, System32 path).
goto :manual

:manual
echo.
echo [install] Add the following directory to your user PATH manually:
echo           %TARGET%
echo [install] (System Properties -^> Environment Variables -^> User variables -^> Path)
echo.
exit /b 1
@echo off
setlocal

set "TARGET=%LOCALAPPDATA%\quietext"
set "TARGET_DIR=%TARGET%\bin"

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
"%PSEXE%" -NoProfile -ExecutionPolicy Bypass -Command ^
  "$dir=$env:TARGET_DIR;" ^
  "$p=[Environment]::GetEnvironmentVariable('Path','User');" ^
  "if($null -ne $p){" ^
  "  $arr=@($p.Split(';') | Where-Object { $_ -and $_ -ne $dir });" ^
  "  [Environment]::SetEnvironmentVariable('Path',($arr -join ';'),'User');" ^
  "  Write-Host '[uninstall] Removed from user PATH.'" ^
  "}"
goto :remove_files

:no_ps
echo [uninstall] No usable PowerShell; please remove manually from user PATH:
echo             %TARGET_DIR%

:remove_files
if exist "%TARGET%" rmdir /S /Q "%TARGET%"
echo [uninstall] quietext files removed.
endlocal

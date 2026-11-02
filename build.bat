@echo off
rem =============================================================================
rem 128x64 mono LCD GUI project - one-click build (double-click to run)
rem
rem   build.bat                build      -> output\gui.ihx / gui.hex
rem   build.bat rebuild        clean + full rebuild
rem   build.bat clean          remove intermediate files, keep firmware
rem   build.bat distclean      remove intermediate files and firmware
rem   build.bat font           regenerate the CN + ASCII font tables
rem   build.bat size           show memory usage
rem
rem Requires: sdcc (+ packihx) in PATH. Optional: make, python (font only).
rem All logic lives in build.ps1 (ASCII only), this file just forwards to it.
rem =============================================================================

setlocal
set "ARGS=%*"
set "PS="

where powershell >nul 2>nul
if not errorlevel 1 set "PS=powershell"
if "%PS%"=="" (
    where pwsh >nul 2>nul
    if not errorlevel 1 set "PS=pwsh"
)

if "%PS%"=="" (
    echo.
    echo [build] ERROR: neither "powershell" nor "pwsh" was found in PATH.
    echo.
    pause
    exit /b 1
)

"%PS%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %ARGS%
set "RC=%ERRORLEVEL%"

if not "%RC%"=="0" (
    echo.
    echo [build] FAILED - see messages above
    echo.
    pause
    exit /b %RC%
)

endlocal

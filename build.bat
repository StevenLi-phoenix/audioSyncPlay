@echo off
setlocal

REM ========================================
REM Building Spotify Sync Audio
REM ========================================
echo ========================================
echo Building Spotify Sync Audio
echo ========================================

REM Remove old build directory
if exist build rmdir /s /q build

REM Create build directory
mkdir build

REM Configure project with MinGW Makefiles generator
cmake -S . -B build -G "MinGW Makefiles"
if errorlevel 1 goto error

REM Build project
cmake --build build
if errorlevel 1 goto error

echo.
echo [SUCCESS] Build completed!
goto end

:error
echo [ERROR] Build failed
:end
endlocal
pause

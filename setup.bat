@echo off
echo ========================================
echo Spotify Sync Audio - Development Setup
echo ========================================
echo.

echo Checking development environment...

REM Check if Visual Studio is installed
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Visual Studio compiler (cl.exe) not found in PATH
    echo Please install Visual Studio 2019 or later with C++ development tools
    echo Download: https://visualstudio.microsoft.com/downloads/
    echo.
) else (
    echo [OK] Visual Studio compiler found
)

REM Check if CMake is installed
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] CMake not found in PATH
    echo Please install CMake 3.15 or later
    echo Download: https://cmake.org/download/
    echo.
    echo After installing CMake, add it to your PATH and run this script again
    echo.
) else (
    echo [OK] CMake found
    cmake --version
    echo.
)

REM Check if Ninja is installed
where ninja >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Ninja build system not found
    echo Please install Ninja for faster builds
    echo Download: https://github.com/ninja-build/ninja/releases
    echo.
    echo After installing Ninja, add it to your PATH and run this script again
    echo.
) else (
    echo [OK] Ninja found
    ninja --version
    echo.
)

echo ========================================
echo Environment Check Complete
echo ========================================
echo.

if exist "build" (
    echo Found existing build directory
    echo.
    echo To build the project:
    echo   1. cmake -S . -B build -G "Ninja"
    echo   2. cmake --build build
    echo.
    echo To run the programs:
    echo   build\sender.exe
    echo   build\receiver.exe
    echo.
) else (
    echo No build directory found
    echo.
    echo After installing the required tools, run:
    echo   cmake -S . -B build -G "Ninja"
    echo   cmake --build build
    echo.
)

echo ========================================
echo Required Tools Summary
echo ========================================
echo 1. Visual Studio 2019+ with C++ tools
echo 2. CMake 3.15+
echo 3. Ninja (optional, for faster builds)
echo 4. VSCode with C++ extensions (recommended)
echo.
echo ========================================
pause

@echo off
echo ========================================
echo Building Spotify Sync Audio
echo ========================================
echo.

REM Check if CMake is available
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found. Please install CMake first.
    echo Download: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

REM Check if Ninja is available
where ninja >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Ninja not found, using default generator
    set GENERATOR="Visual Studio 16 2019"
) else (
    echo [INFO] Using Ninja generator for faster builds
    set GENERATOR="Ninja"
)

echo Creating build directory...
if not exist "build" mkdir build

echo Configuring project...
cmake -S . -B build -G %GENERATOR%
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed
    pause
    exit /b 1
)

echo Building project...
cmake --build build
if %errorlevel% neq 0 (
    echo [ERROR] Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Executables created:
if exist "build\sender.exe" (
    echo   - build\sender.exe
) else (
    echo   - build\Debug\sender.exe
)

if exist "build\receiver.exe" (
    echo   - build\receiver.exe
) else (
    echo   - build\Debug\receiver.exe
)

echo.
echo To run the programs:
echo   sender.exe --help
echo   receiver.exe --help
echo.
pause

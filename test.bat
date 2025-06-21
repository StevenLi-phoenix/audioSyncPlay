@echo off
echo ========================================
echo Spotify Sync Audio - Project Test
echo ========================================
echo.

echo Checking project structure...

REM Check if required directories exist
if not exist "src" (
    echo [ERROR] src directory not found
    goto :error
) else (
    echo [OK] src directory found
)

if not exist "include" (
    echo [ERROR] include directory not found
    goto :error
) else (
    echo [OK] include directory found
)

if not exist ".vscode" (
    echo [WARNING] .vscode directory not found
) else (
    echo [OK] .vscode directory found
)

echo.
echo Checking source files...

REM Check source files
set MISSING_FILES=0

if not exist "src\main.cpp" (
    echo [ERROR] src\main.cpp not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] src\main.cpp found
)

if not exist "src\sender.cpp" (
    echo [ERROR] src\sender.cpp not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] src\sender.cpp found
)

if not exist "src\receiver.cpp" (
    echo [ERROR] src\receiver.cpp not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] src\receiver.cpp found
)

if not exist "src\audio_capture.cpp" (
    echo [ERROR] src\audio_capture.cpp not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] src\audio_capture.cpp found
)

if not exist "src\network_udp.cpp" (
    echo [ERROR] src\network_udp.cpp not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] src\network_udp.cpp found
)

if not exist "src\jitter_buffer.cpp" (
    echo [ERROR] src\jitter_buffer.cpp not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] src\jitter_buffer.cpp found
)

if not exist "src\audio_playback.cpp" (
    echo [ERROR] src\audio_playback.cpp not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] src\audio_playback.cpp found
)

echo.
echo Checking header files...

REM Check header files
if not exist "include\audio_capture.h" (
    echo [ERROR] include\audio_capture.h not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] include\audio_capture.h found
)

if not exist "include\network_udp.h" (
    echo [ERROR] include\network_udp.h not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] include\network_udp.h found
)

if not exist "include\jitter_buffer.h" (
    echo [ERROR] include\jitter_buffer.h not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] include\jitter_buffer.h found
)

if not exist "include\audio_playback.h" (
    echo [ERROR] include\audio_playback.h not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] include\audio_playback.h found
)

echo.
echo Checking build files...

REM Check build files
if not exist "CMakeLists.txt" (
    echo [ERROR] CMakeLists.txt not found
    set /a MISSING_FILES+=1
) else (
    echo [OK] CMakeLists.txt found
)

if not exist "README.md" (
    echo [WARNING] README.md not found
) else (
    echo [OK] README.md found
)

if not exist "plan.md" (
    echo [WARNING] plan.md not found
) else (
    echo [OK] plan.md found
)

echo.
echo ========================================
echo Test Results
echo ========================================

if %MISSING_FILES% gtr 0 (
    echo [FAILED] %MISSING_FILES% required files are missing
    goto :error
) else (
    echo [SUCCESS] All required files found
    echo.
    echo Project structure is valid!
    echo.
    echo Next steps:
    echo 1. Run setup.bat to check your development environment
    echo 2. Run build.bat to compile the project
    echo 3. Open the project in VSCode for development
    echo.
)

goto :end

:error
echo.
echo [ERROR] Project test failed
echo Please ensure all required files are present
echo.

:end
pause

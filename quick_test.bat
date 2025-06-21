@echo off
echo ========================================
echo AudioSync Quick Test
echo ========================================

echo.
echo Step 1: Testing sender help...
build\sender.exe --help
if %errorlevel% neq 0 (
    echo ERROR: Sender help test failed!
    pause
    exit /b 1
)

echo.
echo Step 2: Testing receiver help...
build\receiver.exe --help
if %errorlevel% neq 0 (
    echo ERROR: Receiver help test failed!
    pause
    exit /b 1
)

echo.
echo Step 3: Starting receiver in background...
start "AudioSync Receiver" cmd /k "echo Starting receiver... && build\receiver.exe --port 8889 --stats-interval 2000"

echo.
echo Waiting 3 seconds for receiver to start...
timeout /t 3 /nobreak > nul

echo.
echo Step 4: Starting sender...
echo.
echo INSTRUCTIONS:
echo 1. The receiver should be running in another window
echo 2. This sender will capture your system audio
echo 3. Play some music or audio on your computer
echo 4. Watch the statistics in both windows
echo 5. Press Ctrl+C to stop the sender
echo.
echo Starting sender now...
build\sender.exe --ip 127.0.0.1 --port 8889 --stats-interval 2000

echo.
echo Test completed!
echo Closing receiver window...
taskkill /f /im cmd.exe /fi "WINDOWTITLE eq AudioSync Receiver*" > nul 2>&1

echo.
echo ========================================
echo Test Summary:
echo - Sender help: PASSED
echo - Receiver help: PASSED
echo - Network communication: TESTED
echo ========================================
pause

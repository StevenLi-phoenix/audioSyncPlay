@echo off
echo ========================================
echo Multi-Device Synchronization Test
echo ========================================
echo.

echo Building with clock synchronization...
call build.bat
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Testing Multi-Device Synchronization
echo ========================================
echo.

echo Starting sender...
start "Audio Sender" cmd /c "sender.exe --ip 127.0.0.1 --port 8889 --log-level INFO"

echo Waiting for sender to start...
timeout /t 2 /nobreak > nul

echo.
echo Starting receiver 1...
start "Audio Receiver 1" cmd /c "receiver.exe --port 8889 --log-level INFO"

echo Waiting for receiver 1 to start...
timeout /t 2 /nobreak > nul

echo.
echo Starting receiver 2...
start "Audio Receiver 2" cmd /c "receiver.exe --port 8889 --log-level INFO"

echo.
echo Multi-device test started!
echo.
echo Expected results:
echo - Both receivers should play synchronized audio
echo - Clock sync quality should improve over time
echo - Latency should be consistent across devices
echo - No audio dropouts or synchronization issues
echo.
echo Press any key to stop all processes...
pause

echo.
echo Stopping all processes...
taskkill /f /im sender.exe >nul 2>&1
taskkill /f /im receiver.exe >nul 2>&1

echo.
echo Test completed. Check the logs for synchronization quality.
echo.

pause

@echo off
echo ========================================
echo AudioSync Network Test
echo ========================================

echo.
echo Starting receiver in background...
start "AudioSync Receiver" cmd /k "build\receiver.exe --port 8889 --stats-interval 1000"

echo.
echo Waiting 3 seconds for receiver to start...
timeout /t 3 /nobreak > nul

echo.
echo Starting sender...
echo Press Ctrl+C to stop both sender and receiver
build\sender.exe --ip 127.0.0.1 --port 8889 --stats-interval 1000

echo.
echo Test completed.
echo Closing receiver window...
taskkill /f /im cmd.exe /fi "WINDOWTITLE eq AudioSync Receiver*" > nul 2>&1

pause

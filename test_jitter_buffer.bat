@echo off
echo ========================================
echo AudioSync Week 3 - Jitter Buffer Test
echo ========================================
echo.

echo [1/4] Building components...
cmake --build build --target sender
if %errorlevel% neq 0 (
    echo ERROR: Failed to build sender
    pause
    exit /b 1
)

cmake --build build --target receiver
if %errorlevel% neq 0 (
    echo ERROR: Failed to build receiver
    pause
    exit /b 1
)

echo [2/4] Starting receiver with jitter buffer...
start "AudioSync Receiver (Jitter Buffer)" cmd /k "build\receiver.exe --port 8889 --target-latency 100 --max-latency 200 --stats-interval 1000"

echo [3/4] Waiting for receiver to start...
timeout /t 3 /nobreak > nul

echo [4/4] Starting sender with sequence numbers...
echo.
echo Testing jitter buffer functionality:
echo - Sender includes sequence numbers in packets
echo - Receiver parses sequence numbers and timestamps
echo - Jitter buffer handles frame reordering
echo - Adaptive buffer adjustment based on jitter
echo - Real-time statistics display
echo.
echo Press Ctrl+C to stop the test
echo.

build\sender.exe --ip 127.0.0.1 --port 8889 --stats-interval 1000

echo.
echo Test completed. Check receiver window for jitter buffer statistics.
pause

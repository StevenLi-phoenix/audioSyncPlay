@echo off
echo ========================================
echo Audio Quality Test Script - Fixed Version
echo ========================================
echo.

echo Building with improved audio quality settings...
call build.bat
if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Testing Audio Quality Improvements
echo ========================================
echo.

echo Starting receiver in background...
start "Audio Receiver" cmd /c "receiver.exe --port 8889 --log-level INFO"

echo Waiting for receiver to start...
timeout /t 3 /nobreak > nul

echo.
echo Starting sender with high-quality audio...
echo Playing some music in Spotify or other audio source...
echo.
sender.exe --ip 127.0.0.1 --port 8889 --log-level INFO

echo.
echo Test completed. Check the logs for audio format information.
echo Expected improvements:
echo - Higher sample rate (48kHz instead of 44.1kHz)
echo - Higher bit depth (32-bit instead of 16-bit)
echo - Better audio quality with native format support
echo - Reduced latency (target: 50ms)
echo - Reduced frame drops
echo.

pause

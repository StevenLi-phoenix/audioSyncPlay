@echo off
echo ========================================
echo AudioSync Sender Test
echo ========================================

echo.
echo Testing sender help...
build\sender.exe --help

echo.
echo Testing sender with default configuration...
echo Press Ctrl+C to stop the sender
build\sender.exe --ip 127.0.0.1 --port 8888 --stats-interval 2000

echo.
echo Test completed.
pause

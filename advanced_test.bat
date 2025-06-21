@echo off
echo ========================================
echo AudioSync Advanced Test Suite
echo ========================================

:menu
echo.
echo Select test option:
echo 1. Quick network test (default)
echo 2. Performance test (high frame rate)
echo 3. Stress test (large buffer)
echo 4. Custom configuration test
echo 5. Audio device test
echo 6. Exit
echo.
set /p choice="Enter your choice (1-6): "

if "%choice%"=="1" goto quick_test
if "%choice%"=="2" goto performance_test
if "%choice%"=="3" goto stress_test
if "%choice%"=="4" goto custom_test
if "%choice%"=="5" goto audio_test
if "%choice%"=="6" goto exit
echo Invalid choice. Please try again.
goto menu

:quick_test
echo.
echo ========================================
echo Running Quick Network Test
echo ========================================
echo Starting receiver...
start "Receiver" cmd /k "build\receiver.exe --port 8889 --stats-interval 1000"
timeout /t 3 /nobreak > nul
echo Starting sender...
build\sender.exe --ip 127.0.0.1 --port 8889 --stats-interval 1000
goto cleanup

:performance_test
echo.
echo ========================================
echo Running Performance Test
echo ========================================
echo Starting receiver with optimized settings...
start "Receiver" cmd /k "build\receiver.exe --port 8889 --buffer 131072 --timeout 100 --stats-interval 500"
timeout /t 3 /nobreak > nul
echo Starting sender with optimized settings...
build\sender.exe --ip 127.0.0.1 --port 8889 --buffer 131072 --timeout 100 --stats-interval 500
goto cleanup

:stress_test
echo.
echo ========================================
echo Running Stress Test
echo ========================================
echo Starting receiver with large buffer...
start "Receiver" cmd /k "build\receiver.exe --port 8889 --buffer 262144 --timeout 2000 --stats-interval 2000"
timeout /t 3 /nobreak > nul
echo Starting sender with large buffer...
build\sender.exe --ip 127.0.0.1 --port 8889 --buffer 262144 --timeout 2000 --stats-interval 2000
goto cleanup

:custom_test
echo.
echo ========================================
echo Custom Configuration Test
echo ========================================
set /p custom_port="Enter receiver port (default 8889): "
if "%custom_port%"=="" set custom_port=8889
set /p custom_ip="Enter sender IP (default 127.0.0.1): "
if "%custom_ip%"=="" set custom_ip=127.0.0.1
set /p custom_buffer="Enter buffer size (default 65536): "
if "%custom_buffer%"=="" set custom_buffer=65536
echo.
echo Starting receiver on port %custom_port%...
start "Receiver" cmd /k "build\receiver.exe --port %custom_port% --buffer %custom_buffer% --stats-interval 1000"
timeout /t 3 /nobreak > nul
echo Starting sender to %custom_ip%:%custom_port%...
build\sender.exe --ip %custom_ip% --port %custom_port% --buffer %custom_buffer% --stats-interval 1000
goto cleanup

:audio_test
echo.
echo ========================================
echo Audio Device Test
echo ========================================
echo Testing audio capture functionality...
build\test_audio_capture.exe
echo.
echo Audio test completed. Press any key to continue...
pause > nul
goto menu

:cleanup
echo.
echo Test completed!
echo Closing receiver window...
taskkill /f /im cmd.exe /fi "WINDOWTITLE eq Receiver*" > nul 2>&1
echo.
echo Press any key to return to menu...
pause > nul
goto menu

:exit
echo.
echo Thank you for testing AudioSync!
echo.
pause

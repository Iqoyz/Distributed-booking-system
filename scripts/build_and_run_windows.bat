@echo off
set SERVER_DIR=..\server
set BUILD_DIR=.\build
set SERVER_EXE=booking_system_server.exe
set TEST_EXE=server_test.exe

:: Ensure build directory exists
if not exist "%BUILD_DIR%" (
    echo Creating build directory...
    mkdir "%BUILD_DIR%"
)

:: Check if an argument was provided
if "%1"=="" (
    echo Usage: build_and_run_windows.bat [server|test]
    exit /b 1
)

echo Configuring project with CMake...
cmake -S . -B "%BUILD_DIR%" -G "MinGW Makefiles"
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

echo Building project...
cmake --build "%BUILD_DIR%"
if %errorlevel% neq 0 (
    echo [ERROR] Build failed! Skipping executable run.
    exit /b 1
)

:: Run based on user input
if /I "%1"=="server" (
    echo Running server executable...
    if exist "%BUILD_DIR%\%SERVER_EXE%" (
        pushd "%BUILD_DIR%"
        .\%SERVER_EXE%
        popd
    ) else (
        echo [ERROR] Server executable not found!
    )
) else if /I "%1"=="test" (
    echo Running test executable...
    if exist "%BUILD_DIR%\%TEST_EXE%" (
        pushd "%BUILD_DIR%"
        .\%TEST_EXE%
        popd
    ) else (
        echo [ERROR] Test executable not found!
    )
) else (
    echo [ERROR] Invalid option! Use 'server' or 'test'.
)

exit /b 0

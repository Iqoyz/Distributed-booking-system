@echo off
set SERVER_DIR=..\server
set BUILD_DIR=%SERVER_DIR%\build
set SERVER_EXE=booking_system_server.exe
set TEST_EXE=server_test.exe

:: Check if an argument was provided
if "%1"=="" (
    echo Usage: build_and_run.bat [server|test]
    exit /b 1
)

:: Check if build directory exists, create if not
if not exist %BUILD_DIR% (
    echo Build directory not found. Creating build directory...
    mkdir %BUILD_DIR%
)

echo Configuring project with CMake...
cmake -S %SERVER_DIR% -B %BUILD_DIR% -G "MinGW Makefiles"

echo Building project...
cmake --build %BUILD_DIR%

:: Run based on user input
if /I "%1"=="server" (
    echo Running server executable...
    if exist %BUILD_DIR%\%SERVER_EXE% (
        cd %BUILD_DIR%
        .\%SERVER_EXE%
    ) else (
        echo Server executable not found!
    )
) else if /I "%1"=="test" (
    echo Running test executable...
    if exist %BUILD_DIR%\%TEST_EXE% (
        cd %BUILD_DIR%
        .\%TEST_EXE%
    ) else (
        echo Test executable not found!
    )
) else (
    echo Invalid option! Use 'server' or 'test'.
)

cd %SERVER_DIR%

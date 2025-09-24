@echo off
echo Building Flappy Bird Game with MinGW...

REM 检查MinGW是否安装
where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo MinGW not found! Please install MinGW-w64 first.
    echo.
    echo You can download it from: https://www.mingw-w64.org/downloads/
    echo Or use MSYS2: https://www.msys2.org/
    echo.
    pause
    exit /b 1
)

REM 创建构建目录
if not exist build mkdir build
cd build

REM 使用MinGW配置项目
echo Configuring project with CMake for MinGW...
cmake .. -G "MinGW Makefiles"

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    echo.
    echo If you don't have CMake, download it from: https://cmake.org/download/
    pause
    exit /b 1
)

REM 编译项目
echo Building project...
cmake --build .

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo.
echo To run the game, execute: build\FlappyBird.exe
echo.
pause

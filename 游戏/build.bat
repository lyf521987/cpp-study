@echo off
echo Building Flappy Bird Game...

REM 创建构建目录
if not exist build mkdir build
cd build

REM 配置项目
echo Configuring project with CMake...
cmake .. -G "Visual Studio 16 2019" -A x64

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM 编译项目
echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo.
echo To run the game, execute: build\bin\Release\FlappyBird.exe
echo.
pause

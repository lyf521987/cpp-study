@echo off
echo Starting Flappy Bird Game...

REM 检查是否已编译
if not exist "build\bin\Release\FlappyBird.exe" (
    echo Game not found. Building first...
    call build.bat
    if %ERRORLEVEL% neq 0 (
        echo Build failed!
        pause
        exit /b 1
    )
)

REM 运行游戏
echo Running Flappy Bird...
cd build\bin\Release
FlappyBird.exe
cd ..\..\..

@echo off
echo Starting Flappy Bird Game...

REM 检查游戏是否已编译
if not exist "bin\FlappyBird.exe" (
    echo Game not found. Compiling first...
    call compile_simple.bat
    if %ERRORLEVEL% neq 0 (
        echo Compilation failed!
        pause
        exit /b 1
    )
)

REM 运行游戏
echo Running Flappy Bird...
cd bin
FlappyBird.exe
cd ..

@echo off
echo Compiling Flappy Bird Game directly with g++...

REM 检查g++是否可用
where g++ >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo g++ compiler not found!
    echo.
    echo Please install MinGW-w64 first:
    echo 1. Go to https://www.msys2.org/
    echo 2. Download and install MSYS2
    echo 3. Open MSYS2 terminal and run:
    echo    pacman -S mingw-w64-x86_64-gcc
    echo    pacman -S mingw-w64-x86_64-sfml
    echo 4. Add C:\msys64\mingw64\bin to your PATH
    echo.
    pause
    exit /b 1
)

REM 创建输出目录
if not exist bin mkdir bin

REM 编译游戏
echo Compiling...
g++ -std=c++17 -O2 -I"C:\msys64\mingw64\include" -L"C:\msys64\mingw64\lib" main.cpp -lsfml-graphics -lsfml-window -lsfml-system -o bin\FlappyBird.exe

if %ERRORLEVEL% neq 0 (
    echo Compilation failed!
    echo.
    echo Make sure SFML is installed and the paths are correct.
    echo You may need to adjust the include and library paths above.
    pause
    exit /b 1
)

REM 复制SFML DLL文件
echo Copying SFML DLL files...
if exist "C:\msys64\mingw64\bin\sfml-graphics-2.dll" (
    copy "C:\msys64\mingw64\bin\sfml-graphics-2.dll" "bin\"
    copy "C:\msys64\mingw64\bin\sfml-window-2.dll" "bin\"
    copy "C:\msys64\mingw64\bin\sfml-system-2.dll" "bin\"
    echo DLL files copied successfully!
) else (
    echo Warning: SFML DLL files not found. You may need to copy them manually.
)

echo.
echo Compilation completed successfully!
echo.
echo To run the game, execute: bin\FlappyBird.exe
echo.
pause

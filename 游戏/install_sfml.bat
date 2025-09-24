@echo off
echo Installing SFML using vcpkg...

REM 检查vcpkg是否存在
if not exist vcpkg (
    echo Downloading vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git
    if %ERRORLEVEL% neq 0 (
        echo Failed to clone vcpkg. Please check your internet connection.
        pause
        exit /b 1
    )
)

REM 进入vcpkg目录
cd vcpkg

REM 初始化vcpkg
if not exist vcpkg.exe (
    echo Initializing vcpkg...
    .\bootstrap-vcpkg.bat
    if %ERRORLEVEL% neq 0 (
        echo Failed to initialize vcpkg.
        pause
        exit /b 1
    )
)

REM 安装SFML
echo Installing SFML...
.\vcpkg install sfml:x64-windows

if %ERRORLEVEL% neq 0 (
    echo Failed to install SFML.
    pause
    exit /b 1
)

echo SFML installation completed!
echo.
echo Please add the following to your CMakeLists.txt or set environment variables:
echo set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake")
echo.
pause

@echo off
echo Installing SFML manually...

REM 创建SFML目录
if not exist sfml mkdir sfml
cd sfml

REM 下载SFML（使用PowerShell）
echo Downloading SFML...
powershell -Command "& {Invoke-WebRequest -Uri 'https://github.com/SFML/SFML/releases/download/2.5.1/SFML-2.5.1-windows-vc15-64-bit.zip' -OutFile 'SFML-2.5.1.zip'}"

if not exist "SFML-2.5.1.zip" (
    echo Failed to download SFML. Please check your internet connection.
    echo You can manually download SFML from: https://www.sfml-dev.org/download.php
    pause
    exit /b 1
)

REM 解压SFML
echo Extracting SFML...
powershell -Command "& {Expand-Archive -Path 'SFML-2.5.1.zip' -DestinationPath '.' -Force}"

REM 重命名目录
if exist "SFML-2.5.1" (
    if exist "SFML" rmdir /s /q "SFML"
    ren "SFML-2.5.1" "SFML"
)

REM 设置环境变量
echo Setting up environment...
setx SFML_ROOT "%CD%\SFML" /M
setx CMAKE_PREFIX_PATH "%CD%\SFML" /M

echo SFML installation completed!
echo.
echo Please restart your command prompt and run build.bat
echo.
pause

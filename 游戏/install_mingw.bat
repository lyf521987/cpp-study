@echo off
echo Installing MinGW-w64...

echo.
echo Please choose one of the following options:
echo.
echo 1. Download and install MSYS2 (Recommended)
echo    - Go to https://www.msys2.org/
echo    - Download and install MSYS2
echo    - After installation, open MSYS2 terminal and run:
echo      pacman -S mingw-w64-x86_64-gcc
echo      pacman -S mingw-w64-x86_64-cmake
echo      pacman -S mingw-w64-x86_64-sfml
echo.
echo 2. Download MinGW-w64 directly
echo    - Go to https://www.mingw-w64.org/downloads/
echo    - Download the installer
echo    - Install and add to PATH
echo.
echo 3. Use Chocolatey (if you have it)
echo    - Run: choco install mingw
echo.
echo After installing MinGW, run build_mingw.bat
echo.
pause

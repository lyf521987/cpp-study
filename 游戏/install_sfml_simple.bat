@echo off
echo Installing SFML - Simple Method...

REM 创建目录结构
if not exist "sfml" mkdir sfml
if not exist "sfml\include" mkdir sfml\include
if not exist "sfml\lib" mkdir sfml\lib
if not exist "sfml\bin" mkdir sfml\bin

echo.
echo Please follow these steps:
echo.
echo 1. Go to https://www.sfml-dev.org/download.php
echo 2. Download "SFML 2.5.1 for Visual C++ 15 (2017) - 64-bit"
echo 3. Extract the downloaded zip file
echo 4. Copy the following files to the sfml directory:
echo    - Copy 'include' folder to 'sfml\include'
echo    - Copy 'lib' folder to 'sfml\lib'  
echo    - Copy all .dll files to 'sfml\bin'
echo.
echo 5. After copying files, press any key to continue...
pause

REM 检查是否已复制文件
if not exist "sfml\include\SFML" (
    echo Error: SFML include files not found!
    echo Please make sure you copied the include folder correctly.
    pause
    exit /b 1
)

if not exist "sfml\lib\sfml-graphics.lib" (
    echo Error: SFML library files not found!
    echo Please make sure you copied the lib folder correctly.
    pause
    exit /b 1
)

echo SFML files found! Setting up CMake configuration...

REM 创建CMake配置文件
echo Creating CMake configuration...
(
echo # SFML Configuration
echo set(SFML_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/sfml")
echo set(SFML_INCLUDE_DIR "${SFML_ROOT}/include")
echo set(SFML_LIBRARY_DIR "${SFML_ROOT}/lib")
echo set(SFML_BINARY_DIR "${SFML_ROOT}/bin")
echo.
echo # Find SFML components
echo find_library(SFML_GRAPHICS_LIBRARY sfml-graphics HINTS ${SFML_LIBRARY_DIR})
echo find_library(SFML_WINDOW_LIBRARY sfml-window HINTS ${SFML_LIBRARY_DIR})
echo find_library(SFML_SYSTEM_LIBRARY sfml-system HINTS ${SFML_LIBRARY_DIR})
echo.
echo # Set include directories
echo include_directories(${SFML_INCLUDE_DIR})
echo.
echo # Create SFML target
echo add_library(SFML::Graphics INTERFACE IMPORTED)
echo set_target_properties(SFML::Graphics PROPERTIES
echo     INTERFACE_INCLUDE_DIRECTORIES "${SFML_INCLUDE_DIR}"
echo     INTERFACE_LINK_LIBRARIES "${SFML_GRAPHICS_LIBRARY};${SFML_WINDOW_LIBRARY};${SFML_SYSTEM_LIBRARY}"
echo )
) > sfml_config.cmake

echo.
echo SFML installation completed!
echo You can now run build.bat to compile the game.
echo.
pause

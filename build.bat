@echo off
echo Building Cloth Simulation - Stage 3...

if not exist build_simple mkdir build_simple
cd build_simple

cmake .. -DCMAKE_CXX_FLAGS="/std:c++17" -G "Visual Studio 16 2019" -A x64
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo Build failed
    pause
    exit /b 1
)

echo Build completed successfully!

REM Copy freeglut.dll to the output directory
if exist ..\freeglut.dll (
    copy ..\freeglut.dll Release\ >nul 2>&1
    echo Copied freeglut.dll to output directory
) else (
    echo freeglut.dll not found in project root
)

echo Running cloth simulation...
echo.
cd Release
ClothSimulation.exe
pause 
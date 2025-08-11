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

REM --- Runtime DLL staging (best-effort, non-fatal) ---
REM Try multiple locations for 64-bit freeglut and GLEW DLLs and copy next to the exe
set TARGETDIR=Release

REM freeglut candidates
set FG_CAND1=..\freeglut.dll
set FG_CAND2=..\freeglut\bin\x64\freeglut.dll
set FG_CAND3=..\freeglut\bin\freeglut.dll
if exist %FG_CAND1% copy %FG_CAND1% %TARGETDIR%\ >nul 2>&1
if not exist %TARGETDIR%\freeglut.dll if exist %FG_CAND2% copy %FG_CAND2% %TARGETDIR%\ >nul 2>&1
if not exist %TARGETDIR%\freeglut.dll if exist %FG_CAND3% copy %FG_CAND3% %TARGETDIR%\ >nul 2>&1
if exist %TARGETDIR%\freeglut.dll (echo Staged freeglut.dll) else (echo Note: freeglut.dll not auto-staged)

REM GLEW candidates (vcpkg typical path or local copy)
set GLEW_CAND1=%VCPKG_ROOT%\installed\x64-windows\bin\glew32.dll
set GLEW_CAND2=C:\vcpkg\installed\x64-windows\bin\glew32.dll
set GLEW_CAND3=..\glew32.dll
if exist %GLEW_CAND1% copy %GLEW_CAND1% %TARGETDIR%\ >nul 2>&1
if not exist %TARGETDIR%\glew32.dll if exist %GLEW_CAND2% copy %GLEW_CAND2% %TARGETDIR%\ >nul 2>&1
if not exist %TARGETDIR%\glew32.dll if exist %GLEW_CAND3% copy %GLEW_CAND3% %TARGETDIR%\ >nul 2>&1
if exist %TARGETDIR%\glew32.dll (echo Staged glew32.dll) else (echo Note: glew32.dll not auto-staged)

echo Running cloth simulation...
echo.
cd Release
setlocal
set PATH=%CD%;%PATH%
ClothSimulation.exe
endlocal
pause
@echo off
setlocal
where geode >nul 2>nul
if errorlevel 1 (
  echo ERROR: Geode CLI was not found in PATH.
  pause
  exit /b 1
)
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake was not found in PATH.
  pause
  exit /b 1
)
if "%GEODE_SDK%"=="" (
  echo ERROR: GEODE_SDK is not configured.
  pause
  exit /b 1
)
where clang >nul 2>nul
if errorlevel 1 (
  echo ERROR: Clang was not found.
  echo Install LLVM and Ninja first. The official Geode recommendation is:
  echo     scoop install llvm ninja
  echo Then close this window, open it again and rerun this file.
  pause
  exit /b 1
)
where ninja >nul 2>nul
if errorlevel 1 (
  echo ERROR: Ninja was not found.
  echo Install it with:
  echo     scoop install ninja
  echo Then close this window, open it again and rerun this file.
  pause
  exit /b 1
)
echo Building Top Demons with Clang and Ninja.
cmake -S . -B build-clang -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
if errorlevel 1 (
  echo.
  echo CMake configuration failed. Copy the complete error and send it for diagnosis.
  pause
  exit /b 1
)
cmake --build build-clang
if errorlevel 1 (
  echo.
  echo Build failed. Copy the complete error and send it for diagnosis.
  pause
  exit /b 1
)
echo.
echo Build complete. Check build-clang for mbmb709.topdemons.geode.
pause

@echo off

echo Building the Coyote native scheduler...

REM Create build directory
if EXIST build rmdir /Q /S build
mkdir build
cd build

REM Build the project
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
if not ERRORLEVEL 0 goto error

ninja
if not ERRORLEVEL 0 goto error

echo Successfully built the project.

:done
cd ..
exit /B ERRORLEVEL

:error
echo Failed to build the project.
goto done
@echo off
cls
set BUILD_DIR=build
set GENERATOR="MinGW Makefiles"
set BUILD_TYPE=Debug

if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)
mkdir %BUILD_DIR%
cd %BUILD_DIR%

echo Configuring the project with CMake...
cmake -G %GENERATOR% -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ..

echo Building the project...
cmake --build . --config %BUILD_TYPE%

cd ..
echo Build completed!

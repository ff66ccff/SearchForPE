@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
echo ========================================================
echo   体育理论题库查询系统 - 构建脚本
echo   C++17 / Win32 / Direct2D + DirectWrite / 内嵌 SQLite 题库
echo ========================================================
echo.
if not exist build mkdir build
if not exist dist mkdir dist

echo [1/3] 编译资源（内嵌题库与清单）...
windres src\resource.rc -O coff -o build\resource.res
if errorlevel 1 goto :error

echo [2/3] 编译 C++ 源码...
set CXXFLAGS=-O2 -std=c++17 -DUNICODE -D_UNICODE -I src
g++ %CXXFLAGS% -c src\database.cpp    -o build\database.o
if errorlevel 1 goto :error
g++ %CXXFLAGS% -c src\search.cpp      -o build\search.o
if errorlevel 1 goto :error
g++ %CXXFLAGS% -c src\fonts.cpp       -o build\fonts.o
if errorlevel 1 goto :error
g++ %CXXFLAGS% -c src\gfx.cpp         -o build\gfx.o
if errorlevel 1 goto :error
g++ %CXXFLAGS% -c src\result_list.cpp -o build\result_list.o
if errorlevel 1 goto :error
g++ %CXXFLAGS% -c src\main.cpp        -o build\main.o
if errorlevel 1 goto :error

echo [3/3] 静态链接为独立便携版 EXE...
g++ -O2 -mwindows -static build\database.o build\search.o build\fonts.o build\gfx.o build\result_list.o build\main.o build\resource.res -o dist\SportExamSearch.exe -L. -lwinsqlite3 -ld2d1 -ldwrite -lole32 -lcomctl32 -ldwmapi -lgdi32 -luser32
if errorlevel 1 goto :error
copy /y dist\SportExamSearch.exe "dist\体育理论题库查询.exe" >nul

echo.
echo ========================================================
echo   构建成功：dist\体育理论题库查询.exe（单文件 / 零依赖）
echo ========================================================
pause
exit /b 0

:error
echo.
echo [错误] 构建失败，请检查上面的编译输出。
pause
exit /b 1
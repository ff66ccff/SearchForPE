@echo off
chcp 65001 >nul
echo ========================================================
echo   🏃 体育理论题库查询系统 - 高性能便携版构建工具
echo   技术栈: C++17 / Win32 / GDI+ / WinSQLite3 / UCRT
echo ========================================================
echo.

if not exist "%~dp0build" mkdir "%~dp0build"
if not exist "%~dp0dist" mkdir "%~dp0dist"

echo [1/3] 正在编译 Windows 资源与清单 (包含内嵌题库)...
windres "%~dp0src\resource.rc" -O coff -o "%~dp0build\resource.res"
if errorlevel 1 (
    echo [错误] 编译资源失败!
    pause
    exit /b 1
)

echo [2/3] 正在编译 C++ 源码模块 (O3 极限优化)...
g++ -O3 -std=c++17 -I"%~dp0src" -c "%~dp0src\database.cpp" -o "%~dp0build\database.o"
if errorlevel 1 ( echo [错误] 编译 database.cpp 失败! & pause & exit /b 1 )

g++ -O3 -std=c++17 -I"%~dp0src" -c "%~dp0src\search.cpp" -o "%~dp0build\search.o"
if errorlevel 1 ( echo [错误] 编译 search.cpp 失败! & pause & exit /b 1 )

g++ -O3 -std=c++17 -I"%~dp0src" -c "%~dp0src\ui_card.cpp" -o "%~dp0build\ui_card.o"
if errorlevel 1 ( echo [错误] 编译 ui_card.cpp 失败! & pause & exit /b 1 )

g++ -O3 -std=c++17 -I"%~dp0src" -c "%~dp0src\main.cpp" -o "%~dp0build\main.o"
if errorlevel 1 ( echo [错误] 编译 main.cpp 失败! & pause & exit /b 1 )

echo [3/3] 正在静态链接独立便携版 EXE (零外部依赖)...
g++ -O3 -mwindows -static "%~dp0build\main.o" "%~dp0build\database.o" "%~dp0build\search.o" "%~dp0build\ui_card.o" "%~dp0build\resource.res" -o "%~dp0dist\SportExamSearch.exe" -L"%~dp0" -lwinsqlite3 -lgdiplus -lgdi32 -luser32 -lcomctl32 -ldwmapi -luxtheme
if errorlevel 1 (
    echo [错误] 链接可执行文件失败!
    pause
    exit /b 1
)

copy /y "%~dp0dist\SportExamSearch.exe" "%~dp0dist\体育理论题库查询.exe" >nul

echo.
if exist "%~dp0dist\体育理论题库查询.exe" (
    echo ========================================================
    echo   ✓ 便携版构建成功！
    echo   输出文件: %~dp0dist\体育理论题库查询.exe
    echo   特性说明: 单文件绿色免安装，内嵌6108道全量题库
    echo ========================================================
    echo.
) else (
    echo × 构建失败，请检查编译输出
)

pause

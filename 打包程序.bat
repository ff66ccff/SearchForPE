@echo off
chcp 65001 >nul
echo ========================================
echo   体育理论题库查询系统 - 打包工具
echo ========================================
echo.
echo 正在打包程序，请稍候...
echo 这可能需要几分钟时间...
echo.

"%~dp0.venv\Scripts\pyinstaller.exe" --clean "%~dp0build.spec"

echo.
if exist "%~dp0dist\体育理论题库查询.exe" (
    echo ✓ 打包成功！
    echo.
    echo 输出文件: %~dp0dist\体育理论题库查询.exe
    echo.
    explorer "%~dp0dist"
) else (
    echo × 打包失败，请检查错误信息
)

pause

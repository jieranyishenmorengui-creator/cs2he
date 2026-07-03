@echo off
chcp 65001 >nul
title VisCheck 地图数据安装

echo ========================================
echo  VisCheck 地图碰撞数据安装脚本
echo  解压后放到 cs2_hvh.exe 同一目录即可
echo ========================================
echo.

rem 检测 exe 位置
if exist "%~dp0cs2_hvh.exe" (
    set "target=%~dp0data"
) else if exist "%~dp0..\cs2_hvh.exe" (
    set "target=%~dp0..\data"
) else (
    echo [!] 没找到 cs2_hvh.exe，请把脚本放到 exe 同目录运行
    pause
    exit /b 1
)

if not exist "%target%" mkdir "%target%"

echo [*] 安装到: %target%
echo.
echo 可用地图:
echo   [1] 仅竞技图 7 张 (de_mirage, de_inferno, de_nuke, de_dust2, de_anubis, de_ancient, de_overpass)
echo   [2] 全部 11 张 (含 aim/ar/cs 娱乐图)
echo   [3] 自定义选择
echo.

set /p choice="请选择 [1/2/3]: "

if "%choice%"=="1" (
    echo 安装竞技图... (略 — 实际解压由 Release zip 处理)
) else if "%choice%"=="2" (
    echo 安装全部地图...
) else if "%choice%"=="3" (
    echo 请手动复制对应的 .opt 文件到 data 目录
    echo 地图列表见 VISCHEAT_MAP_DATA.md
)

echo.
echo [*] 安装完成! 启动游戏后按 INSERT → Misc → VisCheck Map 选择地图
pause

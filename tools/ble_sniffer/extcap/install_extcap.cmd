@echo off
REM ======================================================
REM ESP32 BLE Sniffer — Wireshark extcap 部署工具
REM 以管理员身份运行此脚本安装 extcap 插件
REM ======================================================

set EXT_SRC=%~dp0
set EXT_DST="C:\Program Files\Wireshark\extcap"

echo ========================================
echo  ESP32 BLE Sniffer - extcap 部署
echo ========================================
echo.
echo 源文件: %EXT_SRC%
echo 目标:   %EXT_DST%
echo.
echo 请以管理员身份运行此脚本！
echo.

copy /Y "%EXT_SRC%esp_ble_sniffer_extcap.bat" %EXT_DST%
copy /Y "%EXT_SRC%esp_ble_sniffer_extcap.py"  %EXT_DST%

echo.
echo 部署完成！请重启 Wireshark。
echo 在捕获接口列表中应出现 "ESP32 BLE Sniffer"。
echo.

pause

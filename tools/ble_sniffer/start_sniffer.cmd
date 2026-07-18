@echo off
REM ESP32 BLE Sniffer — 一键启动 Wireshark 捕获
REM 用法: 修改下面的 COM_PORT 为你的串口号，双击运行

set COM_PORT=COM4
set BAUD=921600
set WIRESHARK="C:\Program Files\Wireshark\wireshark.exe"

echo ========================================
echo  ESP32 BLE Sniffer
echo ========================================
echo.
echo Starting sniffer on %COM_PORT% at %BAUD% baud...
echo Make sure BLE Sniff app is running on the watch!
echo.
echo Wireshark will open automatically.
echo Press Ctrl+C in this window to stop.
echo ========================================
echo.

python "%~dp0esp_ble_sniffer_pipe.py" %COM_PORT% %BAUD% | %WIRESHARK% -k -i -
pause

#!/usr/bin/env python3
"""
ESP32 BLE Sniffer — USB CDC → Wireshark 桥接工具

使用方法:
  1. 手表上启动 BLE Sniff 应用
  2. 找到 sniffer 的 COM 口（设备管理器中 "USB Serial Device"）
  3. 运行: python esp_ble_sniffer_bridge.py COM4

Wireshark 连接方式:
  - 打开 Wireshark → 捕获 → 捕获选项 (Ctrl+K)
  - 输入接口: \\.\pipe\esp_ble_sniffer
  - 点开始

  或者直接命令行:
  wireshark -k -i \\.\pipe\esp_ble_sniffer
"""

import sys
import os
import time
import threading
import serial

BAUD = 921600
PIPE_NAME = r"\\.\pipe\esp_ble_sniffer"


def create_pipe_server():
    """Create a named pipe server that Wireshark connects to."""
    import win32pipe
    import win32file

    pipe = win32pipe.CreateNamedPipe(
        PIPE_NAME,
        win32pipe.PIPE_ACCESS_OUTBOUND,
        win32pipe.PIPE_TYPE_BYTE | win32pipe.PIPE_WAIT,
        1, 65536, 65536, 0, None
    )
    print("Waiting for Wireshark to connect to %s ..." % PIPE_NAME)
    sys.stdout.flush()

    win32pipe.ConnectNamedPipe(pipe, None)
    print("Wireshark connected!")
    sys.stdout.flush()
    return pipe


def write_pipe(pipe, data):
    """Write data to named pipe."""
    import win32file
    try:
        win32file.WriteFile(pipe, data)
    except Exception:
        return False
    return True


def main():
    if len(sys.argv) < 2:
        print("Usage: python esp_ble_sniffer_bridge.py <COM_PORT>")
        print("Example: python esp_ble_sniffer_bridge.py COM4")
        sys.exit(1)

    port = sys.argv[1]
    print("Opening %s at %d baud ..." % (port, BAUD))

    try:
        ser = serial.Serial(port, BAUD, timeout=1)
    except Exception as e:
        print("Failed to open %s: %s" % (port, e))
        print("Check: 1) Watch plugged in via USB?  2) BLE Sniff started?")
        sys.exit(1)

    time.sleep(2)

    # Create named pipe
    try:
        pipe = create_pipe_server()
    except Exception as e:
        print("Failed to create pipe (try running as admin): %s" % e)
        ser.close()
        sys.exit(1)

    # Forward serial data to pipe
    print("Forwarding data ... (Ctrl+C to stop)")
    sys.stdout.flush()

    try:
        while True:
            data = ser.read(4096)
            if data:
                if not write_pipe(pipe, data):
                    print("Pipe disconnected, waiting for new connection...")
                    try:
                        win32pipe.DisconnectNamedPipe(pipe)
                        win32pipe.ConnectNamedPipe(pipe, None)
                        print("Wireshark reconnected!")
                    except:
                        break
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()
        try:
            win32pipe.CloseHandle(pipe)
        except:
            pass


if __name__ == "__main__":
    main()

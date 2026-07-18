#!/usr/bin/env python3
"""
ESP32 BLE Sniffer — 管道输出到 Wireshark（纯 Python，无需额外安装）

使用说明:
  python esp_ble_sniffer_pipe.py COM4 | wireshark -k -i -
  python esp_ble_sniffer_pipe.py COM4 921600 | wireshark -k -i -
"""

import sys
import serial

DEFAULT_BAUD = 921600

def main():
    if len(sys.argv) < 2:
        print("Usage: python esp_ble_sniffer_pipe.py <COM_PORT> [baud] | wireshark -k -i -")
        print("Example: python esp_ble_sniffer_pipe.py COM4 | wireshark -k -i -")
        sys.exit(1)

    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD
    ser = serial.Serial(port, baud, timeout=1)

    # Output pcapng data to stdout (Wireshark reads from pipe)
    while True:
        try:
            data = ser.read(4096)
            if data:
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
        except (BrokenPipeError, KeyboardInterrupt):
            break

    ser.close()

if __name__ == "__main__":
    main()

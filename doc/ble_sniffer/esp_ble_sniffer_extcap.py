#!/usr/bin/env python3
"""
ESP32 BLE Sniffer — Wireshark extcap plugin.

Place this file in Wireshark's extcap directory:
  Windows: %APPDATA%\\Wireshark\\extcap\\
  macOS:   $HOME/.local/lib/wireshark/extcap/
  Linux:   /usr/lib/x86_64-linux-gnu/wireshark/extcap/

Wireshark will then show "ESP32 BLE Sniffer" as a capture interface.

Usage (standalone):
  python esp_ble_sniffer_extcap.py --capture --fifo <pipe> --serial <COM port>
"""

import sys
import os
import struct
import time
import json
import argparse

# ── Config ──────────────────────────────────────────────────────────
DEFAULT_BAUD = 921600
PCAPNG_MAGIC = b'\x0a\x0d\x0d\x0a'  # pcapng Section Header Block magic


def find_esp32_serial():
    """Auto-detect ESP32-S3 CDC serial port."""
    import serial.tools.list_ports
    for p in serial.tools.list_ports.comports():
        if p.vid is not None:
            # Espressif USB Vendor ID
            if p.vid == 0x303a or p.vid == 0x10c4:
                return p.device
            # Generic CDC (common for ESP32-S3)
            if 'CP210' in p.description or 'USB' in p.description:
                if 'SERIAL' not in p.description.upper():
                    return p.device
    return None


# ── extcap helpers ─────────────────────────────────────────────────

def extcap_version():
    return "1.0"


def extcap_interfaces(serial_port):
    """Output extcap interface description to stdout."""
    value = {
        "value": {
            "name": "esp32_ble_sniffer",
            "display_name": "ESP32 BLE Sniffer",
            "description": "ESP32-S3 BLE Sniffer (VHCI → pcapng via USB CDC)",
            "version": extcap_version(),
            "help_url": "https://github.com/NevermindZZT/esp32_tool",
        }
    }
    # Interface
    print("extcap {version=%s}" % extcap_version())
    print("control {number=%d}" % 0)
    print("value {control=%d}" % 0, end="")
    for k, v in value["value"].items():
        print(" {%s=%s}" % (k, v), end="")
    print()
    # Serial port config option
    print("value {control=%d}" % 1, end="")
    print(" {arg=serial}", end="")
    print(" {display=Serial port}", end="")
    print(" {type=string}", end="")
    print(" {default=%s}" % (serial_port or ""), end="")
    if serial_port:
        print(" {saved=true}", end="")
    print()
    # Baud rate
    print("value {control=%d}" % 2, end="")
    print(" {arg=baud}", end="")
    print(" {display=Baud rate}", end="")
    print(" {type=integer}", end="")
    print(" {default=%d}" % DEFAULT_BAUD, end="")
    print(" {range=9600,2000000}", end="")
    print()


def extcap_dlts():
    """Declare we output pcapng (DLT=USER0, Wireshark interprets as raw)."""
    print("dlt {number=%d}" % 0, end="")
    print(" {name=BLUETOOTH_HCI_H4}", end="")
    print(" {display=Bluetooth HCI H4}", end="")
    print()


def capture(serial_port, baud, fifo):
    """Read CDC serial → write pcapng to Wireshark pipe (fifo)."""
    import serial

    if not fifo:
        # Direct to stdout (works with pipe: `python script | wireshark -k -i -`)
        out = sys.stdout.buffer
    else:
        out = open(fifo, 'wb')

    try:
        ser = serial.Serial(serial_port, baud, timeout=1)
    except Exception as e:
        print("Failed to open %s: %s" % (serial_port, e), file=sys.stderr)
        return 1

    # Forward pcapng data from serial to Wireshark
    while True:
        try:
            data = ser.read(4096)
            if data:
                out.write(data)
                out.flush()
            else:
                time.sleep(0.01)
        except (BrokenPipeError, KeyboardInterrupt):
            break
        except Exception as e:
            print("Error: %s" % e, file=sys.stderr)
            time.sleep(0.1)

    ser.close()
    if fifo:
        out.close()
    return 0


# ── CLI entry point ────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="ESP32 BLE Sniffer - Wireshark extcap",
        add_help=False,
    )
    parser.add_argument("--extcap-interfaces", action="store_true")
    parser.add_argument("--extcap-dlts", action="store_true")
    parser.add_argument("--extcap-version", action="store_true")
    parser.add_argument("--extcap-reload-option", type=str)
    parser.add_argument("--capture", action="store_true")
    parser.add_argument("--fifo", type=str, help="Wireshark pipe")
    parser.add_argument("--serial", type=str, default="", help="Serial port")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="Baud rate")
    parser.add_argument("--help", action="store_true")

    # Wireshark may pass extra args we don't know about
    args, _ = parser.parse_known_args()

    if args.help:
        parser.print_help()
        return 0

    # Auto-detect serial port if not specified
    serial_port = args.serial
    if not serial_port:
        serial_port = find_esp32_serial() or ""

    if args.extcap_interfaces:
        extcap_interfaces(serial_port)
        return 0

    if args.extcap_dlts:
        extcap_dlts()
        return 0

    if args.extcap_version:
        print(extcap_version())
        return 0

    if args.capture:
        if not serial_port:
            print("No serial port specified and auto-detect failed.\n"
                  "Use --serial COMx or set it in Wireshark interface options.",
                  file=sys.stderr)
            return 1
        return capture(serial_port, args.baud, args.fifo)

    return 0


if __name__ == "__main__":
    sys.exit(main())

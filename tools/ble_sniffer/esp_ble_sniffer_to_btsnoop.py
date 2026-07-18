#!/usr/bin/env python3
"""
ESP32 BLE Sniffer — CDC pcapng → btsnoop 转换工具

从 CDC 串口读取 pcapng 数据流，实时转换并保存为 btsnoop 格式文件。
btsnoop 文件可用 Ellisys Bluetooth Analyzer、Frontline 等工具打开。

使用方法:
  python esp_ble_sniffer_to_btsnoop.py COM4 capture.btsnoop
  python esp_ble_sniffer_to_btsnoop.py COM4 capture.btsnoop 921600
"""

import sys
import struct
import time
import serial

DEFAULT_BAUD = 921600


class BtsnoopWriter:
    """btsnoop file writer"""

    HEADER = b'btsnoop\x00\x00\x00\x00\x00\x01\x00\x00\x03\xea'

    def __init__(self, path):
        self.f = open(path, 'wb')
        self.f.write(self.HEADER)
        self.packet_count = 0

    def write_packet(self, h4_type, data, ts_us=None):
        """Write one HCI packet as btsnoop record."""
        if ts_us is None:
            ts_us = int(time.time() * 1000000)

        total_len = 1 + len(data)  # H4 type + payload

        # Record header (24 bytes, big-endian)
        hdr = struct.pack('>IIIIQ',
                          total_len,     # Original Length
                          total_len,     # Included Length
                          0x02,          # Flags: 2 = controller->host
                          0,             # Cumulative Drops
                          ts_us)         # Timestamp (us since epoch)

        self.f.write(hdr)
        self.f.write(bytes([h4_type]))
        if data:
            self.f.write(data)
        self.packet_count += 1

    def close(self):
        self.f.close()
        print("Wrote %d packets to %s" % (self.packet_count, self.f.name))


def parse_pcapng_epb(data, offset):
    """Parse pcapng Enhanced Packet Block, return (h4_type, payload, next_offset) or None."""
    if offset + 28 > len(data):
        return None

    # Read EPB header fields
    block_type = struct.unpack_from('<I', data, offset)[0]
    if block_type != 0x00000006:  # EPB
        return None

    block_len = struct.unpack_from('<I', data, offset + 4)[0]
    if block_len < 28 or offset + block_len > len(data):
        return None

    captured_len = struct.unpack_from('<I', data, offset + 20)[0]
    if captured_len < 1 or captured_len > block_len - 28 - 4:
        return None

    # Packet data starts after 28-byte header
    pkt_start = offset + 28
    h4_type = data[pkt_start]
    payload = data[pkt_start + 1: pkt_start + captured_len]
    next_offset = offset + block_len

    return (h4_type, payload, next_offset)


def main():
    if len(sys.argv) < 3:
        print("用法: python esp_ble_sniffer_to_btsnoop.py <COM_PORT> <OUTPUT.btsnoop> [baud]")
        print("示例: python esp_ble_sniffer_to_btsnoop.py COM4 capture.btsnoop")
        sys.exit(1)

    port = sys.argv[1]
    out_path = sys.argv[2]
    baud = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_BAUD

    print("Reading %s at %d baud ..." % (port, baud))
    ser = serial.Serial(port, baud, timeout=1)
    time.sleep(2)

    writer = BtsnoopWriter(out_path)
    buf = b''
    stats_time = time.time()

    try:
        while True:
            chunk = ser.read(4096)
            if not chunk:
                continue

            buf += chunk

            # Look for pcapng SHB as sync point
            while len(buf) >= 28:
                # Try to find EPB (0x00000006)
                if len(buf) < 8:
                    break

                block_type = struct.unpack_from('<I', buf, 0)[0]

                if block_type == 0x00000006:  # EPB
                    result = parse_pcapng_epb(buf, 0)
                    if result:
                        h4_type, payload, next_offset = result
                        writer.write_packet(h4_type, payload)
                        buf = buf[next_offset:]
                        continue
                    else:
                        # Partial EPB, wait for more data
                        break
                elif block_type == 0x0A0D0D0A:  # SHB - skip 28 bytes
                    buf = buf[28:]
                    continue
                elif block_type == 0x00000001:  # IDB - need to parse length
                    if len(buf) < 8:
                        break
                    block_len = struct.unpack_from('<I', buf, 4)[0]
                    if len(buf) < block_len:
                        break
                    buf = buf[block_len:]
                    continue
                else:
                    # Unknown block, skip 4 bytes and try again
                    buf = buf[4:]
                    continue

            # Print stats periodically
            now = time.time()
            if now - stats_time >= 2:
                sys.stdout.write("\rPackets: %d" % writer.packet_count)
                sys.stdout.flush()
                stats_time = now

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()
        writer.close()


if __name__ == "__main__":
    main()

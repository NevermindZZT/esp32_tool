"""ESP32 BLE Sniffer - Wireshark extcap plugin"""
import sys, time

DEFAULT_BAUD = 921600
DEFAULT_PORT = "COM4"
VERSION = "1.0"
HELP = "https://github.com/NevermindZZT/esp32_tool"

def find_com_port():
    try:
        import serial.tools.list_ports
        for p in serial.tools.list_ports.comports():
            if p.vid in (0x303a, 0x10c4):
                return p.device
    except Exception:
        pass
    return DEFAULT_PORT

def do_interfaces(port):
    sys.stdout.write(
        "extcap {version=%s}{help=%s}{display=ESP32 BLE Sniffer}\n"
        "interface {value=esp32_ble_sniffer}{display=ESP32 BLE Sniffer}\n"
        "control {number=0}\n"
        "value {control=0}{arg=serial}{display=Serial Port}{type=string}"
        "{default=%s}{saved=true}\n"
        "value {control=1}{arg=baud}{display=Baud}{type=integer}"
        "{default=%d}{range=9600,2000000}\n"
        % (VERSION, HELP, port or DEFAULT_PORT, DEFAULT_BAUD)
    )
    sys.stdout.flush()

def do_dlts():
    sys.stdout.write(
        "dlt {number=0}{name=BLUETOOTH_HCI_H4}{display=Bluetooth HCI H4}\n"
    )
    sys.stdout.flush()

def do_capture(port, baud, fifo):
    import serial
    try:
        ser = serial.Serial(port, baud, timeout=1)
    except Exception as e:
        sys.stderr.write("ERROR:%s\n" % e)
        return 1
    out = open(fifo, 'wb') if fifo else sys.stdout.buffer
    while True:
        try:
            d = ser.read(4096)
            if d:
                out.write(d)
                out.flush()
        except (BrokenPipeError, EOFError, KeyboardInterrupt):
            break
        except Exception:
            time.sleep(0.05)
    if fifo:
        out.close()
    ser.close()
    return 0

def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--extcap-capture-filter")]
    if "--extcap-interfaces" in args:
        port = ""
        for i, a in enumerate(args):
            if a == "--serial" and i + 1 < len(args):
                port = args[i + 1]
        do_interfaces(port or find_com_port())
        return 0
    if "--extcap-dlts" in args:
        do_dlts()
        return 0
    if any(a.startswith("--extcap-version") for a in args):
        print(VERSION, flush=True)
        return 0
    if "--capture" in args or "--fifo" in args:
        port, baud = DEFAULT_PORT, DEFAULT_BAUD
        for i, a in enumerate(args):
            if a == "--serial" and i + 1 < len(args):
                port = args[i + 1]
            if a == "--baud" and i + 1 < len(args):
                baud = int(args[i + 1])
        fifo = None
        for i, a in enumerate(args):
            if a == "--fifo" and i + 1 < len(args):
                fifo = args[i + 1]
        return do_capture(port, baud, fifo)
    return 0

if __name__ == "__main__":
    sys.exit(main())

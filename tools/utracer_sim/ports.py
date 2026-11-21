"""The two ways the software uTracer can meet a host program.

PtyPort     Linux and macOS, no extra packages. The simulator makes a
            pseudo-terminal (a pair of connected virtual serial ports) and links
            the host's end to a path such as /tmp/utracer-sim.

SerialPort  Any system, with the pyserial package. The simulator opens a serial
            port by name and the host program uses the other end:
              - Windows: one port of a com0com virtual pair (COM11, say), with
                the host program on the other (COM10). This is how Ronald
                Dekker's own Windows program can be run against the simulator.
              - Any system: a real serial port, e.g. a USB-serial adapter wired
                null-modem to one on another computer, at 9600 baud 8N1 like a
                uTracer.

Both have the same three calls: read(timeout) -> bytes (b"" if nothing came),
write(bytes), close(). read() never blocks for longer than its timeout.

Part of uTmax6. GNU GPL v3 or later.
"""

import os
import time


class PtyPort:
    """A pseudo-terminal, its host end linked to `link` (Linux and macOS)."""

    def __init__(self, link):
        import tty                         # Unix only; imported here so Windows can load this file
        self.link = link
        self.master, slave = os.openpty()
        tty.setraw(slave)
        self.slave_name = os.ttyname(slave)
        # Replace an old link (a simulator that was killed), but never a real file.
        if os.path.exists(link) and not os.path.islink(link):
            os.close(self.master)
            raise SystemExit(f"--link {link}: a file is already there and is not a link; "
                             "choose another path")
        if os.path.islink(link):
            os.unlink(link)
        os.symlink(self.slave_name, link)
        self.description = f"{link} -> {self.slave_name}"
        self.host_name = link

    def read(self, timeout):
        import select
        r, _, _ = select.select([self.master], [], [], timeout)
        if not r:
            return b""
        try:
            return os.read(self.master, 256)
        except OSError:
            time.sleep(0.1)                # host closed the port; wait for it to reopen
            return b""

    def write(self, data):
        os.write(self.master, data)

    def close(self):
        # Remove the link only if it is still ours: a second copy may have
        # replaced it with its own.
        if os.path.islink(self.link) and os.readlink(self.link) == self.slave_name:
            os.unlink(self.link)


class SerialPort:
    """A named serial port through pyserial: COM11, /dev/ttyUSB0, ..."""

    def __init__(self, name, baud=9600):
        try:
            import serial
        except ImportError:
            raise SystemExit("--port needs the pyserial package: "
                             "python -m pip install pyserial") from None
        try:
            self.ser = serial.Serial(name, baudrate=baud, bytesize=8, parity="N",
                                     stopbits=1, timeout=0, write_timeout=2)
        except (serial.SerialException, ValueError) as e:
            raise SystemExit(f"--port {name}: {e}") from None
        self.description = f"{name} at {baud} baud, 8N1"
        self.host_name = name if os.name != "nt" else "the other port of the pair"

    def read(self, timeout):
        # pyserial has no select() on Windows; poll in small steps instead.
        deadline = time.monotonic() + timeout
        while True:
            waiting = self.ser.in_waiting
            if waiting:
                return self.ser.read(waiting)
            if time.monotonic() >= deadline:
                return b""
            time.sleep(0.002)

    def write(self, data):
        self.ser.write(data)

    def close(self):
        self.ser.close()

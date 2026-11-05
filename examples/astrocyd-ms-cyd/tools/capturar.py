"""Pide capturas de pantalla a AstroCYD MS por USB y las guarda como PNG (sin Pillow).

Uso: python capturar.py COM4 CARPETA "MSR_TAB 0" "MSR_TAP 120 80" shot:cielo "MSR_TAB 1" shot:luna ...
Cada "shot:nombre" guarda CARPETA/nombre.png; el resto se manda tal cual como orden.
"""
import os
import struct
import sys
import time
import zlib

import serial


def png(path, w, h, rgb565):
    rows = bytearray()
    for y in range(h):
        rows.append(0)
        for x in range(w):
            v = struct.unpack_from("<H", rgb565, (y * w + x) * 2)[0]
            r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
            rows += bytes(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


def wait_line(s, prefix, timeout=45):
    end = time.time() + timeout
    while time.time() < end:
        line = s.readline().decode("utf-8", "replace").strip()
        if line.startswith(prefix):
            return line
        if "ERROR" in line or "Guru" in line or "rst:" in line:
            print("placa:", line)
    raise TimeoutError(prefix)


port, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
s = serial.Serial()
s.port, s.baudrate, s.timeout = port, 115200, 1
s.dtr = s.rts = False  # sin esto, abrir el puerto reinicia la placa
s.open()
time.sleep(0.5)
s.reset_input_buffer()
for arg in sys.argv[3:]:
    if arg.startswith("shot:"):
        s.write(b"MSR_SHOT\n")
        _, w, h = wait_line(s, "MSR_SHOT_BEGIN").split()[:3]
        w, h = int(w), int(h)
        data = s.read(w * h * 2)
        assert len(data) == w * h * 2, f"captura incompleta: {len(data)} bytes"
        wait_line(s, "MSR_SHOT_END")
        png(os.path.join(out, arg[5:] + ".png"), w, h, data)
        print("guardada", arg[5:])
    else:
        s.write((arg + "\n").encode())
        wait_line(s, "MSR_CMD_OK")
        time.sleep(0.3)
s.close()

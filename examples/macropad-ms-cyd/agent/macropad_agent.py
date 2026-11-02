"""MacroPad MS · programa USB para Windows (para PCs sin Bluetooth).

Lee el cable USB de la CYD y convierte cada atajo en pulsaciones de teclado de Windows.
Solo entiende lineas "MSR_KEY kind=key|media mod=N code=N" y solo pulsa las teclas de
la lista de abajo: no ejecuta comandos ni abre archivos.

Uso:
    python macropad_agent.py              busca la CYD (chip CH340) y se conecta
    python macropad_agent.py --port COM4  elige el puerto a mano
    python macropad_agent.py --dry-run    muestra los atajos sin pulsar nada

Requisitos: Python 3.9+ y pyserial (pip install pyserial). Cierra el programa (Ctrl+C)
antes de grabar otro firmware en la placa: ocupa el puerto COM.
Licencia MIT, como el resto de AB MakerIDE.
"""

from __future__ import annotations

import argparse
import ctypes
import re
import sys
import time

import serial
from serial.tools import list_ports

BAUD = 115200
HELLO_EVERY_S = 2.0
LINE = re.compile(r"^MSR_KEY kind=(key|media) mod=(\d{1,3}) code=(\d{1,5})$")

# Modificadores HID -> teclas virtuales de Windows
MODIFIERS = [(0x01, 0x11), (0x02, 0x10), (0x04, 0x12), (0x08, 0x5B)]  # Ctrl, Mayús, Alt, Win

# Teclas HID permitidas -> tecla virtual de Windows
KEYS = {0x04 + i: 0x41 + i for i in range(26)}  # A..Z
KEYS.update({0x68 + i: 0x7C + i for i in range(12)})  # F13..F24

# Usos de consumo (multimedia) -> tecla virtual de Windows
MEDIA = {
    0xCD: 0xB3,  # play/pausa
    0xB5: 0xB0,  # siguiente
    0xB6: 0xB1,  # anterior
    0xE2: 0xAD,  # silencio
    0xE9: 0xAF,  # volumen +
    0xEA: 0xAE,  # volumen -
}

GUI = 0x08
KEY_L = 0x0F


def find_port() -> str | None:
    """Devuelve el primer puerto con el chip USB-serie CH340 de la CYD."""
    for p in list_ports.comports():
        if p.vid == 0x1A86 and p.pid == 0x7523:
            return p.device
    return None


def describe(kind: str, mod: int, code: int) -> str:
    names = [n for bit, n in ((0x01, "Ctrl"), (0x02, "Mayús"), (0x04, "Alt"), (0x08, "Win")) if mod & bit]
    if kind == "media":
        return {0xCD: "Play/Pausa", 0xB5: "Siguiente", 0xB6: "Anterior", 0xE2: "Silencio",
                0xE9: "Volumen +", 0xEA: "Volumen -"}.get(code, f"multimedia 0x{code:X}")
    if 0x04 <= code <= 0x1D:
        key = chr(ord("A") + code - 0x04)
    elif 0x68 <= code <= 0x73:
        key = f"F{13 + code - 0x68}"
    else:
        key = f"0x{code:X}"
    return "+".join(names + [key])


class Keyboard:
    """Pulsaciones con keybd_event de user32 (solo Windows)."""

    KEYUP = 0x0002
    EXTENDED = 0x0001

    def __init__(self) -> None:
        self.user32 = ctypes.WinDLL("user32", use_last_error=True)

    def _key(self, vk: int, up: bool = False, extended: bool = False) -> None:
        flags = (self.KEYUP if up else 0) | (self.EXTENDED if extended else 0)
        self.user32.keybd_event(vk, 0, flags, 0)

    def press(self, kind: str, mod: int, code: int) -> bool:
        if kind == "media":
            vk = MEDIA.get(code)
            if vk is None:
                return False
            self._key(vk, extended=True)
            self._key(vk, up=True, extended=True)
            return True
        vk = KEYS.get(code)
        if vk is None or mod & ~0x0F:
            return False
        if mod == GUI and code == KEY_L:
            # Windows no deja simular Win+L: se bloquea con la funcion del sistema.
            return bool(self.user32.LockWorkStation())
        mods = [m_vk for bit, m_vk in MODIFIERS if mod & bit]
        for m_vk in mods:
            self._key(m_vk, extended=(m_vk == 0x5B))
        self._key(vk)
        self._key(vk, up=True)
        for m_vk in reversed(mods):
            self._key(m_vk, up=True, extended=(m_vk == 0x5B))
        return True


def run(port: str, dry_run: bool) -> None:
    kb = None if dry_run else Keyboard()
    link = serial.Serial()
    link.port, link.baudrate, link.timeout = port, BAUD, 0.2
    # DTR/RTS bajos antes de abrir: si no, el circuito de autoarranque reinicia la placa.
    link.dtr = False
    link.rts = False
    link.open()
    with link:
        print(f"MacroPad MS · conectado a {port}{' (simulacion: no se pulsa nada)' if dry_run else ''}")
        print("Toca los botones de la CYD. Ctrl+C para salir.")
        last_hello = 0.0
        while True:
            now = time.monotonic()
            if now - last_hello >= HELLO_EVERY_S:
                link.write(b"MSR_HELLO\n")
                last_hello = now
            raw = link.readline()
            if not raw:
                continue
            text = raw.decode("ascii", errors="ignore").strip()
            if text == "MSR_MACROPAD_READY":
                continue
            m = LINE.match(text)
            if not m:
                continue
            kind, mod, code = m.group(1), int(m.group(2)), int(m.group(3))
            label = describe(kind, mod, code)
            if dry_run:
                print(f"  atajo: {label}")
            elif kb.press(kind, mod, code):
                print(f"  pulsado: {label}")
            else:
                print(f"  ignorado (no permitido): {label}")


def main() -> int:
    parser = argparse.ArgumentParser(description="MacroPad MS: atajos de la CYD por USB en Windows")
    parser.add_argument("--port", help="puerto COM de la CYD (por defecto, se busca el CH340)")
    parser.add_argument("--dry-run", action="store_true", help="muestra los atajos sin pulsar teclas")
    args = parser.parse_args()
    if sys.platform != "win32" and not args.dry_run:
        print("Este programa pulsa teclas en Windows. En otro sistema usa --dry-run o el Bluetooth.")
        return 2
    port = args.port or find_port()
    if not port:
        print("No encuentro la CYD. Conectala por USB o indica el puerto con --port COM4.")
        return 1
    while True:
        try:
            run(port, args.dry_run)
        except KeyboardInterrupt:
            print("\nHasta luego.")
            return 0
        except serial.SerialException as exc:
            print(f"Puerto {port} no disponible ({exc}). Reintento en 3 s...")
            time.sleep(3)


if __name__ == "__main__":
    sys.exit(main())

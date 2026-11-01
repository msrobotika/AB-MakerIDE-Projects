"""MS Robotika GhostGuard: read-only Windows telemetry -> ESP32 USB serial.
No privilege elevation, process termination, firewall changes or network listener.
"""
from __future__ import annotations
import argparse
import ctypes
import json
import os
import platform
import subprocess
import sys
import time
from pathlib import Path

try:
    import psutil
    import serial
    from serial.tools import list_ports
except ImportError as exc:
    raise SystemExit(f"Dependencia ausente: {exc}. Ejecuta instalar.bat")

POWERSHELL = 'powershell.exe'
DEFENDER_SCRIPT = r"""
$ErrorActionPreference='Stop'
try {
 $x=Get-MpComputerStatus
 if ($x.AntivirusEnabled -and $x.RealTimeProtectionEnabled) { Write-Output '1' }
 elseif ($x.AntivirusEnabled -eq $false -or $x.RealTimeProtectionEnabled -eq $false) { Write-Output '0' }
 else { Write-Output '-1' }
} catch { Write-Output '-1' }
"""
EVENT_SCRIPT = r"""
$ErrorActionPreference='Stop'
try {
 $start=(Get-Date).AddHours(-24)
 $ev=Get-WinEvent -FilterHashtable @{LogName='Microsoft-Windows-Windows Defender/Operational';StartTime=$start} -MaxEvents 200 -ErrorAction Stop
 $items=@($ev | Where-Object { $_.Id -in @(1006,1007,1008,1015,1116,1117,1118,1119) })
 $last=$items | Select-Object -First 1
 if ($null -eq $last) { Write-Output '{"count":0,"event":"Sin eventos de deteccion"}' }
 else {
  $message=('Defender ID '+$last.Id+' '+$last.TimeCreated.ToString('HH:mm'))
  [pscustomobject]@{count=$items.Count;event=$message} | ConvertTo-Json -Compress
 }
} catch { Write-Output '{"count":-1,"event":"Registro no disponible"}' }
"""

def powershell(script: str, timeout: int = 9) -> str:
    try:
        cp = subprocess.run([POWERSHELL, '-NoProfile', '-NonInteractive', '-Command', script],
                            capture_output=True, text=True, timeout=timeout,
                            creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        return cp.stdout.strip() if cp.returncode == 0 else ''
    except (OSError, subprocess.TimeoutExpired):
        return ''


def discover_port() -> str:
    ports = list(list_ports.comports())
    candidates = [p for p in ports if any(x in (p.description or '').lower()
                  for x in ('cp210', 'ch340', 'usb serial', 'silicon labs'))]
    if len(candidates) == 1:
        return candidates[0].device
    if len(ports) == 1:
        return ports[0].device
    raise RuntimeError('Selecciona puerto con --port COM4 (puertos: ' +
                       ', '.join(p.device for p in ports) + ')')


def clean_ascii(value: str, limit: int) -> str:
    return ''.join(c for c in str(value) if 32 <= ord(c) < 127 and c not in '\\"')[:limit]


def get_disk_percent() -> int:
    try:
        drive = os.environ.get('SystemDrive', 'C:') + '\\'
        return round(psutil.disk_usage(drive).percent)
    except (OSError, ValueError):
        return -1


def get_connections() -> int:
    try:
        return sum(c.status == psutil.CONN_ESTABLISHED for c in psutil.net_connections(kind='inet'))
    except (OSError, psutil.AccessDenied):
        return -1


def run(port: str, interval: float, dry_run: bool) -> None:
    psutil.cpu_percent(interval=None)
    old = psutil.net_io_counters()
    old_time = time.monotonic()
    defender, events, event_text = -1, -1, 'Sin datos'
    next_defender, next_events = 0.0, 0.0
    conn = None
    if not dry_run:
        conn = serial.Serial(port=port, baudrate=115200, timeout=0, write_timeout=2)
        # Serial opening may reset the ESP32; give it time to boot.
        time.sleep(2.5)
        conn.reset_input_buffer()
    print(f'GhostGuard Windows agent | {"DRY RUN" if dry_run else port} | Ctrl+C para salir')
    try:
        while True:
            now = time.monotonic()
            if now >= next_defender:
                raw = powershell(DEFENDER_SCRIPT)
                defender = int(raw) if raw in ('-1', '0', '1') else -1
                next_defender = now + 30
            if now >= next_events:
                raw = powershell(EVENT_SCRIPT)
                try:
                    data = json.loads(raw)
                    events = int(data.get('count', -1))
                    event_text = clean_ascii(data.get('event', ''), 55)
                except (ValueError, TypeError, AttributeError):
                    events, event_text = -1, 'Registro no disponible'
                next_events = now + 45
            net = psutil.net_io_counters()
            elapsed = max(now - old_time, 0.1)
            down = round(max(0, net.bytes_recv - old.bytes_recv) / elapsed / 1024)
            up = round(max(0, net.bytes_sent - old.bytes_sent) / elapsed / 1024)
            old, old_time = net, now
            payload = {
                'type': 'telemetry', 'host': clean_ascii(platform.node(), 20),
                'cpu': round(psutil.cpu_percent(interval=None)),
                'ram': round(psutil.virtual_memory().percent),
                'disk': get_disk_percent(), 'connections': get_connections(),
                'down': down, 'up': up, 'defender': defender,
                'events': events, 'event': event_text,
            }
            wire = json.dumps(payload, separators=(',', ':'), ensure_ascii=True) + '\n'
            if len(wire) >= 320:
                raise ValueError('Trama demasiado larga para firmware')
            if dry_run:
                print(wire.rstrip())
            else:
                assert conn is not None
                conn.write(wire.encode('ascii'))
                conn.flush()
                print(f'PC {payload["host"]} | CPU {payload["cpu"]}% | RAM {payload["ram"]}% | Defender {defender} | TX OK', flush=True)
            time.sleep(interval)
    finally:
        if conn is not None:
            conn.close()


def main() -> None:
    if sys.platform != 'win32':
        raise SystemExit('Este agente es para Windows. Para pruebas, ejecuta tests/test_protocol.py')
    ap = argparse.ArgumentParser(description='GhostGuard PC -> CYD, solo lectura')
    ap.add_argument('--port', default='auto', help='Puerto serie, p.ej. COM4')
    ap.add_argument('--interval', type=float, default=2.0)
    ap.add_argument('--dry-run', action='store_true', help='No abrir puerto; imprimir tramas')
    args = ap.parse_args()
    if args.interval < 1.0:
        ap.error('--interval debe ser >= 1')
    port = args.port if args.port != 'auto' or args.dry_run else discover_port()
    try:
        run(port, args.interval, args.dry_run)
    except serial.SerialException as exc:
        raise SystemExit(f'Error serie: {exc}. Cierra Monitor Serie y reintenta.')
    except KeyboardInterrupt:
        print('\nAgente detenido')

if __name__ == '__main__':
    main()

"""Compile the supplied firmware; never install tools or upload to a device."""
from pathlib import Path
import argparse
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--arduino-cli', help='Arduino CLI executable or absolute path')
    args = parser.parse_args()
    executable = args.arduino_cli or shutil.which('arduino-cli')
    fallback = Path(r'C:\Program Files\Arduino CLI\arduino-cli.exe')
    if not executable and fallback.is_file():
        executable = str(fallback)
    if not executable:
        parser.error('Arduino CLI not found; install it or use --arduino-cli PATH')
    root = Path(__file__).resolve().parent.parent
    source = root / '.build' / 'RadarCYD'
    output = root / '.build' / 'output'
    source.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / 'sketch.ino', source / 'RadarCYD.ino')
    for name in ('radar_logic.h', 'radar_selftest.h'):
        shutil.copy2(root / name, source / name)
    return subprocess.call([
        executable, 'compile', '--fqbn', 'esp32:esp32:jczn_2432s028r',
        '--warnings', 'default', '--output-dir', str(output), str(source),
    ])


if __name__ == '__main__':
    sys.exit(main())

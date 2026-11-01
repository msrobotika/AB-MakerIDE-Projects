"""Compile the complete CYD firmware; never upload or install tools."""
from pathlib import Path
import argparse,shutil,subprocess,sys
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--arduino-cli')
    args=p.parse_args()
    cli=args.arduino_cli or shutil.which('arduino-cli')
    fallback=Path(r'C:\Program Files\Arduino CLI\arduino-cli.exe')
    if not cli and fallback.is_file():cli=str(fallback)
    if not cli:p.error('Install Arduino CLI or pass --arduino-cli PATH')
    root=Path(__file__).resolve().parent.parent
    stage=root/'.build'/'WeatherStationCYD';output=root/'.build'/'firmware'
    stage.mkdir(parents=True,exist_ok=True);output.mkdir(parents=True,exist_ok=True)
    shutil.copy2(root/'sketch.ino',stage/'WeatherStationCYD.ino')
    for name in ('weather_model.h','weather_ca.h'):shutil.copy2(root/name,stage/name)
    return subprocess.call([cli,'compile','--fqbn','esp32:esp32:jczn_2432s028r','--warnings','default','--jobs','4','--output-dir',str(output),str(stage)])
if __name__=='__main__':sys.exit(main())

from pathlib import Path
import argparse,shutil,subprocess,sys
root=Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser(description='Compile the full firmware without uploading or installing anything.')
p.add_argument('--arduino-cli');args=p.parse_args()
cli=args.arduino_cli or shutil.which('arduino-cli')
if not cli:p.error('Install Arduino CLI or pass --arduino-cli PATH')
subprocess.run([sys.executable,str(root/'tools/generate_ui.py'),'--check'],check=True)
stage=root/'.build/MiniNasCYD';output=root/'.build/firmware'
stage.mkdir(parents=True,exist_ok=True);output.mkdir(parents=True,exist_ok=True)
principal=root/'sketch.ino'
if not principal.is_file():principal=root/'MiniNasCYD.ino'
shutil.copy2(principal,stage/'MiniNasCYD.ino')
for name in ['nas_rules.h','nas_selftest.h','nas_touch.h','nas_web.h','nas_qr.h']:shutil.copy2(root/name,stage/name)
shutil.copytree(root/'src',stage/'src',dirs_exist_ok=True)
sys.exit(subprocess.call([cli,'compile','--fqbn','esp32:esp32:jczn_2432s028r','--warnings','all','--jobs','4','--output-dir',str(output),str(stage)]))

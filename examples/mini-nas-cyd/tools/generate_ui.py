from pathlib import Path
import argparse
root=Path(__file__).resolve().parent.parent
p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');args=p.parse_args()
result='#pragma once\n#include <pgmspace.h>\n'
for name,file in [('NAS_HTML','index.html'),('NAS_JS','app.js')]:
    s=(root/'web'/file).read_text(encoding='utf-8')
    assert ')NASRESOURCE"' not in s
    result+=f'const char {name}[] PROGMEM = R"NASRESOURCE({s})NASRESOURCE";\n'
target=root/'nas_web.h'
if args.check:
    assert target.read_text(encoding='utf-8')==result,'Regenerate nas_web.h'
else:target.write_text(result,encoding='utf-8',newline='\n')
print('Embedded UI matches HTML/JS.' if args.check else 'Embedded UI generated.')

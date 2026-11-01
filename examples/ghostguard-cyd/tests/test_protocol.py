import json
import sys
import types
try:
    import serial
except ImportError:
    serial=types.ModuleType("serial")
    serial.SerialException=Exception
    tools=types.ModuleType("serial.tools")
    ports=types.ModuleType("serial.tools.list_ports")
    ports.comports=lambda: []
    tools.list_ports=ports
    serial.tools=tools
    sys.modules["serial"]=serial
    sys.modules["serial.tools"]=tools
    sys.modules["serial.tools.list_ports"]=ports
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'windows_agent'))
from ghostguard_agent import clean_ascii

def test_payload():
    payload={'type':'telemetry','host':clean_ascii('PC"\\BAD\n',20),'cpu':27,'ram':46,'disk':58,'connections':12,'down':32,'up':9,'defender':1,'events':0,'event':'Sin eventos'}
    wire=json.dumps(payload,separators=(',',':'),ensure_ascii=True)+'\n'
    assert len(wire)<320
    assert json.loads(wire)['cpu']==27
    assert payload['host']=='PCBAD'

if __name__=='__main__':
    test_payload()
    print('PASS: protocolo JSON, longitud y saneamiento')

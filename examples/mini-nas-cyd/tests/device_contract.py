"""Optional physical-device tests; never flash hardware.

Default: authenticated read-only status/list checks. --write-tests explicitly
creates two uniquely named test files and removes only the completed test file.
Does not format the card or touch existing user files. Password stays in memory.
"""
import argparse, base64, getpass, hashlib, http.client, json, os, time, uuid
from urllib.request import Request, urlopen
from urllib.error import HTTPError
from urllib.parse import urlparse, urlencode

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--url',default='http://192.168.4.1')
p.add_argument('--write-tests',action='store_true');args=p.parse_args()
base=args.url.rstrip('/');parsed=urlparse(base)
assert parsed.scheme=='http' and parsed.hostname and not parsed.username and not parsed.query and parsed.path in ('','/'), 'Use the local HTTP address of your NAS'
password=getpass.getpass('Clave de sesion mostrada en la pantalla (no se guarda): ')
authorization='Basic '+base64.b64encode(('maker:'+password).encode()).decode()
token=''
def request(path,method='GET',body=None,auth=True,csrf=False,extra=None):
    headers={}
    if auth:headers['Authorization']=authorization
    if csrf:headers['X-NAS-Token']=token
    if extra:headers.update(extra)
    req=Request(base+path,data=body,headers=headers,method=method)
    try:
        with urlopen(req,timeout=30) as response:return response.status,response.read()
    except HTTPError as e:return e.code,e.read()

code,_=request('/api/status',auth=False);assert code==401
code,raw=request('/api/status');assert code==200;status=json.loads(raw)
assert 'token' in status and 'ap_password' not in status and 'password' not in status
assert status['sd_ready'] is True, 'Insert a FAT32 card while powered off and restart'
assert 0<=status['used']<=status['total'] and status['free']==status['total']-status['used']
assert status['max_file']==128*1024*1024
token=status['token']
code,raw=request('/api/files?offset=0');assert code==200
listing=json.loads(raw);assert isinstance(listing['files'],list) and len(listing['files'])<=40
print('PASS: anonymous access rejected; authenticated status and list match contract.')
if not args.write_tests:
    print('Read-only checks finished. Physical upload/download/cancel/delete not run.');raise SystemExit(0)

name='nas-test-'+uuid.uuid4().hex+'.dat';interrupted='nas-partial-'+uuid.uuid4().hex+'.dat'
payload=os.urandom(32*1024+17);created=False
extra={'Content-Type':'application/octet-stream','X-File-Name':name}
try:
    code,_=request('/api/upload','POST',payload,auth=False,extra=extra);assert code==401
    code,_=request('/api/upload','POST',payload,extra=extra);assert code==403
    for bad in ['../x','folder/file','CON.txt','.upload.part','file.','a%2fb']:
        code,_=request('/api/upload','POST',b'x',csrf=True,extra={**extra,'X-File-Name':bad});assert code==400
    code,_=request('/api/upload','POST',payload,csrf=True,extra=extra);assert code==201;created=True
    code,download=request('/api/download?'+urlencode({'name':name}));assert code==200 and hashlib.sha256(download).digest()==hashlib.sha256(payload).digest()
    code,_=request('/api/upload','POST',b'other',csrf=True,extra=extra);assert code==409
    code,download=request('/api/download?'+urlencode({'name':name}));assert code==200 and download==payload
    connection=http.client.HTTPConnection(parsed.hostname,parsed.port or 80,timeout=5)
    connection.putrequest('POST','/api/upload')
    for k,v in {'Authorization':authorization,'X-NAS-Token':token,'X-File-Name':interrupted,'Content-Type':'application/octet-stream','Content-Length':'100000'}.items():connection.putheader(k,v)
    connection.endheaders();connection.send(b'partial bytes');connection.close()
    # Allow the ESP32 raw parser to observe the aborted connection.
    for attempt in range(10):
        try:
            code,_=request('/api/download?'+urlencode({'name':interrupted}))
            if code==404:break
        except (TimeoutError,OSError):pass
        time.sleep(.5)
    else:raise AssertionError('Interrupted upload must never become a completed file')
    print('PASS: authentication/token/name checks, binary round-trip, duplicate preservation and interrupted transfer.')
finally:
    if created:
        code,_=request('/api/delete?'+urlencode({'name':name}),'POST',b'',csrf=True)
        assert code==200,'Remove only the uniquely named nas-test file if cleanup failed'
        code,_=request('/api/download?'+urlencode({'name':name}));assert code==404
        print('PASS: completed test file removed; existing user files left untouched.')

"""Genera los datos de estrellas y los certificados de AstroCYD MS.

Uso: python -I generar_datos.py CARPETA_D3_CELESTIAL CA_BUNDLE.crt
  CARPETA_D3_CELESTIAL: data/stars.6.json, constellations.lines.json, constellations.json y
  starnames.json de https://github.com/ofrohn/d3-celestial (BSD-3, (c) Olaf Frohn).
  CA_BUNDLE.crt: almacén de raíces de Mozilla (por ejemplo el de Git for Windows).
Reescribe los bloques «stars.h» y «certs.h» dentro de ../AstroCYDMS/AstroCYDMS.ino (un solo archivo,
como lo abre AB MakerIDE).
"""
import json
import os
import re
import sys
import unicodedata

src, bundle = sys.argv[1], sys.argv[2]
INO = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "AstroCYDMS", "AstroCYDMS.ino")


def write_block(name, lines):
    """Sustituye el bloque «// ===== inicio name =====» … «fin» del .ino."""
    text = open(INO, encoding="utf-8").read()
    body = "\n".join(l for l in lines if l != "#pragma once").rstrip()
    pat = re.compile(rf"// ===== inicio {re.escape(name)} =====\n.*?\n// ===== fin {re.escape(name)} =====", re.S)
    assert pat.search(text), name
    text = pat.sub(lambda _: f"// ===== inicio {name} =====\n{body}\n// ===== fin {name} =====", text, count=1)
    open(INO, "w", encoding="utf-8", newline="\n").write(text)


def load(name):
    for n in (name, "data_" + name):
        p = os.path.join(src, n)
        if os.path.exists(p):
            return json.load(open(p, encoding="utf-8"))
    raise FileNotFoundError(name)


KEEP = set("áéíóúñüÁÉÍÓÚÑÜ")


def clean(s):
    """Solo ASCII y las letras con acento que pinta la fuente CP437 de la placa."""
    s = s.replace("‎", "").replace(" ", " ").strip()
    o = ""
    for ch in s:
        if ord(ch) < 128 or ch in KEEP:
            o += ch
        else:
            base = unicodedata.normalize("NFD", ch)[0]
            o += base if ord(base) < 128 else ""
    return o.replace('"', "'")


stars = load("stars.6.json")["features"]
lines = load("constellations.lines.json")["features"]
cons = load("constellations.json")["features"]
names = load("starnames.json")

con_ids = [c["id"] for c in cons]
con_es = {c["id"]: clean(c["properties"].get("es") or c["properties"]["name"]) for c in cons}


def ra16(lon):
    return round((lon % 360) / 360 * 65536) % 65536


def dec16(lat):
    return round(lat * 100)


rows, star_names = [], []
for f in sorted(stars, key=lambda f: f["properties"]["mag"]):
    mag = f["properties"]["mag"]
    if mag > 6.0:
        continue
    lon, lat = f["geometry"]["coordinates"]
    try:
        bv = float(f["properties"].get("bv") or 0.6)
    except ValueError:
        bv = 0.6
    info = names.get(str(f["id"]), {})
    con = con_ids.index(info["c"]) if info.get("c") in con_ids else 255
    nm = 255
    proper = info.get("es") or info.get("name") or ""
    if proper and mag <= 3.0:
        star_names.append(clean(proper))
        nm = len(star_names) - 1
    rows.append((ra16(lon), dec16(lat), round(mag * 10), max(-5, min(20, round(bv * 10))), con, nm))
assert len(star_names) < 255 and len(rows) > 4000

pts, polys = [], []
for f in lines:
    ci = con_ids.index(f["id"])
    for pl in f["geometry"]["coordinates"]:
        polys.append((ci, len(pts), len(pl)))
        pts += [(ra16(p[0]), dec16(p[1])) for p in pl]

labels = []
for c in cons:
    d = c["properties"]["display"]
    labels.append((ra16(d[0]), dec16(d[1]), int(c["properties"].get("rank", 3))))

h = ["// Generado por tools/generar_datos.py con datos de d3-celestial (BSD-3, (c) 2015 Olaf Frohn).",
     "// Coordenadas J2000: ra en 1/65536 de vuelta, dec en centésimas de grado, mag y B-V por 10.",
     "#pragma once", "#include <stdint.h>",
     "struct Star { uint16_t ra; int16_t dec; int8_t mag10, bv10; uint8_t con, name; };",
     "struct CPoint { uint16_t ra; int16_t dec; };",
     "struct CPoly { uint8_t con; uint16_t first; uint8_t count; };",
     "struct CLabel { uint16_t ra; int16_t dec; uint8_t rank; };",
     f"constexpr int STAR_COUNT = {len(rows)}, CPOINT_COUNT = {len(pts)}, CPOLY_COUNT = {len(polys)}, CON_COUNT = {len(cons)};",
     "const Star STARS[] PROGMEM = {"]
h += ["  " + ",".join("{%d,%d,%d,%d,%d,%d}" % r for r in rows[i:i + 6]) + "," for i in range(0, len(rows), 6)]
h += ["};", "const CPoint CPOINTS[] PROGMEM = {"]
h += ["  " + ",".join("{%d,%d}" % p for p in pts[i:i + 10]) + "," for i in range(0, len(pts), 10)]
h += ["};", "const CPoly CPOLYS[] PROGMEM = {"]
h += ["  " + ",".join("{%d,%d,%d}" % p for p in polys[i:i + 10]) + "," for i in range(0, len(polys), 10)]
h += ["};", "const CLabel CLABELS[] PROGMEM = {"]
h += ["  " + ",".join("{%d,%d,%d}" % p for p in labels[i:i + 10]) + "," for i in range(0, len(labels), 10)]
h += ["};", "const char* const CON_NAMES[] = {"]
h += ["  " + ",".join('"%s"' % con_es[c] for c in con_ids[i:i + 6]) + "," for i in range(0, len(con_ids), 6)]
h += ["};", "const char* const STAR_NAMES[] = {"]
h += ["  " + ",".join('"%s"' % n for n in star_names[i:i + 6]) + "," for i in range(0, len(star_names), 6)]
h += ["};", ""]
write_block("stars.h", h)

text = open(bundle, encoding="utf-8").read()
want = ["ISRG Root X1", "ISRG Root X2", "Amazon Root CA 1", "Starfield Services Root Certificate Authority - G2",
        "GTS Root R1", "GTS Root R4"]
pems = []
for w in want:
    m = re.search(r"^# ?" + re.escape(w) + r"\n(-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----)", text, re.M | re.S)
    assert m, w
    pems.append(f"// {w}\n" + m.group(1))
c = ["// Raíces públicas (no son claves privadas) del almacén de Mozilla, para validar HTTPS:",
     "// Open-Meteo (ISRG), NOAA SWPC (Amazon/Starfield), ipapi.co y Google Maps (GTS).",
     "#pragma once", 'const char ROOT_CA[] = R"PEM(' + "\n".join(p.split("\n", 1)[1] for p in pems) + '\n)PEM";', ""]
write_block("certs.h", "\n".join(c).split("\n"))
print(f"{len(rows)} estrellas, {len(star_names)} con nombre, {len(polys)} trazos, {len(pts)} puntos, {len(cons)} constelaciones")

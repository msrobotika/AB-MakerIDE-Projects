"""Integracion REAL desde PC: API y raiz TLS que usa el firmware.

No prueba radio, dibujo, parser C++ ni tactil del ESP32.
"""
import datetime as dt
import hashlib
import json
import math
from pathlib import Path
import re
import ssl
import unittest
from urllib.parse import urlencode
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent.parent
CA_TEXT = (ROOT / 'weather_ca.h').read_text(encoding='utf-8')
PEM = re.search(r'-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----', CA_TEXT, re.S).group()
CONTEXT = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
CONTEXT.load_verify_locations(cadata=PEM)
FIXTURES = ROOT / '.build' / 'api-fixtures'


def fetch(host, params):
    req = Request(host + '?' + urlencode(params), headers={'Accept': 'application/json'})
    with urlopen(req, context=CONTEXT, timeout=30) as response:
        assert response.status == 200
        data = response.read(16385)
        assert len(data) <= 16384
        return json.loads(data)


class OpenMeteoContract(unittest.TestCase):
    def test_01_real_geocoding_and_forecast_with_firmware_root(self):
        # Sevilla es un dato de prueba, no la ubicacion atribuida al usuario.
        geo = fetch('https://geocoding-api.open-meteo.com/v1/search',
                    {'name': 'Sevilla', 'count': 1, 'countryCode': 'ES', 'language': 'es'})
        place = geo['results'][0]
        self.assertEqual(place['country_code'], 'ES')
        self.assertTrue(-90 <= place['latitude'] <= 90)
        self.assertTrue(-180 <= place['longitude'] <= 180)
        source = (ROOT / 'sketch.ino').read_text(encoding='utf-8')
        current = re.search(r'&current=([^"\n]+)', source).group(1)
        daily = re.search(r'&daily=([^"\n]+)', source).group(1)
        forecast = fetch('https://api.open-meteo.com/v1/forecast', {
            'latitude': f"{place['latitude']:.5f}", 'longitude': f"{place['longitude']:.5f}",
            'current': current, 'daily': daily, 'forecast_days': 3, 'timezone': 'auto',
            'temperature_unit': 'celsius', 'wind_speed_unit': 'kmh', 'precipitation_unit': 'mm'})
        dt.datetime.strptime(forecast['current']['time'], '%Y-%m-%dT%H:%M')
        expected_units = {'temperature_2m': '°C', 'apparent_temperature': '°C',
                          'relative_humidity_2m': '%', 'wind_speed_10m': 'km/h',
                          'precipitation': 'mm', 'pressure_msl': 'hPa'}
        for key in current.split(','):
            value = forecast['current'][key]
            self.assertIsInstance(value, (int, float), key)
            self.assertTrue(math.isfinite(value), key)
        for key, unit in expected_units.items():
            self.assertEqual(forecast['current_units'][key], unit)
        for key in daily.split(','):
            self.assertEqual(len(forecast['daily'][key]), 3, key)
            for value in forecast['daily'][key]:
                self.assertIsInstance(value, (int, float))
                self.assertTrue(math.isfinite(value))
        dates = forecast['daily']['time']
        self.assertEqual(len(dates), 3)
        self.assertTrue(dates[0] < dates[1] < dates[2])
        for day in dates:
            dt.date.fromisoformat(day)
        for lo, hi, probability in zip(forecast['daily']['temperature_2m_min'],
                                       forecast['daily']['temperature_2m_max'],
                                       forecast['daily']['precipitation_probability_max']):
            self.assertLessEqual(lo, hi)
            self.assertTrue(0 <= probability <= 100)
        for key, unit in {'temperature_2m_min': '°C', 'temperature_2m_max': '°C',
                          'precipitation_probability_max': '%'}.items():
            self.assertEqual(forecast['daily_units'][key], unit)
        FIXTURES.mkdir(parents=True, exist_ok=True)
        (FIXTURES / 'geocoding.json').write_text(json.dumps(geo, ensure_ascii=False, indent=2), encoding='utf-8')
        (FIXTURES / 'forecast.json').write_text(json.dumps(forecast, ensure_ascii=False, indent=2), encoding='utf-8')
        print(f"Contrato real OK: {place['name']} / {place['country_code']}, {forecast['timezone']}; TLS con raiz del firmware")

    def test_02_real_unknown_city_has_no_results(self):
        doc = fetch('https://geocoding-api.open-meteo.com/v1/search',
                    {'name': 'zzqnonexistent123xyzabc', 'count': 1, 'language': 'es'})
        self.assertFalse(doc.get('results'))



if __name__ == '__main__':
    unittest.main(verbosity=2)

#!/usr/bin/env python3
"""Verify that Pages serves this release, its demo and all firmware variants."""
import json
import sys
import time
from urllib.request import Request, urlopen
from urllib.parse import urljoin

base, version = sys.argv[1:]
base = base.rstrip('/') + '/'


def read(path, method='GET'):
    url = urljoin(base, path)
    separator = '&' if '?' in url else '?'
    request = Request(url + separator + 'release=' + version,
                      headers={'Cache-Control': 'no-cache'}, method=method)
    with urlopen(request, timeout=30) as response:
        return response.read().decode() if method == 'GET' else response.status


for attempt in range(12):
    try:
        home = read('')
        assert 'id="lcd"' in home, 'Pages is not serving the interactive demo'
        assert 'instalar.html?animation=static#instalar' in home, 'Static download link missing'
        assert 'animation.js?v=' + version in home, 'Demo assets belong to an older release'
        installer = read('instalar.html')
        assert 'id="animation-mode"' in installer, 'Firmware selector missing'
        assert 'app.js?v=' + version in installer, 'Installer script belongs to an older release'
        build = json.loads(read('firmware/build.json'))
        assert build['version'] == version, 'Firmware belongs to an older release'
        for board in ('ESP32-2432S028R-Room', 'ESP32_2432S028_2USB-Room'):
            for suffix in ('', '-Static'):
                for mode in ('factory', 'update'):
                    path = f'firmware/{board}{suffix}-{mode}.json'
                    manifest = json.loads(read(path))
                    assert manifest['version'] == version, path
                    binary = manifest['builds'][0]['parts'][0]['path']
                    assert read('firmware/' + binary, method='HEAD') == 200, binary
        print(f'OK: live demo, installer and eight firmware downloads at {base} ({version})')
        break
    except Exception as error:
        print(f'Attempt {attempt + 1}/12: {error}', flush=True)
        if attempt == 11:
            raise
        time.sleep(10)

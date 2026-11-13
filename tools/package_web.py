#!/usr/bin/env python3
"""Package only successfully compiled CYD firmware for ESP Web Tools."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ENVIRONMENTS = ('ESP32-2432S028R-Room', 'ESP32_2432S028_2USB-Room',
                'ESP32-2432S028R-Room-Static', 'ESP32_2432S028_2USB-Room-Static')


def package(destination, version):
    destination = Path(destination)
    if destination.exists():
        raise ValueError('Output must be a new directory')
    # Require all four builds before creating any installable site.
    parts = {}
    for environment in ENVIRONMENTS:
        build = ROOT / '.pio/build' / environment
        files = [build / name for name in ('bootloader.bin', 'partitions.bin', 'firmware.bin')]
        boot_app = build / 'boot_app0.bin'
        if not boot_app.exists():
            boot_app = Path.home() / '.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin'
        files.append(boot_app)
        for path in files:
            if not path.is_file() or not path.stat().st_size:
                raise ValueError(f'Missing or empty build output: {path}')
        if files[2].stat().st_size > 0x300000:
            raise ValueError(f'Firmware exceeds huge_app partition: {environment}')
        parts[environment] = files
    shutil.copytree(ROOT / 'web', destination)
    (destination / '.nojekyll').touch()
    assets = destination / 'assets'
    assets.mkdir(exist_ok=True)
    for scene in ('goku', 'saiyan', 'pikachu', 'charmander'):
        shutil.copy2(ROOT / f'artwork/ki-room/{scene}-sunset.png', assets)
    firmware_dir = destination / 'firmware'
    firmware_dir.mkdir()
    hashes = []
    for environment, files in parts.items():
        factory = firmware_dir / f'{environment}-factory.bin'
        # Patch flash parameters with esptool rather than concatenating images.
        subprocess.run([sys.executable, '-m', 'esptool', '--chip', 'esp32', 'merge_bin',
                        '-o', str(factory), '--flash_mode', 'dio', '--flash_freq', '40m',
                        '--flash_size', '4MB', '0x1000', str(files[0]),
                        '0x8000', str(files[1]), '0xe000', str(files[3]),
                        '0x10000', str(files[2])], check=True)
        update = firmware_dir / f'{environment}-update.bin'
        shutil.copy2(files[2], update)
        for mode, binary, offset in (('factory', factory, 0), ('update', update, 0x10000)):
            manifest = {'name': 'KI Room · ' + ('Sin animación' if environment.endswith('-Static') else 'Animada'), 'version': version,
                        'new_install_prompt_erase': True, 'new_install_improv_wait_time': 0,
                        'builds': [{'chipFamily': 'ESP32', 'parts': [{'path': binary.name, 'offset': offset}]}]}
            (firmware_dir / f'{environment}-{mode}.json').write_text(json.dumps(manifest, indent=2) + '\n')
            hashes.append(f'{hashlib.sha256(binary.read_bytes()).hexdigest()}  {binary.name}')
    (firmware_dir / 'SHA256SUMS.txt').write_text('\n'.join(hashes) + '\n')
    (firmware_dir / 'build.json').write_text(json.dumps({'version': version, 'hardware_tested': False}, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    package(args.output, args.version)

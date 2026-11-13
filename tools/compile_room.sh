#!/bin/bash
# Build only. Never flash a connected board or change system packages.
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"
if command -v pio >/dev/null 2>&1; then
    pio_command=(pio)
elif command -v platformio >/dev/null 2>&1; then
    pio_command=(platformio)
else
    build_venv="$repo_dir/.pio/build-tools"
    if [ ! -x "$build_venv/bin/python" ]; then
        python3 -m venv --without-pip "$build_venv"
    fi
    if ! "$build_venv/bin/python" -m pip --version >/dev/null 2>&1; then
        "$build_venv/bin/python" - <<'PY'
from pathlib import Path
import subprocess
import sys
import tempfile
import urllib.request
with tempfile.TemporaryDirectory(prefix="ki-build-") as temp:
    bootstrap = Path(temp) / "get-pip.py"
    print("Preparando pip en el entorno del proyecto...", flush=True)
    try:
        urllib.request.urlretrieve("https://bootstrap.pypa.io/get-pip.py", bootstrap)
    except Exception as error:
        raise SystemExit("No se pudo descargar pip. Ejecuta bash ~/compilar desde tu terminal de Debian con acceso a internet.\n" + str(error))
    subprocess.run([sys.executable, str(bootstrap)], check=True)
PY
    fi
    if ! "$build_venv/bin/python" -m platformio --version >/dev/null 2>&1; then
        "$build_venv/bin/python" -m pip install 'platformio>=6,<7'
    fi
    pio_command=("$build_venv/bin/python" -m platformio)
fi
mkdir -p "$repo_dir/.pio"
"${pio_command[@]}" run -e ESP32-2432S028R-Room -e ESP32_2432S028_2USB-Room -e ESP32-2432S028R-Room-Static -e ESP32_2432S028_2USB-Room-Static 2>&1 | tee "$repo_dir/.pio/room-build.log"
printf '\nCompilacion terminada. Los binarios estan en firmware/.\nNo se ha flasheado ninguna placa.\n'

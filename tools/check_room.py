#!/usr/bin/env python3
"""Compile/run the actual room renderer with a host display, and save review PNGs."""
from pathlib import Path
import os
import struct
import subprocess
import tempfile
import zlib

root = Path(__file__).resolve().parents[1]
dest = root / "artwork/ki-room/checks"
dest.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix="room-check-") as temp:
    temp = Path(temp)
    (temp / "pgmspace.h").write_text("#pragma once\n#define PROGMEM\n")
    # Compile the driver's actual page routing as well as the drawing helpers.
    driver = (root / "src/drivers/displays/esp23_2432s028r.cpp").read_text()
    fragments = []
    for signature in ("static void paintChrome(TFT_eSPI &d, int scr)",
                      "static void ensureChrome(int scr)"):
        start = driver.index(signature + "\n{")
        end = driver.index("{", start) + 1
        depth = 1
        while depth:
            depth += (driver[end] == "{") - (driver[end] == "}")
            end += 1
        fragments.append(driver[start:end])
    (temp / "room_driver_chrome.inc").write_text("\n".join(fragments))
    command = ["g++", "-std=c++11", "-Wall", "-Wextra", "-g", "-DCYD_THEME_ROOM=1",
               "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
               "-I", str(temp), "-I", str(root/"test/native_shim"),
               "-I", str(root/"lib/TFT_eSPI"), "-I", str(root/"src"),
               str(root/"tools/room_host.cpp"), str(root/"src/media/room/RoomAssets.cpp"),
               "-o", str(temp/"room-check")]
    subprocess.run(command, check=True)
    # LeakSanitizer cannot operate in a ptraced sandbox; ASan/UBSan remain on.
    subprocess.run([str(temp/"room-check"), str(temp)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))
    # Compile the same renderer with effects excluded, and verify no scene writes.
    static_command = command.copy()
    static_command.insert(1, "-DCYD_ROOM_STATIC=1")
    static_command[-1] = str(temp/"room-static-check")
    subprocess.run(static_command, check=True)
    static_output = temp / "static"
    static_output.mkdir()
    subprocess.run([str(temp/"room-static-check"), str(static_output)], check=True,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))
    for ppm in temp.glob("*.ppm"):
        header, pixels = ppm.read_bytes().split(b"\n255\n", 1)
        assert header == b"P6\n320 240" and len(pixels) == 320*240*3
        def chunk(kind, data):
            return struct.pack(">I", len(data))+kind+data+struct.pack(">I", zlib.crc32(kind+data)&0xffffffff)
        filtered = b"".join(b"\0"+pixels[y*960:(y+1)*960] for y in range(240))
        png = b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR", struct.pack(">IIBBBBB",320,240,8,2,0,0,0))
        png += chunk(b"IDAT", zlib.compress(filtered))+chunk(b"IEND", b"")
        (dest/(ppm.stem+".png")).write_bytes(png)
print("Review PNGs:", dest)

#!/usr/bin/env python3
"""Exercise the actual monitor moving-average function without ESP32 hardware."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
monitor = (root / 'src/monitor.cpp').read_text()
start = monitor.index('enum EHashRateScale')
end = monitor.index('\nmining_data getMiningData', start)
source = '''#include "Arduino.h"
#include <list>
#include <cassert>
static unsigned long elapsedKHs=990;
''' + monitor[start:end] + '''
int main(){
  assert(getCurrentHashRate(0)=="0.00");
  getCurrentHashRate(1000);
  auto count=s_hashrate_avg_list.size();
  for(int i=0;i<32;i++)getCurrentHashRate(0);
  assert(s_hashrate_avg_list.size()==count);
  assert(isfinite(s_hashrate_summ));
  assert(getCurrentHashRate(1000)=="990.00");
  elapsedKHs=0;
  for(int i=0;i<10;i++)getCurrentHashRate(1000);
  assert(getCurrentHashRate(0)=="0.00");
}
'''
with tempfile.TemporaryDirectory(prefix='hashrate-check-') as folder:
    folder = Path(folder)
    # The native shim uses unsigned decimals, which is ambiguous with its int
    # base overload. Use an int in this private copy; production code is unchanged.
    shim = (root / 'test/native_shim/Arduino.h').read_text()
    (folder / 'Arduino.h').write_text(shim.replace('unsigned decimals = 2', 'int decimals = 2'))
    (folder / 'test.cpp').write_text(source)
    subprocess.run(['g++', '-std=c++11', '-fsanitize=undefined', '-I', str(folder),
                    str(folder / 'test.cpp'), '-o', str(folder / 'test')], check=True)
    subprocess.run([str(folder / 'test')], check=True)
print('OK: zero-interval repaints preserve the actual hashrate moving average')

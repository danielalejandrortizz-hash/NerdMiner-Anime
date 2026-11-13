#!/usr/bin/env python3
"""Exercise the actual local clock/page-entry functions without network services."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
driver = (root / 'src/drivers/displays/esp23_2432s028r.cpp').read_text()


def function(signature):
    start = driver.index(signature + '\n{')
    brace = driver.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (driver[end] == '{') - (driver[end] == '}')
        end += 1
    return driver[start:end]


source = r'''#include "Arduino.h"
#include <cassert>
#define CYD_THEME_ROOM 1
enum { SCR_MINE, SCR_MARKET, SCR_CLOCK };
struct MineV {};
struct MarketV {};
struct ClockV { String hh, mm, date; };
static MineV lastMine;
static MarketV lastMarket;
static ClockV lastClock, painted;
static bool haveMine=false, haveMarket=false, haveClock=false, uiLive=false;
static bool synced=false;
static int uiScreen=-1, paints=0, hourValue=18, minuteValue=42;
static bool roomActive(){return true;}
static void ensureChrome(int screen){uiScreen=screen;}
static void roomChrome(int){}
static void roomStatsScreen(unsigned long){}
static void roomHashrateScreen(unsigned long){}
static void paintMine(const MineV&){}
static void paintMarket(const MarketV&){}
static void paintClock(const ClockV &c){painted=c;++paints;}
static void roomPaintClock(const ClockV &c){paintClock(c);}
static String fmtDate(const char *date){return String(date);}
static bool getLocalClock(int &h,int &m,int &s,int &d,int &mo,int &y){
  h=hourValue;m=minuteValue;s=0;d=13;mo=11;y=2026;return synced;
}
static void roomClockScreen();
''' + function('static void repaintCached(int scr)') + '\n' + function('static void roomClockScreen()') + r'''
int main(){
  // First visit, before time sync: visible placeholders, no network API exists.
  repaintCached(SCR_CLOCK);
  assert(paints==1 && haveClock && uiLive && uiScreen==SCR_CLOCK);
  assert(painted.hh=="--" && painted.mm=="--");
  synced=true;
  for(int visit=0;visit<100;++visit){
    uiScreen=SCR_MINE;minuteValue=visit%60;
    repaintCached(SCR_CLOCK);
    assert(uiScreen==SCR_CLOCK && painted.hh=="18");
    char mm[3];snprintf(mm,sizeof(mm),"%02d",minuteValue);
    assert(painted.mm==String(mm));
  }
  assert(paints==101);
}
'''
with tempfile.TemporaryDirectory(prefix='room-navigation-') as folder:
    folder = Path(folder)
    (folder / 'test.cpp').write_text(source)
    subprocess.run(['g++', '-std=c++11', '-fsanitize=undefined', '-I', str(root/'test/native_shim'),
                    str(folder/'test.cpp'), '-o', str(folder/'test')], check=True)
    subprocess.run([str(folder/'test')], check=True)
print('OK: first clock visit, missing time sync, 100 page changes, no network dependency')

# Use the production monitor delay: a faster particle renderer must not be
# capped by the old 100 ms task sleep. Static/non-Room builds keep that sleep.
mining = (root / 'src/mining.cpp').read_text()
start = mining.index('#if defined(CYD_THEME_ROOM) && !defined(CYD_ROOM_STATIC)')
end = mining.index('#endif', start) + len('#endif')
timing = mining[start:end]
with tempfile.TemporaryDirectory(prefix='room-timing-') as folder:
    folder = Path(folder)
    for flags, expected in [(['-DCYD_THEME_ROOM=1'], 10),
                            (['-DCYD_THEME_ROOM=1', '-DCYD_ROOM_STATIC=1'], 100),
                            ([], 100)]:
        (folder/'timing.cpp').write_text(timing + f'\nstatic_assert(DELAY == {expected}, "UI scheduling");\nint main(){{}}\n')
        subprocess.run(['g++', '-std=c++11', *flags, str(folder/'timing.cpp'),
                        '-o', str(folder/'timing')], check=True)
print('OK: animated monitor scheduling; static and other builds retain their delay')

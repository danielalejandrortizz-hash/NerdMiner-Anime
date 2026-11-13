#!/usr/bin/env python3
"""Compare the firmware's actual motion geometry with the unchanged web renderer."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=r'''#include <cmath>
#include <cstdint>
#include <cstdio>
#define SCR_W 320
#define SCR_H 240
#define RGB565(r,g,b) ((uint16_t)((((r)&248)<<8)|(((g)&252)<<3)|((b)>>3)))
static int roomTheme=0;
static uint16_t roomRawPixel(int,int){return 0;}
static uint16_t roomPixel(int,int){return 0;}
static uint16_t mix565(uint16_t a,uint16_t,uint8_t){return a;}
#include "drivers/displays/roomWebMotion.inc"
int main(){
  printf("[");bool comma=false;
  for(int theme=0;theme<4;++theme)for(float t:{0.0f,.5f,1.125f,2.375f,4.125f}){
    RoomWebFrame f{};roomBuildWebFrame(f,t,theme);
    printf("%s{\"theme\":%d,\"t\":%.6f,\"glows\":[",comma?",":"",theme,t);comma=true;
    for(int i=0;i<f.ng;++i){auto &g=f.glows[i];printf("%s[%.6f,%.6f,%.6f,%.6f,%u]",i?",":"",g.x,g.y,g.r,g.alpha,g.color);}
    printf("],\"rects\":[");
    for(int i=0;i<f.nr;++i){auto &r=f.rects[i];printf("%s[%.6f,%.6f,%.6f,%.6f,%.6f,%u]",i?",":"",r.x,r.y,r.w,r.h,r.alpha,r.color);}
    printf("],\"petals\":[");
    for(int i=0;i<28;++i){auto &p=f.petals[i];printf("%s[%.6f,%.6f,%.6f,%.6f,%.6f,%u]",i?",":"",p.x,p.y,p.rx,p.ry,p.angle,p.color);}
    printf("],\"water\":[");for(int i=0;i<52;++i)printf("%s%.6f",i?",":"",f.water[i]);
    printf("],\"body\":%.6f}",f.goku?f.arm[31]*f.taper[12]:0);
  }
  printf("]");
}
'''.replace('#include <cstdio>','#include <cstdio>\n#include <initializer_list>')
with tempfile.TemporaryDirectory(prefix='room-web-reference-') as folder:
    folder=Path(folder)
    (folder/'reference.cpp').write_text(source)
    subprocess.run(['g++','-std=c++11','-fsanitize=undefined','-I',str(root/'src'),str(folder/'reference.cpp'),'-o',str(folder/'reference')],check=True)
    with (folder/'native.json').open('w') as output:
        subprocess.run([str(folder/'reference')],stdout=output,check=True)
    subprocess.run(['node',str(root/'tools/compare_room_web.cjs'),str(folder/'native.json')],check=True)

// Desktop exercise of the actual room renderer and embedded assets.
// The display stand-in checks bounds and records RGB565 pixels. It is not an ESP32.
#include "Arduino.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <vector>
#define LOAD_GFXFF
#include <Fonts/GFXFF/gfxfont.h>
#include <Fonts/glcdfont.c>
#include <Fonts/Font16.c>
#include "media/room/RoomAssets.h"
#define pgm_read_word(p) (*(p))
#define RGB565(r,g,b) ((uint16_t)((((r)&0xf8)<<8)|(((g)&0xfc)<<3)|((b)>>3)))
enum { TL_DATUM, TC_DATUM, TR_DATUM, MC_DATUM };
enum { SCR_MINE, SCR_MARKET, SCR_CLOCK };
enum { NM_waitingConfig, NM_Connecting, NM_hashing };
enum { W_HERO, W_DATE, W_HDR, W_T0, W_T1, W_T2, W_T3, W_HH, W_COLON, W_MM };
static constexpr int SCR_W=320, SCR_H=240;
static constexpr uint16_t C_BG=RGB565(5,7,13), C_EDGE=RGB565(30,41,62);
static int uiScreen=0;
static uint8_t roomTheme=0;
static bool roomActive(){return roomTheme<4;}
static uint32_t g_shareFoundEvents=0;
static unsigned long fakeMs=0;
unsigned long millis(){return fakeMs;}
static constexpr int WL_CONNECTED=3;
static struct { int state=WL_CONNECTED; int status(){return state;} } WiFi;
static unsigned pushes=0;
static size_t peakSprite=0;

struct TFT_eSPI {
  int width=320,height=240,datum=TL_DATUM,fontId=1;
  const GFXfont *freeFont=nullptr;
  uint16_t color=0;
  bool swap=false;
  std::vector<uint16_t> pixels=std::vector<uint16_t>(320*240);
  void drawPixel(int x,int y,uint16_t c){if(x>=0&&y>=0&&x<width&&y<height)pixels[y*width+x]=c;}
  void pushImage(int x,int y,int w,int h,uint16_t *p){
    assert(x>=0&&y>=0&&x+w<=width&&y+h<=height);++pushes;
    for(int row=0;row<h;row++)for(int col=0;col<w;col++)pixels[(y+row)*width+x+col]=p[row*w+col];
  }
  void setSwapBytes(bool value){swap=value;}bool getSwapBytes(){return swap;}
  void setTextWrap(bool){}void setTextPadding(int){}void setTextSize(int){}
  void setTextFont(int value){fontId=value;freeFont=nullptr;}
  void setTextDatum(int value){datum=value;}void setTextColor(uint16_t c){color=c;}
  void setFreeFont(const GFXfont *f){freeFont=f;fontId=1;}
  int textWidth(const String &s){
    int w=0;for(unsigned i=0;i<s.length();++i){unsigned ch=(unsigned char)s[i];
      if(freeFont&&ch>=freeFont->first&&ch<=freeFont->last)w+=freeFont->glyph[ch-freeFont->first].xAdvance;
      else if(fontId==2&&ch>=32&&ch<128)w+=widtbl_f16[ch-32];else w+=6;
    }return w;
  }
  void drawString(const String &s,int x,int y){
    const int w=textWidth(s);int top=0,bottom=8;
    if(freeFont){top=0;bottom=0;for(unsigned i=0;i<s.length();++i){unsigned ch=(unsigned char)s[i];if(ch<freeFont->first||ch>freeFont->last)continue;const auto &g=freeFont->glyph[ch-freeFont->first];top=std::min(top,int(g.yOffset));bottom=std::max(bottom,int(g.yOffset+g.height));}}
    else if(fontId==2)bottom=16;
    if(datum==TC_DATUM||datum==MC_DATUM)x-=w/2;
    if(datum==TR_DATUM)x-=w;
    if(datum==MC_DATUM)y-=(bottom-top)/2;
    if(freeFont)y-=top;
    for(unsigned i=0;i<s.length();++i){unsigned ch=(unsigned char)s[i];
      if(freeFont&&ch>=freeFont->first&&ch<=freeFont->last){
        const auto &g=freeFont->glyph[ch-freeFont->first];
        for(unsigned row=0;row<g.height;row++)for(unsigned col=0;col<g.width;col++){
          unsigned bit=row*g.width+col;
          if(freeFont->bitmap[g.bitmapOffset+bit/8]&(0x80>>(bit%8)))drawPixel(x+g.xOffset+col,y+g.yOffset+row,color);
        }x+=g.xAdvance;
      }else if(fontId==2&&ch>=32&&ch<128){
        int advance=widtbl_f16[ch-32],stride=(advance+6)/8;
        const unsigned char *bits=chrtbl_f16[ch-32];
        for(int row=0;row<16;row++)for(int col=0;col<advance-1;col++)if(bits[row*stride+col/8]&(0x80>>(col%8)))drawPixel(x+col,y+row,color);
        x+=advance;
      }else{if(ch<128)for(int col=0;col<5;col++)for(int row=0;row<8;row++)if(font[ch*5+col]&(1<<row))drawPixel(x+col,y+row,color);x+=6;}
    }
  }
  void drawRect(int x,int y,int w,int h,uint16_t c){for(int i=0;i<w;i++){drawPixel(x+i,y,c);drawPixel(x+i,y+h-1,c);}for(int i=0;i<h;i++){drawPixel(x,y+i,c);drawPixel(x+w-1,y+i,c);}}
  void drawRoundRect(int x,int y,int w,int h,int,uint16_t c){drawRect(x,y,w,h,c);}
};
static TFT_eSPI tft,wgt;
static TFT_eSPI &out(){return tft;}
static bool wBegin(int w,int h,uint16_t bg){assert(w>0&&h>0&&w<=320&&h<=240);peakSprite=std::max(peakSprite,size_t(w*h*2));wgt.width=w;wgt.height=h;wgt.pixels.assign(w*h,bg);return true;}
static void wEnd(int x,int y){tft.pushImage(x,y,wgt.width,wgt.height,wgt.pixels.data());}
static void clearScreen(TFT_eSPI &d){std::fill(d.pixels.begin(),d.pixels.end(),C_BG);}
static uint16_t mix565(uint16_t a,uint16_t b,uint8_t t){
  int ar=(a>>11)&31,ag=(a>>5)&63,ab=a&31;
  int br=(b>>11)&31,bg=(b>>5)&63,bb=b&31;
  return ((ar+(br-ar)*t/255)<<11)|((ag+(bg-ag)*t/255)<<5)|(ab+(bb-ab)*t/255);
}
static String keys[32];
static bool need(int id,const String &key){return keys[id]!=key;}
static void done(int id,const String &key){keys[id]=key;}
static void resetWidgets(){for(auto &key:keys)key=String("\1");}
struct HdrV{String time;int status=NM_hashing;};
struct MineV{HdrV hdr;String hash,best,accepted,rejected,workers,date;bool poolLinked=true;struct{String v[4];}strip;};
struct ClockV{String hh,mm,date;};
struct MarketV{HdrV hdr;String price,block,fee,diff,remaining;};
static String btcPairLabel(){return String("BTC / EUR");}
#include "drivers/displays/roomTheme.inc"

static void save(const std::string &path){
  std::ofstream file(path,std::ios::binary);file<<"P6\n320 240\n255\n";
  for(uint16_t c:tft.pixels){unsigned char rgb[]={static_cast<unsigned char>(((c>>11)&31)*255/31),static_cast<unsigned char>(((c>>5)&63)*255/63),static_cast<unsigned char>((c&31)*255/31)};file.write(reinterpret_cast<char*>(rgb),3);}
}
int main(int argc,char **argv){
  assert(argc==2);const std::string path=argv[1];
  assert(roomRate("nan")=="--");assert(roomRate("inf")=="--");
  assert(roomRate("-1")=="--");assert(roomRate("990.0")=="990.0");
  assert(roomRate("0.01")=="0.01");assert(roomRate("junk")=="--");
  assert(roomComparison(0.0478,120e12)=="< 0.0001 %");
  assert(roomComparison(1,100)=="1.0000 %");
  assert(roomComparison(1,0)=="--");assert(roomComparison(0,100)=="--");
  MineV m;m.hdr.time="18:42";m.hash="990.0";m.best="0.0478";m.accepted="24";m.rejected="0";m.workers="3";m.date="THU  12 NOV 2026";m.strip.v[3]="1d 04h";
  for(roomTheme=0;roomTheme<4;roomTheme++){
    uiScreen=-1;roomPaintWelcome(tft,"KI ROOM");save(path+"/loading-"+std::to_string(roomTheme)+".ppm");
    roomPaintSetup(tft,"NerdMinerAP","MineYourCoins");save(path+"/setup-"+std::to_string(roomTheme)+".ppm");
    uiScreen=SCR_MINE;resetWidgets();roomChrome(uiScreen);const auto background=tft.pixels;roomPaintMine(m);
    // The right-hand character area remains byte-for-byte untouched.
    for(int y=70;y<240;y++)for(int x=170;x<320;x++)assert(tft.pixels[y*320+x]==background[y*320+x]);
    unsigned before=pushes;roomPaintMine(m);assert(pushes==before);
    save(path+"/mine-"+std::to_string(roomTheme)+".ppm");
    uiScreen=SCR_MARKET;resetWidgets();roomChrome(uiScreen);
    MarketV market;market.hdr.time="18:42";market.price="75,000 EUR";market.block="900,000";market.fee="2 sat/vB";market.diff="120.00T";market.remaining="150000";
    roomPaintMarket(market);save(path+"/market-"+std::to_string(roomTheme)+".ppm");
    before=pushes;roomPaintMarket(market);assert(pushes==before);
    uiScreen=4;resetWidgets();roomChrome(uiScreen);
    roomPaintStats(m.best, "120.00T", "3.98e-14 %", "24", "0", "990.0");save(path+"/stats-"+std::to_string(roomTheme)+".ppm");
    uiScreen=5;resetWidgets();roomChrome(uiScreen);
    roomPaintHashrate("990.0", "192.168.1.123");save(path+"/hashrate-"+std::to_string(roomTheme)+".ppm");
    before=pushes;roomPaintHashrate("990.0", "192.168.1.123");assert(pushes==before);
    for (const char *value : {"0.0", "1000.0", "12345.6", "--", "9.0"}) roomPaintHashrate(value);
    uiScreen=SCR_CLOCK;resetWidgets();roomChrome(uiScreen);ClockV c;c.hh="18";c.mm="42";c.date=m.date;roomPaintClock(c);save(path+"/clock-"+std::to_string(roomTheme)+".ppm");
  }
#ifdef CYD_ROOM_STATIC
  // Static firmware must never repaint a decorative scene, across all pages.
  for (roomTheme=0; roomTheme<4; ++roomTheme) {
    for (uiScreen=0; uiScreen<=5; ++uiScreen) {
      resetWidgets();roomChrome(uiScreen);
      const auto pixels=tft.pixels;
      const unsigned before=pushes;
      for (int frame=0; frame<32; ++frame) {
        fakeMs+=125;roomAnimate(frame);
      }
      assert(pushes==before);
      assert(tft.pixels==pixels);
    }
  }
#else
  // Exercise actual firmware animation: every character moves, widgets remain
  // intact, updates are throttled, and menu/Pulse pages stay untouched.
  for (roomTheme=0; roomTheme<4; ++roomTheme) {
    for (uiScreen=0; uiScreen<=5; ++uiScreen) {
      if (uiScreen==3) continue;
      resetWidgets();roomChrome(uiScreen);
      if (uiScreen==SCR_MINE) roomPaintMine(m);
      if (uiScreen==SCR_MARKET) {
        MarketV market;market.hdr.time="18:42";market.price="75,000 EUR";
        market.block="900,000";market.fee="2 sat/vB";market.diff="120T";market.remaining="150000";
        roomPaintMarket(market);
      }
      if (uiScreen==SCR_CLOCK) {ClockV c;c.hh="18";c.mm="42";c.date=m.date;roomPaintClock(c);}
      if (uiScreen==4) roomPaintStats(m.best,"120T","3.98e-14 %","24","0","990");
      if (uiScreen==5) roomPaintHashrate("990.0", "192.168.1.123");
      auto staticPixels=tft.pixels;
      fakeMs+=1000;roomAnimate(1);auto pose=tft.pixels;
      const unsigned before=pushes;roomAnimate(2);assert(pushes==before);
      bool moves=false;
      for (int frame=0; frame<32; ++frame) {
        fakeMs+=125;roomAnimate(frame);
        if (tft.pixels!=pose) moves=true;
        for(int y=0;y<240;++y)for(int x=0;x<320;++x)
          if(roomAnimationProtected(x,y)) assert(tft.pixels[y*320+x]==staticPixels[y*320+x]);
      }
      assert(moves);
      if(uiScreen==SCR_MINE) save(path+"/animated-"+std::to_string(roomTheme)+".ppm");
    }
    uiScreen=3;roomChrome(3);unsigned before=pushes;fakeMs+=1000;roomAnimate(1);assert(pushes==before);
  }
#endif
  roomTheme=4;uiScreen=SCR_MINE;unsigned beforePulse=pushes;roomAnimate(1);assert(pushes==beforePulse);
  roomTheme=0;uiScreen=SCR_MINE;resetWidgets();roomChrome(uiScreen);roomPaintMine(m);
  roomShareKnown=false;g_shareFoundEvents=7;roomPaintMine(m);assert(!roomShareVisible);
  WiFi.state=0;m.workers="--";m.poolLinked=false;roomPaintMine(m);save(path+"/offline.ppm");
  WiFi.state=WL_CONNECTED;g_shareFoundEvents=8;roomPaintMine(m);assert(roomShareVisible);fakeMs=2000;roomAnimate(5);assert(!roomShareVisible);
  uiScreen=3;roomChrome(3);save(path+"/themes.ppm");
  assert(peakSprite<=10000);
  printf("OK: actual room renderer, four assets, scene/clock/menu, offline, caching, share expiry; peak widget %zu bytes. Hardware not tested.\n",peakSprite);
}


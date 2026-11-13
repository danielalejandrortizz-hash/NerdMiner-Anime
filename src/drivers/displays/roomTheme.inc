// Included only by the CYD driver when CYD_THEME_ROOM is enabled.
// Images stay in flash. Only one small widget is buffered at a time.
static const uint16_t *const ROOM_IMAGES[] = {roomGoku, roomSaiyan, roomPikachu, roomCharmander};
static const char *const ROOM_NAMES[] = {"GOKU", "SUPER SAIYAN", "PIKACHU", "CHARMANDER"};
static const uint16_t ROOM_TEXT = RGB565(255, 240, 219);
static const uint16_t ROOM_DIM = RGB565(228, 219, 201);
static unsigned long roomShareAt = 0;
static bool roomShareVisible = false;
static uint32_t roomLastShareEvent = 0;
static bool roomShareKnown = false;

static uint16_t roomRawPixel(int x, int y)
{
  if (x < 0) x = 0;
  if (x >= SCR_W) x = SCR_W - 1;
  if (y < 0) y = 0;
  if (y >= SCR_H) y = SCR_H - 1;
  return pgm_read_word(ROOM_IMAGES[roomTheme < 4 ? roomTheme : 0] + y * SCR_W + x);
}

static bool roomRoundedBox(int x, int y, int bx, int by, int width, int height, int radius)
{
  if (x < bx || x >= bx + width || y < by || y >= by + height) return false;
  const int dx = x < bx + radius ? bx + radius - x : x >= bx + width - radius ? x - (bx + width - radius - 1) : 0;
  const int dy = y < by + radius ? by + radius - y : y >= by + height - radius ? y - (by + height - radius - 1) : 0;
  return dx*dx + dy*dy <= radius*radius;
}

static uint16_t roomBlurredPixel(int x,int y,int step)
{
  int red=0,green=0,blue=0;
  const int weight[3]={1,2,1};
  for(int yy=-1;yy<=1;++yy)for(int xx=-1;xx<=1;++xx) {
    const uint16_t c=roomRawPixel(x+xx*step,y+yy*step);
    const int w=weight[xx+1]*weight[yy+1];
    red+=((c>>11)&31)*w;green+=((c>>5)&63)*w;blue+=(c&31)*w;
  }
  return uint16_t(((red/16)<<11)|((green/16)<<5)|(blue/16));
}

static uint16_t roomPixel(int x, int y)
{
  uint16_t c = uiScreen==3 ? roomBlurredPixel(x,y,3) :
    uiScreen==4 && roomRoundedBox(x,y,37,35,246,175,7) ? roomBlurredPixel(x,y,7) : roomRawPixel(x,y);
  // Port of web/room.css .scene-shade, in the original 320 x 240 CSS pixels.
  if (uiScreen == SCR_CLOCK) {
    c = mix565(c, RGB565(32 - 10*y/239, 36 - 6*y/239, 57 - 2*y/239), 74 + 41*y/239);
  } else if (uiScreen == SCR_MARKET) {
    c = mix565(c, RGB565(20,28,44), x < 256 ? 112 * (256-x) / 256 : 0);
  } else if (uiScreen == 3 || uiScreen == 4) {
    c = mix565(c, uiScreen == 3 ? RGB565(18,28,40) : RGB565(20,28,44), uiScreen == 3 ? 173 : 153);
  } else {
    if (y < 108) c = mix565(c, RGB565(17,21,42), 48 * (108-y) / 108);
    if (y > 172) c = mix565(c, RGB565(18,27,29), 107 * (y-172) / 67);
  }
  // The translucent sheets belong to the web composition, not opaque cards.
  if (uiScreen == SCR_MARKET && roomRoundedBox(x,y,16,14,150,155,7)) {
    const bool edge = !roomRoundedBox(x,y,17,15,148,153,6);
    c = mix565(c, edge ? RGB565(246,233,221) : RGB565(27,35,51), edge ? 24 : 38);
    if (y == 80 && x >= 25 && x < 157) c = mix565(c, RGB565(230,217,200), 28);
  }
  if (uiScreen == 4 && roomRoundedBox(x,y,37,35,246,175,7)) {
    const bool edge = !roomRoundedBox(x,y,38,36,244,173,6);
    c = mix565(c, edge ? RGB565(246,233,221) : RGB565(27,35,51), edge ? 34 : 77);
  }
  if(roomShareVisible && uiScreen==SCR_MINE && roomRoundedBox(x,y,14,194,132,33,5)) {
    const bool edge=!roomRoundedBox(x,y,15,195,130,31,4);
    c=mix565(c,edge?RGB565(190,201,161):RGB565(36,49,33),edge?77:230);
  }
  return c;
}

static void roomPushRow(int x, int y, int width, uint16_t *pixels)
{
#ifdef CYD_UI_SCREENSHOT
  if (shotOn)
  {
    shot.setSwapBytes(true);
    shot.pushImage(x, y, width, 1, pixels);
    return;
  }
#endif
  bool swapped = tft.getSwapBytes();
  tft.setSwapBytes(true);
  tft.pushImage(x, y, width, 1, pixels);
  tft.setSwapBytes(swapped);
}

static void roomRestore(int x, int y, int width, int height)
{
  // 640 bytes on the stack; no 150 KB framebuffer and no image decoding at runtime.
  uint16_t row[SCR_W];
  for (int dy = 0; dy < height; ++dy)
  {
    for (int dx = 0; dx < width; ++dx)
      row[dx] = roomPixel(x + dx, y + dy);
    roomPushRow(x, y + dy, width, row);
  }
}

#include "roomWebText.inc"

static bool roomBegin(int x, int y, int width, int height)
{
  if (!wBegin(width, height, C_BG))
    return false;
  uint16_t row[SCR_W];
  wgt.setSwapBytes(true);
  for (int dy = 0; dy < height; ++dy)
  {
    for (int dx = 0; dx < width; ++dx)
      row[dx] = roomPixel(x + dx, y + dy);
    wgt.pushImage(0, dy, width, 1, row);
  }
  wgt.setTextWrap(false);
  wgt.setTextPadding(0);
  wgt.setTextSize(1);
  return true;
}

// Text is painted over the restored image, never over a solid tile.
static void roomText(TFT_eSPI &d, const String &value, int x, int y, uint16_t color,
                     uint8_t font = 1, uint8_t datum = TL_DATUM)
{
  d.setTextFont(font);
  d.setTextSize(1);
  d.setTextDatum(datum);
  d.setTextColor(RGB565(39, 35, 46));
  d.drawString(value, x + 1, y + 1);
  d.setTextColor(color);
  d.drawString(value, x, y);
}

// Reject non-numeric startup/disconnected values before painting statistics.
static String roomRate(const String &raw)
{
  if (raw == "--") return raw;
  char *end = nullptr;
  const double value = strtod(raw.c_str(), &end);
  if (end == raw.c_str() || *end || !isfinite(value) || value < 0) return String("--");
  char text[24];
  snprintf(text, sizeof(text), value < 100 ? "%.2f" : "%.1f", value);
  return String(text);
}

static String roomComparison(double best, double target)
{
  if (!isfinite(best) || !isfinite(target) || best <= 0 || target <= 0) return String("--");
  const double percent = best / target * 100.0;
  if (percent < 0.0001) return String("< 0.0001 %");
  char text[32];
  snprintf(text, sizeof(text), "%.4f %%", percent);
  return String(text);
}

static String roomCount(const String &value)
{
  if (value.length() <= 5)
    return value;
  const double n = atof(value.c_str());
  char b[16];
  if (n >= 1e9)
    snprintf(b, sizeof(b), "%.1fG", n / 1e9);
  else if (n >= 1e6)
    snprintf(b, sizeof(b), "%.1fM", n / 1e6);
  else
    snprintf(b, sizeof(b), "%.1fk", n / 1e3);
  return String(b);
}

static void roomMetric(int id, int x, int y, const char *label, const String &value, const String &unit = "", const RoomWebFont &unitFont = roomFont4)
{
  const String key = String(label) + "|" + value + unit;
  if (!need(id,key)) return;
  roomWebText(x,y,65,7,label,roomFont4,RGB565(227,222,205),0,0,175);
  roomWebText(x,y+9,65,14,value,roomFont10,RGB565(255,240,220));
  const int valueWidth=roomWebWidth(value,roomFont10)+3;
  if(unit.length() && valueWidth<65)
    roomWebText(x+valueWidth,y+9+roomFont10.baseline-unitFont.baseline,65-valueWidth,unitFont.size+3,unit,unitFont,RGB565(241,228,201),0,0,176);
  done(id,key);
}

static void roomPaintMine(const MineV &m)
{
  roomWebField(W_HERO,17,15,125,39,m.hdr.time,roomFont34,RGB565(255,240,221),0,-2);
  roomWebField(W_DATE,17,57,146,9,m.date,roomFont5,RGB565(255,240,223),0,0,209);
  const bool online = WiFi.status() == WL_CONNECTED;
  roomMetric(W_T0,17,82,"HASHRATE",online ? roomRate(m.hash) : String("--"),"KH/s");
  roomMetric(W_T1,87,82,"BEST DIFF",m.best);
  roomMetric(W_T2,17,114,"WORKERS / POOL",online ? m.workers : String("--"),"activos");
  roomMetric(W_T3,87,114,"SHARES / OK / KO",roomCount(m.accepted),"/ "+roomCount(m.rejected),roomFont8);
  const char *status = m.hdr.status == NM_waitingConfig ? "Configurar" :
                       !online ? "Sin conexión" : !m.poolLinked ? "Conectando" : "Minando";
  const String key = String(status) + "|" + m.strip.v[3];
  if (need(W_HDR,key)) {
    roomWebText(210,19,94,8,status,roomFont5,RGB565(255,241,223),2,0,220);
    roomWebText(210,30,94,8,m.strip.v[3]+" encendido",roomFont5,RGB565(255,240,223),2,0,196);
    done(W_HDR,key);
  }
  const uint32_t events = g_shareFoundEvents;
  if (roomShareKnown && events > roomLastShareEvent) {
    roomShareAt = millis(); roomShareVisible = true;
    roomRestore(14,194,132,33);
    // Same short, calm notification as the web; this is a share, not a block.
    roomWebText(24,199,122,10,"Una share más.",roomFont6,RGB565(244,237,206));
    roomWebText(24,212,122,7,"La aventura continúa.",roomFont4,RGB565(181,196,164));
  }
  roomLastShareEvent = events; roomShareKnown = true;
}

// Large readouts are composited directly into a single scanline. They never
// allocate sprites or depend on the TFT library's retained font/viewport state.
static int roomNumberWidth(const String &value, const GFXfont *font)
{
  int width = 0;
  for (unsigned i = 0; i < value.length(); ++i) {
    const unsigned ch = (unsigned char)value[i];
    if (ch >= font->first && ch <= font->last)
      width += font->glyph[ch - font->first].xAdvance;
  }
  return width;
}

static void roomNumber(int x, int y, int width, int height,
                       const String &value, const GFXfont *font)
{
  int above = 0, below = 0;
  for (unsigned i = 0; i < value.length(); ++i) {
    const unsigned ch = (unsigned char)value[i];
    if (ch < font->first || ch > font->last) continue;
    const GFXglyph &g = font->glyph[ch - font->first];
    if (-g.yOffset > above) above = -g.yOffset;
    if (g.yOffset + g.height > below) below = g.yOffset + g.height;
  }
  const int left = (width - roomNumberWidth(value, font)) / 2;
  const int baseline = (height + above - below) / 2;
  uint16_t row[SCR_W];
  for (int dy = 0; dy < height; ++dy) {
    for (int dx = 0; dx < width; ++dx) row[dx] = roomPixel(x + dx, y + dy);
    for (int pass = 0; pass < 2; ++pass) {
      const int offset = pass == 0 ? 1 : 0;
      const uint16_t color = pass == 0 ? RGB565(42, 34, 52) : ROOM_TEXT;
      int pen = left + offset;
      for (unsigned i = 0; i < value.length(); ++i) {
        const unsigned ch = (unsigned char)value[i];
        if (ch < font->first || ch > font->last) continue;
        const GFXglyph &g = font->glyph[ch - font->first];
        const int gy = dy - baseline - g.yOffset - offset;
        if (gy >= 0 && gy < g.height) {
          for (int gx = 0; gx < g.width; ++gx) {
            const int dx = pen + g.xOffset + gx;
            const unsigned bit = gy * g.width + gx;
            if (dx >= 0 && dx < width &&
                (pgm_read_byte(font->bitmap + g.bitmapOffset + bit / 8) & (0x80 >> (bit % 8))))
              row[dx] = color;
          }
        }
        pen += g.xAdvance;
      }
    }
    roomPushRow(x, y + dy, width, row);
  }
}

static void roomPaintClock(const ClockV &c)
{
  static const char *lower[]={"domingo","lunes","martes","miércoles","jueves","viernes","sábado"};
  static const char *upper[]={"DOMINGO","LUNES","MARTES","MIÉRCOLES","JUEVES","VIERNES","SÁBADO"};
  String weekday=c.weekday;
  for(int i=0;i<7;++i)if(weekday==lower[i]){weekday=upper[i];break;}
  roomWebField(W_HDR,42,66,236,9,weekday,roomFont6,RGB565(255,240,225),1,2,214);
  roomWebField(W_HH,78,81,72,62,c.hh,roomFont62,RGB565(255,241,227),1,-3);
  roomWebField(W_COLON,153,86,14,46,":",roomFont46,RGB565(255,241,227),1);
  roomWebField(W_MM,170,81,72,62,c.mm,roomFont62,RGB565(255,241,227),1,-3);
  roomWebField(W_DATE,42,152,236,10,c.date,roomFont6,RGB565(255,240,225),1,0,214);
  roomWebField(W_T0,35,169,250,9,"diseñado por Dani con ayuda de codex",roomFont5,RGB565(255,240,225),1,0,201);
  roomWebField(W_T1,42,221,236,7,"El mundo sigue. Tú, a tu ritmo.",roomFont4,RGB565(247,232,215),1,0,184);
}

static uint16_t roomThemeCardPixel(int x,int y)
{
  const int col=(x-22)/142,row=(y-47)/78;
  const int i=row*2+col,bx=22+col*142,by=47+row*78;
  if(i<0 || i>=4 || !roomRoundedBox(x,y,bx,by,134,70,4))return roomPixel(x,y);
  const int dx=x-bx,dy=y-by,sx=dx*320/134,sy=36+dy*320/134;
  uint16_t c=pgm_read_word(ROOM_IMAGES[i]+sy*320+sx);
  c=mix565(RGB565(34,44,53),c,184);
  if(dy>45)c=mix565(c,RGB565(21,32,43),230*(dy-45)/24);
  if(!roomRoundedBox(x,y,bx+1,by+1,132,68,3))
    c=roomTheme==i?RGB565(237,212,169):mix565(c,RGB565(241,222,199),59);
  return c;
}

static void roomChrome(int screen)
{
  roomShareVisible=false;roomRestore(0,0,SCR_W,SCR_H);
  if(screen!=3)return;
  roomWebText(22,28,276,9,"TU PEQUEÑO UNIVERSO",roomFont5,RGB565(232,220,201),1,1,179);
  static const char *names[]={"Goku","Super Saiyan","Pikachu","Charmander"};
  for(int i=0;i<4;++i) {
    const int x=22+(i%2)*142,y=47+(i/2)*78;
    uint16_t row[134];
    for(int dy=0;dy<70;++dy) {
      for(int dx=0;dx<134;++dx)row[dx]=roomThemeCardPixel(x+dx,y+dy);
      roomPushRow(x,y+dy,134,row);
    }
    roomWebText(x+6,y+57,122,9,names[i],roomFont5,RGB565(242,232,216),0,0,255,roomThemeCardPixel);
  }
}

// Animation is generated from the flash image, one row at a time. No video
// decoder, extra image frames, full-screen framebuffer or heap allocation.

#ifndef CYD_ROOM_STATIC
#include "roomWebMotion.inc"

static bool roomAnimationProtected(int x, int y)
{
  if (uiScreen == SCR_MINE)
    return (x>=17 && x<163 && y>=15 && y<66) ||
           (x>=17 && x<152 && y>=82 && y<138) ||
           (x>=210 && x<304 && y>=19 && y<38) ||
           (roomShareVisible && x>=14 && x<146 && y>=194 && y<227);
  if (uiScreen == SCR_MARKET) return roomRoundedBox(x,y,16,14,150,155,7);
  if (uiScreen == SCR_CLOCK)
    return (x>=42 && x<278 && y>=66 && y<76) ||
           (x>=78 && x<242 && y>=81 && y<144) ||
           (x>=35 && x<285 && y>=152 && y<178) ||
           (x>=42 && x<278 && y>=221 && y<228);
  if (uiScreen == 3) {
    if(x>=22 && x<298 && y>=28 && y<37)return true;
    for(int i=0;i<4;++i)
      if(roomRoundedBox(x,y,22+(i%2)*142,47+(i/2)*78,134,70,4))return true;
    return false;
  }
  if (uiScreen == 4) return roomRoundedBox(x,y,37,35,246,175,7);
  if (uiScreen == 5)
    return (y>=40 && y<60) || (y>=87 && y<174 && x>=12 && x<308) || y>=186;
  return true;
}


#endif
static void roomAnimate(unsigned long frame)
{
  (void)frame;
  if (!roomActive() || uiScreen < 0 || uiScreen > 5)
    return;
  const unsigned long now = millis();
  if (roomShareVisible && (unsigned long)(now - roomShareAt) >= 3000)
  {
    roomShareVisible = false;
    roomRestore(14,194,132,33);
  }
  // Static builds still expire share notifications but perform no scene redraws.
#ifndef CYD_ROOM_STATIC
  static unsigned long last=0;
  static int lastTheme=-1,lastScreen=-1;
  static RoomWebFrame current,previous;
  static bool havePrevious=false;
  if(lastTheme==roomTheme && lastScreen==uiScreen && (unsigned long)(now-last)<62)return;
  last=now;lastTheme=roomTheme;lastScreen=uiScreen;
  roomBuildWebFrame(current,now/1000.0f,roomTheme);
  uint16_t row[SCR_W];
  for(int y=0;y<SCR_H;++y) {
    uint8_t dirty[SCR_W]={};
    roomWebDirty(dirty,y,current);
    if(havePrevious)roomWebDirty(dirty,y,previous);
    for(int x=0;x<SCR_W;++x) {
      if(roomAnimationProtected(x,y))dirty[x]=0;
      if(dirty[x])row[x]=roomWebBase(x,y,current);
    }
    for(int i=0;i<current.nc;++i) {
      const RoomWebCommand &c=current.commands[i];
      if(c.kind==0)roomWebCompositeGlow(row,dirty,y,current.glows[c.index]);
      else roomWebCompositeRect(row,dirty,y,current.rects[c.index]);
    }
    roomWebCompositePetals(row,dirty,y,current);
    roomWebCompositeLights(row,dirty,y,current);
    int x=0;
    while(x<SCR_W) {
      while(x<SCR_W && !dirty[x])++x;
      int end=x;while(end<SCR_W && dirty[end])++end;
      if(end>x)roomPushRow(x,y,end-x,row+x);
      x=end;
    }
  }
  previous=current;havePrevious=true;

#endif
}

// Setup uses the selected illustration.
static void roomPaintWelcome(TFT_eSPI &d, const String &version)
{
  roomRestore(0, 0, SCR_W, SCR_H);
  roomText(d, "NERDMINER ANIME", 12, 16, ROOM_TEXT, 2);
  roomText(d, "KI ROOM", 14, 40, ROOM_DIM);
  roomText(d, "CARGANDO TU UNIVERSO", 14, 177, ROOM_TEXT);
  roomText(d, version, 14, 195, ROOM_DIM);
  roomText(d, ROOM_NAMES[roomTheme < 4 ? roomTheme : 0], 14, 218, ROOM_TEXT);
}

static void roomPaintSetup(TFT_eSPI &d, const String &ap, const String &password)
{
  roomRestore(0, 0, SCR_W, SCR_H);
  roomText(d, "NERDMINER ANIME", 12, 10, ROOM_TEXT, 2);
  roomText(d, "CONFIGURA TU WIFI", 14, 34, ROOM_DIM);
  // Narrow opaque cards leave the character visible and keep credentials legible.
  const String values[] = {ap, password, "192.168.4.1"};
  const char *labels[] = {"1. CONECTATE A ESTA RED", "2. CLAVE WIFI", "3. ABRE EN EL NAVEGADOR"};
  for (int i = 0; i < 3; ++i)
  {
    const int y = 61 + i * 47;
    if (!wBegin(200, 25, C_BG))
      continue;
    roomText(wgt, labels[i], 6, 4, ROOM_DIM);
    roomText(wgt, values[i], 6, 15, ROOM_TEXT);
    wEnd(10, y);
  }
  roomText(d, "Guarda tu red, wallet BTC y pool.", 14, 211, ROOM_TEXT);
  roomText(d, "El minero se reiniciara al guardar.", 14, 225, ROOM_DIM);
}


static void roomPaintMarket(const MarketV &m)
{
  roomWebField(W_HDR,25,24,130,8,"BITCOIN",roomFont5,RGB565(232,220,201),0,1,179);
  String price=m.price;
  unsigned first=0;while(first<price.length() && (price[first]<'0' || price[first]>'9'))++first;
  if(first>0 && first<price.length())price=price.substring(first)+" "+price.substring(0,first);
  roomWebField(W_HERO,25,37,132,36,price,roomFont27,ROOM_TEXT,0,-1);
  const char *labels[] = {"Mejor dificultad","Altura del bloque","Shares aceptadas"};
  const String values[] = {m.best,m.block,m.shares};
  for (int i=0;i<3;++i) {
    const String key=values[i];
    if (!need(W_T0+i,key)) continue;
    roomWebText(25,96+i*14,79,9,labels[i],roomFont5,RGB565(219,216,201),0,0,173);
    roomWebText(105,96+i*14,52,9,values[i],roomFont5,RGB565(242,232,213),2);
    done(W_T0+i,key);
  }
  roomWebField(W_T3,25,145,132,8,m.poolLinked?"Conectado":"Sin conexión",roomFont4,RGB565(216,223,195),0,0,168);
}

static void roomPaintStats(const String &best, const String &target, const String &ratio,
                           const String &ok, const String &ko, const String &hash)
{
  roomWebField(W_HDR,56,52,208,9,"TU MEJOR INTENTO",roomFont5,RGB565(232,220,201),0,1,179);
  roomWebField(W_HERO,56,72,208,28,ratio,roomFont21,RGB565(255,240,221));
  const char *labels[] = {"Mejor dificultad","Objetivo de red","Shares >=1 / OK / KO","Hashrate"};
  const String values[] = {best,target,roomCount(ok)+" / "+roomCount(ko),roomRate(hash)+" KH/s"};
  for(int i=0;i<4;++i) {
    if(!need(W_T0+i,values[i])) continue;
    roomWebText(56,111+i*14,120,9,labels[i],roomFont5,RGB565(219,216,201),0,0,173);
    roomWebText(177,111+i*14,87,9,values[i],roomFont5,RGB565(242,232,213),2);
    done(W_T0+i,values[i]);
  }
  if(need(W_DATE,"note")) {
    roomWebText(56,175,208,9,"No es progreso acumulado.",roomFont5,RGB565(232,220,201));
    roomWebText(56,185,208,9,"Cada hash es un nuevo intento.",roomFont5,RGB565(232,220,201));
    done(W_DATE,"note");
  }
}

// Restore and draw the complete number without temporary heap allocations.
static void roomPaintHashrate(const String &hash, const String &ip = "--")
{
  const String address = "http://" + ip + "/settings";
  if (need(W_DATE, address) && roomBegin(24, 216, 272, 18)) {
    roomText(wgt, "CONFIGURAR (MISMA WIFI)", 136, 0, ROOM_DIM, 1, TC_DATUM);
    roomText(wgt, address, 136, 9, ROOM_TEXT, 1, TC_DATUM);
    wEnd(24, 216);
    done(W_DATE, address);
  }
  if (need(W_HDR, "hashrate")) {
    roomText(out(), "VELOCIDAD DE MINERIA", 160, 42, ROOM_DIM, 2, TC_DATUM);
    roomText(out(), "kH/s", 160, 153, ROOM_TEXT, 2, TC_DATUM);
    done(W_HDR, "hashrate");
  }
  const String value = roomRate(hash);
  if (!need(W_HERO, value)) return;
  const GFXfont *font = roomNumberWidth(value, &FreeSans24pt7b) <= 280 ?
                        &FreeSans24pt7b : &FreeSans18pt7b;
  roomNumber(12, 87, 296, 60, value, font);
  roomRestore(42, 188, 236, 20);
  roomText(out(), value == "--" ? "SIN DATOS" : "MEDIA LOCAL: ULTIMOS 10 S", 160, 191, ROOM_DIM, 1, TC_DATUM);
  done(W_HERO, value);
}

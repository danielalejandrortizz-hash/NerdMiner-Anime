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

static uint16_t roomPixel(int x, int y)
{
  uint16_t c = pgm_read_word(ROOM_IMAGES[roomTheme < 4 ? roomTheme : 0] + y * SCR_W + x);
  // Dim only the clock scene, to keep the centered digits legible.
  return (uiScreen == SCR_CLOCK || uiScreen == 5) ? mix565(c, RGB565(15, 20, 37), 95) : c;
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

static void roomFreeText(TFT_eSPI &d, const String &value, int x, int y, const GFXfont *font)
{
  d.setFreeFont(font);
  d.setTextSize(1);
  d.setTextDatum(MC_DATUM);
  d.setTextColor(RGB565(42, 34, 52));
  d.drawString(value, x + 1, y + 1);
  d.setTextColor(ROOM_TEXT);
  d.drawString(value, x, y);
  d.setFreeFont(NULL);
  d.setTextDatum(TL_DATUM);
}

static void roomTiny(TFT_eSPI &d, const char *value, int x, int y)
{
  d.setFreeFont(&TomThumb);
  d.setTextSize(1);
  d.setTextDatum(TL_DATUM);
  d.setTextColor(RGB565(39, 35, 46));
  d.drawString(value, x + 1, y + 1);
  d.setTextColor(ROOM_DIM);
  d.drawString(value, x, y);
  d.setFreeFont(NULL);
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

static void roomMetric(int id, int x, int y, const char *label, const String &value)
{
  String key = value + "|" + String(roomTheme);
  if (!need(id, key) || !roomBegin(x, y, 70, 34))
    return;
  roomTiny(wgt, label, 0, 0);
  // Long counters remain inside their own clear area on the left.
  wgt.setTextFont(2);
  uint8_t font = wgt.textWidth(value) <= 66 ? 2 : 1;
  roomText(wgt, value, 0, 12, ROOM_TEXT, font);
  wEnd(x, y);
  done(id, key);
}

static void roomPaintMine(const MineV &m)
{
  String key = m.hdr.time + "|" + String(roomTheme);
  if (need(W_HERO, key) && roomBegin(14, 9, 116, 43))
  {
    roomFreeText(wgt, m.hdr.time, 56, 22, &FreeSans18pt7b);
    wEnd(14, 9);
    done(W_HERO, key);
  }
  key = m.date;
  if (need(W_DATE, key) && roomBegin(16, 56, 146, 13))
  {
    roomText(wgt, m.date, 0, 1, ROOM_DIM);
    wEnd(16, 56);
    done(W_DATE, key);
  }
  roomMetric(W_T0, 16, 81, "HASHRATE KH/s", WiFi.status() == WL_CONNECTED ? roomRate(m.hash) : String("--"));
  roomMetric(W_T1, 91, 81, "BEST DIFF", m.best);
  roomMetric(W_T2, 16, 124, "WORKERS / POOL", WiFi.status() == WL_CONNECTED ? m.workers : String("--"));
  roomMetric(W_T3, 91, 124, "SHARES OK / KO", roomCount(m.accepted) + "/" + roomCount(m.rejected));

  const char *status = m.hdr.status == NM_waitingConfig ? "CONFIGURAR" :
                       WiFi.status() != WL_CONNECTED ? "SIN RED" :
                       !m.poolLinked ? "CONECTANDO" : "MINANDO";
  key = String(status) + "|" + m.strip.v[3];
  if (need(W_HDR, key) && roomBegin(216, 12, 96, 31))
  {
    roomTiny(wgt, status, 2, 0);
    roomText(wgt, m.strip.v[3], 2, 13, ROOM_DIM);
    wEnd(216, 12);
    done(W_HDR, key);
  }
  const uint32_t events = g_shareFoundEvents;
  if (roomShareKnown && events > roomLastShareEvent)
  {
    roomShareAt = millis();
    roomShareVisible = true;
    if (roomBegin(14, 198, 144, 19))
    {
      roomTiny(wgt, "SHARE >= 1 ACEPTADA", 0, 3);
      wEnd(14, 198);
    }
  }
  roomLastShareEvent = events;
  roomShareKnown = true;
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

static void roomClockPart(int id, int x, int width, const String &value)
{
  if (!need(id, value)) return;
  roomNumber(x, 88, width, 64, value, &FreeSans24pt7b);
  done(id, value);
}

static void roomPaintClock(const ClockV &c)
{
  String credit = "Dani | codex";
  if (need(W_HDR, credit) && roomBegin(35, 159, 250, 15)) {
    const String message = "disenado por Dani con ayuda de codex";
    roomText(wgt, message, 125, 4, ROOM_DIM, 1, TC_DATUM);
    // Add the tilde to the n explicitly: the built-in font is ASCII/CP437.
    int nx = 125 - wgt.textWidth(message) / 2 + 4 * 6;
    wgt.drawPixel(nx + 1, 2, ROOM_DIM);
    wgt.drawPixel(nx + 2, 1, ROOM_DIM);
    wgt.drawPixel(nx + 3, 2, ROOM_DIM);
    wgt.drawPixel(nx + 4, 1, ROOM_DIM);
    wEnd(35, 159); done(W_HDR, credit);
  }
  // Symmetric geometry: 72 + 26 + 72 = 170px, centered on x=160, y=120.
  roomClockPart(W_HH, 75, 72, c.hh);
  roomClockPart(W_COLON, 147, 26, ":");
  roomClockPart(W_MM, 173, 72, c.mm);
  if (need(W_DATE, c.date) && roomBegin(42, 68, 236, 16))
  {
    roomText(wgt, c.date, 118, 3, ROOM_DIM, 1, TC_DATUM);
    wEnd(42, 68);
    done(W_DATE, c.date);
  }
}

static void roomChrome(int screen)
{
  roomShareVisible = false;
  if (screen != 3)
  {
    roomRestore(0, 0, SCR_W, SCR_H);
    return;
  }
  TFT_eSPI &d = out();
  clearScreen(d);
  roomText(d, "ELIGE TU UNIVERSO", 12, 15, ROOM_TEXT);
  for (int i = 0; i < 4; ++i)
  {
    int x = 12 + (i % 2) * 152, y = 45 + (i / 2) * 72;
    uint16_t line[144];
    for (int row = 0; row < 64; ++row)
    {
      for (int col = 0; col < 144; ++col)
      {
        const int sx = col * 320 / 144, sy = 64 + row * 142 / 64;
        line[col] = pgm_read_word(ROOM_IMAGES[i] + sy * SCR_W + sx);
        if (row > 43)
          line[col] = mix565(line[col], C_BG, 120);
      }
      roomPushRow(x, y + row, 144, line);
    }
    roomText(d, ROOM_NAMES[i], x + 6, y + 49, ROOM_TEXT);
    if (roomTheme == i)
      d.drawRect(x, y, 144, 64, ROOM_TEXT);
  }
  roomTiny(d, "LADOS: CAMBIAR PAGINA   ARRIBA DERECHA: LUZ", 12, 228);
}

// Animation is generated from the flash image, one row at a time. No video
// decoder, extra image frames, full-screen framebuffer or heap allocation.

#ifndef CYD_ROOM_STATIC
static bool roomAnimationProtected(int x, int y)
{
  if (uiScreen == SCR_MINE)
    return (x < 166 && y < 161) || (x >= 216 && y < 43) ||
           (roomShareVisible && x >= 14 && x < 158 && y >= 198 && y < 217);
  if (uiScreen == SCR_MARKET)
    return x >= 12 && x < 206 && y < 227;
  if (uiScreen == SCR_CLOCK)
    return (x >= 42 && x < 278 && y >= 68 && y < 84) ||
           (x >= 75 && x < 245 && y >= 88 && y < 152) ||
           (x >= 35 && x < 285 && y >= 159 && y < 174);
  if (uiScreen == 4)
    return x >= 12 && (x < 206 || y >= 196);
  if (uiScreen == 5)
    return (y >= 40 && y < 60) ||
           (y >= 87 && y < 174 && x >= 12 && x < 308) ||
           (y >= 186);
  return true;
}


#endif
static void roomAnimate(unsigned long frame)
{
  (void)frame;
  if (!roomActive() || uiScreen < 0 || uiScreen > 5 || uiScreen == 3)
    return;
  const unsigned long now = millis();
  if (roomShareVisible && (unsigned long)(now - roomShareAt) >= 1800)
  {
    roomRestore(14, 198, 144, 19);
    roomShareVisible = false;
  }
  // Static builds still expire share notifications but perform no scene redraws.
#ifndef CYD_ROOM_STATIC
  static unsigned long last = 0;
  static int lastTheme = -1, lastScreen = -1;
  if (lastTheme == roomTheme && lastScreen == uiScreen &&
      (unsigned long)(now - last) < 125)
    return;
  last = now; lastTheme = roomTheme; lastScreen = uiScreen;
  const unsigned phase = (now / 125) % 32;
  static const int8_t wave[32] = {0,3,6,9,11,13,15,16,16,16,15,13,11,9,6,3,
                                0,-3,-6,-9,-11,-13,-15,-16,-16,-16,-15,-13,-11,-9,-6,-3};
  const bool goku = roomTheme < 2;
  const uint16_t energy = roomTheme == 0 ? RGB565(140,220,255) :
                          roomTheme == 1 ? RGB565(255,213,72) :
                          roomTheme == 2 ? RGB565(255,244,120) : RGB565(255,159,45);
  struct Mote { int x, y; } motes[12];
  for (int i = 0; i < 12; ++i)
  {
    const int age = (phase + i * 7) % 32;
    if (goku)
    {
      // Crystal impact sparks for both Gokus; a rising aura for Super Saiyan.
      if (roomTheme == 1 && i < 6)
        motes[i] = {190 + (i % 2) * 42 + wave[(age + i) % 32] / 3, 210 - age * 2};
      else
        motes[i] = {249 - age * (1 + i % 3) / 4, 183 - age + age * age / 40 + i % 4};
    }
    else if (roomTheme == 2)
      motes[i] = {257 + wave[(phase + i * 3) % 32], 177 + wave[(phase + i * 3 + 8) % 32] / 2};
    else
      motes[i] = {286 + wave[(age + i) % 32] / 4, 176 - age};
  }
  // Restore every animated pixel from flash before drawing the next pose/effect.
  // Regions stay clear of UI widgets; protected spans are never sent to the LCD.
  uint16_t row[140];
  for (int y = 118; y < 219; ++y)
  {
    for (int dx = 0; dx < 140; ++dx)
    {
      const int x = 174 + dx;
      int sy = y;
      if (goku && x >= 177 && x < 250 && y >= 145 && y < 214)
      {
        // Taper the demo's breathing/mining stroke to zero at every edge.
        const int taperX = wave[(x - 177) * 16 / 73];
        const int taperY = wave[(y - 145) * 16 / 69];
        const int strike = wave[(phase * 2) % 32];
        const int arm = y >= 165 && y < 190 && strike > 0 ? strike : 0;
        sy -= (wave[phase] + arm * 2) * taperX * taperY / 4096;
      }
      else if (!goku && x > 236 && x < 277 && y > 155 && y < 205)
        sy += wave[phase] / 16; // gentle breathing, one pixel each way
      uint16_t color = roomPixel(x, sy);
      int glowX = roomTheme == 0 ? 223 : roomTheme == 1 ? 213 : roomTheme == 2 ? 260 : 286;
      int glowY = roomTheme == 0 ? 179 : roomTheme == 1 ? 141 : roomTheme == 2 ? 181 : 174;
      const int radius = roomTheme == 1 ? 21 : roomTheme == 3 ? 13 : 11;
      const int distance = (x-glowX)*(x-glowX) + (y-glowY)*(y-glowY);
      if (distance < radius*radius)
      {
        const int strength = 50 + (wave[(phase * (roomTheme == 3 ? 3 : 1)) % 32] + 16) * 2;
        color = mix565(color, energy, strength * (radius*radius-distance) / (radius*radius));
      }
      for (int i = 0; i < 12; ++i)
      {
        const int mx = x - motes[i].x, my = y - motes[i].y;
        if (mx*mx + my*my <= (i % 3 == 0 ? 2 : 1))
          color = mix565(color, energy, 190);
      }
      // Pikachu's short lightning arcs change with the animation phase.
      if (roomTheme == 2 && phase % 12 < 4 && x >= 278 && x <= 287 &&
          y == 169 + (x-278)/2 + ((x + int(phase)) % 3))
        color = energy;
      for (int i = 0; i < 20; ++i) {
        const int px = int((i * 37 + now / (125 - (i % 3) * 12)) % 370) - 25;
        const int py = 30 + int((i * 29 + now / (i % 2 ? 166 : 200)) % 205);
        if (y >= py && y < py + 2 && x >= px && x < px + 4)
          color = mix565(color, i % 2 ? RGB565(245,186,216) : RGB565(255,229,170), 230);
      }
      row[dx] = color;
    }
    int start = 0;
    while (start < 140)
    {
      while (start < 140 && roomAnimationProtected(174 + start, y)) ++start;
      int end = start;
      while (end < 140 && !roomAnimationProtected(174 + end, y)) ++end;
      if (end > start) roomPushRow(174 + start, y, end-start, row+start);
      start = end;
    }
  }
  // The demo's lake polygon and drifting petals share one restored row pass.
  // Every old petal is erased from flash; widgets are never overwritten.
  static unsigned long previousLandscapeAt = 0;
  const unsigned long oldLandscapeAt = previousLandscapeAt;
  previousLandscapeAt = now;
  uint16_t landscape[SCR_W];
  for (int y = 30; y < 240; ++y)
  {
    bool petalRow = false, oldPetalRow = false;
    for (int i = 0; i < 20; ++i)
    {
      if (y >= 30 + int((i * 29 + now / (i % 2 ? 166 : 200)) % 205) &&
          y < 32 + int((i * 29 + now / (i % 2 ? 166 : 200)) % 205)) petalRow = true;
      if (y >= 30 + int((i * 29 + oldLandscapeAt / (i % 2 ? 166 : 200)) % 205) &&
          y < 32 + int((i * 29 + oldLandscapeAt / (i % 2 ? 166 : 200)) % 205)) oldPetalRow = true;
    }
    if (!petalRow && !oldPetalRow && (y < 165 || y >= 214)) continue;
    for (int x = 0; x < SCR_W; ++x)
    {
      // Character region was already composited above.
      if (x >= 174 && x < 314 && y >= 118 && y < 219) continue;
      int sx = x;
      const bool water = y >= 165 && y < 214 &&
        y >= 178 - x * 13 / 90 &&
        (y < 191 ? x < 224 - (y - 165) * 38 / 26 :
         y < 205 ? x < 186 - (y - 191) * 68 / 14 : x < 118 - (y - 205) * 118 / 9);
      if (water) sx += wave[(phase + y * 3) % 32] / 7;
      if (sx < 0) sx = 0;
      if (sx >= SCR_W) sx = SCR_W - 1;
      uint16_t color = roomPixel(sx, y);
      if (water && x > 103 - (y - 168) / 4 &&
          x < 112 + (y - 168) / 2 && (y + phase) % 5 == 0)
        color = mix565(color, RGB565(255,218,145), 65);
      if (petalRow)
        for (int i = 0; i < 20; ++i)
        {
          const int px = int((i * 37 + now / (125 - (i % 3) * 12)) % 370) - 25;
          const int py = 30 + int((i * 29 + now / (i % 2 ? 166 : 200)) % 205);
          if (y >= py && y < py + 2 && x >= px && x < px + 4)
            color = mix565(color, i % 2 ? RGB565(245,186,216) : RGB565(255,229,170), 230);
        }
      landscape[x] = color;
    }
    int start = 0;
    while (start < SCR_W)
    {
      while (start < SCR_W && (roomAnimationProtected(start,y) ||
             (start >= 174 && start < 314 && y >= 118 && y < 219))) ++start;
      int end = start;
      while (end < SCR_W && !roomAnimationProtected(end,y) &&
             !(end >= 174 && end < 314 && y >= 118 && y < 219)) ++end;
      if (end > start) roomPushRow(start,y,end-start,landscape+start);
      start = end;
    }
  }
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


static void roomDataLine(int id, int y, const char *label, const String &value)
{
  String key = String(label) + value;
  if (!need(id, key) || !roomBegin(12, y, 194, 25)) return;
  roomTiny(wgt, label, 0, 0);
  wgt.setTextFont(2);
  const uint8_t font = wgt.textWidth(value) <= 190 ? 2 : 1;
  roomText(wgt, value, 0, 9, ROOM_TEXT, font);
  wEnd(12, y); done(id, key);
}

static void roomPaintMarket(const MarketV &m)
{
  roomDataLine(W_HDR, 12, "BITCOIN", m.hdr.time);
  roomDataLine(W_HERO, 50, btcPairLabel().c_str(), m.price);
  roomDataLine(W_T0, 91, "ULTIMO BLOQUE", m.block);
  roomDataLine(W_T1, 128, "COMISION 30 MIN", m.fee);
  roomDataLine(W_T2, 165, "DIFICULTAD DE RED", m.diff);
  roomDataLine(W_T3, 202, "BLOQUES HASTA HALVING", m.remaining);
}

static void roomPaintStats(const String &best, const String &target, const String &ratio,
                           const String &ok, const String &ko, const String &hash)
{
  roomDataLine(W_HDR, 8, "MEJOR INTENTO / RED (%)", ratio);
  roomDataLine(W_T0, 45, "MEJOR DIFICULTAD", best);
  roomDataLine(W_T1, 82, "OBJETIVO DE RED", target);
  roomDataLine(W_T2, 119, "SHARES >=1 ACEPTADAS / RECHAZADAS", roomCount(ok) + "/" + roomCount(ko));
  roomDataLine(W_T3, 156, "HASHRATE kH/s", roomRate(hash));
  if (need(W_DATE, "note")) {
    roomRestore(12, 196, 296, 35);
    roomTiny(out(), "COMPARACION, NO PROGRESO ACUMULADO", 12, 201);
    roomTiny(out(), "CADA HASH ES UN NUEVO INTENTO", 12, 216);
    done(W_DATE, "note");
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

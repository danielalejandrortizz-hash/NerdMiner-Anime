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
  return uiScreen == SCR_CLOCK ? mix565(c, RGB565(15, 20, 37), 95) : c;
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
  roomMetric(W_T0, 16, 81, "HASHRATE KH/s", WiFi.status() == WL_CONNECTED ? m.hash : String("--"));
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

static void roomClockPart(int id, int x, int width, const String &value)
{
  if (!need(id, value) || !roomBegin(x, 88, width, 64))
    return;
  roomFreeText(wgt, value, width / 2, 32, &FreeSans24pt7b);
  wEnd(x, 88);
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
  d.drawRoundRect(12, 190, 296, 28, 4, C_EDGE);
  roomText(d, "VOLVER A PULSE / ORIGINAL", 24, 200, ROOM_DIM);
  roomTiny(d, "LADOS: CAMBIAR PAGINA   ARRIBA DERECHA: LUZ", 12, 228);
}

static void roomAnimate(unsigned long frame)
{
  if (uiScreen != SCR_MINE || !roomActive())
    return;
  if (roomShareVisible && (unsigned long)(millis() - roomShareAt) >= 1800)
  {
    roomRestore(14, 198, 144, 19);
    roomShareVisible = false;
  }
  // One subtle light in open sky; no full-screen repaint per animation frame.
  if (frame % 5 == 0)
  {
    uint16_t pixel = roomPixel(181, 46);
    if (frame % 40 < 15)
      pixel = mix565(pixel, ROOM_TEXT, 110);
    uint16_t row[1] = {pixel};
    roomPushRow(181, 46, 1, row);
  }
}

// Setup uses the selected illustration even when the live page is Pulse.
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
  roomText(wgt, value, 0, 9, ROOM_TEXT, 2);
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
  roomDataLine(W_HDR, 8, "TU MEJOR INTENTO / RED", ratio);
  roomDataLine(W_T0, 45, "MEJOR DIFICULTAD", best);
  roomDataLine(W_T1, 82, "OBJETIVO DE RED", target);
  roomDataLine(W_T2, 119, "SHARES >=1 ACEPTADAS / RECHAZADAS", roomCount(ok) + "/" + roomCount(ko));
  roomDataLine(W_T3, 156, "HASHRATE KH/s", hash);
  if (need(W_DATE, "note")) {
    roomRestore(12, 196, 296, 35);
    roomTiny(out(), "COMPARACION, NO PROGRESO ACUMULADO", 12, 201);
    roomTiny(out(), "CADA HASH ES UN NUEVO INTENTO", 12, 216);
    done(W_DATE, "note");
  }
}

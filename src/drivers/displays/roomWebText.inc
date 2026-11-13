// The browser's CSS pixel sizes, rendered as antialiased glyphs in flash.
#include "media/room/RoomWebFonts.h"

static const RoomWebGlyph *roomWebGlyph(const RoomWebFont &font, uint16_t code)
{
  int low = 0, high = font.count - 1;
  while (low <= high) {
    const int mid = (low + high) / 2;
    const RoomWebGlyph *g = font.glyphs + mid;
    if (g->code == code) return g;
    if (g->code < code) low = mid + 1; else high = mid - 1;
  }
  return nullptr;
}

static uint16_t roomWebCode(const String &s, unsigned &at)
{
  uint16_t code = (uint8_t)s[at++];
  if (code >= 0xc0 && code < 0xe0 && at < s.length())
    code = ((code & 31) << 6) | ((uint8_t)s[at++] & 63);
  else if (code >= 0xe0 && at + 1 < s.length()) {
    code = (code & 15) << 12;
    code |= ((uint8_t)s[at++] & 63) << 6;
    code |= (uint8_t)s[at++] & 63;
  }
  return code;
}

static int roomWebWidth(const String &s, const RoomWebFont &font, int tracking = 0)
{
  int width = 0;
  for (unsigned at = 0; at < s.length();) {
    const RoomWebGlyph *g = roomWebGlyph(font, roomWebCode(s, at));
    if (g) width += g->advance + tracking;
  }
  return width;
}

// A complete transparent text rectangle is restored and painted one scanline
// at a time. No retained font, viewport, sprite allocation or solid text box.
static void roomWebText(int x, int y, int width, int height, const String &value,
                        const RoomWebFont &font, uint16_t color,
                        int align = 0, int tracking = 0, uint8_t opacity = 255,
                        uint16_t (*background)(int,int) = nullptr)
{
  const int textWidth = roomWebWidth(value, font, tracking);
  const int start = align == 1 ? (width - textWidth) / 2 : align == 2 ? width - textWidth : 0;
  uint16_t row[SCR_W];
  for (int dy = 0; dy < height; ++dy) {
    for (int dx = 0; dx < width; ++dx) row[dx] = background ? background(x + dx, y + dy) : roomPixel(x + dx, y + dy);
    int pen = start;
    for (unsigned at = 0; at < value.length();) {
      const RoomWebGlyph *g = roomWebGlyph(font, roomWebCode(value, at));
      if (!g) continue;
      const int gy = dy - font.baseline - g->y;
      if (gy >= 0 && gy < g->h) {
        for (int gx = 0; gx < g->w; ++gx) {
          const int dx = pen + g->x + gx;
          if (dx < 0 || dx >= width) continue;
          const unsigned bit = gy * g->w + gx;
          const uint8_t packed = pgm_read_byte(font.pixels + g->offset + bit / 2);
          const uint8_t coverage = (bit & 1 ? packed & 15 : packed >> 4) * 17;
          if (coverage) row[dx] = mix565(row[dx], color, unsigned(coverage) * opacity / 255);
        }
      }
      pen += g->advance + tracking;
    }
    roomPushRow(x, y + dy, width, row);
  }
}

static void roomWebField(int id, int x, int y, int width, int height, const String &value,
                         const RoomWebFont &font, uint16_t color,
                         int align = 0, int tracking = 0, uint8_t opacity = 255)
{
  if (!need(id, value)) return;
  roomWebText(x, y, width, height, value, font, color, align, tracking, opacity);
  done(id, value);
}

static void roomDateParts(int day, int month, int year, String &weekday, String &date)
{
  static const char *days[] = {"domingo", "lunes", "martes", "miércoles", "jueves", "viernes", "sábado"};
  static const char *months[] = {"enero", "febrero", "marzo", "abril", "mayo", "junio", "julio", "agosto", "septiembre", "octubre", "noviembre", "diciembre"};
  static const int offsets[] = {0,3,2,5,0,3,5,1,4,6,2,4};
  if (month < 1 || month > 12) { weekday = "--"; date = "--"; return; }
  const int yy = year - (month < 3);
  weekday = days[(yy + yy/4 - yy/100 + yy/400 + offsets[month-1] + day) % 7];
  date = String(day) + " de " + months[month-1];
}

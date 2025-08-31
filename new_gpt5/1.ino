/*
  ESP32 + Adafruit GFX: Widget System with Dual Callbacks, Static Layouts, Dynamic State
  - Display driver: Adafruit ILI9341 (change to your GFX-compatible display if needed)
  - Serial controls (115200 baud):
      '1' -> switch to Layout 0 (Dashboard A)
      '2' -> switch to Layout 1 (Dashboard B)
      'r' -> randomize positions in current layout (demonstrates dynamic x/y updates)
      'f' -> double process Hz of all widgets (clamped)
      's' -> halve  process Hz of all widgets (clamped)
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// --------------------------- Hardware Pins (ESP32 VSPI default) ---------------------------
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_CLK  18

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// --------------------------- Global Color Scheme (single for all widgets/layouts) ----------
struct ColorScheme {
  uint16_t screenBG;
  uint16_t panelBG;
  uint16_t border;
  uint16_t text;
  uint16_t accent;
  uint16_t muted;
};

static ColorScheme COLORS;

static void initColorScheme() {
  // Use one cohesive palette (tweak if you like)
  COLORS.screenBG = tft.color565(12, 16, 24);     // very dark blue
  COLORS.panelBG  = tft.color565(22, 30, 44);     // dark slate
  COLORS.border   = tft.color565(90, 110, 140);   // cool gray
  COLORS.text     = tft.color565(240, 248, 255);  // near white
  COLORS.accent   = tft.color565(0, 185, 255);    // cyan
  COLORS.muted    = tft.color565(160, 170, 180);  // light gray
}

// --------------------------- Decor Flags & Helpers ----------------------------------------
enum : uint32_t {
  DECOR_NONE   = 0,
  DECOR_BG     = 1 << 0,   // fill panel background before drawing content
  DECOR_BORDER = 1 << 1,   // draw rectangle border
  DECOR_TITLE  = 1 << 2    // draw title at top
};

// Forward types
struct WidgetState;
struct WidgetDef;

// --------------------------- Widget State (dynamic per-instance) --------------------------
struct WidgetState {
  // Position & size (dynamic, can change at runtime)
  int16_t x, y, w, h;

  // Process scheduling (dynamic)
  uint32_t processHz;          // process frequency per widget
  uint32_t lastProcessUs;      // last process timestamp (micros)

  // Decorations & text
  uint32_t decorFlags;         // DECOR_* bitfield
  uint8_t  textSize;           // 1..N (Adafruit GFX text size)

  // Visibility
  bool visible;

  // Widget-specific dynamic memory (opaque to system, managed per widget)
  void* user;

  // Internal: did process produce new data worth re-drawing? (used as optimization)
  volatile bool dirty;
};

// --------------------------- Widget Definition (static) -----------------------------------
typedef void (*WidgetProcessFn)(WidgetState* ws, uint32_t nowUs);
typedef void (*WidgetDisplayFn)(WidgetState* ws, Adafruit_GFX& gfx);

struct WidgetDef {
  const char*      id;                 // stable identifier
  const char*      title;              // optional title for DECOR_TITLE
  WidgetProcessFn  process;            // process logic (at ws->processHz)
  WidgetDisplayFn  display;            // render logic (called each frame pass)
  uint16_t         userStateSizeBytes; // widget-specific storage to malloc per instance
};

// --------------------------- Layout (static) + placement -----------------------------------
struct WidgetPlacement {
  const WidgetDef* def;
  int16_t x, y, w, h;
  uint32_t processHz;
  uint32_t decorFlags;
  uint8_t  textSize;
  bool     visible;
};

struct LayoutDef {
  const char*           name;
  const WidgetPlacement*items;
  uint8_t               count;
};

// --------------------------- System Container for current layout --------------------------
static const uint8_t MAX_WIDGETS = 10;
static WidgetState g_states[MAX_WIDGETS];
static const WidgetDef* g_defs[MAX_WIDGETS];
static uint8_t g_count = 0;

static uint8_t g_currentLayout = 0;
static bool    g_forceFullRedraw = true;

// Frame cadence (display pass)
static const uint32_t FRAME_HZ = 30;
static uint32_t lastFrameUs = 0;

// --------------------------- Utility: clamp & random helpers ------------------------------
template <typename T>
static T clampT(T v, T lo, T hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

static uint32_t hzToPeriodUs(uint32_t hz) {
  if (hz == 0) return 0;
  return 1000000UL / hz;
}

// --------------------------- Drawing helpers ----------------------------------------------
static void drawDecorations(const WidgetDef* def, WidgetState* ws, Adafruit_GFX& gfx) {
  if (!ws->visible) return;

  if (ws->decorFlags & DECOR_BG) {
    gfx.fillRect(ws->x, ws->y, ws->w, ws->h, COLORS.panelBG);
  }

  if (ws->decorFlags & DECOR_TITLE) {
    gfx.setTextSize(1);
    gfx.setTextColor(COLORS.muted);
    gfx.setCursor(ws->x + 4, ws->y + 2);
    gfx.print(def->title ? def->title : def->id);
    // Thin underline
    gfx.drawLine(ws->x, ws->y + 12, ws->x + ws->w - 1, ws->y + 12, COLORS.muted);
  }

  if (ws->decorFlags & DECOR_BORDER) {
    gfx.drawRect(ws->x, ws->y, ws->w, ws->h, COLORS.border);
  }
}

// --------------------------- Sample Widgets ------------------------------------------------
// 1) Uptime Clock (HH:MM:SS of device uptime)
struct ClockData {
  uint32_t seconds;
};

static void clockProcess(WidgetState* ws, uint32_t nowUs) {
  (void)nowUs;
  ClockData* d = (ClockData*)ws->user;
  d->seconds = millis() / 1000UL;
  ws->dirty = true;
}

static void clockDisplay(WidgetState* ws, Adafruit_GFX& gfx) {
  ClockData* d = (ClockData*)ws->user;
  drawDecorations(nullptr, ws, gfx); // Title already drawn by drawDecorations
  // Content area (leave some margin if title exists)
  int16_t cx = ws->x + 6;
  int16_t cy = ws->y + ((ws->decorFlags & DECOR_TITLE) ? 18 : 6);

  uint32_t s = d->seconds;
  uint32_t h = (s / 3600UL) % 100UL;
  uint32_t m = (s / 60UL) % 60UL;
  uint32_t sec = s % 60UL;

  char buf[16];
  snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)sec);

  if (ws->decorFlags & DECOR_BG) {
    // clear content area to panelBG to avoid ghosting
    int16_t hPad = (ws->decorFlags & DECOR_TITLE) ? 18 : 6;
    gfx.fillRect(ws->x + 1, ws->y + hPad, ws->w - 2, ws->h - hPad - 2, COLORS.panelBG);
  }

  gfx.setTextSize(ws->textSize);
  gfx.setTextColor(COLORS.text);
  gfx.setCursor(cx, cy);
  gfx.print(buf);
}

// 2) Sine Wave Preview (simple oscilloscope)
struct SineData {
  float   phase;          // current phase
  float   freqHz;         // waveform frequency
  uint32_t lastUs;        // last update timestamp
};

static void sineProcess(WidgetState* ws, uint32_t nowUs) {
  SineData* d = (SineData*)ws->user;
  if (d->lastUs == 0) d->lastUs = nowUs;
  float dt = (nowUs - d->lastUs) / 1e6f;
  d->lastUs = nowUs;

  // Advance phase
  const float TWO_PI = 6.283185307179586f;
  d->phase += TWO_PI * d->freqHz * dt;
  if (d->phase > TWO_PI) d->phase -= floorf(d->phase / TWO_PI) * TWO_PI;

  ws->dirty = true;
}

static void sineDisplay(WidgetState* ws, Adafruit_GFX& gfx) {
  SineData* d = (SineData*)ws->user;

  drawDecorations(nullptr, ws, gfx);

  // Content viewport
  int16_t top = ws->y + ((ws->decorFlags & DECOR_TITLE) ? 18 : 2);
  int16_t height = ws->h - ((ws->decorFlags & DECOR_TITLE) ? 20 : 4);
  int16_t left = ws->x + 2;
  int16_t width = ws->w - 4;

  // Clear
  if (ws->decorFlags & DECOR_BG) {
    gfx.fillRect(left, top, width, height, COLORS.panelBG);
  }

  // Midline
  int16_t midY = top + height / 2;
  gfx.drawLine(left, midY, left + width - 1, midY, COLORS.muted);

  // Plot one period across the width
  for (int16_t i = 0; i < width; ++i) {
    float t = d->phase + (float)i * 0.04f; // horizontal scaling
    float s = sinf(t);
    int16_t y = midY - (int16_t)(s * (height * 0.45f));
    // draw a short vertical segment to avoid gaps
    gfx.drawPixel(left + i, y, COLORS.accent);
  }

  // Text readout
  gfx.setTextSize(ws->textSize);
  gfx.setTextColor(COLORS.text);
  gfx.setCursor(left + 2, top + 2);
  gfx.print("f=");
  gfx.print(d->freqHz, 2);
  gfx.print("Hz");
}

// 3) Random Bars (simple mini bar graph)
struct BarsData {
  static const uint8_t N = 12;
  uint8_t v[N];
};

static void barsProcess(WidgetState* ws, uint32_t nowUs) {
  (void)nowUs;
  BarsData* d = (BarsData*)ws->user;
  for (uint8_t i = 0; i < BarsData::N; ++i) {
    // simple smoothing random walk
    int16_t target = random(0, 100);
    int16_t cur = d->v[i];
    cur += (target - cur) / 6; // ease
    d->v[i] = (uint8_t)clampT<int16_t>(cur, 0, 100);
  }
  ws->dirty = true;
}

static void barsDisplay(WidgetState* ws, Adafruit_GFX& gfx) {
  BarsData* d = (BarsData*)ws->user;

  drawDecorations(nullptr, ws, gfx);
  int16_t top = ws->y + ((ws->decorFlags & DECOR_TITLE) ? 18 : 4);
  int16_t height = ws->h - ((ws->decorFlags & DECOR_TITLE) ? 22 : 8);
  int16_t left = ws->x + 4;
  int16_t width = ws->w - 8;

  // Clear
  if (ws->decorFlags & DECOR_BG) {
    gfx.fillRect(left, top, width, height, COLORS.panelBG);
  }

  // Bars
  uint8_t N = BarsData::N;
  int16_t gap = 2;
  int16_t bw = max<int16_t>(1, (width - (N - 1) * gap) / N);

  for (uint8_t i = 0; i < N; ++i) {
    int16_t h = (int16_t)((d->v[i] / 100.0f) * (float)height);
    int16_t x = left + i * (bw + gap);
    int16_t y = top + (height - h);
    gfx.fillRect(x, y, bw, h, COLORS.accent);
    // faint baseline
    gfx.drawFastHLine(left, top + height - 1, width, COLORS.muted);
  }
}

// --------------------------- Static Widget Definitions ------------------------------------
static const WidgetDef W_CLOCK = {
  "clock", "Uptime",
  &clockProcess, &clockDisplay,
  sizeof(ClockData)
};

static const WidgetDef W_SINE = {
  "sine", "Sine Preview",
  &sineProcess, &sineDisplay,
  sizeof(SineData)
};

static const WidgetDef W_BARS = {
  "bars", "Bars",
  &barsProcess, &barsDisplay,
  sizeof(BarsData)
};

// --------------------------- Static Layouts -----------------------------------------------
static const WidgetPlacement LAYOUT_A_ITEMS[] = {
  { &W_CLOCK,  8,   8, 140,  48,  4,  DECOR_BG | DECOR_BORDER | DECOR_TITLE, 2, true },
  { &W_SINE,   8,  64, 304, 100, 20,  DECOR_BG | DECOR_BORDER | DECOR_TITLE, 1, true },
  { &W_BARS, 160,   8, 152,  48, 12,  DECOR_BG | DECOR_BORDER | DECOR_TITLE, 1, true }
};

static const LayoutDef LAYOUT_A = { "Dashboard A", LAYOUT_A_ITEMS, (uint8_t)(sizeof(LAYOUT_A_ITEMS)/sizeof(LAYOUT_A_ITEMS[0])) };

static const WidgetPlacement LAYOUT_B_ITEMS[] = {
  { &W_SINE,   0,   0, 320, 140, 30, DECOR_BG | DECOR_BORDER | DECOR_TITLE, 1, true },
  { &W_CLOCK,  8, 142, 120,  36,  2, DECOR_BG | DECOR_BORDER,               2, true },
  { &W_BARS, 132, 142, 180,  36, 15, DECOR_BG | DECOR_BORDER,               1, true }
};

static const LayoutDef LAYOUT_B = { "Dashboard B", LAYOUT_B_ITEMS, (uint8_t)(sizeof(LAYOUT_B_ITEMS)/sizeof(LAYOUT_B_ITEMS[0])) };

static const LayoutDef* const LAYOUTS[] = { &LAYOUT_A, &LAYOUT_B };
static const uint8_t NUM_LAYOUTS = (uint8_t)(sizeof(LAYOUTS) / sizeof(LAYOUTS[0]));

// --------------------------- Layout Switching ---------------------------------------------
static void freeCurrentLayout() {
  for (uint8_t i = 0; i < g_count; ++i) {
    if (g_states[i].user) {
      free(g_states[i].user);
      g_states[i].user = nullptr;
    }
  }
  g_count = 0;
}

static void switchToLayout(uint8_t idx) {
  if (idx >= NUM_LAYOUTS) return;
  freeCurrentLayout();

  const LayoutDef* L = LAYOUTS[idx];

  // Initialize display background once per layout switch
  tft.fillScreen(COLORS.screenBG);

  for (uint8_t i = 0; i < L->count && i < MAX_WIDGETS; ++i) {
    const WidgetPlacement& p = L->items[i];
    g_defs[i] = p.def;

    WidgetState& ws = g_states[i];
    ws.x = p.x; ws.y = p.y; ws.w = p.w; ws.h = p.h;
    ws.processHz = p.processHz;
    ws.lastProcessUs = 0;
    ws.decorFlags = p.decorFlags;
    ws.textSize = p.textSize;
    ws.visible = p.visible;
    ws.dirty = true;

    ws.user = nullptr;
    if (p.def->userStateSizeBytes > 0) {
      ws.user = malloc(p.def->userStateSizeBytes);
      if (ws.user) memset(ws.user, 0, p.def->userStateSizeBytes);
    }
    // Widget-specific defaults (init user state)
    if (p.def == &W_SINE && ws.user) {
      SineData* sd = (SineData*)ws.user;
      sd->freqHz = 1.5f; // default waveform freq
      sd->phase = 0.f;
      sd->lastUs = 0;
    }
    if (p.def == &W_BARS && ws.user) {
      BarsData* bd = (BarsData*)ws.user;
      for (uint8_t k = 0; k < BarsData::N; ++k) bd->v[k] = random(20, 80);
    }
  }

  g_count = min<uint8_t>(L->count, MAX_WIDGETS);
  g_currentLayout = idx;
  g_forceFullRedraw = true;

  // Draw static elements (background fill already done)
  for (uint8_t i = 0; i < g_count; ++i) {
    if (!g_states[i].visible) continue;
    drawDecorations(g_defs[i], &g_states[i], tft);
  }
}

// --------------------------- Scheduler -----------------------------------------------------
static void processPass(uint32_t nowUs) {
  for (uint8_t i = 0; i < g_count; ++i) {
    WidgetState& ws = g_states[i];
    if (!ws.visible) continue;
    uint32_t hz = ws.processHz;
    if (hz == 0) continue;

    uint32_t periodUs = hzToPeriodUs(hz);
    if (periodUs == 0) continue;

    if ((uint32_t)(nowUs - ws.lastProcessUs) >= periodUs) {
      ws.lastProcessUs = nowUs;
      if (g_defs[i]->process) {
        g_defs[i]->process(&ws, nowUs);
      }
    }
  }
}

static void displayPass(uint32_t nowUs) {
  (void)nowUs;
  // Throttle to FRAME_HZ
  uint32_t framePeriod = hzToPeriodUs(FRAME_HZ);
  if ((uint32_t)(nowUs - lastFrameUs) < framePeriod && !g_forceFullRedraw) return;
  lastFrameUs = nowUs;

  for (uint8_t i = 0; i < g_count; ++i) {
    WidgetState& ws = g_states[i];
    if (!ws.visible) continue;

    // Optionally skip if not dirty; here we refresh every frame to keep it simple
    if (g_defs[i]->display) {
      // Always redraw widget area background if DECOR_BG set to prevent artifacts
      if (ws.decorFlags & DECOR_BG) {
        // Redraw decorations before content each frame (cheap & robust)
        drawDecorations(g_defs[i], &ws, tft);
      }
      g_defs[i]->display(&ws, tft);
      ws.dirty = false;
    }
  }
  g_forceFullRedraw = false;
}

// --------------------------- Runtime Tweaks (Serial commands) -----------------------------
static void randomizePositions() {
  for (uint8_t i = 0; i < g_count; ++i) {
    WidgetState& ws = g_states[i];
    // Keep within screen (320x240 for ILI9341). If your display differs, adjust bounds.
    int16_t maxX = max<int16_t>(0, 320 - ws.w);
    int16_t maxY = max<int16_t>(0, 240 - ws.h);
    ws.x = random(0, maxX + 1);
    ws.y = random(0, maxY + 1);
  }
  g_forceFullRedraw = true;
}

static void scaleFrequencies(float factor) {
  for (uint8_t i = 0; i < g_count; ++i) {
    WidgetState& ws = g_states[i];
    uint32_t hz = ws.processHz;
    float f = (float)hz * factor;
    ws.processHz = clampT<uint32_t>((uint32_t)roundf(f), 1, 200); // reasonable range
  }
}

// --------------------------- Arduino Setup/Loop -------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(50);

  // Display init
  SPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI);
  tft.begin();
  tft.setRotation(1); // landscape (320x240)
  tft.fillScreen(ILI9341_BLACK); // temporary until scheme init

  initColorScheme();
  tft.fillScreen(COLORS.screenBG);
  tft.setTextWrap(false);

  // Seed RNG
  randomSeed(esp_random());

  // Start at layout 0
  switchToLayout(0);

  // Immediate draw
  lastFrameUs = micros();
  g_forceFullRedraw = true;
}

void loop() {
  // Input handling
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c >= '1' && c <= ('0' + NUM_LAYOUTS)) {
      uint8_t idx = (uint8_t)(c - '1');
      switchToLayout(idx);
      Serial.print("Switched to layout "); Serial.println(idx);
    } else if (c == 'r' || c == 'R') {
      randomizePositions();
      Serial.println("Positions randomized (dynamic x/y).");
    } else if (c == 'f' || c == 'F') {
      scaleFrequencies(2.0f);
      Serial.println("Process Hz doubled for all widgets.");
    } else if (c == 's' || c == 'S') {
      scaleFrequencies(0.5f);
      Serial.println("Process Hz halved for all widgets.");
    }
  }

  uint32_t nowUs = micros();
  processPass(nowUs);
  displayPass(nowUs);
}

// --------------------------- Notes to adapt to other displays -----------------------------
/*
  - To use another Adafruit GFX-compatible driver (e.g., ST7735, SSD1306):
      * Replace the #include <Adafruit_ILI9341.h> and the 'tft' instance
      * Adjust tft.setRotation(...) and screen size constraints in randomizePositions()
  - The system enforces a single global color scheme (COLORS).
  - Widgets are defined statically (WidgetDef), layouts are static (LayoutDef),
    but WidgetState is fully dynamic (position, size, frequency, decorations, text size, user data).
  - Dual callback model:
      * process(ws, nowUs) runs at ws->processHz
      * display(ws, gfx) renders using values computed by process()
*/

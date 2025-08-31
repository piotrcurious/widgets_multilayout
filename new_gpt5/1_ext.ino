// --------------------------- NEW WIDGETS --------------------------------------------------

// 4) Sensor Tile (float readout, e.g. temperature)
struct SensorTileData {
  float value;      // dynamic value
  const char* unit; // e.g. "°C"
};

static void sensorTileProcess(WidgetState* ws, uint32_t nowUs) {
  (void)nowUs;
  SensorTileData* d = (SensorTileData*)ws->user;
  // Fake sensor data: slowly drifting sinusoid
  static float t = 0.f;
  t += 0.05f;
  d->value = 25.f + 5.f * sinf(t); // 25±5
  ws->dirty = true;
}

static void sensorTileDisplay(WidgetState* ws, Adafruit_GFX& gfx) {
  SensorTileData* d = (SensorTileData*)ws->user;
  drawDecorations(nullptr, ws, gfx);

  int16_t top = ws->y + ((ws->decorFlags & DECOR_TITLE) ? 18 : 4);
  if (ws->decorFlags & DECOR_BG) {
    gfx.fillRect(ws->x+1, top, ws->w-2, ws->h-(top-ws->y)-2, COLORS.panelBG);
  }

  gfx.setTextSize(ws->textSize);
  gfx.setTextColor(COLORS.text);
  gfx.setCursor(ws->x + 4, top + (ws->h/2) - 8);
  gfx.print(d->value, 1);
  gfx.print(" ");
  if (d->unit) gfx.print(d->unit);
}

// 5) Analog Gauge (circular, 0–100)
struct GaugeData {
  float value; // 0–100
};

static void gaugeProcess(WidgetState* ws, uint32_t nowUs) {
  (void)nowUs;
  GaugeData* d = (GaugeData*)ws->user;
  // Fake input: triangular wave
  static int dir = 1;
  d->value += dir * 1.f;
  if (d->value > 100.f) { d->value = 100.f; dir = -1; }
  if (d->value < 0.f)   { d->value = 0.f;   dir = 1; }
  ws->dirty = true;
}

static void gaugeDisplay(WidgetState* ws, Adafruit_GFX& gfx) {
  GaugeData* d = (GaugeData*)ws->user;
  drawDecorations(nullptr, ws, gfx);

  int16_t cx = ws->x + ws->w/2;
  int16_t cy = ws->y + ws->h/2 + 10; // lower to make room for needle
  int16_t r  = min(ws->w, ws->h) / 2 - 6;

  // Clear dial area if BG set
  if (ws->decorFlags & DECOR_BG) {
    gfx.fillCircle(cx, cy, r+2, COLORS.panelBG);
  }

  // Dial arc (semi-circle)
  for (int a=-90; a<=90; a+=10) {
    float rad = a * DEG_TO_RAD;
    int16_t x1 = cx + cosf(rad)*r;
    int16_t y1 = cy + sinf(rad)*r;
    gfx.drawPixel(x1,y1,COLORS.muted);
  }

  // Needle
  float angle = (-90 + (d->value/100.f)*180.f) * DEG_TO_RAD;
  int16_t nx = cx + cosf(angle)*(r-2);
  int16_t ny = cy + sinf(angle)*(r-2);
  gfx.drawLine(cx, cy, nx, ny, COLORS.accent);
  gfx.fillCircle(cx, cy, 3, COLORS.border);

  // Text value
  gfx.setTextSize(ws->textSize);
  gfx.setTextColor(COLORS.text);
  gfx.setCursor(ws->x+4, ws->y+4);
  gfx.print(d->value, 0);
}

// 6) Rolling Graph (circular buffer of samples, drawn at slower display Hz)
struct RollingGraphData {
  static const int N = 120;
  float buf[N];
  int head;
};

static void rollingGraphProcess(WidgetState* ws, uint32_t nowUs) {
  (void)nowUs;
  RollingGraphData* d = (RollingGraphData*)ws->user;
  // Fake input: noisy sine
  static float t = 0.f;
  t += 0.15f;
  float v = 0.5f + 0.4f * sinf(t) + 0.1f*((float)random(-100,100)/100.f);
  d->buf[d->head] = v;
  d->head = (d->head+1) % RollingGraphData::N;
  // Note: don't set dirty; display will update at its own rate
}

static void rollingGraphDisplay(WidgetState* ws, Adafruit_GFX& gfx) {
  RollingGraphData* d = (RollingGraphData*)ws->user;
  drawDecorations(nullptr, ws, gfx);

  int16_t top = ws->y + ((ws->decorFlags & DECOR_TITLE)?18:2);
  int16_t height = ws->h - ((ws->decorFlags & DECOR_TITLE)?20:4);
  int16_t left = ws->x+2;
  int16_t width = ws->w-4;

  // Clear if BG
  if (ws->decorFlags & DECOR_BG) {
    gfx.fillRect(left, top, width, height, COLORS.panelBG);
  }

  int idx = d->head;
  for (int i=0; i<width && i<RollingGraphData::N; ++i) {
    idx = (idx - 1 + RollingGraphData::N) % RollingGraphData::N;
    float v = d->buf[idx];
    int16_t y = top + height - (int16_t)(v * (float)height);
    gfx.drawPixel(left + width-1 - i, y, COLORS.accent);
  }
}

// --------------------------- WidgetDefs ----------------------------------------------------
static const WidgetDef W_SENSOR = {
  "sensor", "Temperature",
  &sensorTileProcess, &sensorTileDisplay,
  sizeof(SensorTileData)
};

static const WidgetDef W_GAUGE = {
  "gauge", "Gauge",
  &gaugeProcess, &gaugeDisplay,
  sizeof(GaugeData)
};

static const WidgetDef W_GRAPH = {
  "graph", "Rolling Graph",
  &rollingGraphProcess, &rollingGraphDisplay,
  sizeof(RollingGraphData)
};

// --------------------------- NEW Layout Example -------------------------------------------
static const WidgetPlacement LAYOUT_C_ITEMS[] = {
  { &W_SENSOR,  10,  10, 100, 50,  5, DECOR_BG|DECOR_BORDER|DECOR_TITLE, 2, true },
  { &W_GAUGE,  120,  10, 180, 100, 10, DECOR_BG|DECOR_BORDER|DECOR_TITLE, 1, true },
  { &W_GRAPH,   10, 120, 290, 100, 30, DECOR_BG|DECOR_BORDER|DECOR_TITLE, 1, true }
};

static const LayoutDef LAYOUT_C = {
  "Dashboard C",
  LAYOUT_C_ITEMS,
  (uint8_t)(sizeof(LAYOUT_C_ITEMS)/sizeof(LAYOUT_C_ITEMS[0]))
};

// Add to LAYOUTS[] array:
static const LayoutDef* const LAYOUTS[] = { &LAYOUT_A, &LAYOUT_B, &LAYOUT_C };
static const uint8_t NUM_LAYOUTS = (uint8_t)(sizeof(LAYOUTS)/sizeof(LAYOUTS[0]));

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <math.h>

// ==== Display setup ====
#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// ==== Colors ====
const uint16_t COLOR_BG     = ILI9341_BLACK;
const uint16_t COLOR_TEXT   = ILI9341_WHITE;
const uint16_t COLOR_FRAME  = ILI9341_BLUE;
const uint16_t COLOR_ACCENT = ILI9341_YELLOW;
const uint16_t COLOR_RED    = ILI9341_RED;
const uint16_t COLOR_GREEN  = ILI9341_GREEN;

// ==== Button to switch layouts ====
const uint8_t BUTTON_PIN = 0; // change if needed
const uint16_t DEBOUNCE_MS = 50;

// ==== Widget framework ====
typedef void (*ProcessCallback)(void *);
typedef void (*DisplayCallback)(void *);

struct Widget {
  int16_t x, y, width, height;
  ProcessCallback processCb;
  DisplayCallback displayCb;
  uint16_t processFrequency;   // ms
  uint32_t lastProcessTime;
  bool hasFrame;
  bool hasBackground;
  uint16_t bgColor;
  void *data;                  // pointer to widget-specific data
};

// ---- Data structures for widgets ----
// Voltage text widget uses a simple integer mV
int voltageData = 0;

// Rolling graph - now dynamically sized per-GraphData
struct GraphData {
  int *values;     // dynamically allocated buffer
  int points;      // number of samples
  int index;       // next write position
  int minVal;
  int maxVal;
  GraphData() : values(nullptr), points(0), index(0), minVal(99999), maxVal(-99999) {}
};

// allocate and initialize graph buffer
bool initGraph(GraphData *g, int points) {
  if (!g) return false;
  if (g->values) delete[] g->values; // free if present
  if (points <= 0) return false;
  g->values = new int[points];
  if (!g->values) return false;
  g->points = points;
  g->index = 0;
  g->minVal = 99999;
  g->maxVal = -99999;
  for (int i = 0; i < points; ++i) g->values[i] = 0;
  return true;
}
void destroyGraph(GraphData *g) {
  if (!g) return;
  if (g->values) {
    delete[] g->values;
    g->values = nullptr;
  }
  g->points = 0;
  g->index = 0;
  g->minVal = 99999;
  g->maxVal = -99999;
}

// Gauge
struct GaugeData { float value; };

// Min/Max tracker
struct MinMaxData {
  int value;
  int minV;
  int maxV;
  unsigned long lastReset;
  MinMaxData() : value(0), minV(99999), maxV(-99999), lastReset(0) {}
};

// ---- Instantiate per-widget data ----
GraphData graphDataA; // for graph widget in layout A
GraphData graphDataB; // for graph widget in layout B
GaugeData gaugeData = {0.0f};
MinMaxData minmaxDataA;
MinMaxData minmaxDataB;

// ---- Helper to find widget pointer by data (search current layout) ----
Widget **currentLayout = nullptr;
int currentLayoutSize = 0;
Widget * findWidgetByData(void *data) {
  if (!currentLayout) return nullptr;
  for (int i = 0; i < currentLayoutSize; ++i) {
    if (currentLayout[i]->data == data) return currentLayout[i];
  }
  return nullptr;
}

// ==== Process & Display callbacks ====
// Voltage widget
void processVoltage(void *data) {
  int *v = (int*)data;
  *v = random(0, 5000); // mV (replace with ADC)
}
void displayVoltage(void *data) {
  int *v = (int*)data;
  Widget *w = findWidgetByData(data);
  int tx = (w ? w->x : 10) + 5;
  int ty = (w ? w->y : 10) + 8;

  char buf[20];
  sprintf(buf, "%4d mV", *v);
  if (w && w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
  tft.setCursor(tx, ty);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print(buf);
  if (w && w->hasFrame) tft.drawRect(w->x, w->y, w->width, w->height, COLOR_FRAME);
}

// Rolling Graph
void processGraph(void *data) {
  GraphData *g = (GraphData*)data;
  if (!g || !g->values || g->points <= 0) return;
  int newVal = random(100, 4800); // replace with actual sample
  g->values[g->index] = newVal;
  g->index = (g->index + 1) % g->points;

  // recompute min/max
  g->minVal = 99999; g->maxVal = -99999;
  for (int i = 0; i < g->points; ++i) {
    if (g->values[i] < g->minVal) g->minVal = g->values[i];
    if (g->values[i] > g->maxVal) g->maxVal = g->values[i];
  }
  if (g->minVal == 99999) { g->minVal = 0; g->maxVal = 1000; }
}

void displayGraph(void *data) {
  GraphData *g = (GraphData*)data;
  Widget *w = findWidgetByData(data);
  if (!g || !g->values || g->points <= 0 || !w) return;

  int gx = w->x, gy = w->y, gw = max(2, w->width), gh = max(2, w->height);
  tft.fillRect(gx, gy, gw, gh, (w->hasBackground ? w->bgColor : COLOR_BG));

  if (g->maxVal == g->minVal) {
    tft.drawRect(gx, gy, gw, gh, COLOR_FRAME);
    return;
  }

  // draw polyline scaled to widget width
  // Map sample-space [0..points-1] to pixel x positions across gw
  for (int i = 1; i < g->points; ++i) {
    int idx1 = (g->index + i - 1) % g->points;
    int idx2 = (g->index + i) % g->points;
    int x1 = gx + (i - 1) * (gw - 1) / max(1, g->points - 1);
    int x2 = gx + i * (gw - 1) / max(1, g->points - 1);
    int y1 = gy + gh - (g->values[idx1] - g->minVal) * gh / max(1, (g->maxVal - g->minVal));
    int y2 = gy + gh - (g->values[idx2] - g->minVal) * gh / max(1, (g->maxVal - g->minVal));
    tft.drawLine(x1, y1, x2, y2, COLOR_ACCENT);
  }

  // min/max text
  char buf[32];
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  sprintf(buf, "min:%d", g->minVal);
  tft.setCursor(gx + 3, gy + 2); tft.print(buf);
  sprintf(buf, "max:%d", g->maxVal);
  tft.setCursor(gx + 3, gy + 12); tft.print(buf);

  if (w->hasFrame) tft.drawRect(gx, gy, gw, gh, COLOR_FRAME);
}

// Gauge
void processGauge(void *data) {
  GaugeData *g = (GaugeData*)data;
  g->value = (float)random(0, 1000) / 10.0f; // 0..100%
}
void displayGauge(void *data) {
  GaugeData *g = (GaugeData*)data;
  Widget *w = findWidgetByData(data);
  if (!w) return;
  int cx = w->x + w->width / 2;
  int cy = w->y + w->height / 2;
  int r = min(w->width, w->height) / 2 - 4;
  if (w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
  tft.drawCircle(cx, cy, r, COLOR_FRAME);
  float angleDeg = (g->value / 100.0f) * 270.0f - 135.0f;
  float rad = angleDeg * (3.14159265f / 180.0f);
  int nx = cx + (int)(cos(rad) * (r - 6));
  int ny = cy + (int)(sin(rad) * (r - 6));
  tft.drawLine(cx, cy, nx, ny, COLOR_ACCENT);
  char buf[16];
  sprintf(buf, "%3.0f%%", g->value);
  tft.setTextSize(1);
  tft.setCursor(cx - 12, cy + r - 2);
  tft.setTextColor(COLOR_TEXT);
  tft.print(buf);
  if (w->hasFrame) tft.drawRect(w->x, w->y, w->width, w->height, COLOR_FRAME);
}

// Min/Max over time widget
void processMinMax(void *data) {
  MinMaxData *mm = (MinMaxData*)data;
  mm->value = random(100, 4800); // sample
  if (mm->value < mm->minV) mm->minV = mm->value;
  if (mm->value > mm->maxV) mm->maxV = mm->value;
  if (millis() - mm->lastReset > 10000) {
    mm->minV = 99999;
    mm->maxV = -99999;
    mm->lastReset = millis();
  }
}
void displayMinMax(void *data) {
  MinMaxData *mm = (MinMaxData*)data;
  Widget *w = findWidgetByData(data);
  if (!w) return;
  if (w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
  char buf[32];
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT);
  sprintf(buf, "Val:%4d", mm->value);
  tft.setCursor(w->x + 4, w->y + 6); tft.print(buf);
  sprintf(buf, "Min:%4d", (mm->minV==99999?0:mm->minV));
  tft.setCursor(w->x + 4, w->y + 18); tft.print(buf);
  sprintf(buf, "Max:%4d", (mm->maxV==-99999?0:mm->maxV));
  tft.setCursor(w->x + 4, w->y + 30); tft.print(buf);
  if (w->hasFrame) tft.drawRect(w->x, w->y, w->width, w->height, COLOR_FRAME);
}

// ==== Widget instances ====
// Layout A widgets (dashboard)
Widget voltageWidgetA = { 10, 10, 120, 36, processVoltage, displayVoltage, 500, 0, true, true, COLOR_BG, &voltageData };
Widget graphWidgetA   = { 10, 54, 220, 100, processGraph,   displayGraph,   150, 0, true, false, COLOR_BG, &graphDataA };
Widget gaugeWidgetA   = { 240, 54, 76,  100, processGauge,   displayGauge,   300, 0, true, false, COLOR_BG, &gaugeData };
Widget minmaxWidgetA  = { 10, 160, 150, 44, processMinMax,  displayMinMax,  400, 0, true, true, COLOR_BG, &minmaxDataA };

// Layout B widgets (big graph, compact right column)
Widget graphWidgetB   = { 10, 10, 300, 160, processGraph,  displayGraph,   120, 0, true, false, COLOR_BG, &graphDataB };
Widget voltageWidgetB = { 320, 14, 88, 36, processVoltage, displayVoltage, 500, 0, true, true, COLOR_BG, &voltageData };
Widget minmaxWidgetB  = { 320, 60, 88, 60, processMinMax, displayMinMax, 300, 0, true, true, COLOR_BG, &minmaxDataB };

// Layout arrays
Widget *layoutA[] = { &voltageWidgetA, &graphWidgetA, &gaugeWidgetA, &minmaxWidgetA };
Widget *layoutB[] = { &graphWidgetB, &voltageWidgetB, &minmaxWidgetB };

// screen management
Widget **layouts[] = { layoutA, layoutB };
int layoutSizes[] = { sizeof(layoutA)/sizeof(layoutA[0]), sizeof(layoutB)/sizeof(layoutB[0]) };
int currentLayoutIndex = 0;

// Switch layout function
void switchToLayout(int idx) {
  if (idx < 0 || idx >= (int)(sizeof(layouts)/sizeof(layouts[0]))) return;
  currentLayout = layouts[idx];
  currentLayoutSize = layoutSizes[idx];
  tft.fillScreen(COLOR_BG);
  for (int i = 0; i < currentLayoutSize; ++i) {
    Widget *w = currentLayout[i];
    if (w->processCb && w->data) {
      w->processCb(w->data); // give initial data
      w->lastProcessTime = millis();
    }
    if (w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
    if (w->displayCb && w->data) w->displayCb(w->data);
    if (w->hasFrame) tft.drawRect(w->x, w->y, w->width, w->height, COLOR_FRAME);
  }
}

// ==== Setup & Loop ====
uint32_t lastButtonChange = 0;
bool lastButtonState = HIGH;
void setup() {
  randomSeed(analogRead(A0));
  tft.begin();
  tft.fillScreen(COLOR_BG);
  tft.setRotation(1);

  // Initialize graph buffers with different sizes per widget:
  // Layout A graph: 160 points, Layout B graph: 320 points (example)
  initGraph(&graphDataA, 160); // configure per-widget buffer size here
  initGraph(&graphDataB, 320); // independent size for the other graph

  // button setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(BUTTON_PIN);
  lastButtonChange = millis();

  // start on layout 0
  switchToLayout(0);
}

void loop() {
  uint32_t now = millis();

  // Button handling (debounced toggle)
  bool bstate = digitalRead(BUTTON_PIN);
  static bool wasPressed = false;
  if (bstate != lastButtonState) {
    lastButtonChange = now;
    lastButtonState = bstate;
  } else if (now - lastButtonChange > DEBOUNCE_MS) {
    if (bstate == LOW && !wasPressed) {
      currentLayoutIndex = (currentLayoutIndex + 1) % (sizeof(layouts)/sizeof(layouts[0]));
      switchToLayout(currentLayoutIndex);
      wasPressed = true;
    } else if (bstate == HIGH && wasPressed) {
      wasPressed = false;
    }
  }

  // Iterate widgets and run process/display if it's time
  if (currentLayout) {
    for (int i = 0; i < currentLayoutSize; ++i) {
      Widget *w = currentLayout[i];
      if (!w || !w->data) continue;
      if ((long)(now - w->lastProcessTime) >= (long)w->processFrequency) {
        if (w->processCb) w->processCb(w->data);
        w->lastProcessTime = now;
        if (w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
        if (w->displayCb) w->displayCb(w->data);
        if (w->hasFrame) tft.drawRect(w->x, w->y, w->width, w->height, COLOR_FRAME);
      }
    }
  }

  delay(10);
}

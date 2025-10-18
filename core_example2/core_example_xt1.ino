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
typedef void (*ProcessCallback)(void *); // signature: process(data)
typedef void (*DisplayCallback)(void *); // signature: display(data)

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

// Rolling graph
#define GRAPH_POINTS 160
struct GraphData {
  int values[GRAPH_POINTS];
  int index;
  int minVal;
  int maxVal;
};
GraphData graphDataA = {{0}, 0, 99999, -99999}; // used in layout A
GraphData graphDataB = {{0}, 0, 99999, -99999}; // used in layout B (independent history)

// Gauge
struct GaugeData { float value; };
GaugeData gaugeData = {0.0};

// Min/Max tracker
struct MinMaxData {
  int value;
  int minV;
  int maxV;
  unsigned long lastReset;
};
MinMaxData minmaxDataA = {0, 99999, -99999, 0};
MinMaxData minmaxDataB = {0, 99999, -99999, 0};

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
  // Replace with real ADC reading; here a simulated value:
  *v = random(0, 5000); // mV
}
void displayVoltage(void *data) {
  int *v = (int*)data;
  Widget *w = findWidgetByData(data);
  int tx = (w ? w->x : 10) + 5;
  int ty = (w ? w->y : 10) + 8;

  char buf[20];
  sprintf(buf, "%4d mV", *v);
  // draw background portion for text
  if (w && w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
  tft.setCursor(tx, ty);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print(buf);

  if (w && w->hasFrame) tft.drawRect(w->x, w->y, w->width, w->height, COLOR_FRAME);
}

// Rolling Graph (shared logic for both graphs using widget area)
void processGraph(void *data) {
  GraphData *g = (GraphData*)data;
  int newVal = random(100, 4800); // Replace with sampled sensor
  g->values[g->index] = newVal;
  g->index = (g->index + 1) % GRAPH_POINTS;
  // recompute min/max
  g->minVal = 99999; g->maxVal = -99999;
  for (int i = 0; i < GRAPH_POINTS; ++i) {
    if (g->values[i] < g->minVal) g->minVal = g->values[i];
    if (g->values[i] > g->maxVal) g->maxVal = g->values[i];
  }
  // If buffer was empty initially, ensure reasonable defaults
  if (g->minVal == 99999) { g->minVal = 0; g->maxVal = 1000; }
}
void displayGraph(void *data) {
  GraphData *g = (GraphData*)data;
  Widget *w = findWidgetByData(data);
  if (!w) return;

  int gx = w->x, gy = w->y, gw = max(2, w->width), gh = max(2, w->height);
  // Clear plot region (if widget requests bg, we respect it, otherwise draw black behind)
  tft.fillRect(gx, gy, gw, gh, (w->hasBackground ? w->bgColor : COLOR_BG));

  if (g->maxVal == g->minVal) {
    // nothing to draw yet
    tft.drawRect(gx, gy, gw, gh, COLOR_FRAME);
    return;
  }

  // Draw polyline
  for (int i = 1; i < GRAPH_POINTS; ++i) {
    int idx1 = (g->index + i - 1) % GRAPH_POINTS;
    int idx2 = (g->index + i) % GRAPH_POINTS;
    int x1 = gx + (i - 1) * gw / (GRAPH_POINTS - 1);
    int x2 = gx + i * gw / (GRAPH_POINTS - 1);
    int y1 = gy + gh - (g->values[idx1] - g->minVal) * gh / max(1, (g->maxVal - g->minVal));
    int y2 = gy + gh - (g->values[idx2] - g->minVal) * gh / max(1, (g->maxVal - g->minVal));
    tft.drawLine(x1, y1, x2, y2, COLOR_ACCENT);
  }

  // Draw min/max text small (top-left inside graph)
  char buf[32];
  sprintf(buf, "min:%d", g->minVal);
  tft.setTextSize(1);
  tft.setCursor(gx + 3, gy + 2);
  tft.setTextColor(COLOR_TEXT);
  tft.print(buf);
  sprintf(buf, "max:%d", g->maxVal);
  tft.setCursor(gx + 3, gy + 12);
  tft.print(buf);

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

  // background
  if (w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
  // gauge rim
  tft.drawCircle(cx, cy, r, COLOR_FRAME);

  // needle
  float angleDeg = (g->value / 100.0f) * 270.0f - 135.0f; // -135..+135
  float rad = angleDeg * (3.14159265f / 180.0f);
  int nx = cx + (int)(cos(rad) * (r - 6));
  int ny = cy + (int)(sin(rad) * (r - 6));
  tft.drawLine(cx, cy, nx, ny, COLOR_ACCENT);

  // value text below
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
  if (millis() - mm->lastReset > 10000) { // reset every 10s
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
  // clear screen and force immediate redraw of each widget (process+display)
  tft.fillScreen(COLOR_BG);
  for (int i = 0; i < currentLayoutSize; ++i) {
    Widget *w = currentLayout[i];
    // run one process so the display has data immediately
    if (w->processCb && w->data) {
      w->processCb(w->data);
      w->lastProcessTime = millis();
    }
    // draw background and display
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

  // display init
  tft.begin();
  tft.fillScreen(COLOR_BG);
  tft.setRotation(1);

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
  if (bstate != lastButtonState) {
    lastButtonChange = now;
    lastButtonState = bstate;
  } else if (now - lastButtonChange > DEBOUNCE_MS) {
    // stable state; detect falling edge (pressed if pulled-up)
    static bool wasPressed = false;
    if (bstate == LOW && !wasPressed) {
      // button pressed
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
      if (!w) continue;
      void *data = w->data;
      if (!data) continue;
      if ((long)(now - w->lastProcessTime) >= (long)w->processFrequency) {
        if (w->processCb) w->processCb(data);
        w->lastProcessTime = now;
        // partial redraw: background + display + frame
        if (w->hasBackground) tft.fillRect(w->x, w->y, w->width, w->height, w->bgColor);
        if (w->displayCb) w->displayCb(data);
        if (w->hasFrame) tft.drawRect(w->x, w->y, w->width, w->height, COLOR_FRAME);
      }
    }
  }

  delay(10); // small yield to reduce CPU hogging
}

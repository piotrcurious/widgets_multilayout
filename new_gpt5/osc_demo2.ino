#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>   // Example TFT, adjust to your hardware

// -------------------- Display Setup --------------------
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// -------------------- Global Color Scheme --------------------
struct ColorScheme {
  uint16_t bg;
  uint16_t fg;
  uint16_t grid;
  uint16_t ch1;
  uint16_t ch2;
  uint16_t trigger;
};
static const ColorScheme scheme = {
  ILI9341_BLACK,
  ILI9341_WHITE,
  ILI9341_DARKGREY,
  ILI9341_YELLOW,
  ILI9341_CYAN,
  ILI9341_RED
};

// -------------------- Widget System --------------------
struct WidgetDynamic;   // forward

typedef void (*ProcessCallback)(WidgetDynamic*);
typedef void (*DisplayCallback)(WidgetDynamic*, Adafruit_GFX*);

struct WidgetStatic {
  int16_t x, y, w, h;
  ProcessCallback process;
  DisplayCallback display;
  bool border;
  bool background;
};

struct WidgetDynamic {
  const WidgetStatic* def;
  uint32_t lastProcess;
  uint32_t lastDisplay;
  uint32_t processInterval;
  uint32_t displayInterval;
  void* data;
};

// -------------------- Oscilloscope Buffers --------------------
const int SAMPLE_BUFFER = 1024;   // larger buffer
volatile uint16_t ch1Buffer[SAMPLE_BUFFER];
volatile uint16_t ch2Buffer[SAMPLE_BUFFER];
volatile int writeIndex = 0;
volatile bool bufferFull = false;

#define CH1_PIN 34
#define CH2_PIN 35
#define TRIG_PIN 32

// trigger configuration
volatile int triggerLevel = 2000;  // ADC mid value
volatile bool triggerRising = true;

// -------------------- Widget Data Structures --------------------
struct OscilloscopeData {
  int lastDrawIndex;
};

// -------------------- Process Callbacks --------------------
void oscilloscopeProcess(WidgetDynamic* dyn) {
  // Acquire samples into circular buffer
  int idx = writeIndex;
  ch1Buffer[idx] = analogRead(CH1_PIN);
  ch2Buffer[idx] = analogRead(CH2_PIN);
  idx++;
  if (idx >= SAMPLE_BUFFER) {
    idx = 0;
    bufferFull = true;
  }
  writeIndex = idx;
}

// -------------------- Display Callbacks --------------------
void oscilloscopeDisplay(WidgetDynamic* dyn, Adafruit_GFX* gfx) {
  OscilloscopeData* d = (OscilloscopeData*)dyn->data;

  // Clear widget area
  if (dyn->def->background) {
    gfx->fillRect(dyn->def->x, dyn->def->y, dyn->def->w, dyn->def->h, scheme.bg);
  }
  if (dyn->def->border) {
    gfx->drawRect(dyn->def->x, dyn->def->y, dyn->def->w, dyn->def->h, scheme.fg);
  }

  int x0 = dyn->def->x;
  int y0 = dyn->def->y;
  int w  = dyn->def->w;
  int h  = dyn->def->h;

  if (!bufferFull) return; // wait until buffer filled once

  // Find trigger point
  int trigIndex = -1;
  for (int i = 1; i < SAMPLE_BUFFER; i++) {
    int prev = analogRead(TRIG_PIN); // could also use CH1Buffer
    int curr = ch1Buffer[i];
    if (triggerRising && prev < triggerLevel && curr >= triggerLevel) {
      trigIndex = i;
      break;
    }
    if (!triggerRising && prev > triggerLevel && curr <= triggerLevel) {
      trigIndex = i;
      break;
    }
  }
  if (trigIndex < 0) trigIndex = 0;

  // Draw waveforms relative to trigger
  int samplesPerPixel = SAMPLE_BUFFER / w;
  if (samplesPerPixel < 1) samplesPerPixel = 1;

  int prevY1 = -1, prevY2 = -1;
  for (int px = 0; px < w; px++) {
    int sampleIndex = (trigIndex + px * samplesPerPixel) % SAMPLE_BUFFER;
    int val1 = ch1Buffer[sampleIndex];
    int val2 = ch2Buffer[sampleIndex];

    int y1 = y0 + h - map(val1, 0, 4095, 0, h);
    int y2 = y0 + h - map(val2, 0, 4095, 0, h);

    if (prevY1 >= 0) gfx->drawLine(x0 + px - 1, prevY1, x0 + px, y1, scheme.ch1);
    if (prevY2 >= 0) gfx->drawLine(x0 + px - 1, prevY2, x0 + px, y2, scheme.ch2);

    prevY1 = y1;
    prevY2 = y2;
  }
}

// -------------------- Static Widget Definitions --------------------
static const WidgetStatic oscilloscopeWidget = {
  0, 0, 320, 240,   // full screen
  oscilloscopeProcess,
  oscilloscopeDisplay,
  true,   // border
  true    // background
};

// -------------------- Layout --------------------
static WidgetDynamic widgets[] = {
  { &oscilloscopeWidget, 0, 0, 1, 50, NULL }   // process ~20kHz, display 20Hz
};

// -------------------- Core Update Loop --------------------
void updateWidgets() {
  uint32_t now = micros();
  for (auto &w : widgets) {
    if ((now - w.lastProcess) >= w.processInterval * 1000UL) {
      w.def->process(&w);
      w.lastProcess = now;
    }
    if ((millis() - w.lastDisplay) >= w.displayInterval) {
      w.def->display(&w, &tft);
      w.lastDisplay = millis();
    }
  }
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(scheme.bg);

  // allocate widget data
  static OscilloscopeData oscData = {0};
  widgets[0].data = &oscData;
}

// -------------------- Loop --------------------
void loop() {
  updateWidgets();
}

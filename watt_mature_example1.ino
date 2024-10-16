#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Define color scheme
const uint16_t COLOR_BG = ILI9341_BLACK;
const uint16_t COLOR_TEXT = ILI9341_WHITE;
const uint16_t COLOR_FRAME = ILI9341_BLUE;
const uint16_t COLOR_GRAPH = ILI9341_GREEN;

// Macro to simplify widget creation
#define CREATE_WIDGET(_x, _y, _w, _h, _processCb, _displayCb, _frequency) \
  Widget(_x, _y, _w, _h, _processCb, _displayCb, _frequency)

// Forward declaration for WidgetManager class
class Widget;

// Widget class representing a single widget
class Widget {
  public:
    int16_t x, y, width, height;
    std::function<void()> processCb;  // Lambda for process callback
    std::function<void()> displayCb;  // Lambda for display callback
    uint16_t processFrequency;
    uint32_t lastProcessTime;
    bool hasFrame;
    bool hasBackground;
    uint16_t bgColor;

    Widget(int16_t _x, int16_t _y, int16_t _w, int16_t _h,
           std::function<void()> _processCb, std::function<void()> _displayCb, uint16_t _frequency)
      : x(_x), y(_y), width(_w), height(_h),
        processCb(_processCb), displayCb(_displayCb),
        processFrequency(_frequency), lastProcessTime(0),
        hasFrame(true), hasBackground(false), bgColor(COLOR_BG) {}

    void process(uint32_t currentTime) {
      if (currentTime - lastProcessTime >= processFrequency) {
        processCb();
        lastProcessTime = currentTime;
      }
    }

    void display() {
      if (hasBackground) {
        tft.fillRect(x, y, width, height, bgColor);
      }
      displayCb();
      if (hasFrame) {
        tft.drawRect(x, y, width, height, COLOR_FRAME);
      }
    }
};

// WidgetManager class to handle layout switching and updates
class WidgetManager {
  public:
    WidgetManager(Widget **layout, int layoutSize) : currentLayout(layout), currentLayoutSize(layoutSize) {}

    void switchLayout(Widget **newLayout, int newSize) {
      currentLayout = newLayout;
      currentLayoutSize = newSize;
      tft.fillScreen(COLOR_BG);  // Clear the screen when switching layouts
    }

    void update(uint32_t currentTime) {
      for (int i = 0; i < currentLayoutSize; i++) {
        currentLayout[i]->process(currentTime);
        currentLayout[i]->display();
      }
    }

  private:
    Widget **currentLayout;
    int currentLayoutSize;
};

// Dynamic data for widgets
float watts = 0, volts = 0, amperes = 0;
float wattsHistory[50];  // Rolling history for graph
int historyIndex = 0;

// Process functions using lambdas
auto processWatts = []() { watts = volts * amperes; };
auto processVolts = []() { volts = random(220, 230); };
auto processAmperes = []() { amperes = random(1, 5); };
auto updateWattsGraph = []() {
  wattsHistory[historyIndex] = watts;
  historyIndex = (historyIndex + 1) % 50;  // Circular buffer for history
};

// Display functions using lambdas
auto displayWatts = []() {
  tft.setCursor(10, 10);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print("Watts: ");
  tft.println(watts);
};

auto displayVolts = []() {
  tft.setCursor(10, 40);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print("Volts: ");
  tft.println(volts);
};

auto displayAmperes = []() {
  tft.setCursor(10, 70);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print("Amperes: ");
  tft.println(amperes);
};

auto displayWattsGraph = []() {
  tft.fillRect(10, 100, 220, 50, COLOR_BG);  // Clear graph area
  for (int i = 0; i < 50; i++) {
    int index = (historyIndex + i) % 50;
    int graphX = 10 + i * 4;
    int graphY = 150 - wattsHistory[index] * 10; // Scale watts to graph
    tft.drawPixel(graphX, graphY, COLOR_GRAPH);
  }
};

// Define Widgets
Widget wattsWidget = CREATE_WIDGET(10, 10, 100, 30, processWatts, displayWatts, 100);
Widget voltsWidget = CREATE_WIDGET(10, 40, 100, 30, processVolts, displayVolts, 1000);
Widget amperesWidget = CREATE_WIDGET(10, 70, 100, 30, processAmperes, displayAmperes, 1000);
Widget wattsGraphWidget = CREATE_WIDGET(10, 100, 220, 50, updateWattsGraph, displayWattsGraph, 1000);

// Layouts
Widget *layout1[] = { &wattsWidget, &voltsWidget, &wattsGraphWidget };
Widget *layout2[] = { &wattsWidget, &voltsWidget, &amperesWidget };
Widget *layout3[] = { &voltsWidget, &wattsGraphWidget };
Widget *layout4[] = { &wattsWidget };

// Layout sizes
int layout1Size = sizeof(layout1) / sizeof(layout1[0]);
int layout2Size = sizeof(layout2) / sizeof(layout2[0]);
int layout3Size = sizeof(layout3) / sizeof(layout3[0]);
int layout4Size = sizeof(layout4) / sizeof(layout4[0]);

// Initialize widget manager
WidgetManager manager(layout1, layout1Size);

void setup() {
  tft.begin();
  tft.fillScreen(COLOR_BG);
}

void loop() {
  uint32_t currentTime = millis();

  // Update the current layout
  manager.update(currentTime);

  // Simulate layout switching (for testing, replace with button input)
  if (currentTime % 15000 < 5000) {
    manager.switchLayout(layout1, layout1Size);
  } else if (currentTime % 15000 < 10000) {
    manager.switchLayout(layout2, layout2Size);
  } else if (currentTime % 15000 < 15000) {
    manager.switchLayout(layout3, layout3Size);
  } else {
    manager.switchLayout(layout4, layout4Size);
  }

  delay(100);  // Adjust for smooth updates
}

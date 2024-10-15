#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>  // Example: For TFT display
#include <SPI.h>

#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Define color scheme (single global scheme)
const uint16_t COLOR_BG = ILI9341_BLACK;
const uint16_t COLOR_TEXT = ILI9341_WHITE;
const uint16_t COLOR_FRAME = ILI9341_BLUE;
const uint16_t COLOR_GRAPH = ILI9341_GREEN;

// Callback function types for widgets
typedef void (*ProcessCallback)(void *);
typedef void (*DisplayCallback)(void *);

// Define the Widget structure
struct Widget {
  int16_t x, y, width, height;  // Position and size
  ProcessCallback processCb;    // Process callback (logic)
  DisplayCallback displayCb;    // Display callback (rendering)
  uint16_t processFrequency;    // Frequency for process update
  uint32_t lastProcessTime;     // Last time process callback ran
  bool hasFrame;                // Whether to draw a frame
  bool hasBackground;           // Whether to fill a background
  uint16_t bgColor;             // Background color
};

// Dynamic data variables
float watts = 0, volts = 0, amperes = 0;
float wattsHistory[50]; // Rolling history for graph
int historyIndex = 0;

// Process Callbacks
void processWatts(void *data) {
  watts = volts * amperes;  // Compute watts
}

void processVolts(void *data) {
  volts = random(220, 230); // Simulate voltage readings (220-230V)
}

void processAmperes(void *data) {
  amperes = random(1, 5);   // Simulate amperage readings (1-5A)
}

void updateWattsGraph(void *data) {
  wattsHistory[historyIndex] = watts;
  historyIndex = (historyIndex + 1) % 50;  // Circular buffer for history
}

// Display Callbacks
void displayWatts(void *data) {
  tft.setCursor(10, 10);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print("Watts: ");
  tft.println(watts);
}

void displayVolts(void *data) {
  tft.setCursor(10, 40);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print("Volts: ");
  tft.println(volts);
}

void displayAmperes(void *data) {
  tft.setCursor(10, 70);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print("Amperes: ");
  tft.println(amperes);
}

void displayWattsGraph(void *data) {
  tft.fillRect(10, 100, 220, 50, COLOR_BG);  // Clear graph area
  for (int i = 0; i < 50; i++) {
    int index = (historyIndex + i) % 50;
    int graphX = 10 + i * 4;
    int graphY = 150 - wattsHistory[index] * 10; // Scale watts to graph
    tft.drawPixel(graphX, graphY, COLOR_GRAPH);
  }
}

// Define Widgets
Widget wattsWidget = {10, 10, 100, 30, processWatts, displayWatts, 100, 0, true, false, COLOR_BG};
Widget voltsWidget = {10, 40, 100, 30, processVolts, displayVolts, 1000, 0, true, false, COLOR_BG};
Widget amperesWidget = {10, 70, 100, 30, processAmperes, displayAmperes, 1000, 0, true, false, COLOR_BG};
Widget wattsGraphWidget = {10, 100, 220, 50, updateWattsGraph, displayWattsGraph, 1000, 0, true, false, COLOR_BG};

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

// Current layout
Widget **currentLayout = layout1;
int currentLayoutSize = layout1Size;

// Function to switch layouts
void switchLayout(Widget **newLayout, int newSize) {
  currentLayout = newLayout;
  currentLayoutSize = newSize;
  tft.fillScreen(COLOR_BG);  // Clear the screen when switching layouts
}

void setup() {
  tft.begin();
  tft.fillScreen(COLOR_BG);
  
  // Set initial data
  wattsWidget.processCb(&watts);
  voltsWidget.processCb(&volts);
  amperesWidget.processCb(&amperes);
}

void loop() {
  uint32_t currentTime = millis();
  
  // Iterate over current layout's widgets
  for (int i = 0; i < currentLayoutSize; i++) {
    Widget *widget = currentLayout[i];
    
    // Process logic based on frequency
    if (currentTime - widget->lastProcessTime >= widget->processFrequency) {
      widget->processCb(&watts);  // Update the relevant data
      widget->lastProcessTime = currentTime;
    }

    // Call the display callback to render the widget
    widget->displayCb(&watts);
    
    // Draw a frame around the widget if enabled
    if (widget->hasFrame) {
      tft.drawRect(widget->x, widget->y, widget->width, widget->height, COLOR_FRAME);
    }
  }

  // Simulate layout switching (for testing, replace with button input)
  if (currentTime % 15000 < 5000) {
    switchLayout(layout1, layout1Size);
  } else if (currentTime % 15000 < 10000) {
    switchLayout(layout2, layout2Size);
  } else if (currentTime % 15000 < 15000) {
    switchLayout(layout3, layout3Size);
  } else {
    switchLayout(layout4, layout4Size);
  }

  delay(100);  // Adjust for smooth updates
}

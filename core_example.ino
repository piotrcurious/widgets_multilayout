#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>  // Example: For TFT display
#include <SPI.h>

// Define global constants for the display
#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Define color scheme (single global scheme)
const uint16_t COLOR_BG = ILI9341_BLACK;
const uint16_t COLOR_TEXT = ILI9341_WHITE;
const uint16_t COLOR_FRAME = ILI9341_BLUE;

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

// Example of process and display callback for a widget (e.g., voltage display)
void processVoltage(void *data) {
  // Simulate dynamic voltage reading (replace with actual logic)
  int *voltage = (int *)data;
  *voltage = random(0, 500);  // Simulating 0-5V in mV
}

void displayVoltage(void *data) {
  int *voltage = (int *)data;
  char voltageStr[10];
  sprintf(voltageStr, "%d mV", *voltage);
  
  tft.setCursor(10, 10);
  tft.setTextColor(COLOR_TEXT);
  tft.setTextSize(2);
  tft.print(voltageStr);
}

// Define widgets statically
int voltageData = 0;  // Example dynamic data for voltage
Widget voltageWidget = {
  10, 10, 100, 50, processVoltage, displayVoltage, 1000, 0, true, true, ILI9341_RED
};

// Multiple widget sets (layouts)
Widget *layout1[] = { &voltageWidget };

// Current layout pointer
Widget **currentLayout = layout1;
int currentLayoutSize = sizeof(layout1) / sizeof(layout1[0]);

void setup() {
  tft.begin();
  tft.fillScreen(COLOR_BG);
  
  // Set initial data
  voltageWidget.processCb(&voltageData);
}

void loop() {
  uint32_t currentTime = millis();
  
  // Iterate over current layout's widgets
  for (int i = 0; i < currentLayoutSize; i++) {
    Widget *widget = currentLayout[i];
    
    // Process logic based on frequency
    if (currentTime - widget->lastProcessTime >= widget->processFrequency) {
      widget->processCb(&voltageData);
      widget->lastProcessTime = currentTime;
    }

    // Draw the widget's background if enabled
    if (widget->hasBackground) {
      tft.fillRect(widget->x, widget->y, widget->width, widget->height, widget->bgColor);
    }

    // Call the display callback to render the widget
    widget->displayCb(&voltageData);

    // Draw a frame around the widget if enabled
    if (widget->hasFrame) {
      tft.drawRect(widget->x, widget->y, widget->width, widget->height, COLOR_FRAME);
    }
  }
  
  // Delay to allow for smooth updates (adjust as needed)
  delay(100);
}

// Function to switch layouts (can be called dynamically)
void switchLayout(Widget **newLayout, int newSize) {
  currentLayout = newLayout;
  currentLayoutSize = newSize;
  tft.fillScreen(COLOR_BG);  // Clear the screen when switching layouts
}

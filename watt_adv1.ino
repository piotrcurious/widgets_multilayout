#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// TFT pins
#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Global color scheme
const uint16_t COLOR_BG = ILI9341_BLACK;
const uint16_t COLOR_TEXT = ILI9341_WHITE;
const uint16_t COLOR_FRAME = ILI9341_BLUE;
const uint16_t COLOR_GRAPH = ILI9341_GREEN;

// Abstract base class for widgets
class Widget {
public:
    Widget(int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency)
        : x(_x), y(_y), width(_w), height(_h), processFrequency(_processFrequency), lastProcessTime(0) {}

    virtual void process(uint32_t currentTime) {
        if (currentTime - lastProcessTime >= processFrequency) {
            processLogic();
            lastProcessTime = currentTime;
        }
    }

    virtual void display() = 0;

protected:
    int16_t x, y, width, height;
    uint32_t lastProcessTime;
    uint16_t processFrequency;

    virtual void processLogic() = 0;
};

// Derived class for Text Widgets
class TextWidget : public Widget {
public:
    TextWidget(int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency, const char* _label, std::function<float()> _dataSource)
        : Widget(_x, _y, _w, _h, _processFrequency), label(_label), dataSource(_dataSource), value(0) {}

    void display() override {
        tft.setCursor(x, y);
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(2);
        tft.fillRect(x, y, width, height, COLOR_BG); // Clear old value
        tft.print(label);
        tft.print(": ");
        tft.println(value, 2);  // Display value with 2 decimal places
    }

protected:
    const char* label;
    std::function<float()> dataSource;
    float value;

    void processLogic() override {
        value = dataSource();
    }
};

// Derived class for Graph Widgets
class GraphWidget : public Widget {
public:
    GraphWidget(int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency, std::function<float()> _dataSource)
        : Widget(_x, _y, _w, _h, _processFrequency), dataSource(_dataSource), index(0) {
        memset(history, 0, sizeof(history));
    }

    void display() override {
        tft.fillRect(x, y, width, height, COLOR_BG);  // Clear graph area
        for (int i = 0; i < 50; i++) {
            int graphX = x + i * (width / 50);
            int graphY = y + height - (history[(index + i) % 50] * height / maxValue);
            tft.drawPixel(graphX, graphY, COLOR_GRAPH);
        }
    }

protected:
    std::function<float()> dataSource;
    float history[50];
    int index;
    const float maxValue = 500;

    void processLogic() override {
        history[index] = dataSource();
        index = (index + 1) % 50;
    }
};

// State-based WidgetManager for handling layouts
class WidgetManager {
public:
    WidgetManager(Widget **_allWidgets, int _totalWidgetCount) 
        : allWidgets(_allWidgets), totalWidgetCount(_totalWidgetCount), currentLayout(nullptr), currentLayoutSize(0) {}

    void switchLayout(Widget **newLayout, int newSize) {
        currentLayout = newLayout;
        currentLayoutSize = newSize;
        tft.fillScreen(COLOR_BG);  // Clear the screen when switching layouts
    }

    void processAllWidgets(uint32_t currentTime) {
        for (int i = 0; i < totalWidgetCount; i++) {
            allWidgets[i]->process(currentTime);
        }
    }

    void updateDisplay() {
        for (int i = 0; i < currentLayoutSize; i++) {
            currentLayout[i]->display();
        }
    }

private:
    Widget **allWidgets;
    int totalWidgetCount;
    Widget **currentLayout;
    int currentLayoutSize;
};

// Global dynamic data
float watts = 0, volts = 0, amperes = 0, wattHours = 0;
uint32_t lastWhCalculationTime = 0;

// Functions to compute dynamic data
float getWatts() { return watts; }
float getVolts() { return volts; }
float getAmperes() { return amperes; }
float getWattHours() { return wattHours; }

// Time integration for watt-hours
void updateWattHours() {
    uint32_t currentTime = millis();
    float deltaTime = (currentTime - lastWhCalculationTime) / 3600000.0;  // ms to hours
    wattHours += watts * deltaTime;
    lastWhCalculationTime = currentTime;
}

// Widget Definitions
TextWidget wattsWidget(10, 10, 100, 30, 100, "Watts", getWatts);
TextWidget voltsWidget(10, 40, 100, 30, 1000, "Volts", getVolts);
TextWidget amperesWidget(10, 70, 100, 30, 1000, "Amperes", getAmperes);
GraphWidget wattsGraphWidget(10, 100, 220, 50, 1000, getWatts);
TextWidget wattHoursWidget(10, 10, 100, 30, 1000, "Watt Hours", getWattHours);
GraphWidget wattHoursGraphWidget(10, 100, 220, 50, 1000, getWattHours);

// Widget arrays for different layouts
Widget* layout1[] = { &wattsWidget, &voltsWidget, &wattsGraphWidget };
Widget* layout2[] = { &wattsWidget, &voltsWidget, &amperesWidget };
Widget* layout3[] = { &wattsWidget, &wattHoursWidget, &wattHoursGraphWidget };

// All widgets list
Widget* allWidgets[] = { &wattsWidget, &voltsWidget, &amperesWidget, &wattsGraphWidget, &wattHoursWidget, &wattHoursGraphWidget };

// Total widget count
int totalWidgetCount = sizeof(allWidgets) / sizeof(allWidgets[0]);

// Initialize WidgetManager
WidgetManager manager(allWidgets, totalWidgetCount);

void setup() {
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(COLOR_BG);
    lastWhCalculationTime = millis();

    // Initial layout
    manager.switchLayout(layout1, sizeof(layout1) / sizeof(layout1[0]));
}

void loop() {
    uint32_t currentTime = millis();

    // Process all widgets
    manager.processAllWidgets(currentTime);

    // Display current layout
    manager.updateDisplay();

    // Update watt-hours integration in background
    updateWattHours();

    // Example layout switching (can be triggered by buttons or conditions)
    if (currentTime > 20000 && currentTime < 40000) {
        manager.switchLayout(layout2, sizeof(layout2) / sizeof(layout2[0]));
    } else if (currentTime > 40000 && currentTime < 60000) {
        manager.switchLayout(layout3, sizeof(layout3) / sizeof(layout3[0]));
    } else if (currentTime > 60000) {
        manager.switchLayout(layout1, sizeof(layout1) / sizeof(layout1[0]));
    }
}

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include "color_theme.h"

// TFT pins
#define TFT_CS     15
#define TFT_DC     2
#define TFT_RST    4

// Abstract Display Interface
class Display {
public:
    virtual void begin() = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void setTextColor(uint16_t color) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void print(const char* text) = 0;
    virtual void print(float value, int decimals) = 0;
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
};

// Adafruit GFX Wrapper Implementation
class AdafruitDisplay : public Display {
private:
    Adafruit_ILI9341 tft;

public:
    AdafruitDisplay(uint8_t cs, uint8_t dc, uint8_t rst) : tft(cs, dc, rst) {}

    void begin() override {
        tft.begin();
    }

    void setRotation(uint8_t rotation) override {
        tft.setRotation(rotation);
    }

    void fillScreen(uint16_t color) override {
        tft.fillScreen(color);
    }

    void setCursor(int16_t x, int16_t y) override {
        tft.setCursor(x, y);
    }

    void setTextColor(uint16_t color) override {
        tft.setTextColor(color);
    }

    void setTextSize(uint8_t size) override {
        tft.setTextSize(size);
    }

    void print(const char* text) override {
        tft.print(text);
    }

    void print(float value, int decimals) override {
        tft.print(value, decimals);
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        tft.drawPixel(x, y, color);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        tft.fillRect(x, y, w, h, color);
    }
};

// Abstract base class for widgets
class Widget {
protected:
    Display* display;
    int16_t x, y, width, height;
    uint32_t lastProcessTime;
    uint16_t processFrequency;

public:
    Widget(Display* _display, int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency)
        : display(_display), x(_x), y(_y), width(_w), height(_h), processFrequency(_processFrequency), lastProcessTime(0) {}

    virtual void process(uint32_t currentTime) {
        if (currentTime - lastProcessTime >= processFrequency) {
            processLogic();
            lastProcessTime = currentTime;
        }
    }

    virtual void displayWidget() = 0;
    
    // Add public method to access display
    Display* getDisplay() const { return display; }

protected:
    virtual void processLogic() = 0;
};

// Derived class for Text Widgets
class TextWidget : public Widget {
private:
    const char* label;
    std::function<float()> dataSource;
    float value;

public:
    TextWidget(Display* _display, int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency, const char* _label, std::function<float()> _dataSource)
        : Widget(_display, _x, _y, _w, _h, _processFrequency), label(_label), dataSource(_dataSource), value(0) {}

    void displayWidget() override {
        // Clear background with widget background color
        display->fillRect(x, y, width, height, COLOR_WIDGET_BG);
        
        display->setCursor(x, y);
        display->setTextColor(COLOR_TEXT);  // Use main text color for label
        display->setTextSize(2);
        display->print(label);
        display->print(": ");
        
        // Set value color based on range
        uint16_t valueColor = COLOR_VALUE_NORMAL;
        if (value > 400) valueColor = COLOR_VALUE_HIGH;
        else if (value < 100) valueColor = COLOR_VALUE_LOW;
        
        display->setTextColor(valueColor);
        display->print(value, 2);
    }

protected:
    void processLogic() override {
        value = dataSource();
    }
};

// Derived class for Graph Widgets
class GraphWidget : public Widget {
private:
    std::function<float()> dataSource;
    float history[50];
    int index;
    const float maxValue = 500;

public:
    GraphWidget(Display* _display, int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency, std::function<float()> _dataSource)
        : Widget(_display, _x, _y, _w, _h, _processFrequency), dataSource(_dataSource), index(0) {
        memset(history, 0, sizeof(history));
    }

    void displayWidget() override {
        // Clear graph area with widget background
        display->fillRect(x, y, width, height, COLOR_WIDGET_BG);
        
        // Draw grid lines
        for (int i = 0; i < width; i += 20) {
            for (int j = 0; j < height; j += 10) {
                display->drawPixel(x + i, y + j, COLOR_GRAPH_GRID);
            }
        }
        
        // Draw axes
        for (int i = 0; i < width; i++) {
            display->drawPixel(x + i, y + height - 1, COLOR_GRAPH_AXIS);
        }
        for (int i = 0; i < height; i++) {
            display->drawPixel(x, y + i, COLOR_GRAPH_AXIS);
        }
        
        // Draw graph line
        for (int i = 0; i < 49; i++) {
            int x1 = x + i * (width / 50);
            int x2 = x + (i + 1) * (width / 50);
            int y1 = y + height - (history[(index + i) % 50] * height / maxValue);
            int y2 = y + height - (history[(index + i + 1) % 50] * height / maxValue);
            
            // Use highlight color for peaks
            uint16_t lineColor = COLOR_GRAPH;
            if (history[(index + i) % 50] > maxValue * 0.8) {
                lineColor = COLOR_GRAPH_HIGHLIGHT;
            }
            
            display->drawPixel(x1, y1, lineColor);
        }
    }

protected:
    void processLogic() override {
        history[index] = dataSource();
        index = (index + 1) % 50;
    }
};

// Updated WidgetManager
class WidgetManager {
private:
    Widget **allWidgets;
    int totalWidgetCount;
    Widget **currentLayout;
    int currentLayoutSize;

public:
    WidgetManager(Widget **_allWidgets, int _totalWidgetCount) 
        : allWidgets(_allWidgets), totalWidgetCount(_totalWidgetCount), currentLayout(nullptr), currentLayoutSize(0) {}

    void switchLayout(Widget **newLayout, int newSize) {
        currentLayout = newLayout;
        currentLayoutSize = newSize;
        if (currentLayoutSize > 0 && currentLayout[0]) {
            currentLayout[0]->getDisplay()->fillScreen(COLOR_BG);  // Use themed background color
        }
    }

    void processAllWidgets(uint32_t currentTime) {
        for (int i = 0; i < totalWidgetCount; i++) {
            allWidgets[i]->process(currentTime);
        }
    }

    void updateDisplay() {
        for (int i = 0; i < currentLayoutSize; i++) {
            currentLayout[i]->displayWidget();
        }
    }
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
Display* display;
TextWidget wattsWidget(display, 10, 10, 100, 30, 100, "Watts", getWatts);
TextWidget voltsWidget(display, 10, 40, 100, 30, 1000, "Volts", getVolts);
TextWidget amperesWidget(display, 10, 70, 100, 30, 1000, "Amperes", getAmperes);
GraphWidget wattsGraphWidget(display, 10, 100, 220, 50, 1000, getWatts);
TextWidget wattHoursWidget(display, 10, 10, 100, 30, 1000, "Watt Hours", getWattHours);
GraphWidget wattHoursGraphWidget(display, 10, 100, 220, 50, 1000, getWattHours);

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
    // Initialize the display
    display = new AdafruitDisplay(TFT_CS, TFT_DC, TFT_RST);
    display->begin();
    display->setRotation(3);
    display->fillScreen(COLOR_BG);  // Use themed background color

    lastWhCalculationTime = millis();

    // Initial layout
    manager.switchLayout(layout1, sizeof(layout1) / sizeof(layout1[0]));
}

void loop() {
    uint32_t currentTime = millis();

    // Process all widgets even if they're not displayed to keep data updated
    manager.processAllWidgets(currentTime);

    // Update current layout's display
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

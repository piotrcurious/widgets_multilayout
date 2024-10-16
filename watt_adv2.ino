#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// TFT pins
#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2

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
        display->setCursor(x, y);
        display->setTextColor(COLOR_TEXT);
        display->setTextSize(2);
        display->fillRect(x, y, width, height, COLOR_BG);  // Clear old value
        display->print(label);
        display->print(": ");
        display->print(value, 2);  // Display value with 2 decimal places
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
        display->fillRect(x, y, width, height, COLOR_BG);  // Clear graph area
        for (int i = 0; i < 50; i++) {
            int graphX = x + i * (width / 50);
            int graphY = y + height - (history[(index + i) % 50] * height / maxValue);
            display->drawPixel(graphX, graphY, COLOR_GRAPH);
        }
    }

protected:
    void processLogic() override {
        history[index] = dataSource();
        index = (index + 1) % 50;
    }
};

// WidgetManager for handling layouts
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
        allWidgets[0]->display->fillScreen(COLOR_BG);  // Clear the screen when switching layouts
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
    display->fillScreen(COLOR_BG);

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

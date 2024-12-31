#ifndef UI_FRAMEWORK_H
#define UI_FRAMEWORK_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRremote.h>
#include <vector>
#include <functional>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Pin definitions
#define IR_RECEIVE_PIN 15
#define VOLTAGE_PIN 36
#define CURRENT_PIN 39
#define TEMP_PIN 34
#define AMBIENT_TEMP_PIN 35
#define PWM_PIN 25

// Battery parameters
#define CELL_COUNT 4
#define CHARGE_CURRENT_MA 1000
#define TRICKLE_CURRENT_MA 50
#define CELL_VOLTAGE_MAX 1.45
#define CELL_VOLTAGE_MIN 1.0
#define CAPACITY_MAH 2000

// Temperature parameters
#define DT_THRESHOLD 2.0f
#define MAX_DT_RATE 1.0f
#define MAX_TEMP 45.0f

// IR Remote codes
#define IR_RED 0xF720DF
#define IR_GREEN 0xA720DF
#define IR_BLUE 0x6720DF

// Input event structure
struct InputEvent {
    enum Type {
        NONE,
        IR_BUTTON,
        TIMER
    } type;
    uint32_t value;
};

// Charger states
enum ChargerState {
    IDLE,
    CHARGING,
    TRICKLE,
    COMPLETE,
    ERROR
};

// Screen identifiers
enum ScreenType {
    MAIN_SCREEN,
    GRAPH_SCREEN
};

// Graph types
enum GraphType {
    VOLTAGE_GRAPH,
    CURRENT_GRAPH,
    TEMP_DELTA_GRAPH
};

// History point structure for graphs
struct HistoryPoint {
    float voltage;
    float current;
    float tempDelta;
    unsigned long timestamp;
};

// Base Widget class
class Widget {
protected:
    int16_t x, y, width, height;
    bool focused;
    bool dirty;

public:
    Widget(int16_t x, int16_t y, int16_t w, int16_t h) 
        : x(x), y(y), width(w), height(h), focused(false), dirty(true) {}
    
    virtual ~Widget() {}
    
    virtual void draw(Adafruit_SSD1306& display) = 0;
    virtual void handleInput(const InputEvent& event) = 0;
    virtual void update() = 0;
    
    void setFocus(bool focus) { focused = focus; dirty = true; }
    bool isFocused() const { return focused; }
    bool isDirty() const { return dirty; }
    void clearDirty() { dirty = false; }
    void markDirty() { dirty = true; }
};

// Screen class
class Screen {
private:
    std::vector<Widget*> widgets;
    size_t focusedWidgetIndex;
    
public:
    Screen() : focusedWidgetIndex(0) {}
    
    virtual ~Screen() {
        for (auto widget : widgets) {
            delete widget;
        }
    }
    
    void addWidget(Widget* widget) {
        widgets.push_back(widget);
        if (widgets.size() == 1) {
            widget->setFocus(true);
        }
    }

    Widget* getWidget(size_t index) {
        return index < widgets.size() ? widgets[index] : nullptr;
    }
    
    virtual void handleInput(const InputEvent& event) {
        if (event.type == InputEvent::IR_BUTTON) {
            if (event.value == 0xFF629D) { // UP
                changeFocus(-1);
            } else if (event.value == 0xFFA857) { // DOWN
                changeFocus(1);
            } else {
                if (focusedWidgetIndex < widgets.size()) {
                    widgets[focusedWidgetIndex]->handleInput(event);
                }
            }
        }
    }
    
    virtual void update() {
        for (auto widget : widgets) {
            widget->update();
        }
    }
    
    void draw(Adafruit_SSD1306& display) {
        for (auto widget : widgets) {
            if (widget->isDirty()) {
                widget->draw(display);
                widget->clearDirty();
            }
        }
    }
    
private:
    void changeFocus(int direction) {
        if (widgets.empty()) return;
        
        widgets[focusedWidgetIndex]->setFocus(false);
        
        if (direction > 0) {
            focusedWidgetIndex = (focusedWidgetIndex + 1) % widgets.size();
        } else {
            focusedWidgetIndex = (focusedWidgetIndex + widgets.size() - 1) % widgets.size();
        }
        
        widgets[focusedWidgetIndex]->setFocus(true);
    }
};

// Button widget
class Button : public Widget {
private:
    const char* label;
    std::function<void()> callback;
    
public:
    Button(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, std::function<void()> callback)
        : Widget(x, y, w, h), label(label), callback(callback) {}
    
    void setLabel(const char* newLabel) {
        label = newLabel;
        dirty = true;
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.drawRect(x, y, width, height, WHITE);
        if (focused) {
            display.fillRect(x + 2, y + 2, width - 4, height - 4, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        
        int16_t textX = x + (width - strlen(label) * 6) / 2;
        int16_t textY = y + (height - 8) / 2;
        display.setCursor(textX, textY);
        display.print(label);
    }
    
    void handleInput(const InputEvent& event) override {
        if (event.type == InputEvent::IR_BUTTON && event.value == 0xFF02FD) {
            if (callback) callback();
        }
    }
    
    void update() override {}
};

// Label widget
class Label : public Widget {
private:
    String text;
    bool centered;
    
public:
    Label(int16_t x, int16_t y, int16_t w, int16_t h, const String& text, bool centered = false)
        : Widget(x, y, w, h), text(text), centered(centered) {}
    
    void setText(const String& newText) {
        if (text != newText) {
            text = newText;
            dirty = true;
        }
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.setTextColor(WHITE);
        if (centered) {
            int16_t textX = x + (width - text.length() * 6) / 2;
            display.setCursor(textX, y);
        } else {
            display.setCursor(x, y);
        }
        display.print(text);
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// FloatDisplay widget
class FloatDisplay : public Widget {
private:
    float value;
    int precision;
    const char* units;
    
public:
    FloatDisplay(int16_t x, int16_t y, int16_t w, int16_t h, int precision, const char* units)
        : Widget(x, y, w, h), value(0), precision(precision), units(units) {}
    
    void setValue(float newValue) {
        if (value != newValue) {
            value = newValue;
            dirty = true;
        }
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.setTextColor(WHITE);
        display.setCursor(x, y);
        display.print(value, precision);
        display.print(" ");
        display.print(units);
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// BatteryWidget
class BatteryWidget : public Widget {
private:
    float totalVoltage;
    float current;
    float percentage;
    
public:
    BatteryWidget(int16_t x, int16_t y, int16_t w, int16_t h)
        : Widget(x, y, w, h), totalVoltage(0), current(0), percentage(0) {}
    
    void updateValues(float voltage, float curr) {
        totalVoltage = voltage;
        current = curr;
        percentage = ((voltage/CELL_COUNT) - CELL_VOLTAGE_MIN) / 
                    (CELL_VOLTAGE_MAX - CELL_VOLTAGE_MIN) * 100;
        percentage = constrain(percentage, 0, 100);
        dirty = true;
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.drawRect(x, y + 2, width - 10, height - 4, WHITE);
        display.fillRect(x + width - 10, y + height/3, 10, height/3, WHITE);
        
        int fillWidth = ((width - 14) * percentage) / 100;
        display.fillRect(x + 2, y + 4, fillWidth, height - 8, WHITE);
        
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2fV %.0fmA", totalVoltage, current);
        display.setCursor(x + 2, y + height/2 - 4);
        display.setTextColor(percentage > 50 ? BLACK : WHITE);
        display.print(buffer);
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// FunctionPlotter widget
class FunctionPlotter : public Widget {
private:
    std::function<float(float)> plotFunction;
    float xMin, xMax;
    float yMin, yMax;
    bool autoScale;
    
public:
    FunctionPlotter(int16_t x, int16_t y, int16_t w, int16_t h,
                   std::function<float(float)> func,
                   float xMin = -1, float xMax = 1)
        : Widget(x, y, w, h), plotFunction(func),
          xMin(xMin), xMax(xMax), yMin(0), yMax(1),
          autoScale(true) {}
    
    void setYRange(float min, float max) {
        yMin = min;
        yMax = max;
        autoScale = false;
        dirty = true;
    }
    
    void enableAutoScale(bool enable) {
        autoScale = enable;
        dirty = true;
    }
    
    void draw(Adafruit_SSD1306& display) override {
        // Draw axes
        display.drawLine(x, y + height - 1, x + width - 1, y + height - 1, WHITE);
        display.drawLine(x, y, x, y + height - 1, WHITE);
        
        // Plot function
        int16_t lastY = -1;
        for(int16_t i = 0; i < width; i++) {
            float xVal = xMin + (xMax - xMin) * i / (width - 1);
            float yVal = plotFunction(xVal);
            int16_t plotY = y + height - 1 - 
                           (height - 1) * (yVal - yMin) / (yMax - yMin);
            plotY = constrain(plotY, y, y + height - 1);
            
            if(lastY != -1) {
                display.drawLine(x + i - 1, lastY, x + i, plotY, WHITE);
            }
            lastY = plotY;
        }
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// GraphScreen class
class GraphScreen : public Screen {
private:
    FunctionPlotter* plotter;
    Label* graphLabel;
    Button* switchButton;
    std::vector<HistoryPoint> getVisibleHistory();
    float graphFunction(float x);
    
public:
    GraphScreen();
    void update() override;
};

// UIManager class
class UIManager {
private:
    Adafruit_SSD1306 display;
    IRrecv irReceiver;
    std::vector<Screen*> screens;
    ScreenType currentScreenType;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 50;
    
public:
    UIManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
                 irReceiver(IR_RECEIVE_PIN),
                 currentScreenType(MAIN_SCREEN),
                 lastUpdateTime(0) {}
    
    bool begin() {
        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            return false;
        }
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.display();
        
        irReceiver.enableIRIn();
        return true;
    }
    
    void addScreen(ScreenType type, Screen* screen) {
        if (type >= screens.size()) {
            screens.resize(type + 1, nullptr);
        }
        if (screens[type]) {
            delete screens[type];
        }
        screens[type] = screen;
    }
    
    Screen* getScreen(ScreenType type) {
        return type < screens.size() ? screens[type] : nullptr;
    }
    
    void setScreen(ScreenType type) {
        if (type < screens.size() && screens[type]) {
            currentScreenType = type;
            display.clearDisplay();
        }
    }
    
    void update() {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
            return;
        }
        lastUpdateTime = currentTime;
        
        Screen* currentScreen = getScreen(currentScreenType);
        if (!currentScreen) return;
        
        InputEvent event{InputEvent::NONE, 0};
        if (irReceiver.decode()) {
            event.type = InputEvent::IR_BUTTON;
            event.value = irReceiver.decodedIRData.decodedRawData;
            irReceiver.resume();
        }
        
        currentScreen->handleInput(event);
        currentScreen->update();
        
        display.clearDisplay();
        currentScreen->draw(display);
        display.display();
    }
};

#endif // UI_FRAMEWORK_H



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

// IR Remote settings
#define IR_RECEIVE_PIN 15

// UI Components forward declarations
class Widget;
class Screen;
class Button;
class Label;
class ProgressBar;

// Input event structure
struct InputEvent {
    enum Type {
        NONE,
        IR_BUTTON,
        TIMER
    } type;
    uint32_t value;
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

// Screen class to manage widgets
class Screen {
private:
    std::vector<Widget*> widgets;
    size_t focusedWidgetIndex;
    
public:
    Screen() : focusedWidgetIndex(0) {}
    
    ~Screen() {
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
    
    void handleInput(const InputEvent& event) {
        if (event.type == InputEvent::IR_BUTTON) {
            // Handle focus navigation
            if (event.value == 0xFF629D) { // UP button
                changeFocus(-1);
            } else if (event.value == 0xFFA857) { // DOWN button
                changeFocus(1);
            } else {
                // Pass input to focused widget
                if (focusedWidgetIndex < widgets.size()) {
                    widgets[focusedWidgetIndex]->handleInput(event);
                }
            }
        }
    }
    
    void update() {
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
        if (event.type == InputEvent::IR_BUTTON && event.value == 0xFF02FD) { // OK button
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

// Progress Bar widget
class ProgressBar : public Widget {
private:
    uint8_t progress;
    uint8_t maxValue;
    
public:
    ProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t maxValue = 100)
        : Widget(x, y, w, h), progress(0), maxValue(maxValue) {}
    
    void setProgress(uint8_t value) {
        if (value != progress && value <= maxValue) {
            progress = value;
            dirty = true;
        }
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.drawRect(x, y, width, height, WHITE);
        uint16_t fillWidth = (width - 4) * progress / maxValue;
        display.fillRect(x + 2, y + 2, fillWidth, height - 4, WHITE);
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// Main UI Manager class
class UIManager {
private:
    Adafruit_SSD1306 display;
    IRrecv irReceiver;
    Screen* currentScreen;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 50; // 20 FPS
    
public:
    UIManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
                 irReceiver(IR_RECEIVE_PIN),
                 currentScreen(nullptr),
                 lastUpdateTime(0) {}
    
    bool begin() {
        // Initialize display
        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
            return false;
        }
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.display();
        
        // Initialize IR receiver
        irReceiver.enableIRIn();
        
        return true;
    }
    
    void setScreen(Screen* screen) {
        if (currentScreen) {
            delete currentScreen;
        }
        currentScreen = screen;
        display.clearDisplay();
    }
    
    void update() {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
            return;
        }
        lastUpdateTime = currentTime;
        
        if (!currentScreen) return;
        
        // Handle IR input
        InputEvent event{InputEvent::NONE, 0};
        if (irReceiver.decode()) {
            event.type = InputEvent::IR_BUTTON;
            event.value = irReceiver.decodedIRData.decodedRawData;
            irReceiver.resume();
        }
        
        // Update screen and widgets
        currentScreen->handleInput(event);
        currentScreen->update();
        
        // Redraw if needed
        display.clearDisplay();
        currentScreen->draw(display);
        display.display();
    }
};

#endif // UI_FRAMEWORK_H

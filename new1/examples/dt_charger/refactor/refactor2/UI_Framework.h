#ifndef UI_FRAMEWORK_H
#define UI_FRAMEWORK_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include <functional>
#include "IR_CommandManager.h"

// Display Settings
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr int OLED_RESET = -1;
constexpr uint8_t SCREEN_ADDRESS = 0x3C;

// Base Widget Class
class Widget {
protected:
    int16_t x, y, width, height;
    bool focused, dirty;

public:
    Widget(int16_t x, int16_t y, int16_t width, int16_t height)
        : x(x), y(y), width(width), height(height), focused(false), dirty(true) {}

    virtual ~Widget() = default;
    virtual void draw(Adafruit_SSD1306& display) = 0;
    virtual void handleInput(uint32_t irCode) = 0;

    void setFocus(bool focus) {
        focused = focus;
        dirty = true;
    }

    bool isDirty() const { return dirty; }
    void clearDirty() { dirty = false; }
};

// Label Widget
class Label : public Widget {
private:
    String text;
    bool centered;

public:
    Label(int16_t x, int16_t y, int16_t width, const String& text, bool centered = false)
        : Widget(x, y, width, 10), text(text), centered(centered) {}

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

    void handleInput(uint32_t irCode) override {
        // Labels are static and don't handle input.
    }
};

// Button Widget
class Button : public Widget {
private:
    String label;
    std::function<void()> callback;

public:
    Button(int16_t x, int16_t y, int16_t width, const String& label, std::function<void()> callback)
        : Widget(x, y, width, 10), label(label), callback(callback) {}

    void draw(Adafruit_SSD1306& display) override {
        display.drawRect(x, y, width, height, WHITE);
        if (focused) {
            display.fillRect(x + 2, y + 2, width - 4, height - 4, WHITE);
            display.setTextColor(BLACK);
        } else {
            display.setTextColor(WHITE);
        }
        display.setCursor(x + 4, y + 2);
        display.print(label);
    }

    void handleInput(uint32_t irCode) override {
        if (focused && irCode == IRCodes::OK) {
            if (callback) callback();
        }
    }
};

// Base Screen Class
class Screen {
protected:
    std::vector<Widget*> widgets;
    size_t focusedWidgetIndex = 0;

public:
    virtual ~Screen() {
        for (auto widget : widgets) delete widget;
    }

    void addWidget(Widget* widget) {
        widgets.push_back(widget);
        if (widgets.size() == 1) widget->setFocus(true);
    }

    virtual void handleInput(uint32_t irCode) {
        if (!widgets.empty()) widgets[focusedWidgetIndex]->handleInput(irCode);
    }

    virtual void draw(Adafruit_SSD1306& display) {
        for (auto widget : widgets) {
            if (widget->isDirty()) {
                widget->draw(display);
                widget->clearDirty();
            }
        }
    }

    void navigate(int direction) {
        if (widgets.empty()) return;

        widgets[focusedWidgetIndex]->setFocus(false);
        focusedWidgetIndex = (focusedWidgetIndex + widgets.size() + direction) % widgets.size();
        widgets[focusedWidgetIndex]->setFocus(true);
    }
};

// UIManager Class
class UIManager {
private:
    Adafruit_SSD1306 display;
    IRrecv irReceiver;
    IRCommandManager irManager;
    std::vector<Screen*> screens;
    size_t currentScreenIndex = 0;

public:
    UIManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET), irReceiver(IR_RECEIVE_PIN) {}

    bool begin() {
        if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) return false;
        display.clearDisplay();
        irReceiver.enableIRIn();
        return true;
    }

    void addScreen(Screen* screen) {
        screens.push_back(screen);
    }

    void setScreen(size_t index) {
        if (index < screens.size()) currentScreenIndex = index;
    }

    IRCommandManager& getIRManager() { return irManager; }

    void update() {
        if (irReceiver.decode()) {
            uint32_t code = irReceiver.decodedIRData.decodedRawData;
            if (!irManager.handleCommand(code)) {
                if (currentScreenIndex < screens.size()) {
                    screens[currentScreenIndex]->handleInput(code);
                }
            }
            irReceiver.resume();
        }

        display.clearDisplay();
        if (currentScreenIndex < screens.size()) {
            screens[currentScreenIndex]->draw(display);
        }
        display.display();
    }
};

#endif // UI_FRAMEWORK_H

#ifndef UI_FRAMEWORK_H
#define UI_FRAMEWORK_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include <functional>
#include "IR_CommandManager.h"

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

class Widget {
protected:
    int16_t x, y, width, height;
    bool focused, dirty;

public:
    Widget(int16_t x, int16_t y, int16_t w, int16_t h)
        : x(x), y(y), width(w), height(h), focused(false), dirty(true) {}

    virtual ~Widget() {}
    virtual void draw(Adafruit_SSD1306& display) = 0;
    virtual void handleInput(uint32_t irCode) = 0;
    void setFocus(bool focus) { focused = focus; dirty = true; }
    bool isDirty() const { return dirty; }
    void clearDirty() { dirty = false; }
};

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

    void handleInput(uint32_t irCode) {
        if (!widgets.empty()) widgets[focusedWidgetIndex]->handleInput(irCode);
    }

    void draw(Adafruit_SSD1306& display) {
        for (auto widget : widgets) {
            if (widget->isDirty()) {
                widget->draw(display);
                widget->clearDirty();
            }
        }
    }

    void navigateUp() {
        changeFocus(-1);
    }

    void navigateDown() {
        changeFocus(1);
    }

private:
    void changeFocus(int direction) {
        if (widgets.empty()) return;

        widgets[focusedWidgetIndex]->setFocus(false);
        focusedWidgetIndex = (focusedWidgetIndex + widgets.size() + direction) % widgets.size();
        widgets[focusedWidgetIndex]->setFocus(true);
    }
};

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
        if (index < screens.size()) {
            currentScreenIndex = index;
            display.clearDisplay();
        }
    }

    IRCommandManager& getIRManager() {
        return irManager;
    }

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

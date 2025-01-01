// ui_components.h (additional components)
#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

// ... (previous declarations) ...

class Label : public Widget {
private:
    String text;
    bool centered;
    
public:
    Label(int16_t x, int16_t y, int16_t w, int16_t h, const String& text, bool centered = false);
    void setText(const String& newText);
    void draw(Adafruit_SSD1306& display) override;
    void handleInput(const IRCommand& cmd) override;
    void update() override;
};

class Button : public Widget {
private:
    String label;
    std::function<void()> callback;
    
public:
    Button(int16_t x, int16_t y, int16_t w, int16_t h, 
           const String& label, std::function<void()> callback);
    void setLabel(const String& newLabel);
    void draw(Adafruit_SSD1306& display) override;
    void handleInput(const IRCommand& cmd) override;
    void update() override;
};

class FloatDisplay : public Widget {
private:
    float value;
    int precision;
    const char* units;
    
public:
    FloatDisplay(int16_t x, int16_t y, int16_t w, int16_t h, 
                int precision, const char* units);
    void setValue(float newValue);
    void draw(Adafruit_SSD1306& display) override;
    void handleInput(const IRCommand& cmd) override;
    void update() override;
};

class BatteryWidget : public Widget {
private:
    float voltage;
    float current;
    float percentage;
    
public:
    BatteryWidget(int16_t x, int16_t y, int16_t w, int16_t h);
    void updateValues(float voltage, float current);
    void draw(Adafruit_SSD1306& display) override;
    void handleInput(const IRCommand& cmd) override;
    void update() override;
};

class GraphWidget : public Widget {
private:
    const HistoryManager& historyManager;
    GraphType graphType;
    float timeScale;
    
public:
    GraphWidget(int16_t x, int16_t y, int16_t w, int16_t h, 
               const HistoryManager& history);
    void setGraphType(GraphType type);
    void adjustTimeScale(float factor);
    void draw(Adafruit_SSD1306& display) override;
    void handleInput(const IRCommand& cmd) override;
    void update() override;
    
private:
    float getValueForPoint(const HistoryPoint& point) const;
    std::pair<float, float> calculateRange() const;
};

#endif // UI_COMPONENTS_H

// ui_components.cpp (implementations)
#include "ui_components.h"

// Label Implementation
Label::Label(int16_t x, int16_t y, int16_t w, int16_t h, 
            const String& text, bool centered)
    : Widget(x, y, w, h), text(text), centered(centered) {}

void Label::setText(const String& newText) {
    if (text != newText) {
        text = newText;
        dirty = true;
    }
}

void Label::draw(Adafruit_SSD1306& display) {
    display.setTextColor(WHITE);
    if (centered) {
        int16_t textWidth = text.length() * 6; // Assuming 6px font
        int16_t textX = x + (width - textWidth) / 2;
        display.setCursor(textX, y);
    } else {
        display.setCursor(x, y);
    }
    display.print(text);
}

void Label::handleInput(const IRCommand& cmd) {
    // Labels don't handle input
}

void Label::update() {
    // Labels don't need regular updates
}

// Button Implementation
Button::Button(int16_t x, int16_t y, int16_t w, int16_t h,
               const String& label, std::function<void()> callback)
    : Widget(x, y, w, h), label(label), callback(callback) {}

void Button::setLabel(const String& newLabel) {
    if (label != newLabel) {
        label = newLabel;
        dirty = true;
    }
}

void Button::draw(Adafruit_SSD1306& display) {
    display.drawRect(x, y, width, height, WHITE);
    
    if (focused) {
        display.fillRect(x + 2, y + 2, width - 4, height - 4, WHITE);
        display.setTextColor(BLACK);
    } else {
        display.setTextColor(WHITE);
    }
    
    int16_t textX = x + (width - label.length() * 6) / 2;
    int16_t textY = y + (height - 8) / 2;
    display.setCursor(textX, textY);
    display.print(label);
}

void Button::handleInput(const IRCommand& cmd) {
    if (focused && cmd.code == IRCodes::OK && callback) {
        callback();
    }
}

void Button::update() {
    // Buttons don't need regular updates
}

// FloatDisplay Implementation
FloatDisplay::FloatDisplay(int16_t x, int16_t y, int16_t w, int16_t h,
                         int precision, const char* units)
    : Widget(x, y, w, h), value(0), precision(precision), units(units) {}

void FloatDisplay::setValue(float newValue) {
    if (value != newValue) {
        value = newValue;
        dirty = true;
    }
}

void FloatDisplay::draw(Adafruit_SSD1306& display) {
    display.setTextColor(WHITE);
    display.setCursor(x, y);
    display.print(value, precision);
    display.print(" ");
    display.print(units);
}

void FloatDisplay::handleInput(const IRCommand& cmd) {
    // FloatDisplays don't handle input
}

void FloatDisplay::update() {
    // FloatDisplays are updated through setValue
}

// BatteryWidget Implementation
BatteryWidget::BatteryWidget(int16_t x, int16_t y, int16_t w, int16_t h)
    : Widget(x, y, w, h), voltage(0), current(0), percentage(0) {}

void BatteryWidget::updateValues(float newVoltage, float newCurrent) {
    voltage = newVoltage;
    current = newCurrent;
    percentage = ((voltage/Config::CELL_COUNT) - Config::CELL_VOLTAGE_MIN) / 
                (Config::CELL_VOLTAGE_MAX - Config::CELL_VOLTAGE_MIN) * 100;
    percentage = constrain(percentage, 0, 100);
    dirty = true;
}

void BatteryWidget::draw(Adafruit_SSD1306& display) {
    // Draw battery outline
    display.drawRect(x, y + 2, width - 10, height - 4, WHITE);
    display.fillRect(x + width - 10, y + height/3, 10, height/3, WHITE);
    
    // Draw fill level
    int fillWidth = ((width - 14) * percentage) / 100;
    display.fillRect(x + 2, y + 4, fillWidth, height - 8, WHITE);
    
    // Draw text
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2fV %.0fmA", voltage, current);
    display.setCursor(x + 2, y + height/2 - 4);
    display.setTextColor(percentage > 50 ? BLACK : WHITE);
    display.print(buffer);
}

void BatteryWidget::handleInput(const IRCommand& cmd) {
    // BatteryWidget doesn't handle input
}

void BatteryWidget::update() {
    // BatteryWidget is updated through updateValues
}

// GraphWidget Implementation
GraphWidget::GraphWidget(int16_t x, int16_t y, int16_t w, int16_t h,
                       const HistoryManager& history)
    : Widget(x, y, w, h), historyManager(history),
      graphType(GraphType::VOLTAGE_GRAPH), timeScale(1.0f) {}

void GraphWidget::setGraphType(GraphType type) {
    if (graphType != type) {
        graphType = type;
        dirty = true;
    }
}

void GraphWidget::adjustTimeScale(float factor) {
    timeScale *= factor;
    timeScale = constrain(timeScale, 0.1f, 10.0f);
    dirty = true;
}

float GraphWidget::getValueForPoint(const HistoryPoint& point) const {
    switch (graphType) {
        case GraphType::VOLTAGE_GRAPH:
            return point.voltage;
        case GraphType::CURRENT_GRAPH:
            return point.current;
        case GraphType::TEMP_DELTA_GRAPH:
            return point.tempDelta;
        default:
            return 0;
    }
}

std::pair<float, float> GraphWidget::calculateRange() const {
    auto history = historyManager.getVisibleHistory();
    if (history.empty()) {
        return {0, 1};
    }
    
    float minVal = getValueForPoint(history[0]);
    float maxVal = minVal;
    
    for (const auto& point : history) {
        float value = getValueForPoint(point);
        minVal = min(minVal, value);
        maxVal = max(maxVal, value);
    }
    
    // Add 10% padding
    float padding = (maxVal - minVal) * 0.1f;
    return {minVal - padding, maxVal + padding};
}

void GraphWidget::draw(Adafruit_SSD1306& display) {
    // Draw axes
    display.drawLine(x, y + height - 1, x + width - 1, y + height - 1, WHITE);
    display.drawLine(x, y, x, y + height - 1, WHITE);
    
    auto history = historyManager.getVisibleHistory();
    if (history.empty()) return;
    
    // Calculate value range
    auto [minVal, maxVal] = calculateRange();
    float valueRange = maxVal - minVal;
    
    // Plot points
    int lastX = -1;
    int lastY = -1;
    
    for (size_t i = 0; i < history.size(); i++) {
        float progress = i / float(history.size() - 1);
        int plotX = x + progress * (width - 1);
        
        float value = getValueForPoint(history[i]);
        int plotY = y + height - 1 - 
                   (height - 1) * (value - minVal) / valueRange;
        plotY = constrain(plotY, y, y + height - 1);
        
        if (lastX != -1) {
            display.drawLine(lastX, lastY, plotX, plotY, WHITE);
        }
        
        lastX = plotX;
        lastY = plotY;
    }
}

void GraphWidget::handleInput(const IRCommand& cmd) {
    if (!focused) return;
    
    if (cmd.code == IRCodes::LEFT) {
        adjustTimeScale(0.8f);
    } else if (cmd.code == IRCodes::RIGHT) {
        adjustTimeScale(1.25f);
    }
}

void GraphWidget::update() {
    // Graph is updated through history updates
}

// screen.h
#ifndef SCREEN_H
#define SCREEN_H

#include <vector>
#include <memory>
#include "ui_components.h"

class Screen {
protected:
    std::vector<std::unique_ptr<Widget>> widgets;
    size_t focusedWidget;

public:
    Screen() : focusedWidget(0) {}
    virtual ~Screen() = default;

    template<typename T, typename... Args>
    T* addWidget(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = widget.get();
        widgets.push_back(std::move(widget));
        if (widgets.size() == 1) {
            ptr->setFocus(true);
        }
        return ptr;
    }

    virtual void handleInput(const IRCommand& cmd);
    virtual void update();
    virtual void draw(Adafruit_SSD1306& display);

protected:
    void changeFocus(int direction);
};

// MainScreen and GraphScreen implementations...
class MainScreen : public Screen {
private:
    Label* titleLabel;
    BatteryWidget* batteryWidget;
    FloatDisplay* tempDisplay;
    FloatDisplay* capacityDisplay;
    Button* chargeButton;
    
public:
    MainScreen();
    void update() override;
};

class GraphScreen : public Screen {
private:
    Label* titleLabel;
    GraphWidget* graphWidget;
    Button* switchButton;
    
public:
    GraphScreen(const HistoryManager& history);
    void update() override;
};

#endif // SCREEN_H

// screen.cpp
#include "screen.h"

void Screen::handleInput(const IRCommand& cmd) {
    if (cmd.code == IRCodes::UP || cmd.code == IRCodes::DOWN) {
        changeFocus(cmd.code == IRCodes::DOWN ? 1 : -1);
    } else if (!widgets.empty()) {
        widgets[focusedWidget]->handleInput(cmd);
    }
}

void Screen::update() {
    for (auto& widget : widgets) {
        widget->update();
    }
}

void Screen::draw(Adafruit_SSD1306& display) {
    for (auto& widget : widgets) {
        if (widget->isDirty()) {
            widget->draw(display);
            widget->clearDirty();
        }
    }
}

void Screen::changeFocus(int direction) {
    if (widgets.empty()) return;
    
    widgets[focusedWidget]->setFocus(false);
    
    if (direction > 0) {
        focusedWidget = (focusedWidget + 1) % widgets.size();
    } else {
        focusedWidget = (focusedWidget + widgets.size() - 1) % widgets.size();
    }
    
    widgets[focusedWidget]->setFocus(true);
}

// MainScreen Implementation
MainScreen::MainScreen() {
    titleLabel = addWidget<Label>(0, 0, 128, 10, "NiMH Charger", true);
    batteryWidget = addWidget<BatteryWidget>(0, 12, 128, 20);
    tempDisplay = addWidget<FloatDisplay>(0, 34, 128, 10, 1, "°C");
    capacityDisplay = addWidget<FloatDisplay>(0, 44, 128, 10, 0, "mAh");
    chargeButton = addWidget<Button>(14, 54, 100, 10, "Start Charging",
        []() { chargerController.startCharging(); });
}

void MainScreen::update() {
    auto sensorData = SensorManager::readSensors();
    
    batteryWidget->updateValues(sensorData.voltage, sensorData.current);
    tempDisplay->setValue(sensorData.temperature);
    capacityDisplay->setValue(chargerController.getCapacity());
    
    // Update charge button label based on state
    chargeButton->setLabel(chargerController.isCharging() ? 
                          "Stop Charging" : "Start</antArtifact>

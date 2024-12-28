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
class FloatDisplay;
class FunctionPlotter;

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

// Float Display widget
class FloatDisplay : public Widget {
private:
    float value;
    uint8_t precision;
    char format[16];
    char label[32];
    bool showLabel;
    
public:
    FloatDisplay(int16_t x, int16_t y, int16_t w, int16_t h, 
                uint8_t precision = 2, const char* label = nullptr)
        : Widget(x, y, w, h), value(0.0f), precision(precision) {
        snprintf(format, sizeof(format), "%%.%df", precision);
        if (label) {
            strncpy(this->label, label, sizeof(this->label) - 1);
            this->label[sizeof(this->label) - 1] = '\0';
            showLabel = true;
        } else {
            showLabel = false;
        }
    }
    
    void setValue(float newValue) {
        if (abs(newValue - value) > pow(10, -precision)) {
            value = newValue;
            dirty = true;
        }
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.setTextColor(WHITE);
        
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), format, value);
        
        if (showLabel) {
            display.setCursor(x, y);
            display.print(label);
            display.print(": ");
            int16_t valueWidth = strlen(valueStr) * 6;
            display.setCursor(x + width - valueWidth, y);
        } else {
            int16_t valueWidth = strlen(valueStr) * 6;
            display.setCursor(x + (width - valueWidth) / 2, y + (height - 8) / 2);
        }
        
        display.print(valueStr);
        
        if (focused) {
            display.drawRect(x, y, width, height, WHITE);
        }
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// Function Plotter widget
class FunctionPlotter : public Widget {
public:
    typedef std::function<float(float)> PlotFunction;
    
private:
    PlotFunction function;
    float xMin, xMax, yMin, yMax;
    uint16_t numPoints;
    bool autoScale;
    static const uint16_t MAX_POINTS = 128;
    
    struct Point {
        float x, y;
    };
    Point points[MAX_POINTS];
    
public:
    FunctionPlotter(int16_t x, int16_t y, int16_t w, int16_t h,
                   PlotFunction func, float xMin = -5.0f, float xMax = 5.0f)
        : Widget(x, y, w, h), function(func),
          xMin(xMin), xMax(xMax), yMin(0), yMax(0),
          numPoints(min(w, (int16_t)MAX_POINTS)),
          autoScale(true) {
        calculatePoints();
    }
    
    void setFunction( PlotFunction func) {
        function = func;
        calculatePoints();
        dirty = true;
    }
    
    void setXRange(float min, float max) {
        xMin = min;
        xMax = max;
        calculatePoints();
        dirty = true;
    }
    
    void setYRange(float min, float max) {
        yMin = min;
        yMax = max;
        autoScale = false;
        calculatePoints();
        dirty = true;
    }
    
    void enableAutoScale(bool enable = true) {
        autoScale = enable;
        calculatePoints();
        dirty = true;
    }
    
private:
    void calculatePoints() {
        if (!function) return;
        
        float xStep = (xMax - xMin) / (numPoints - 1);
        
        if (autoScale) {
            yMin = INFINITY;
            yMax = -INFINITY;
            
            for (uint16_t i = 0; i < numPoints; i++) {
                float x = xMin + i * xStep;
                float y = function(x);
                
                yMin = min(yMin, y);
                yMax = max(yMax, y);
            }
            
            float yMargin = (yMax - yMin) * 0.1f;
            yMin -= yMargin;
            yMax += yMargin;
        }
        
        for (uint16_t i = 0; i < numPoints; i++) {
            points[i].x = xMin + i * xStep;
            points[i].y = function(points[i].x);
        }
    }
    
    int16_t mapToPixelX(float x) const {
        return x + (width - 1) * (x - xMin) / (xMax - xMin);
    }
    
    int16_t mapToPixelY(float y) const {
        return y + height - 1 - (height - 1) * (y - yMin) / (yMax - yMin);
    }
    
public:
    void draw(Adafruit_SSD1306& display) override {
        display.drawRect(x, y, width, height, WHITE);
        
        if (yMin <= 0 && yMax >= 0) {
            int16_t yAxis = mapToPixelY(0);
            display.drawFastHLine(x, y + yAxis, width, WHITE);
        }
        
        if (xMin <= 0 && xMax >= 0) {
            int16_t xAxis = mapToPixelX(0);
            display.drawFastVLine(x + xAxis, y, height, WHITE);
        }
        
        for (uint16_t i = 1; i < numPoints; i++) {
            int16_t x1 = mapToPixelX(points[i-1].x);
            int16_t y1 = mapToPixelY(points[i-1].y);
            int16_t x2 = mapToPixelX(points[i].x);
            int16_t y2 = mapToPixelY(points[i].y);
            
            display.drawLine(x + x1, y + y1, x + x2, y + y2, WHITE);
        }
        
        if (focused) {
            for (int16_t i = 0; i < 4; i++) {
                display.drawPixel(x + i, y + i, WHITE);
                display.drawPixel(x + width - 1 - i, y + i, WHITE);
                display.drawPixel(x + i, y + height - 1 - i, WHITE);
                display.drawPixel(x + width - 1 - i, y + height - 1 - i, WHITE);
            }
        }
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
    
    void setScreen(Screen* screen) {
        if (currentScreen) {
            delete currentScreen;
        }
        currentScreen = screen;
        display.clear ```cpp
Display();
    }
    
    void update() {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
            return;
        }
        lastUpdateTime = currentTime;
        
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

#include <Wire.h>

// Pin definitions
#define VOLTAGE_PIN 36    // ADC pin for voltage measurement
#define CURRENT_PIN 39    // ADC pin for current measurement
#define TEMP_PIN 34       // ADC pin for temperature measurement
#define PWM_PIN 25        // PWM output for charge control

// Battery parameters
#define CELL_COUNT 4
#define CHARGE_CURRENT_MA 1000
#define TRICKLE_CURRENT_MA 50
#define CELL_VOLTAGE_MAX 1.45
#define CELL_VOLTAGE_MIN 1.0
#define CAPACITY_MAH 2000

// Charger states
enum ChargerState {
    IDLE,
    CHARGING,
    TRICKLE,
    COMPLETE,
    ERROR
};

// Global variables
UIManager ui;
Screen* mainScreen;
ChargerState chargerState = IDLE;
float batteryVoltage = 0.0f;
float chargeCurrent = 0.0f;
float temperature = 0.0f;
float capacityMah = 0.0f;
unsigned long chargeStartTime = 0;
bool charging = false;

// Voltage measurement history for -dV detection
const int VOLTAGE_HISTORY_SIZE = 60;  // 1 minute history at 1s intervals
float voltageHistory[VOLTAGE_HISTORY_SIZE];
int voltageHistoryIndex = 0;

// Forward declarations
void startCharging();
void stopCharging();
float calculateCapacity();
const char* getStateString();
float getBatteryTemperature();

class BatteryWidget : public Widget {
private:
    float cellVoltages[CELL_COUNT];
    float totalVoltage;
    float current;
    float percentage;
    
public:
    BatteryWidget(int16_t x, int16_t y, int16_t w, int16_t h)
        : Widget(x, y, w, h), totalVoltage(0), current(0), percentage(0) {
        for(int i = 0; i < CELL_COUNT; i++) {
            cellVoltages[i] = 0;
        }
    }
    
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

// Initialize hardware and UI
void setup() {
    Serial.begin(115200);
    
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    ledcSetup(0, 20000, 8);  // 20kHz PWM, 8-bit resolution
    ledcAttachPin(PWM_PIN, 0);
    
    if (!ui.begin()) {
        Serial.println("Failed to initialize UI");
        return;
    }
    
    mainScreen = new Screen();
    
    mainScreen->addWidget(new Label(0, 0, 128, 10, "NiMH Charger", true));
    mainScreen->addWidget(new BatteryWidget(0, 12, 128, 20));
    mainScreen ```cpp
->addWidget(new FloatDisplay(0, 34, 128, 10, 1, "Temp C"));
    mainScreen->addWidget(new FloatDisplay(0, 44, 128, 10, 0, "mAh"));
    mainScreen->addWidget(new Button(14, 54, 100, 10, "Start Charging", startCharging));
    
    ui.setScreen(mainScreen);
    
    for(int i = 0; i < VOLTAGE_HISTORY_SIZE; i++) {
        voltageHistory[i] = 0;
    }
}

void startCharging() {
    if(!charging) {
        charging = true;
        chargerState = CHARGING;
        chargeStartTime = millis();
        capacityMah = 0;
        voltageHistoryIndex = 0;
        ((Button*)mainScreen->getWidget(4))->setLabel("Stop Charging");
    } else {
        stopCharging();
    }
}

void stopCharging() {
    charging = false;
    chargerState = IDLE;
    ledcWrite(0, 0);
    ((Button*)mainScreen->getWidget(4))->setLabel("Start Charging");
}

float readBatteryVoltage() {
    const float ADC_VOLTAGE_DIVIDER = 2.0f;  // Adjust based on your voltage divider
    const float ADC_REFERENCE = 3.3f;
    float adcValue = analogRead(VOLTAGE_PIN);
    return (adcValue / 4095.0f) * ADC_REFERENCE * ADC_VOLTAGE_DIVIDER;
}

float readChargeCurrent() {
    const float CURRENT_SENSE_FACTOR = 0.1f;  // Adjust based on your current sense resistor
    float adcValue = analogRead(CURRENT_PIN);
    return ((adcValue / 4095.0f) * 3.3f) / CURRENT_SENSE_FACTOR;
}

void updateCharging() {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    if(currentTime - lastUpdate >= 100) {
        lastUpdate = currentTime;
        
        batteryVoltage = readBatteryVoltage();
        chargeCurrent = readChargeCurrent();
        temperature = getBatteryTemperature();
        
        static unsigned long lastVoltageUpdate = 0;
        if(currentTime - lastVoltageUpdate >= 1000) {
            lastVoltageUpdate = currentTime;
            voltageHistory[voltageHistoryIndex] = batteryVoltage;
            voltageHistoryIndex = (voltageHistoryIndex + 1) % VOLTAGE_HISTORY_SIZE;
        }
        
        switch(chargerState) {
            case CHARGING:
                if(temperature > 45.0f) {
                    chargerState = ERROR;
                    stopCharging();
                } else if(detectMinusAV()) {
                    chargerState = TRICKLE;
                    setPWMDutyCycle(TRICKLE_CURRENT_MA);
                } else {
                    setPWMDutyCycle(CHARGE_CURRENT_MA);
                }
                break;
                
            case TRICKLE:
                if(capacityMah >= CAPACITY_MAH) {
                    chargerState = COMPLETE;
                    stopCharging();
                }
                break;
                
            case ERROR:
                stopCharging();
                break;
                
            default:
                break;
        }
        
        if(charging) {
            capacityMah += (chargeCurrent * 0.1f) / 3600.0f;  // Add mAh for 100ms period
        }
        
        ((BatteryWidget*)mainScreen->getWidget(1))->updateValues(batteryVoltage, chargeCurrent);
        ((FloatDisplay*)mainScreen->getWidget(2))->setValue(temperature);
        ((FloatDisplay*)mainScreen->getWidget(3))->setValue(capacityMah);
        ((Label*)mainScreen->getWidget(0))->setText(getStateString());
    }
}

float getBatteryTemperature() {
    const float BETA = 3950.0f;
    const float R25 = 10000.0f;
    const float R_SERIES = 10000.0f;
    
    float adcValue = analogRead(TEMP_PIN);
    float resistance = R_SERIES * ((4095.0f / adcValue) - 1.0f);
    float steinhart = log(resistance / R25) / BETA;
    steinhart += 1.0f / (25.0f + 273.15f);
    return (1.0f / steinhart) - 273.15f;
}

const char* getStateString() {
    switch(chargerState) {
        case IDLE: return "Ready to Charge";
        case CHARGING: return "Fast Charging";
        case TRICKLE: return "Trickle Charging";
        case COMPLETE: return "Charge Complete";
        case ERROR: ```cpp
            return "Error - Overtemp";
        default: return "Unknown State";
    }
}

void loop() {
    updateCharging();
    ui.update();
}

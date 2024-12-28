/*
 * Advanced NiMH Battery Charger
 * Features:
 * - Closed loop current control with soft start
 * - Temperature monitoring (battery and ambient)
 * - Multiple charging termination methods (-dV, dT, maximum time)
 * - OLED display with multiple screens
 * - Real-time graphing
 * - IR remote control
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRremote.h>
#include <vector>
#include <functional>

// Pin definitions
#define VOLTAGE_PIN 36        // ADC pin for voltage measurement
#define CURRENT_PIN 39        // ADC pin for current measurement
#define TEMP_PIN 34          // ADC pin for battery temperature
#define AMBIENT_TEMP_PIN 35  // ADC pin for ambient temperature
#define PWM_PIN 25           // PWM output for charge control
#define IR_RECEIVE_PIN 15    // IR receiver input

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Battery parameters
#define CELL_COUNT 4
#define CHARGE_CURRENT_MA 1000
#define TRICKLE_CURRENT_MA 50
#define CELL_VOLTAGE_MAX 1.45
#define CELL_VOLTAGE_MIN 1.0
#define CAPACITY_MAH 2000

// Current control parameters
#define RAMP_TIME_MS 30000
#define MIN_CHARGE_CURRENT_MA 50
#define CONTROL_INTERVAL_MS 10
#define PWM_FREQUENCY 20000

// Temperature parameters
#define DT_THRESHOLD 2.0f
#define MAX_DT_RATE 1.0f
#define MAX_TEMP 45.0f

// IR Remote codes
#define IR_RED 0xF720DF
#define IR_GREEN 0xA720DF
#define IR_BLUE 0x6720DF

// Forward declarations of major classes
class Screen;
class Widget;
class UIManager;
class CurrentController;

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
    CURRENT_CONTROL_GRAPH,
    TEMP_DELTA_GRAPH
};

// Input event structure
struct InputEvent {
    enum Type {
        NONE,
        IR_BUTTON,
        TIMER
    } type;
    uint32_t value;
};

// Global variables
UIManager* ui;
Screen* mainScreen;
ChargerState chargerState = IDLE;
ScreenType currentScreenType = MAIN_SCREEN;
GraphType currentGraph = VOLTAGE_GRAPH;

float batteryVoltage = 0.0f;
float chargeCurrent = 0.0f;
float temperature = 0.0f;
float ambientTemperature = 0.0f;
float capacityMah = 0.0f;

bool charging = false;
unsigned long chargeStartTime = 0;

// History tracking
const int GRAPH_HISTORY_SIZE = 128;
const int VOLTAGE_HISTORY_SIZE = 60;
float voltageHistory[VOLTAGE_HISTORY_SIZE];
int voltageHistoryIndex = 0;

struct HistoryPoint {
    float voltage;
    float current;
    float tempDelta;
    float currentSetpoint;
    unsigned long timestamp;
};
HistoryPoint history[GRAPH_HISTORY_SIZE];
int historyIndex = 0;
unsigned long lastHistoryUpdate = 0;

// PID Controller Parameters
struct PIDParameters {
    float Kp = 0.5f;
    float Ki = 0.8f;
    float Kd = 0.01f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;
    float integralMin = -50.0f;
    float integralMax = 50.0f;
};

// Current Controller Class
class CurrentController {
private:
    PIDParameters params;
    float targetCurrent;
    float currentSetpoint;
    float lastError;
    float integral;
    unsigned long lastUpdateTime;
    unsigned long rampStartTime;
    bool ramping;
    
public:
    CurrentController() : targetCurrent(0), currentSetpoint(0), 
                         lastError(0), integral(0), lastUpdateTime(0),
                         rampStartTime(0), ramping(false) {}
    
    void start(float targetCurrentMA) {
        targetCurrent = targetCurrentMA;
        currentSetpoint = MIN_CHARGE_CURRENT_MA;
        lastError = 0;
        integral = 0;
        rampStartTime = millis();
        ramping = true;
        lastUpdateTime = millis();
    }
    
    void stop() {
        targetCurrent = 0;
        currentSetpoint = 0;
        lastError = 0;
        integral = 0;
        ramping = false;
    }
    
    float update(float measuredCurrent) {
        unsigned long currentTime = millis();
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f;
        
        if(deltaTime < 0.001f) return -1;
        
        if(ramping) {
            unsigned long rampElapsed = currentTime - rampStartTime;
            if(rampElapsed >= RAMP_TIME_MS) {
                currentSetpoint = targetCurrent;
                ramping = false;
            } else {
                float rampProgress = (float)rampElapsed / RAMP_TIME_MS;
                currentSetpoint = MIN_CHARGE_CURRENT_MA + 
                    (targetCurrent - MIN_CHARGE_CURRENT_MA) * rampProgress;
            }
        }
        
        float error = currentSetpoint - measuredCurrent;
        
        float proportional = error * params.Kp;
        integral += error * deltaTime * params.Ki;
        integral = constrain(integral, params.integralMin, params.integralMax);
        float derivative = ((error - lastError) / deltaTime) * params.Kd;
        
        float output = proportional + integral + derivative;
        output = constrain(output, params.outputMin, params.outputMax);
        
        lastError = error;
        lastUpdateTime = currentTime;
        
        return output;
    }
    
    float getCurrentSetpoint() const { return currentSetpoint; }
    bool isRamping() const { return ramping; }
    void setPIDParameters(const PIDParameters& newParams) { params = newParams; }
};

// Global current controller instance
CurrentController currentController;

// Base Widget Class
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

// Battery Widget Class
class BatteryWidget : public Widget {
private:
    float voltage;
    float current;
    float percentage;
    
public:
    BatteryWidget(int16_t x, int16_t y, int16_t w, int16_t h)
        : Widget(x, y, w, h), voltage(0), current(0), percentage(0) {}
    
    void updateValues(float v, float c) {
        voltage = v;
        current = c;
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
        snprintf(buffer, sizeof(buffer), "%.2fV %.0fmA", voltage, current);
        display.setCursor(x + 2, y + height/2 - 4);
        display.setTextColor(percentage > 50 ? BLACK : WHITE);
        display.print(buffer);
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// Label Widget Class
class Label : public Widget {
private:
    String text;
    bool centered;
    
public:
    Label(int16_t x, int16_t y, int16_t w, int16_t h, const String& text, bool centered = false)
        : Widget(x, y, w, h), text(text), centered(centered) {}
    
    void setText(const String& newText) {
        if(text != newText) {
            text = newText;
            dirty = true;
        }
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.setTextColor(WHITE);
        if(centered) {
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

// Float Display Widget Class
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
        if(label) {
            strncpy(this->label, label, sizeof(this->label) - 1);
            this->label[sizeof(this->label) - 1] = '\0';
            showLabel = true;
        } else {
            showLabel = false;
        }
    }
    
    void setValue(float newValue) {
        if(abs(newValue - value) > pow(10, -precision)) {
            value = newValue;
            dirty = true;
        }
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.setTextColor(WHITE);
        
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), format, value);
        
        if(showLabel) {
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
        
        if(focused) {
            display.drawRect(x, y, width, height, WHITE);
        }
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// Button Widget Class
class Button : public Widget {
private:
    const char* label;
    std::function<void()> callback;
    
public:
    Button(int16_t x, int16_t y, int16_t w, int16_t h, 
           const char* label, std::function<void()> callback)
        : Widget(x, y, w, h), label(label), callback(callback) {}
    
    void setLabel(const char* newLabel) {
        label = newLabel;
        dirty = true;
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.drawRect(x, y, width, height, WHITE);
        if(focused) {
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
        if(event.type == InputEvent::IR_BUTTON && 
           event.value == 0xFF02FD && callback) {
            callback();
        }
    }
    
    void update() override {}
};

// Function Plotter Widget Class
class FunctionPlotter : public Widget {
public:
    typedef std::function<float(float)> PlotFunction;
    
private:
    PlotFunction function;
    float xMin, xMax, yMin, yMax;
    bool autoScale;
    
public:
    FunctionPlotter(int16_t x, int16_t y, int16_t w, int16_t h,
                   PlotFunction func, float xMin = -1.0f, float xMax = 1.0f)
        : Widget(x, y, w, h), function(func),
          xMin(xMin), xMax(xMax), yMin(0), yMax(0),
          autoScale(true) {}
    
    void setFunction(PlotFunction func) {
        function = func;
        dirty = true;
    }
    
    void setXRange(float min, float max) {
        xMin = min;
        xMax = max;
        dirty = true;
    }
    
    void setYRange(float min, float max) {
        yMin = min;
        yMax = max;
        autoScale = false;
        dirty = true;
    }
    
    void enableAutoScale(bool enable = true) {
        autoScale = enable;
        dirty = true;
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.drawRect(x, y, width, height, WHITE);
        
        if(!function)</antArtifact>

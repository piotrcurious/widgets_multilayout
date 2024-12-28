/*
 * Advanced NiMH Battery Charger
 * Features:
 * - PID-based current control with soft start
 * - Delta-T and -dV/dt termination
 * - Dual temperature monitoring
 * - Multi-screen OLED interface
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
#define VOLTAGE_PIN 36        // Battery voltage ADC
#define CURRENT_PIN 39        // Charge current ADC
#define TEMP_PIN 34          // Battery temperature ADC
#define AMBIENT_TEMP_PIN 35  // Ambient temperature ADC
#define PWM_PIN 25           // Charge current control PWM
#define IR_RECEIVE_PIN 15    // IR remote control input

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Battery parameters
#define CELL_COUNT 4
#define CHARGE_CURRENT_MA 1000
#define TRICKLE_CURRENT_MA 50
#define CELL_VOLTAGE_MAX 1.45f
#define CELL_VOLTAGE_MIN 1.0f
#define CAPACITY_MAH 2000

// Current control parameters
#define RAMP_TIME_MS 30000          // 30 second ramp-up
#define MIN_CHARGE_CURRENT_MA 50     // Initial current
#define CONTROL_INTERVAL_MS 10       // 100Hz control loop
#define PWM_FREQUENCY 20000         // 20kHz PWM

// Temperature parameters
#define DT_THRESHOLD 2.0f           // Max temperature rise (°C)
#define MAX_DT_RATE 1.0f           // Max rise rate (°C/min)
#define MAX_TEMP 45.0f             // Absolute max temp

// IR Remote codes
#define IR_RED 0xF720DF    // Main screen
#define IR_GREEN 0xA720DF  // Graph screen
#define IR_BLUE 0x6720DF   // Switch graph type

// Forward declarations
class Screen;
class Widget;
class UIManager;
class CurrentController;

// Enums
enum ChargerState {
    IDLE,
    CHARGING,
    TRICKLE,
    COMPLETE,
    ERROR
};

enum ScreenType {
    MAIN_SCREEN,
    GRAPH_SCREEN
};

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

// PID controller parameters
struct PIDParameters {
    float Kp = 0.5f;
    float Ki = 0.8f;
    float Kd = 0.01f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;
    float integralMin = -50.0f;
    float integralMax = 50.0f;
};

// History point structure
struct HistoryPoint {
    float voltage;
    float current;
    float tempDelta;
    float currentSetpoint;
    unsigned long timestamp;
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

HistoryPoint history[GRAPH_HISTORY_SIZE];
int historyIndex = 0;
unsigned long lastHistoryUpdate = 0;

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
        
        // Update setpoint during ramp-up
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
        
        // PID calculations
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

// Screen Class
class Screen {
protected:
    std::vector<Widget*> widgets;
    size_t focusedWidgetIndex;
    
public:
    Screen() : focusedWidgetIndex(0) {}
    
    virtual ~Screen() {
        for(auto widget : widgets) {
            delete widget;
        }
    }
    
    void addWidget(Widget* widget) {
        widgets.push_back(widget);
        if(widgets.size() == 1) {
            widget->setFocus(true);
        }
    }
    
    virtual void handleInput(const InputEvent& event) {
        if(event.type == InputEvent::IR_BUTTON) {
            if(event.value == 0xFF629D) { // UP
                changeFocus(-1);
            } else if(event.value == 0xFFA857) { // DOWN
                changeFocus(1);
            } else if(focusedWidgetIndex < widgets.size()) {
                widgets[focusedWidgetIndex]->handleInput(event);
            }
        }
    }
    
    virtual void update() {
        for(auto widget : widgets) {
            widget->update();
        }
    }
    
    virtual void draw(Adafruit_SSD1306& display) {
        for(auto widget : widgets) {
            if(widget->isDirty()) {
                widget->draw(display);
                widget->clearDirty();
            }
        }
    }
    
    Widget* getWidget(size_t index) {
        return index < widgets.size() ? widgets[index] : nullptr;
    }
    
protected:
    void changeFocus(int direction) {
        if(widgets.empty()) return;
        
        widgets[focusedWidgetIndex]->setFocus(false);
        
        if(direction > 0) {
            focusedWidgetIndex = (focusedWidgetIndex + 1) % widgets.size();
        } else {
            focusedWidgetIndex = (focusedWidgetIndex + widgets.size() - 1) % widgets.size();
        }
        
        widgets[focusedWidgetIndex]->setFocus(true);
    }
};

// UI Manager Class
class UIManager {
private:
    Adafruit_SSD1306 display;
    IRrecv irReceiver;
    std::vector<Screen*> screens;
    Screen* currentScreen;
    unsigned long lastUpdateTime;
    static const unsigned long UPDATE_INTERVAL = 50;
    
public:
    UIManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
                 irReceiver(IR_RECEIVE_PIN),
                 currentScreen(nullptr),
                 lastUpdateTime(0) {
        screens.resize(2, nullptr);  // Space for main and graph screens
    }
    
    bool begin() {
        if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
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
        if(screens[type]) {
            delete screens[type];
        }
        screens[type] = screen;
    }
    
    void setScreen(ScreenType type) {
        currentScreen = screens[type];
        display.clearDisplay();
    }
    
    Screen* getScreen(ScreenType type) {
        return screens[type];
    }
    
    void update() {
        unsigned long currentTime = millis();
        if(currentTime - lastUpdateTime < UPDATE_INTERVAL) {
            return;
        }
        lastUpdateTime = currentTime;
        
        if(!currentScreen) return;
        
        InputEvent event{InputEvent::NONE, 0};
        if(irReceiver.decode()) {
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

// Sensor reading functions
float readBatteryVoltage() {
    const float VOLTAGE_DIVIDER = 11.0f;  // Voltage divider ratio
    const float ADC_REF = 3.3f;
    const float ADC_STEPS = 4095.0f;
    
    float adcValue = analogRead(VOLTAGE_PIN);
    return (adcValue / ADC_STEPS) * ADC_REF * VOLTAGE_DIVIDER;
}

float readChargeCurrent() {
    const float CURRENT_SENSE_RATIO = 1.0f;  // mA per ADC step
    return analogRead(CURRENT_PIN) * CURRENT_SENSE_RATIO;
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

float getAmbientTemperature() {
    const float BETA = 3950.0f;
    const float R25 = 10000.0f;
    const float R_SERIES = 10000.0f;
    
    float adcValue = analogRead(AMBIENT_TEMP_PIN);
    float resistance = R_SERIES * ((4095.0f / adcValue) - 1.0f);
    float steinhart = log(resistance / R25) / BETA;
    steinhart += 1.0f / (25.0f + 273.15f);
    return (1.0f / steinhart) - 273.15f;
}

// Charging control functions
bool detectMinusAV() {
    if(voltageHistoryIndex < VOLTAGE_HISTORY_SIZE) {
        return false;
    }
    
    float maxVoltage = voltageHistory[0];
    for(int i = 1; i < VOLTAGE_HISTORY_SIZE; i++) {
        if(voltageHistory[i] > maxVoltage) {
            maxVoltage = voltageHistory[i];
        }
    }
    
    return (maxVoltage - batteryVoltage) > (0.01f * CELL_COUNT);  // 10mV per cell
}

bool checkTemperatureTermination() {
    static float lastTempDelta = 0;
    static unsigned long lastTempCheck = 0;
    
    unsigned long currentTime = millis();
    if(currentTime - lastTempCheck >= 60000) {  // Check every minute
        float currentTempDelta = temperature - ambientTemperature;
        float tempRise = currentTempDelta - lastTempDelta;
        
        lastTempDelta = currentTempDelta;
        lastTempCheck = currentTime;
        
        return (currentTempDelta ></antArtifact>

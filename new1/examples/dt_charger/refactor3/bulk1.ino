// sensor_manager.cpp
#include "sensor_manager.h"
#include "config.h"
#include <Arduino.h>

SensorManager::SensorData SensorManager::readSensors() {
    SensorData data;
    data.voltage = readBatteryVoltage();
    data.current = readChargeCurrent();
    data.temperature = readTemperature(Config::TEMP_PIN);
    data.ambientTemperature = readTemperature(Config::AMBIENT_TEMP_PIN);
    return data;
}

float SensorManager::readBatteryVoltage() {
    constexpr float ADC_VOLTAGE_DIVIDER = 2.0f;
    constexpr float ADC_REFERENCE = 3.3f;
    float adcValue = analogRead(Config::VOLTAGE_PIN);
    return (adcValue / 4095.0f) * ADC_REFERENCE * ADC_VOLTAGE_DIVIDER;
}

float SensorManager::readChargeCurrent() {
    constexpr float CURRENT_SENSE_FACTOR = 0.1f;
    float adcValue = analogRead(Config::CURRENT_PIN);
    return ((adcValue / 4095.0f) * 3.3f) / CURRENT_SENSE_FACTOR;
}

float SensorManager::readTemperature(int pin) {
    constexpr float BETA = 3950.0f;
    constexpr float R25 = 10000.0f;
    constexpr float R_SERIES = 10000.0f;
    
    float adcValue = analogRead(pin);
    float resistance = R_SERIES * ((4095.0f / adcValue) - 1.0f);
    float steinhart = log(resistance / R25) / BETA;
    steinhart += 1.0f / (25.0f + 273.15f);
    return (1.0f / steinhart) - 273.15f;
}

// history_manager.cpp
#include "history_manager.h"

void HistoryManager::addPoint(const SensorManager::SensorData& data) {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastUpdate >= Config::HISTORY_UPDATE_INTERVAL) {
        lastUpdate = currentTime;
        
        // Update main history
        history[historyIndex] = {
            data.voltage,
            data.current,
            data.temperature - data.ambientTemperature,
            currentTime
        };
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
        
        // Update voltage history for -dV detection
        voltageHistory[voltageHistoryIndex] = data.voltage;
        voltageHistoryIndex = (voltageHistoryIndex + 1) % VOLTAGE_HISTORY_SIZE;
    }
}

bool HistoryManager::detectMinusAV() const {
    if (voltageHistoryIndex < VOLTAGE_HISTORY_SIZE) return false;
    
    float maxVoltage = voltageHistory[0];
    float currentVoltage = voltageHistory[voltageHistoryIndex - 1];
    
    for (int i = 1; i < VOLTAGE_HISTORY_SIZE; i++) {
        if (voltageHistory[i] > maxVoltage) {
            maxVoltage = voltageHistory[i];
        }
    }
    
    return (maxVoltage - currentVoltage) > (0.005f * Config::CELL_COUNT);
}

std::vector<HistoryPoint> HistoryManager::getVisibleHistory() const {
    std::vector<HistoryPoint> visible;
    int start = (historyIndex + 1) % HISTORY_SIZE;
    
    for (int i = 0; i < HISTORY_SIZE; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        if (history[idx].timestamp > 0) {
            visible.push_back(history[idx]);
        }
    }
    
    return visible;
}

// charger_controller.cpp
#include "charger_controller.h"

void ChargerController::begin() {
    state = ChargerState::IDLE;
    charging = false;
    capacityMah = 0.0f;
    ledcWrite(0, 0);
}

void ChargerController::update() {
    if (!charging) return;
    
    auto sensorData = SensorManager::readSensors();
    
    switch (state) {
        case ChargerState::CHARGING:
            if (sensorData.temperature > Config::MAX_TEMP || 
                checkTemperatureTermination(sensorData)) {
                state = ChargerState::ERROR;
                stopCharging();
            } else if (historyManager.detectMinusAV()) {
                state = ChargerState::TRICKLE;
                setPWMDutyCycle(Config::TRICKLE_CURRENT_MA);
            } else {
                setPWMDutyCycle(Config::CHARGE_CURRENT_MA);
            }
            break;
            
        case ChargerState::TRICKLE:
            if (capacityMah >= Config::CAPACITY_MAH) {
                state = ChargerState::COMPLETE;
                stopCharging();
            }
            break;
            
        case ChargerState::ERROR:
        case ChargerState::COMPLETE:
            stopCharging();
            break;
            
        default:
            break;
    }
    
    updateCapacity(sensorData.current);
}

void ChargerController::startCharging() {
    if (!charging) {
        charging = true;
        state = ChargerState::CHARGING;
        chargeStartTime = millis();
        capacityMah = 0.0f;
    }
}

void ChargerController::stopCharging() {
    charging = false;
    ledcWrite(0, 0);
}

void ChargerController::setPWMDutyCycle(float targetCurrentMA) {
    constexpr float Kp = 0.1f;
    auto sensorData = SensorManager::readSensors();
    float error = targetCurrentMA - sensorData.current;
    int pwm = constrain(ledcRead(0) + Kp * error, 0, 255);
    ledcWrite(0, pwm);
}

bool ChargerController::checkTemperatureTermination(const SensorManager::SensorData& data) {
    static float lastTempDelta = 0;
    static unsigned long lastTempCheck = 0;
    
    unsigned long currentTime = millis();
    if (currentTime - lastTempCheck >= 60000) {
        float currentTempDelta = data.temperature - data.ambientTemperature;
        float tempRise = currentTempDelta - lastTempDelta;
        
        lastTempDelta = currentTempDelta;
        lastTempCheck = currentTime;
        
        return (currentTempDelta > Config::DT_THRESHOLD || 
                tempRise > Config::MAX_DT_RATE);
    }
    return false;
}

void ChargerController::updateCapacity(float current) {
    capacityMah += (current * 0.1f) / 3600.0f;
}

const char* ChargerController::getStateString() const {
    switch (state) {
        case ChargerState::IDLE: return "Ready to Charge";
        case ChargerState::CHARGING: return "Fast Charging";
        case ChargerState::TRICKLE: return "Trickle Charging";
        case ChargerState::COMPLETE: return "Charge Complete";
        case ChargerState::ERROR: return "Error - Overtemp";
        default: return "Unknown State";
    }
}

// ir_manager.cpp
#include "ir_manager.h"

void IRManager::begin(int pin) {
    receiver = IRrecv(pin);
    receiver.enableIRIn();
}

void IRManager::update() {
    if (receiver.decode()) {
        uint32_t code = receiver.decodedIRData.decodedRawData;
        bool repeat = receiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
        
        if (code == 0xFFFFFFFF) {
            if (lastCode != 0 && 
                millis() - lastCommandTime >= REPEAT_DELAY) {
                code = lastCode;
            } else {
                receiver.resume();
                return;
            }
        }
        
        for (const auto& cmd : commands) {
            if (cmd.code == code) {
                if (cmd.handler) {
                    cmd.handler();
                }
                lastCode = code;
                lastCommandTime = millis();
                break;
            }
        }
        
        receiver.resume();
    }
}

void IRManager::addCommand(uint32_t code, std::function<void()> handler, const char* description) {
    commands.push_back({code, handler, description});
}

const std::vector<IRCommand>& IRManager::getCommands() const {
    return commands;
}

// ui_components.cpp
#include "ui_components.h"

Widget::Widget(int16_t x, int16_t y, int16_t w, int16_t h)
    : x(x), y(y), width(w), height(h), focused(false), dirty(true) {}

void Widget::setFocus(bool focus) {
    if (focused != focus) {
        focused = focus;
        dirty = true;
    }
}

bool Widget::isFocused() const {
    return focused;
}

bool Widget::isDirty() const {
    return dirty;
}

void Widget::clearDirty() {
    dirty = false;
}

void Widget::markDirty() {
    dirty = true;
}

// ui_manager.cpp
#include "ui_manager.h"

class UIManager {
private:
    Adafruit_SSD1306 display;
    IRManager irManager;
    std::vector<std::unique_ptr<Screen>> screens;
    ScreenType currentScreen;
    unsigned long lastUpdate;

public:
    UIManager() 
        : display(Config::SCREEN_WIDTH, Config::SCREEN_HEIGHT, &Wire, Config::OLED_RESET),
          currentScreen(ScreenType::MAIN_SCREEN),
          lastUpdate(0) {}

    bool begin() {
        if (!display.begin(SSD1306_SWITCHCAPVCC, Config::SCREEN_ADDRESS)) {
            return false;
        }

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.display();

        irManager.begin(Config::IR_RECEIVE_PIN);
        setupScreens();
        setupCommands();

        return true;
    }

    void update() {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdate < Config::UI_UPDATE_INTERVAL) {
            return;
        }
        lastUpdate = currentTime;

        irManager.update();
        
        if (auto* screen = getCurrentScreen()) {
            screen->update();
            display.clearDisplay();
            screen->draw(display);
            display.display();
        }
    }

    void setScreen(ScreenType type) {
        if (type < screens.size() && screens[type]) {
            currentScreen = type;
            display.clearDisplay();
        }
    }

private:
    Screen* getCurrentScreen() {
        return currentScreen < screens.size() ? screens[currentScreen].get() : nullptr;
    }

    void setupScreens() {
        screens.resize(2);  // Main and Graph screens
        screens[ScreenType::MAIN_SCREEN] = std::make_unique<MainScreen>();
        screens[ScreenType::GRAPH_SCREEN] = std::make_unique<GraphScreen>();
    }

    void setupCommands() {
        irManager.addCommand(IRCodes::RED, 
            [this]() { setScreen(ScreenType::MAIN_SCREEN); },
            "Switch to Main Screen");
            
        irManager.addCommand(IRCodes::GREEN,
            [this]() { setScreen(ScreenType::GRAPH_SCREEN); },
            "Switch to Graph Screen");
            
        // Add other common commands...
    }
};

// Add the missing Screen implementations if needed
class MainScreen : public Screen {
    // Implementation of the main screen...
};

class GraphScreen : public Screen {
    // Implementation of the graph screen...
};

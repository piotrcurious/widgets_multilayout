// config.h
#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
    // Display settings
    constexpr int SCREEN_WIDTH = 128;
    constexpr int SCREEN_HEIGHT = 64;
    constexpr int OLED_RESET = -1;
    constexpr int SCREEN_ADDRESS = 0x3C;

    // Pin definitions
    constexpr int IR_RECEIVE_PIN = 15;
    constexpr int VOLTAGE_PIN = 36;
    constexpr int CURRENT_PIN = 39;
    constexpr int TEMP_PIN = 34;
    constexpr int AMBIENT_TEMP_PIN = 35;
    constexpr int PWM_PIN = 25;

    // Battery parameters
    constexpr int CELL_COUNT = 4;
    constexpr float CHARGE_CURRENT_MA = 1000.0f;
    constexpr float TRICKLE_CURRENT_MA = 50.0f;
    constexpr float CELL_VOLTAGE_MAX = 1.45f;
    constexpr float CELL_VOLTAGE_MIN = 1.0f;
    constexpr float CAPACITY_MAH = 2000.0f;

    // Temperature parameters
    constexpr float DT_THRESHOLD = 2.0f;
    constexpr float MAX_DT_RATE = 1.0f;
    constexpr float MAX_TEMP = 45.0f;

    // Update intervals
    constexpr unsigned long UI_UPDATE_INTERVAL = 50;
    constexpr unsigned long SENSOR_UPDATE_INTERVAL = 100;
    constexpr unsigned long HISTORY_UPDATE_INTERVAL = 1000;
}

#endif // CONFIG_H

// sensor_manager.h
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

class SensorManager {
public:
    struct SensorData {
        float voltage;
        float current;
        float temperature;
        float ambientTemperature;
    };

    static SensorData readSensors();

private:
    static float readBatteryVoltage();
    static float readChargeCurrent();
    static float readTemperature(int pin);
};

#endif // SENSOR_MANAGER_H

// history_manager.h
#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <vector>

struct HistoryPoint {
    float voltage;
    float current;
    float tempDelta;
    unsigned long timestamp;
};

class HistoryManager {
public:
    static constexpr int HISTORY_SIZE = 128;
    static constexpr int VOLTAGE_HISTORY_SIZE = 60;

    void addPoint(const SensorManager::SensorData& data);
    bool detectMinusAV() const;
    std::vector<HistoryPoint> getVisibleHistory() const;
    
private:
    HistoryPoint history[HISTORY_SIZE];
    float voltageHistory[VOLTAGE_HISTORY_SIZE];
    int historyIndex = 0;
    int voltageHistoryIndex = 0;
};

#endif // HISTORY_MANAGER_H

// charger_controller.h
#ifndef CHARGER_CONTROLLER_H
#define CHARGER_CONTROLLER_H

enum class ChargerState {
    IDLE,
    CHARGING,
    TRICKLE,
    COMPLETE,
    ERROR
};

class ChargerController {
public:
    void begin();
    void update();
    void startCharging();
    void stopCharging();
    
    const char* getStateString() const;
    float getCapacity() const { return capacityMah; }
    ChargerState getState() const { return state; }
    bool isCharging() const { return charging; }

private:
    ChargerState state = ChargerState::IDLE;
    float capacityMah = 0.0f;
    bool charging = false;
    unsigned long chargeStartTime = 0;

    void setPWMDutyCycle(float targetCurrentMA);
    bool checkTemperatureTermination(const SensorManager::SensorData& data);
    void updateCapacity(float current);
};

#endif // CHARGER_CONTROLLER_H

// ui_components.h
#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <functional>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

class Widget {
protected:
    int16_t x, y, width, height;
    bool focused;
    bool dirty;

public:
    Widget(int16_t x, int16_t y, int16_t w, int16_t h);
    virtual ~Widget() = default;
    
    virtual void draw(Adafruit_SSD1306& display) = 0;
    virtual void handleInput(const IRCommand& cmd) = 0;
    virtual void update() = 0;
    
    void setFocus(bool focus);
    bool isFocused() const;
    bool isDirty() const;
    void clearDirty();
    void markDirty();
};

// [Other UI component declarations...]

#endif // UI_COMPONENTS_H

// ir_manager.h
#ifndef IR_MANAGER_H
#define IR_MANAGER_H

#include <vector>
#include <functional>
#include "IRremote.h"

struct IRCommand {
    uint32_t code;
    std::function<void()> handler;
    const char* description;
};

class IRManager {
public:
    void begin(int pin);
    void update();
    void addCommand(uint32_t code, std::function<void()> handler, const char* description);
    const std::vector<IRCommand>& getCommands() const;

private:
    IRrecv receiver;
    std::vector<IRCommand> commands;
    uint32_t lastCode = 0;
    unsigned long lastCommandTime = 0;
    
    static constexpr unsigned long REPEAT_DELAY = 250;
};

#endif // IR_MANAGER_H

// main.cpp
#include "config.h"
#include "sensor_manager.h"
#include "history_manager.h"
#include "charger_controller.h"
#include "ui_manager.h"

SensorManager sensorManager;
HistoryManager historyManager;
ChargerController chargerController;
UIManager uiManager;

void setup() {
    Serial.begin(115200);
    
    // Initialize hardware
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    ledcSetup(0, 20000, 8);
    ledcAttachPin(Config::PWM_PIN, 0);
    
    chargerController.begin();
    uiManager.begin();
}

void loop() {
    static unsigned long lastSensorUpdate = 0;
    unsigned long currentTime = millis();

    // Update sensors
    if (currentTime - lastSensorUpdate >= Config::SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = currentTime;
        
        auto sensorData = SensorManager::readSensors();
        historyManager.addPoint(sensorData);
        
        chargerController.update();
    }
    
    // Update UI
    uiManager.update();
}

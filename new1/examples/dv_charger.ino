#include "UI_Framework.h"
#include <Wire.h>

// Pin definitions
#define VOLTAGE_PIN 36    // ADC pin for voltage measurement
#define CURRENT_PIN 39    // ADC pin for current measurement
#define TEMP_PIN 34      // ADC pin for temperature measurement
#define PWM_PIN 25       // PWM output for charge control

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
        // Calculate rough percentage based on voltage
        percentage = ((voltage/CELL_COUNT) - CELL_VOLTAGE_MIN) / 
                    (CELL_VOLTAGE_MAX - CELL_VOLTAGE_MIN) * 100;
        percentage = constrain(percentage, 0, 100);
        dirty = true;
    }
    
    void draw(Adafruit_SSD1306& display) override {
        // Draw battery outline
        display.drawRect(x, y + 2, width - 10, height - 4, WHITE);
        display.fillRect(x + width - 10, y + height/3, 10, height/3, WHITE);
        
        // Draw fill level
        int fillWidth = ((width - 14) * percentage) / 100;
        display.fillRect(x + 2, y + 4, fillWidth, height - 8, WHITE);
        
        // Draw voltage and current
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
    
    // Initialize ADC and PWM
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    ledcSetup(0, 20000, 8);  // 20kHz PWM, 8-bit resolution
    ledcAttachPin(PWM_PIN, 0);
    
    // Initialize UI
    if (!ui.begin()) {
        Serial.println("Failed to initialize UI");
        return;
    }
    
    mainScreen = new Screen();
    
    // Add status label at top
    mainScreen->addWidget(new Label(0, 0, 128, 10, "NiMH Charger", true));
    
    // Add battery visualization widget
    mainScreen->addWidget(new BatteryWidget(0, 12, 128, 20));
    
    // Add temperature display
    mainScreen->addWidget(new FloatDisplay(0, 34, 128, 10, 1, "Temp C"));
    
    // Add capacity display
    mainScreen->addWidget(new FloatDisplay(0, 44, 128, 10, 0, "mAh"));
    
    // Add Start/Stop button
    mainScreen->addWidget(new Button(14, 54, 100, 10, "Start Charging", startCharging));
    
    ui.setScreen(mainScreen);
    
    // Initialize voltage history
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
    
    // Update measurements every 100ms
    if(currentTime - lastUpdate >= 100) {
        lastUpdate = currentTime;
        
        // Read sensors
        batteryVoltage = readBatteryVoltage();
        chargeCurrent = readChargeCurrent();
        temperature = getBatteryTemperature();
        
        // Update voltage history every second
        static unsigned long lastVoltageUpdate = 0;
        if(currentTime - lastVoltageUpdate >= 1000) {
            lastVoltageUpdate = currentTime;
            voltageHistory[voltageHistoryIndex] = batteryVoltage;
            voltageHistoryIndex = (voltageHistoryIndex + 1) % VOLTAGE_HISTORY_SIZE;
        }
        
        // State machine
        switch(chargerState) {
            case CHARGING:
                // Check for termination conditions
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
        
        // Update capacity
        if(charging) {
            capacityMah += (chargeCurrent * 0.1f) / 3600.0f;  // Add mAh for 100ms period
        }
        
        // Update UI widgets
        ((BatteryWidget*)mainScreen->getWidget(1))->updateValues(batteryVoltage, chargeCurrent);
        ((FloatDisplay*)mainScreen->getWidget(2))->setValue(temperature);
        ((FloatDisplay*)mainScreen->getWidget(3))->setValue(capacityMah);
        ((Label*)mainScreen->getWidget(0))->setText(getStateString());
    }
}

void setPWMDutyCycle(float targetCurrentMA) {
    // Simple P controller for current
    const float Kp = 0.1f;
    float error = targetCurrentMA - chargeCurrent;
    int pwm = constrain(ledcRead(0) + Kp * error, 0, 255);
    ledcWrite(0, pwm);
}

bool detectMinusAV() {
    if(voltageHistoryIndex < VOLTAGE_HISTORY_SIZE) return false;
    
    float maxVoltage = voltageHistory[0];
    float currentVoltage = batteryVoltage;
    
    for(int i = 1; i < VOLTAGE_HISTORY_SIZE; i++) {
        if(voltageHistory[i] > maxVoltage) {
            maxVoltage = voltageHistory[i];
        }
    }
    
    // Detect if voltage has dropped by more than 5mV per cell
    return (maxVoltage - currentVoltage) > (0.005f * CELL_COUNT);
}

float getBatteryTemperature() {
    // Using NTC thermistor
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
        case ERROR: return "Error - Overtemp";
        default: return "Unknown State";
    }
}

void loop() {
    updateCharging();
    ui.update();
}

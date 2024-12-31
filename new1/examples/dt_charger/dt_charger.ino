
// Main implementation file
#include "UI_Framework.h"

// Global variables
UIManager ui;
Screen* mainScreen;
ChargerState chargerState = IDLE;
float batteryVoltage = 0.0f;
float chargeCurrent = 0.0f;
float temperature = 0.0f;
float ambientTemperature = 0.0f;
float capacityMah = 0.0f;
unsigned long chargeStartTime = 0;
bool charging = false;
GraphType currentGraph = VOLTAGE_GRAPH;

// History buffers for graphs
const int GRAPH_HISTORY_SIZE = 128;
HistoryPoint history[GRAPH_HISTORY_SIZE];
int historyIndex = 0;
unsigned long lastHistoryUpdate = 0;

// Voltage measurement history for -dV detection
const int VOLTAGE_HISTORY_SIZE = 60;
float voltageHistory[VOLTAGE_HISTORY_SIZE];
int voltageHistoryIndex = 0;

// Helper functions
float readBatteryVoltage() {
    const float ADC_VOLTAGE_DIVIDER = 2.0f;
    const float ADC_REFERENCE = 3.3f;
    float adcValue = analogRead(VOLTAGE_PIN);
    return (adcValue / 4095.0f) * ADC_REFERENCE * ADC_VOLTAGE_DIVIDER;
}

float readChargeCurrent() {
    const float CURRENT_SENSE_FACTOR = 0.1f;
    float adcValue = analogRead(CURRENT_PIN);
    return ((adcValue / 4095.0f) * 3.3f) / CURRENT_SENSE_FACTOR;
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

void setPWMDutyCycle(float targetCurrentMA) {
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
    
    return (maxVoltage - currentVoltage) > (0.005f * CELL_COUNT);
}

bool checkTemperatureTermination() {
    static float lastTempDelta = 0;
    static unsigned long lastTempCheck = 0;
    
    unsigned long currentTime = millis();
    if(currentTime - lastTempCheck >= 60000) {
        float currentTempDelta = temperature - ambientTemperature;
        float tempRise = currentTempDelta - lastTempDelta;
        
        lastTempDelta = currentTempDelta;
        lastTempCheck = currentTime;
        
        return (currentTempDelta > DT_THRESHOLD || tempRise > MAX_DT_RATE);
    }
    return false;
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

void updateHistory() {
    unsigned long currentTime = millis();
    if(currentTime - lastHistoryUpdate >= 1000) {
        lastHistoryUpdate = currentTime;
        
        history[historyIndex] = {
            batteryVoltage,
            chargeCurrent,
            temperature - ambientTemperature,
            currentTime
        };
        
        historyIndex = (historyIndex + 1) % GRAPH_HISTORY_SIZE;
        
        // Update voltage history for -dV detection
        voltageHistory[voltageHistoryIndex] = batteryVoltage;
        voltageHistoryIndex = (voltageHistoryIndex + 1) % VOLTAGE_HISTORY_SIZE;
    }
}

// GraphScreen implementation
std::vector<HistoryPoint> GraphScreen::getVisibleHistory() {
    std::vector<HistoryPoint> visible;
    int start = (historyIndex + 1) % GRAPH_HISTORY_SIZE;
    for(int i = 0; i < GRAPH_HISTORY_SIZE; i++) {
        int idx = (start + i) % GRAPH_HISTORY_SIZE;
        if(history[idx].timestamp > 0) {
            visible.push_back(history[idx]);
        }
    }
    return visible;
}

float GraphScreen::graphFunction(float x) {
    auto visible = getVisibleHistory();
    if(visible.empty()) return 0;
    
    int index = (int)((x + 1) * visible.size() / 2);
    index = constrain(index, 0, visible.size() - 1);
    
    switch(currentGraph) {
        case VOLTAGE_GRAPH:
            return visible[index].voltage;
        case CURRENT_GRAPH:
            return visible[index].current;
        case TEMP_DELTA_GRAPH:
            return visible[index].tempDelta;
        default:
            return 0;
    }
}

GraphScreen::GraphScreen() {
    graphLabel = new Label(0, 0, 128, 10, "Voltage Graph", true);
    addWidget(graphLabel);
    
    plotter = new FunctionPlotter(0, 10, 128, 44,
        [this](float x) { return this->graphFunction(x); },
        -1, 1);
    addWidget(plotter);
    
    switchButton = new Button(14, 54, 100, 10, "Switch Graph", 
        []() {
            currentGraph = (GraphType)((currentGraph + 1) % 3);
            // Label update will happen in update()
        });
    addWidget(switchButton);
}

void GraphScreen::update() {
    Screen::update();
    
    // Update graph label
    switch(currentGraph) {
        case VOLTAGE_GRAPH:
            graphLabel->setText("Voltage Graph");
            plotter->setYRange(0, CELL_COUNT * CELL_VOLTAGE_MAX * 1.1f);
            break;
        case CURRENT_GRAPH:
            graphLabel->setText("Current Graph");
            plotter->setYRange(0, CHARGE_CURRENT_MA * 1.2f);
            break;
        case TEMP_DELTA_GRAPH:
            graphLabel->setText("Temp Delta Graph");
            plotter->setYRange(-5, 15);
            break;
    }
}

// Charging control functions
void startCharging() {
    if(!charging) {
        charging = true;
        chargerState = CHARGING;
        chargeStartTime = millis();
        capacityMah = 0;
        voltageHistoryIndex = 0;
        historyIndex = 0;
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

void updateCharging() {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    if(currentTime - lastUpdate >= 100) {
        lastUpdate = currentTime;
        
        // Read all sensors
        batteryVoltage = readBatteryVoltage();
        chargeCurrent = readChargeCurrent();
        temperature = getBatteryTemperature();
        ambientTemperature = getAmbientTemperature();
        
        updateHistory();
        
        // State machine
        if(charging) {
            switch(chargerState) {
                case CHARGING:
                    if(temperature > MAX_TEMP || checkTemperatureTermination()) {
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
                case COMPLETE:
                    stopCharging();
                    break;
                    
                default:
                    break;
            }
            
            // Update capacity
            capacityMah += (chargeCurrent * 0.1f) / 3600.0f;
        }
        
        // Update UI for current screen
        auto* currentScreen = ui.getScreen(ui.getCurrentScreenType());
        if(currentScreen) {
            currentScreen->update();
        }
    }
}

void handleIRInput(const InputEvent& event) {
    if(event.type == InputEvent::IR_BUTTON) {
        switch(event.value) {
            case IR_RED:
                ui.setScreen(MAIN_SCREEN);
                break;
            case IR_GREEN:
                ui.setScreen(GRAPH_SCREEN);
                break;
            case IR_BLUE:
                if(ui.getCurrentScreenType() == GRAPH_SCREEN) {
                    currentGraph = (GraphType)((currentGraph + 1) % 3);
                }
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize ADC and PWM
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    ledcSetup(0, 20000, 8);
    ledcAttachPin(PWM_PIN, 0);
    
    if (!ui.begin()) {
        Serial.println("Failed to initialize UI");
        return;
    }
    
    // Create main screen
    mainScreen = new Screen();
    mainScreen->addWidget(new Label(0, 0, 128, 10, "NiMH Charger", true));
    mainScreen->addWidget(new BatteryWidget(0, 12, 128, 20));
    mainScreen->addWidget(new FloatDisplay(0, 34, 128, 10, 1, "Temp C"));
    mainScreen->addWidget(new FloatDisplay(0, 44, 128, 10, 0, "mAh"));
    mainScreen->addWidget(new Button(14, 54, 100, 10, "Start Charging", startCharging));
    
    // Create graph screen
    auto* graphScreen = new GraphScreen();
    
    // Initialize screens in UI manager
    ui.addScreen(MAIN_SCREEN, mainScreen);
    ui.addScreen(GRAPH_SCREEN, graphScreen);
    ui.setScreen(MAIN_SCREEN);
}

void loop() {
    updateCharging();
    ui.update();
    
    if(irReceiver.decode()) {
        InputEvent event{InputEvent::IR_BUTTON, irReceiver.decodedIRData.decodedRawData};
        handleIRInput(event);
        irReceiver.resume();
    }
}

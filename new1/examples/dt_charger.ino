// ... [Previous includes and basic definitions remain the same] ...

// Additional pin definition
#define AMBIENT_TEMP_PIN 35  // ADC pin for ambient temperature sensor

// Temperature parameters
#define DT_THRESHOLD 2.0f    // Temperature rise threshold in °C
#define MAX_DT_RATE 1.0f     // Max temperature rise rate in °C/minute

// IR Remote codes for RGB remote
#define IR_RED 0xF720DF
#define IR_GREEN 0xA720DF
#define IR_BLUE 0x6720DF

// Screen identifiers
enum ScreenType {
    MAIN_SCREEN,
    GRAPH_SCREEN
};

// Global variables (adding to previous ones)
float ambientTemperature = 0.0f;
ScreenType currentScreenType = MAIN_SCREEN;

// History buffers for graphs
const int GRAPH_HISTORY_SIZE = 128;  // Match screen width
struct HistoryPoint {
    float voltage;
    float current;
    float tempDelta;
    unsigned long timestamp;
};
HistoryPoint history[GRAPH_HISTORY_SIZE];
int historyIndex = 0;
unsigned long lastHistoryUpdate = 0;

// Graph selection enum
enum GraphType {
    VOLTAGE_GRAPH,
    CURRENT_GRAPH,
    TEMP_DELTA_GRAPH
};
GraphType currentGraph = VOLTAGE_GRAPH;

class GraphScreen : public Screen {
private:
    FunctionPlotter* plotter;
    Label* graphLabel;
    Button* switchButton;
    std::vector<HistoryPoint> getVisibleHistory() {
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
    
    float graphFunction(float x) {
        auto visible = getVisibleHistory();
        if(visible.empty()) return 0;
        
        // Map x from -1 to 1 to array index
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
    
public:
    GraphScreen() {
        // Add title label
        graphLabel = new Label(0, 0, 128, 10, "Voltage Graph", true);
        addWidget(graphLabel);
        
        // Add graph plotter
        plotter = new FunctionPlotter(0, 10, 128, 44,
            [this](float x) { return this->graphFunction(x); },
            -1, 1);
        addWidget(plotter);
        
        // Add switch graph button
        switchButton = new Button(14, 54, 100, 10, "Switch Graph", 
            []() {
                currentGraph = (GraphType)((currentGraph + 1) % 3);
                updateGraphLabel();
            });
        addWidget(switchButton);
    }
    
    void update() override {
        Screen::update();
        updateGraphLabel();
        plotter->enableAutoScale(true);
        
        // Update Y range based on graph type
        switch(currentGraph) {
            case VOLTAGE_GRAPH:
                plotter->setYRange(0, CELL_COUNT * CELL_VOLTAGE_MAX * 1.1f);
                break;
            case CURRENT_GRAPH:
                plotter->setYRange(0, CHARGE_CURRENT_MA * 1.2f);
                break;
            case TEMP_DELTA_GRAPH:
                plotter->setYRange(-5, 15);
                break;
        }
    }
};

// Update history buffer
void updateHistory() {
    unsigned long currentTime = millis();
    if(currentTime - lastHistoryUpdate >= 1000) {  // Update every second
        lastHistoryUpdate = currentTime;
        
        history[historyIndex] = {
            batteryVoltage,
            chargeCurrent,
            temperature - ambientTemperature,
            currentTime
        };
        
        historyIndex = (historyIndex + 1) % GRAPH_HISTORY_SIZE;
    }
}

void updateGraphLabel() {
    auto* graphScreen = (GraphScreen*)ui.getScreen(GRAPH_SCREEN);
    if(!graphScreen) return;
    
    Label* label = (Label*)graphScreen->getWidget(0);
    if(!label) return;
    
    switch(currentGraph) {
        case VOLTAGE_GRAPH:
            label->setText("Voltage Graph");
            break;
        case CURRENT_GRAPH:
            label->setText("Current Graph");
            break;
        case TEMP_DELTA_GRAPH:
            label->setText("Temp Delta Graph");
            break;
    }
}

float getAmbientTemperature() {
    // Similar to getBatteryTemperature but for ambient sensor
    const float BETA = 3950.0f;
    const float R25 = 10000.0f;
    const float R_SERIES = 10000.0f;
    
    float adcValue = analogRead(AMBIENT_TEMP_PIN);
    float resistance = R_SERIES * ((4095.0f / adcValue) - 1.0f);
    float steinhart = log(resistance / R25) / BETA;
    steinhart += 1.0f / (25.0f + 273.15f);
    return (1.0f / steinhart) - 273.15f;
}

// Enhance the temperature termination check
bool checkTemperatureTermination() {
    static float lastTempDelta = 0;
    static unsigned long lastTempCheck = 0;
    
    unsigned long currentTime = millis();
    if(currentTime - lastTempCheck >= 60000) {  // Check every minute
        float currentTempDelta = temperature - ambientTemperature;
        float tempRise = currentTempDelta - lastTempDelta;
        
        lastTempDelta = currentTempDelta;
        lastTempCheck = currentTime;
        
        // Check both absolute delta-T and rate of rise
        if(currentTempDelta > DT_THRESHOLD || tempRise > MAX_DT_RATE) {
            return true;
        }
    }
    return false;
}

// Modify the state machine in updateCharging()
void updateCharging() {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    if(currentTime - lastUpdate >= 100) {
        lastUpdate = currentTime;
        
        // Read sensors including ambient temperature
        batteryVoltage = readBatteryVoltage();
        chargeCurrent = readChargeCurrent();
        temperature = getBatteryTemperature();
        ambientTemperature = getAmbientTemperature();
        
        updateHistory();
        
        switch(chargerState) {
            case CHARGING:
                if(temperature > 45.0f || checkTemperatureTermination()) {
                    chargerState = ERROR;
                    stopCharging();
                } else if(detectMinusAV()) {
                    chargerState = TRICKLE;
                    setPWMDutyCycle(TRICKLE_CURRENT_MA);
                } else {
                    setPWMDutyCycle(CHARGE_CURRENT_MA);
                }
                break;
                
            // ... [Rest of the state machine remains the same] ...
        }
        
        // Update UI widgets for current screen
        if(currentScreenType == MAIN_SCREEN) {
            ((BatteryWidget*)mainScreen->getWidget(1))->updateValues(batteryVoltage, chargeCurrent);
            ((FloatDisplay*)mainScreen->getWidget(2))->setValue(temperature);
            ((FloatDisplay*)mainScreen->getWidget(3))->setValue(capacityMah);
            ((Label*)mainScreen->getWidget(0))->setText(getStateString());
        }
    }
}

// Setup modification
void setup() {
    // ... [Previous setup code remains the same until UI initialization] ...
    
    // Create both screens
    mainScreen = new Screen();
    auto* graphScreen = new GraphScreen();
    
    // Add widgets to main screen as before
    // ... [Previous widget additions remain the same] ...
    
    // Initialize both screens in UI manager
    ui.addScreen(MAIN_SCREEN, mainScreen);
    ui.addScreen(GRAPH_SCREEN, graphScreen);
    ui.setScreen(MAIN_SCREEN);
}

// Handle IR remote screen switching
void handleIRInput(const InputEvent& event) {
    if(event.type == InputEvent::IR_BUTTON) {
        switch(event.value) {
            case IR_RED:
                currentScreenType = MAIN_SCREEN;
                ui.setScreen(MAIN_SCREEN);
                break;
            case IR_GREEN:
                currentScreenType = GRAPH_SCREEN;
                ui.setScreen(GRAPH_SCREEN);
                break;
            case IR_BLUE:
                if(currentScreenType == GRAPH_SCREEN) {
                    currentGraph = (GraphType)((currentGraph + 1) % 3);
                    updateGraphLabel();
                }
                break;
        }
    }
}

void loop() {
    updateCharging();
    ui.update();
    
    // Handle IR input for screen switching
    if(irReceiver.decode()) {
        InputEvent event{InputEvent::IR_BUTTON, irReceiver.decodedIRData.decodedRawData};
        handleIRInput(event);
        irReceiver.resume();
    }
}

// ... [Previous includes remain the same] ...

// Current control parameters
#define RAMP_TIME_MS 30000          // 30 second ramp-up time
#define MIN_CHARGE_CURRENT_MA 50     // Starting current
#define CONTROL_INTERVAL_MS 10       // 100Hz control loop
#define PWM_FREQUENCY 20000         // 20kHz PWM

// PID controller parameters
struct PIDParameters {
    float Kp = 0.5f;              // Proportional gain
    float Ki = 0.8f;              // Integral gain
    float Kd = 0.01f;             // Derivative gain
    float outputMin = 0.0f;       // Minimum PWM duty cycle (0-255)
    float outputMax = 255.0f;     // Maximum PWM duty cycle (0-255)
    float integralMin = -50.0f;   // Anti-windup limits
    float integralMax = 50.0f;
};

// Current controller class
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
        float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
        
        if(deltaTime < 0.001f) return -1; // Too soon to update
        
        // Update setpoint based on ramp
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
        
        // Calculate error
        float error = currentSetpoint - measuredCurrent;
        
        // Calculate PID terms
        float proportional = error * params.Kp;
        
        integral += error * deltaTime * params.Ki;
        integral = constrain(integral, params.integralMin, params.integralMax);
        
        float derivative = ((error - lastError) / deltaTime) * params.Kd;
        
        // Calculate output
        float output = proportional + integral + derivative;
        output = constrain(output, params.outputMin, params.outputMax);
        
        // Update state
        lastError = error;
        lastUpdateTime = currentTime;
        
        return output;
    }
    
    float getCurrentSetpoint() const {
        return currentSetpoint;
    }
    
    bool isRamping() const {
        return ramping;
    }
    
    // Allow PID parameter tuning
    void setPIDParameters(const PIDParameters& newParams) {
        params = newParams;
    }
};

// Global current controller instance
CurrentController currentController;

// Add current control information to the history
struct HistoryPoint {
    float voltage;
    float current;
    float tempDelta;
    float currentSetpoint;    // Added setpoint tracking
    unsigned long timestamp;
};

// Modify the updateCharging function for new current control
void updateCharging() {
    static unsigned long lastUpdate = 0;
    unsigned long currentTime = millis();
    
    // Fast current control loop
    if(currentTime - lastUpdate >= CONTROL_INTERVAL_MS) {
        lastUpdate = currentTime;
        
        // Read sensors
        batteryVoltage = readBatteryVoltage();
        chargeCurrent = readChargeCurrent();
        temperature = getBatteryTemperature();
        ambientTemperature = getAmbientTemperature();
        
        // Current control when charging
        if(charging) {
            float pwmDuty = currentController.update(chargeCurrent);
            if(pwmDuty >= 0) {
                ledcWrite(0, (int)pwmDuty);
            }
        }
        
        // Update history at a slower rate
        updateHistory();
        
        // State machine
        switch(chargerState) {
            case CHARGING:
                if(temperature > 45.0f || checkTemperatureTermination()) {
                    chargerState = ERROR;
                    stopCharging();
                } else if(detectMinusAV()) {
                    chargerState = TRICKLE;
                    currentController.start(TRICKLE_CURRENT_MA);
                }
                // Current control is handled above
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
            capacityMah += (chargeCurrent * CONTROL_INTERVAL_MS) / 3600000.0f;
        }
        
        // Update UI based on current screen
        if(currentScreenType == MAIN_SCREEN) {
            updateMainScreenWidgets();
        }
    }
}

// Modify the startCharging function
void startCharging() {
    if(!charging) {
        charging = true;
        chargerState = CHARGING;
        chargeStartTime = millis();
        capacityMah = 0;
        voltageHistoryIndex = 0;
        
        // Initialize current controller with ramp-up
        currentController.start(CHARGE_CURRENT_MA);
        
        // Update button label
        ((Button*)mainScreen->getWidget(4))->setLabel("Stop Charging");
    } else {
        stopCharging();
    }
}

// Modify the stopCharging function
void stopCharging() {
    charging = false;
    chargerState = IDLE;
    currentController.stop();
    ledcWrite(0, 0);
    ((Button*)mainScreen->getWidget(4))->setLabel("Start Charging");
}

// Modify the setup function for PWM initialization
void setup() {
    // ... [Previous setup code remains the same until PWM setup] ...
    
    // Initialize PWM with higher resolution
    ledcSetup(0, PWM_FREQUENCY, 8);  // 20kHz PWM, 8-bit resolution
    ledcAttachPin(PWM_PIN, 0);
    ledcWrite(0, 0);  // Start with PWM off
    
    // ... [Rest of setup code remains the same] ...
}

// Add current setpoint to graph options
enum GraphType {
    VOLTAGE_GRAPH,
    CURRENT_GRAPH,
    CURRENT_CONTROL_GRAPH,  // New graph type showing actual vs setpoint
    TEMP_DELTA_GRAPH
};

// Modify the GraphScreen class to show current control
class GraphScreen : public Screen {
    // ... [Previous GraphScreen code remains the same until graphFunction] ...
    
    float graphFunction(float x) {
        auto visible = getVisibleHistory();
        if(visible.empty()) return 0;
        
        int index = (int)((x + 1) * visible.size() / 2);
        index = constrain(index, 0, visible.size() - 1);
        
        switch(currentGraph) {
            case VOLTAGE_GRAPH:
                return visible[index].voltage;
            case CURRENT_GRAPH:
                return visible[index].current;
            case CURRENT_CONTROL_GRAPH:
                // Show both actual and setpoint
                if(x >= 0) {
                    return visible[index].current;
                } else {
                    return visible[index].currentSetpoint;
                }
            case TEMP_DELTA_GRAPH:
                return visible[index].tempDelta;
            default:
                return 0;
        }
    }
    
    // ... [Rest of GraphScreen implementation remains the same] ...
};

// Update the history recording to include current setpoint
void updateHistory() {
    unsigned long currentTime = millis();
    if(currentTime - lastHistoryUpdate >= 1000) {
        lastHistoryUpdate = currentTime;
        
        history[historyIndex] = {
            batteryVoltage,
            chargeCurrent,
            temperature - ambientTemperature,
            currentController.getCurrentSetpoint(),
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
        case CURRENT_CONTROL_GRAPH:
            label->setText("Current Control");
            break;
        case TEMP_DELTA_GRAPH:
            label->setText("Temp Delta Graph");
            break;
    }
}

// Add these definitions to the top of UI_Framework.h
// IR Remote Command Codes
struct IRCommand {
    uint32_t code;
    const char* name;
};

// Common IR remote codes for different remote types
namespace IRCodes {
    // Navigation
    constexpr uint32_t UP = 0xFF629D;
    constexpr uint32_t DOWN = 0xFFA857;
    constexpr uint32_t LEFT = 0xFF22DD;
    constexpr uint32_t RIGHT = 0xFFC23D;
    constexpr uint32_t OK = 0xFF02FD;
    
    // Colors (for screen switching)
    constexpr uint32_t RED = 0xF720DF;
    constexpr uint32_t GREEN = 0xA720DF;
    constexpr uint32_t BLUE = 0x6720DF;
    
    // Numbers
    constexpr uint32_t NUM_0 = 0xFF4AB5;
    constexpr uint32_t NUM_1 = 0xFF6897;
    constexpr uint32_t NUM_2 = 0xFF9867;
    constexpr uint32_t NUM_3 = 0xFFB04F;
    constexpr uint32_t NUM_4 = 0xFF30CF;
    constexpr uint32_t NUM_5 = 0xFF18E7;
    constexpr uint32_t NUM_6 = 0xFF7A85;
    constexpr uint32_t NUM_7 = 0xFF10EF;
    constexpr uint32_t NUM_8 = 0xFF38C7;
    constexpr uint32_t NUM_9 = 0xFF5AA5;
    
    // Function buttons
    constexpr uint32_t FUNC_1 = 0xFF22DD;
    constexpr uint32_t FUNC_2 = 0xFF02FD;
    constexpr uint32_t FUNC_3 = 0xFFC23D;
}

// Command handler type
using CommandHandler = std::function<void(void)>;

// Command mapping structure
struct CommandMapping {
    uint32_t code;
    CommandHandler handler;
    const char* description;
};

// IR Command Manager class
class IRCommandManager {
private:
    std::vector<CommandMapping> commandMappings;
    uint32_t lastCode = 0;
    unsigned long lastCommandTime = 0;
    static constexpr unsigned long REPEAT_DELAY = 250; // ms between repeated commands
    
public:
    void addCommand(uint32_t code, CommandHandler handler, const char* description) {
        commandMappings.push_back({code, handler, description});
    }
    
    void clearCommands() {
        commandMappings.clear();
    }
    
    bool handleCommand(uint32_t code, bool repeat = false) {
        unsigned long currentTime = millis();
        
        // Handle repeat codes
        if (code == 0xFFFFFFFF) {
            if (lastCode != 0 && 
                currentTime - lastCommandTime >= REPEAT_DELAY) {
                code = lastCode;
                repeat = true;
            } else {
                return false;
            }
        }
        
        // Look for matching command
        for (const auto& mapping : commandMappings) {
            if (mapping.code == code) {
                if (mapping.handler) {
                    mapping.handler();
                }
                lastCode = code;
                lastCommandTime = currentTime;
                return true;
            }
        }
        
        return false;
    }
    
    void printCommands() {
        Serial.println("Available IR Commands:");
        for (const auto& mapping : commandMappings) {
            Serial.printf("Code: 0x%06X - %s\n", 
                         mapping.code, mapping.description);
        }
    }
};

// Modify the UIManager class to include IR command management
class UIManager {
private:
    // ... (previous private members) ...
    IRCommandManager irManager;
    
    void setupDefaultCommands() {
        // Screen navigation
        irManager.addCommand(IRCodes::RED, 
            [this]() { setScreen(MAIN_SCREEN); },
            "Switch to Main Screen");
            
        irManager.addCommand(IRCodes::GREEN,
            [this]() { setScreen(GRAPH_SCREEN); },
            "Switch to Graph Screen");
            
        // Menu navigation
        irManager.addCommand(IRCodes::UP,
            [this]() { getCurrentScreen()->navigateUp(); },
            "Navigate Up");
            
        irManager.addCommand(IRCodes::DOWN,
            [this]() { getCurrentScreen()->navigateDown(); },
            "Navigate Down");
            
        irManager.addCommand(IRCodes::OK,
            [this]() { getCurrentScreen()->selectCurrent(); },
            "Select Current Item");
            
        // Quick actions
        irManager.addCommand(IRCodes::NUM_1,
            []() { startCharging(); },
            "Start/Stop Charging");
            
        irManager.addCommand(IRCodes::NUM_2,
            []() { resetCapacityCounter(); },
            "Reset Capacity Counter");
            
        irManager.addCommand(IRCodes::NUM_3,
            []() { toggleBacklight(); },
            "Toggle Display Backlight");
    }
    
public:
    UIManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
                 irReceiver(IR_RECEIVE_PIN),
                 currentScreenType(MAIN_SCREEN),
                 lastUpdateTime(0) {
        setupDefaultCommands();
    }
    
    // Add method to get IR manager for custom command registration
    IRCommandManager& getIRManager() { return irManager; }
    
    void update() {
        unsigned long currentTime = millis();
        if (currentTime - lastUpdateTime < UPDATE_INTERVAL) {
            return;
        }
        lastUpdateTime = currentTime;
        
        Screen* currentScreen = getScreen(currentScreenType);
        if (!currentScreen) return;
        
        // Handle IR input
        if (irReceiver.decode()) {
            uint32_t code = irReceiver.decodedIRData.decodedRawData;
            bool repeat = irReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;
            
            if (irManager.handleCommand(code, repeat)) {
                // Command was handled by IR manager
            } else {
                // Pass unhandled commands to current screen
                InputEvent event{InputEvent::IR_BUTTON, code};
                currentScreen->handleInput(event);
            }
            
            irReceiver.resume();
        }
        
        currentScreen->update();
        display.clearDisplay();
        currentScreen->draw(display);
        display.display();
    }
};

// Enhance the Screen class with better navigation
class Screen {
public:
    // ... (previous public members) ...
    
    virtual void navigateUp() {
        changeFocus(-1);
    }
    
    virtual void navigateDown() {
        changeFocus(1);
    }
    
    virtual void selectCurrent() {
        if (focusedWidgetIndex < widgets.size()) {
            InputEvent event{InputEvent::IR_BUTTON, IRCodes::OK};
            widgets[focusedWidgetIndex]->handleInput(event);
        }
    }
    
    // Add navigation in both directions
    virtual void navigateLeft() {
        // Default implementation - can be overridden by derived classes
    }
    
    virtual void navigateRight() {
        // Default implementation - can be overridden by derived classes
    }
};

// Enhance GraphScreen with additional IR commands
class GraphScreen : public Screen {
public:
    GraphScreen() {
        // ... (previous constructor code) ...
        
        // Add graph-specific IR commands
        auto& irManager = ui.getIRManager();
        
        irManager.addCommand(IRCodes::LEFT,
            [this]() { adjustTimeScale(-1); },
            "Decrease Time Scale");
            
        irManager.addCommand(IRCodes::RIGHT,
            [this]() { adjustTimeScale(1); },
            "Increase Time Scale");
            
        irManager.addCommand(IRCodes::BLUE,
            [this]() { cycleGraphType(); },
            "Cycle Graph Type");
    }
    
private:
    float timeScale = 1.0f;
    
    void adjustTimeScale(int direction) {
        if (direction > 0) {
            timeScale *= 1.5f;
        } else {
            timeScale /= 1.5f;
        }
        timeScale = constrain(timeScale, 0.1f, 10.0f);
        plotter->markDirty();
    }
    
    void cycleGraphType() {
        currentGraph = (GraphType)((currentGraph + 1) % 3);
        plotter->markDirty();
    }
};

// In the main setup, we can now add custom commands if needed
void setup() {
    // ... (previous setup code) ...
    
    // Add any additional custom IR commands
    auto& irManager = ui.getIRManager();
    
    irManager.addCommand(IRCodes::FUNC_1,
        []() {
            // Toggle between different charge current settings
            static int currentIndex = 0;
            const float currents[] = {500, 1000, 1500, 2000};
            currentIndex = (currentIndex + 1) % 4;
            setChargeCurrentMA(currents[currentIndex]);
        },
        "Cycle Charge Current");
        
    irManager.addCommand(IRCodes::FUNC_2,
        []() {
            // Toggle temperature unit (C/F)
            toggleTemperatureUnit();
        },
        "Toggle Temperature Unit");
        
    // Print available commands to Serial for debugging
    irManager.printCommands();
}

// Add these helper functions
void setChargeCurrentMA(float current) {
    CHARGE_CURRENT_MA = current;
    if (chargerState == CHARGING) {
        setPWMDutyCycle(CHARGE_CURRENT_MA);
    }
}

void resetCapacityCounter() {
    capacityMah = 0;
}

void toggleBacklight() {
    static bool backlightOn = true;
    backlightOn = !backlightOn;
    // Implement based on your hardware
}

void toggleTemperatureUnit() {
    static bool useCelsius = true;
    useCelsius = !useCelsius;
    // Update temperature display format
}

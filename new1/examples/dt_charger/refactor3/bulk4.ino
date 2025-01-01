// ui_manager.h
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Adafruit_SSD1306.h>
#include <memory>
#include <vector>
#include "config.h"
#include "ir_manager.h"
#include "screen.h"

// Forward declarations
class Screen;
class MainScreen;
class GraphScreen;

enum class ScreenType {
    MAIN_SCREEN,
    GRAPH_SCREEN
};

enum class GraphType {
    VOLTAGE_GRAPH,
    CURRENT_GRAPH,
    TEMP_DELTA_GRAPH
};

class UIManager {
public:
    UIManager();
    bool begin();
    void update();
    void setScreen(ScreenType type);

private:
    Adafruit_SSD1306 display;
    IRManager irManager;
    std::vector<std::unique_ptr<Screen>> screens;
    ScreenType currentScreen;
    unsigned long lastUpdate;

    Screen* getCurrentScreen();
    void setupScreens();
    void setupCommands();
};

#endif // UI_MANAGER_H

// globals.h
#ifndef GLOBALS_H
#define UI_MANAGER_H

#include "sensor_manager.h"
#include "history_manager.h"
#include "charger_controller.h"
#include "ui_manager.h"

// Global objects declaration
extern SensorManager sensorManager;
extern HistoryManager historyManager;
extern ChargerController chargerController;
extern UIManager uiManager;

// IR Codes namespace
namespace IRCodes {
    constexpr uint32_t UP = 0x18;
    constexpr uint32_t DOWN = 0x52;
    constexpr uint32_t LEFT = 0x08;
    constexpr uint32_t RIGHT = 0x5A;
    constexpr uint32_t OK = 0x1C;
    constexpr uint32_t RED = 0x45;
    constexpr uint32_t GREEN = 0x46;
    constexpr uint32_t BLUE = 0x47;
}

#endif // GLOBALS_H

// globals.cpp
#include "globals.h"

// Global objects definition
SensorManager sensorManager;
HistoryManager historyManager;
ChargerController chargerController;
UIManager uiManager;

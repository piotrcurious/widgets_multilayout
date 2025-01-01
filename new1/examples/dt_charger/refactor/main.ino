#include "UI_Framework.h"
#include "MainScreen.h"
#include "GraphScreen.h"

UIManager ui;

void setup() {
    Serial.begin(115200);

    if (!ui.begin()) {
        Serial.println("Failed to initialize UI.");
        return;
    }

    ui.addScreen(new MainScreen());
    ui.addScreen(new GraphScreen());
    ui.setScreen(0);

    auto& irManager = ui.getIRManager();
    irManager.addCommand(IRCodes::FUNC_1, []() { toggleBacklight(); }, "Toggle Backlight");
    irManager.printCommands();
}

void loop() {
    ui.update();
}

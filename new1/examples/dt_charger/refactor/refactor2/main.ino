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
    irManager.addCommand(IRCodes::RED, []() { ui.setScreen(0); }, "Main Screen");
    irManager.addCommand(IRCodes::GREEN, []() { ui.setScreen(1); }, "Graph Screen");
    irManager.printCommands();
}

void loop() {
    ui.update();
}

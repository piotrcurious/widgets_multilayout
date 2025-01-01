#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

#include "UI_Framework.h"

class MainScreen : public Screen {
public:
    MainScreen() {
        addWidget(new Label(0, 0, SCREEN_WIDTH, "NiMH Charger", true));
        addWidget(new Label(0, 12, SCREEN_WIDTH, "Status: Idle"));
        addWidget(new Button(14, 54, 100, "Start Charging", []() { startCharging(); }));
    }
};

#endif // MAIN_SCREEN_H

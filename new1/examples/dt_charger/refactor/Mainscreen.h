#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

#include "UI_Framework.h"

class MainScreen : public Screen {
public:
    MainScreen() {
        addWidget(new Label(0, 0, 128, 10, "NiMH Charger", true));
        addWidget(new BatteryWidget(0, 12, 128, 20));
        addWidget(new FloatDisplay(0, 34, 128, 10, 1, "Temp C"));
        addWidget(new FloatDisplay(0, 44, 128, 10, 0, "mAh"));
        addWidget(new Button(14, 54, 100, 10, "Start Charging", []() { toggleCharging(); }));
    }
};

#endif // MAIN_SCREEN_H

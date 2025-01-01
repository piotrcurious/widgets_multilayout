#ifndef GRAPH_SCREEN_H
#define GRAPH_SCREEN_H

#include "UI_Framework.h"

class GraphScreen : public Screen {
private:
    float timeScale = 1.0f;

public:
    GraphScreen() {
        addWidget(new Label(0, 0, 128, 10, "Graph Screen", true));
        addWidget(new FunctionPlotter(0, 10, 128, 44, [](float x) { return x * x; }, -1, 1));
        auto& irManager = UIManager::getIRManager();
        irManager.addCommand(IRCodes::BLUE, [this]() { cycleGraphType(); }, "Cycle Graph Type");
    }

    void cycleGraphType() {
        // Logic to cycle graph types
    }
};

#endif // GRAPH_SCREEN_H

#ifndef GRAPH_SCREEN_H
#define GRAPH_SCREEN_H

#include "UI_Framework.h"

class GraphScreen : public Screen {
public:
    GraphScreen() {
        addWidget(new Label(0, 0, SCREEN_WIDTH, "Voltage Graph", true));
        addWidget(new Button(14, 54, 100, "Next Graph", []() { cycleGraphType(); }));
    }

private:
    void cycleGraphType() {
        // Logic to cycle graph types
    }
};

#endif // GRAPH_SCREEN_H

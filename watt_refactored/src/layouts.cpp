#include "layouts.h"
#include "data_source.h"

extern DataSource dataSource;

float getWatts() { return dataSource.getWatts(); }
float getVolts() { return dataSource.getVolts(); }
float getAmperes() { return dataSource.getAmperes(); }
float getWattHours() { return dataSource.getWattHours(); }

Layouts::Layouts(Display& display, DataSource& dataSource, const Theme& theme) {
    layoutCount = 3;
    allLayouts = new Widget**[layoutCount];
    layoutSizes = new int[layoutCount];

    // Define widgets
    TextWidget* wattsWidget = new TextWidget(display, 10, 10, 220, 30, "Watts", getWatts, theme);
    TextWidget* voltsWidget = new TextWidget(display, 10, 50, 220, 30, "Volts", getVolts, theme);
    TextWidget* ampsWidget = new TextWidget(display, 10, 90, 220, 30, "Amps", getAmperes, theme);
    GraphWidget* wattsGraph = new GraphWidget(display, 10, 130, 220, 100, getWatts, theme, 1500);
    TextWidget* whWidget = new TextWidget(display, 10, 10, 220, 30, "Watt-Hours", getWattHours, theme);
    GraphWidget* whGraph = new GraphWidget(display, 10, 50, 220, 180, getWattHours, theme, 100);

    // Layout 1: Watts, Volts, Amps
    layoutSizes[0] = 3;
    allLayouts[0] = new Widget*[layoutSizes[0]];
    allLayouts[0][0] = wattsWidget;
    allLayouts[0][1] = voltsWidget;
    allLayouts[0][2] = ampsWidget;

    // Layout 2: Watts and Watts Graph
    layoutSizes[1] = 2;
    allLayouts[1] = new Widget*[layoutSizes[1]];
    allLayouts[1][0] = wattsWidget;
    allLayouts[1][1] = wattsGraph;

    // Layout 3: Watt-Hours and Watt-Hours Graph
    layoutSizes[2] = 2;
    allLayouts[2] = new Widget*[layoutSizes[2]];
    allLayouts[2][0] = whWidget;
    allLayouts[2][1] = whGraph;
}

Layouts::~Layouts() {
    for (int i = 0; i < layoutCount; ++i) {
        for (int j = 0; j < layoutSizes[i]; ++j) {
            delete allLayouts[i][j];
        }
        delete[] allLayouts[i];
    }
    delete[] allLayouts;
    delete[] layoutSizes;
}

Widget** Layouts::getLayout(int index, int& size) const {
    size = layoutSizes[index];
    return allLayouts[index];
}

int Layouts::getLayoutCount() const {
    return layoutCount;
}
#ifndef LAYOUTS_H
#define LAYOUTS_H

#include "widget.h"

// Forward declaration
class Layouts;

// Getter functions
float getWatts();
float getVolts();
float getAmperes();
float getWattHours();

// Defines and manages all screen layouts
class Layouts {
public:
    Layouts(Display& display, DataSource& dataSource, const Theme& theme, WidgetManager& manager);
    ~Layouts();

    Widget** getLayout(int index, int& size) const;
    int getLayoutCount() const;

private:
    Widget*** allLayouts;
    int* layoutSizes;
    int layoutCount;
};

#endif // LAYOUTS_H
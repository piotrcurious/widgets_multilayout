#ifndef WIDGET_H
#define WIDGET_H

#include "display_manager.h"
#include "data_source.h"
// Forward declaration
class Widget;

// Callback function type
typedef float (*Getter)();

// Manages the active layout and updates widgets
class WidgetManager {
public:
    WidgetManager(Display& display, DataSource& dataSource);
    void add(Widget* widget);
    void setLayout(Widget** layout, int size);
    void update(uint32_t currentTime);

private:
    Display& display;
    DataSource& dataSource;
    Widget** allWidgets;
    int allWidgetsCount;
    Widget** currentLayout;
    int currentLayoutSize;
};

// Abstract base class for all widgets
class Widget {
public:
    Widget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h);
    virtual ~Widget() {}
    virtual void update(uint32_t currentTime) = 0;
    virtual void draw() = 0;

protected:
    Display& display;
    int16_t x, y, w, h;
};

// Displays a text label and a value from a data source
class TextWidget : public Widget {
public:
    TextWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, const char* label, Getter getter, const Theme& theme);
    void update(uint32_t currentTime) override;
    void draw() override;

private:
    const char* label;
    Getter getter;
    const Theme& theme;
    float value;
};

// Displays a graph of a value from a data source
class GraphWidget : public Widget {
public:
    GraphWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, Getter getter, const Theme& theme, float max_value);
    ~GraphWidget();
    void update(uint32_t currentTime) override;
    void draw() override;

private:
    Getter getter;
    const Theme& theme;
    float max_value;
    float* history;
    int history_size;
    int history_index;
};

#endif // WIDGET_H
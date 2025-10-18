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
    void processAll(uint32_t currentTime);
    void displayVisible(uint32_t currentTime);

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
    Widget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t process_freq, uint16_t display_freq);
    virtual ~Widget() {}

    void process(uint32_t currentTime);
    void display(uint32_t currentTime);

protected:
    virtual void processLogic() = 0;
    virtual void displayLogic() = 0;

    Display& _display;
    int16_t x, y, w, h;

    uint16_t processFrequency;
    uint32_t lastProcessTime;

    uint16_t displayFrequency;
    uint32_t lastDisplayTime;
};

// Displays a text label and a value from a data source
class TextWidget : public Widget {
public:
    TextWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t process_freq, uint16_t display_freq, const char* label, Getter getter, const Theme& theme);

protected:
    void processLogic() override;
    void displayLogic() override;

private:
    const char* label;
    Getter getter;
    const Theme& theme;
    float value;
};

// Displays a graph of a value from a data source
class GraphWidget : public Widget {
public:
    GraphWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t process_freq, uint16_t display_freq, Getter getter, const Theme& theme, float max_value);
    ~GraphWidget();

protected:
    void processLogic() override;
    void displayLogic() override;

private:
    Getter getter;
    const Theme& theme;
    float max_value;
    float* history;
    int history_size;
    int history_index;
};

#endif // WIDGET_H
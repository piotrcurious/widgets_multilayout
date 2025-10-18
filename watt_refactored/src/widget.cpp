#include "widget.h"

// WidgetManager implementation
WidgetManager::WidgetManager(Display& display, DataSource& dataSource) : display(display), dataSource(dataSource), allWidgets(nullptr), allWidgetsCount(0), currentLayout(nullptr), currentLayoutSize(0) {}

void WidgetManager::add(Widget* widget) {
    Widget** newAllWidgets = new Widget*[allWidgetsCount + 1];
    for (int i = 0; i < allWidgetsCount; ++i) {
        newAllWidgets[i] = allWidgets[i];
    }
    newAllWidgets[allWidgetsCount] = widget;
    delete[] allWidgets;
    allWidgets = newAllWidgets;
    allWidgetsCount++;
}

void WidgetManager::setLayout(Widget** layout, int size) {
    currentLayout = layout;
    currentLayoutSize = size;
    display.fillScreen(0); // Clear screen on layout change
}

void WidgetManager::processAll(uint32_t currentTime) {
    dataSource.update();
    for (int i = 0; i < allWidgetsCount; ++i) {
        allWidgets[i]->process(currentTime);
    }
}

void WidgetManager::displayVisible(uint32_t currentTime) {
    for (int i = 0; i < currentLayoutSize; ++i) {
        currentLayout[i]->display(currentTime);
    }
}

// Widget implementation
Widget::Widget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t process_freq, uint16_t display_freq)
    : _display(display), x(x), y(y), w(w), h(h), processFrequency(process_freq), lastProcessTime(0), displayFrequency(display_freq), lastDisplayTime(0) {}

void Widget::process(uint32_t currentTime) {
    if (currentTime - lastProcessTime > processFrequency) {
        processLogic();
        lastProcessTime = currentTime;
    }
}

void Widget::display(uint32_t currentTime) {
    if (currentTime - lastDisplayTime > displayFrequency) {
        displayLogic();
        lastDisplayTime = currentTime;
    }
}

// TextWidget implementation
TextWidget::TextWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t process_freq, uint16_t display_freq, const char* label, Getter getter, const Theme& theme)
    : Widget(display, x, y, w, h, process_freq, display_freq), label(label), getter(getter), theme(theme), value(0) {}

void TextWidget::processLogic() {
    value = getter();
}

void TextWidget::displayLogic() {
    _display.fillRect(x, y, w, h, theme.bg);
    _display.setCursor(x + 2, y + 2);
    _display.setTextColor(theme.text);
    _display.setTextSize(2);
    _display.print(label);
    _display.print(": ");

    uint16_t valueColor = theme.valueNormal;
    if (value > 400) valueColor = theme.valueHigh;
    else if (value < 100) valueColor = theme.valueLow;

    _display.setTextColor(valueColor);
    _display.print(value, 2);
}

// GraphWidget implementation
GraphWidget::GraphWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t process_freq, uint16_t display_freq, Getter getter, const Theme& theme, float max_value)
    : Widget(display, x, y, w, h, process_freq, display_freq), getter(getter), theme(theme), max_value(max_value), history_size(w), history_index(0) {
    history = new float[history_size];
    for (int i = 0; i < history_size; ++i) {
        history[i] = 0;
    }
}

GraphWidget::~GraphWidget() {
    delete[] history;
}

void GraphWidget::processLogic() {
    history[history_index] = getter();
    history_index = (history_index + 1) % history_size;
}

void GraphWidget::displayLogic() {
    _display.fillRect(x, y, w, h, theme.bg);
    _display.drawRect(x, y, w, h, theme.frame);
    for (int i = 0; i < w; ++i) {
        int current_index = (history_index + i) % history_size;

        float y1 = y + h - (history[current_index] / max_value) * h;

        uint16_t color = (history[current_index] > max_value * 0.8) ? theme.graphHighlight : theme.graph;
        _display.drawPixel(x + i, y1, color);
    }
}
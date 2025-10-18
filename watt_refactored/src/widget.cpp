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

void WidgetManager::update(uint32_t currentTime) {
    dataSource.update();
    for (int i = 0; i < currentLayoutSize; ++i) {
        currentLayout[i]->update(currentTime);
        currentLayout[i]->draw();
    }
}

// Widget implementation
Widget::Widget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h) : display(display), x(x), y(y), w(w), h(h) {}

// TextWidget implementation
TextWidget::TextWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, const char* label, Getter getter, const Theme& theme)
    : Widget(display, x, y, w, h), label(label), getter(getter), theme(theme), value(0) {}

void TextWidget::update(uint32_t currentTime) {
    value = getter();
}

void TextWidget::draw() {
    display.fillRect(x, y, w, h, theme.bg);
    display.setCursor(x + 2, y + 2);
    display.setTextColor(theme.text);
    display.setTextSize(2);
    display.print(label);
    display.print(": ");

    uint16_t valueColor = theme.valueNormal;
    if (value > 400) valueColor = theme.valueHigh;
    else if (value < 100) valueColor = theme.valueLow;

    display.setTextColor(valueColor);
    display.print(value, 2);
}

// GraphWidget implementation
GraphWidget::GraphWidget(Display& display, int16_t x, int16_t y, int16_t w, int16_t h, Getter getter, const Theme& theme, float max_value)
    : Widget(display, x, y, w, h), getter(getter), theme(theme), max_value(max_value), history_size(w), history_index(0) {
    history = new float[history_size];
    for (int i = 0; i < history_size; ++i) {
        history[i] = 0;
    }
}

GraphWidget::~GraphWidget() {
    delete[] history;
}

void GraphWidget::update(uint32_t currentTime) {
    history[history_index] = getter();
    history_index = (history_index + 1) % history_size;
}

void GraphWidget::draw() {
    display.fillRect(x, y, w, h, theme.bg);
    display.drawRect(x, y, w, h, theme.frame);
    for (int i = 0; i < w; ++i) {
        int current_index = (history_index + i) % history_size;

        float y1 = y + h - (history[current_index] / max_value) * h;

        uint16_t color = (history[current_index] > max_value * 0.8) ? theme.graphHighlight : theme.graph;
        display.drawPixel(x + i, y1, color);
    }
}
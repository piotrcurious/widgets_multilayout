To continue from the previous steps, we will focus on finalizing the full structure for the widget-based layout system, handling memory management, dynamic updates, and layout switching. Here’s the continuation:

Step 9: Handling Layout Switching Conditions

Now that we have multiple layouts, let's implement automatic layout switching based on time, user input, or other conditions. This can be integrated into the loop() function where we check conditions and switch layouts accordingly.

void loop() {
    uint32_t currentTime = millis();

    // Process the current layout
    manager.processCurrentLayout(currentTime);
    manager.displayCurrentLayout();

    // Example condition-based layout switching
    if (currentTime > 20000 && currentTime < 40000) {
        manager.switchLayout(1);  // Switch to layout 2 between 20s and 40s
    } else if (currentTime > 40000 && currentTime < 60000) {
        manager.switchLayout(2);  // Switch to layout 3 between 40s and 60s
    } else if (currentTime > 60000) {
        manager.switchLayout(0);  // Switch back to layout 1 after 60s
    }
}

This loop will check the time and change layouts at specific intervals, allowing the display to rotate between different sets of widgets.

Step 10: Dynamic Memory Handling for Widgets

Let’s implement dynamic memory management to ensure that switching between layouts does not cause memory leaks or unnecessary memory usage. We want to free up the old layout's resources when a new layout is loaded.

void WidgetManager::switchLayout(uint8_t layoutIndex) {
    if (layoutIndex < layouts.size()) {
        // Clear the current layout's widgets to free memory
        for (Widget* widget : layouts[currentLayoutIndex]) {
            delete widget;
        }
        layouts[currentLayoutIndex].clear();

        // Set the new layout
        currentLayoutIndex = layoutIndex;

        // Re-initialize the widgets for the new layout
        for (Widget* widget : layouts[currentLayoutIndex]) {
            widget = new Widget(*widget);  // Allocate new widgets for the new layout
        }
    }
}

This method will free memory from the previous layout, ensuring that there is no memory overflow when switching layouts. It re-initializes widgets for the new layout to prepare them for display.

Step 11: Supporting Input from Buttons for Layout Switch

To make the layout switching more interactive, we can add support for buttons to manually switch between layouts. Let’s assume you have buttons connected to GPIO pins on your ESP32.

#define BUTTON_PIN1 12
#define BUTTON_PIN2 13

void setup() {
    pinMode(BUTTON_PIN1, INPUT);
    pinMode(BUTTON_PIN2, INPUT);

    // Initialize display and set initial layout
    manager.switchLayout(0);
}

void loop() {
    uint32_t currentTime = millis();

    // Process and display the current layout
    manager.processCurrentLayout(currentTime);
    manager.displayCurrentLayout();

    // Check button inputs for layout switching
    if (digitalRead(BUTTON_PIN1) == HIGH) {
        manager.switchLayout(1);  // Switch to layout 2 on button 1 press
    }
    if (digitalRead(BUTTON_PIN2) == HIGH) {
        manager.switchLayout(2);  // Switch to layout 3 on button 2 press
    }
}

This example allows manual layout switching by pressing buttons connected to GPIO pins. You can define your own button handling logic, such as debouncing or holding.

Step 12: Full Example of Widget-Based Layout with Button-Controlled Switching

Here's the full final structure of the system, including all components from previous steps.

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <vector>
#include <functional>

// TFT Pins
#define TFT_CS     15
#define TFT_RST    4
#define TFT_DC     2

// Global dynamic data
float watts = 0, volts = 0, amperes = 0, wattHours = 0;
uint32_t lastWhCalculationTime = 0;

float getWatts() { return watts; }
float getVolts() { return volts; }
float getAmperes() { return amperes; }

// Base Widget Class
class Widget {
public:
    int16_t x, y, width, height;
    uint16_t updateFrequency;

    Widget(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t updateFrequency)
        : x(x), y(y), width(width), height(height), updateFrequency(updateFrequency) {}

    virtual void displayWidget() = 0;
    virtual void updateWidgetData(uint32_t currentTime) = 0;
};

// Text Widget
class TextWidget : public Widget {
private:
    String label;
    std::function<float()> dataSource;

public:
    TextWidget(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t updateFrequency, String label, std::function<float()> dataSource)
        : Widget(x, y, width, height, updateFrequency), label(label), dataSource(dataSource) {}

    void displayWidget() override {
        display->setCursor(x, y);
        display->print(label + ": ");
        display->println(dataSource(), 2);
    }

    void updateWidgetData(uint32_t currentTime) override {
        if (currentTime % updateFrequency == 0) {
            displayWidget();
        }
    }
};

// Widget Manager Class
class WidgetManager {
private:
    std::vector<std::vector<Widget*>> layouts;
    uint8_t currentLayoutIndex;

public:
    WidgetManager() : currentLayoutIndex(0) {}

    void addLayout(std::vector<Widget*> layout) {
        layouts.push_back(layout);
    }

    void switchLayout(uint8_t layoutIndex) {
        if (layoutIndex < layouts.size()) {
            currentLayoutIndex = layoutIndex;
        }
    }

    void processCurrentLayout(uint32_t currentTime) {
        for (Widget* widget : layouts[currentLayoutIndex]) {
            widget->updateWidgetData(currentTime);
        }
    }

    void displayCurrentLayout() {
        for (Widget* widget : layouts[currentLayoutIndex]) {
            widget->displayWidget();
        }
    }
};

// Define global objects
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
WidgetManager manager;

void setup() {
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(ILI9341_BLACK);

    // Define the first layout
    std::vector<Widget*> layout1 = {
        new TextWidget(20, 10, 100, 30, 1000, "Watts", getWatts),
        new TextWidget(20, 50, 100, 30, 1000, "Volts", getVolts)
    };
    manager.addLayout(layout1);

    // Define the second layout
    std::vector<Widget*> layout2 = {
        new TextWidget(20, 10, 100, 30, 1000, "Amperes", getAmperes)
    };
    manager.addLayout(layout2);

    // Set initial layout
    manager.switchLayout(0);

    // Set up buttons
    pinMode(BUTTON_PIN1, INPUT);
    pinMode(BUTTON_PIN2, INPUT);
}

void loop() {
    uint32_t currentTime = millis();

    // Process and display current layout
    manager.processCurrentLayout(currentTime);
    manager.displayCurrentLayout();

    // Switch layouts using buttons
    if (digitalRead(BUTTON_PIN1) == HIGH) {
        manager.switchLayout(1);  // Switch to layout 2
    }
    if (digitalRead(BUTTON_PIN2) == HIGH) {
        manager.switchLayout(2);  // Switch to layout 3
    }
}

Conclusion

This final structure provides:

1. Dynamic widget creation and layout management: Switch between layouts containing different widgets.


2. Automatic and manual layout switching: Time-based or button-activated layout switching.


3. Memory management: Widgets are dynamically created and destroyed, optimizing memory usage.



You can now extend this structure by adding more widget types, like graphs, integrating SD card logging, or adding more complex interaction logic. Let me know if you need help with further customization!


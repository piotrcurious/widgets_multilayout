Continuing from where we left off, the existing code provides a solid foundation for managing multiple layouts and dynamically switching between them. Now, we can further enhance the functionality, ensure robustness, and add features that will make the system more suitable for embedded environments.

Step 13: Enhance Widget Updating with Delta Times

To ensure widgets update smoothly and at the correct intervals, we can introduce a deltaTime mechanism. This way, we aren't relying solely on the millisecond count, which may introduce minor inaccuracies.

void loop() {
    static uint32_t lastUpdateTime = 0;
    uint32_t currentTime = millis();
    uint32_t deltaTime = currentTime - lastUpdateTime;

    // Process the current layout's widgets, using deltaTime for precision
    manager.processCurrentLayout(deltaTime);

    // Display the current layout
    manager.displayCurrentLayout();

    // Update the last update time
    lastUpdateTime = currentTime;

    // Handle layout switching with buttons
    if (digitalRead(BUTTON_PIN1) == HIGH) {
        manager.switchLayout(1);  // Switch to layout 2
    }
    if (digitalRead(BUTTON_PIN2) == HIGH) {
        manager.switchLayout(2);  // Switch to layout 3
    }
}

By introducing deltaTime, we can achieve smoother widget updates. This approach will be particularly useful in scenarios where widget updates are not strictly tied to frame rates but need to reflect changes accurately.

Step 14: Add Support for SD Card Logging (Optional)

If your project involves logging data (e.g., power consumption or sensor values), we can add SD card support to store data periodically. This can be done by introducing a logger class that interfaces with an SD card module.

#include <SD.h>
#define SD_CS_PIN 5  // SD card chip select pin

class DataLogger {
private:
    File logFile;

public:
    bool begin() {
        if (!SD.begin(SD_CS_PIN)) {
            return false;
        }
        logFile = SD.open("log.txt", FILE_WRITE);
        return logFile;
    }

    void logData(const String& data) {
        if (logFile) {
            logFile.println(data);
            logFile.flush();  // Ensure data is written
        }
    }

    void end() {
        if (logFile) {
            logFile.close();
        }
    }
};

// Create logger instance
DataLogger logger;

void setup() {
    // Initialize logger
    if (!logger.begin()) {
        Serial.println("SD card initialization failed!");
    }
}

With this simple logger, we can log values like power consumption or voltages to an SD card. You can call logger.logData() within your widgets or loop to store values as needed.

Step 15: Improving Button Handling with Debouncing

Button presses are often noisy, leading to multiple layout switches or unreliable behavior. To solve this, we can introduce a simple debounce mechanism.

#define DEBOUNCE_DELAY 50

bool buttonPressed(uint8_t pin) {
    static uint32_t lastDebounceTime = 0;
    static bool lastButtonState = LOW;
    bool buttonState = digitalRead(pin);

    if (buttonState != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        if (buttonState == HIGH) {
            lastButtonState = buttonState;
            return true;
        }
    }

    lastButtonState = buttonState;
    return false;
}

void loop() {
    uint32_t currentTime = millis();
    manager.processCurrentLayout(currentTime - lastUpdateTime);
    manager.displayCurrentLayout();
    lastUpdateTime = currentTime;

    if (buttonPressed(BUTTON_PIN1)) {
        manager.switchLayout(1);  // Switch to layout 2
    }
    if (buttonPressed(BUTTON_PIN2)) {
        manager.switchLayout(2);  // Switch to layout 3
    }
}

This buttonPressed() function implements basic debouncing, ensuring that noisy button inputs won’t trigger multiple unwanted layout switches.

Step 16: Memory and Performance Optimization

Embedded systems have limited resources, so it’s essential to manage memory efficiently. If you're working on a device with very limited RAM, avoid dynamic memory allocations during runtime, especially when switching layouts or updating widgets.

Here’s how you can statically allocate your widgets to avoid heap fragmentation and improve performance.

1. Remove Dynamic Memory Allocation: Instead of using new and delete, declare widgets statically.



TextWidget wattsWidget(20, 10, 100, 30, 1000, "Watts", getWatts);
TextWidget voltsWidget(20, 50, 100, 30, 1000, "Volts", getVolts);
TextWidget amperesWidget(20, 10, 100, 30, 1000, "Amperes", getAmperes);

std::vector<Widget*> layout1 = { &wattsWidget, &voltsWidget };
std::vector<Widget*> layout2 = { &amperesWidget };

2. Avoid Frequent fillRect() or fillScreen(): Constantly clearing and redrawing large portions of the screen can slow down your system. Instead, try to only redraw parts of the screen that have changed.



void TextWidget::displayWidget() {
    display->setCursor(x, y);
    display->print(label + ": ");
    float newValue = dataSource();
    if (newValue != lastValue) {  // Only update if value has changed
        display->println(newValue, 2);
        lastValue = newValue;
    }
}

This optimization prevents unnecessary screen refreshes and reduces flickering.

Step 17: Wrap-up and Testing

Now that we've incorporated:

Layout switching (manual and time-based),

Button debouncing,

SD card logging (optional),

Memory optimizations and efficient updates,


it’s time to test the system on your ESP32 or Arduino-compatible device to ensure everything works as expected. Make sure you have all the required libraries installed (Adafruit_GFX, Adafruit_ILI9341, and SD if logging is needed), and confirm that the display is refreshing correctly.

Conclusion

The code is now structured and optimized for embedded systems, using a flexible widget-based architecture. You can expand it by adding more widget types (e.g., buttons, progress bars), additional input sources (e.g., sensors), or more complex layout switching logic.

Let me know if you need further help refining the functionality or adding new features!


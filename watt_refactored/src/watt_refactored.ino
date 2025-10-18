#include "display_manager.h"
#include "data_source.h"
#include "widget.h"
#include "layouts.h"

#if defined(USE_ADAFRUIT_DISPLAY)
// Pin definitions
#define TFT_CS  15
#define TFT_DC  2
#define TFT_RST 4
#endif
#define BTN_PIN 0

// Global objects
#if defined(USE_ADAFRUIT_DISPLAY)
AdafruitDisplay display(TFT_CS, TFT_DC, TFT_RST);
#elif defined(USE_TFT_ESPI_DISPLAY)
TFT_eSPI_Display display;
#endif
DataSource dataSource;
Theme theme = {0x0000, 0xFFFF, 0x07E0, 0x07E0, 0xF800, 0xFFFF, 0xF800, 0x001F};
Layouts layouts(display, dataSource, theme);
WidgetManager widgetManager(display, dataSource);

int currentLayoutIndex = 0;
uint32_t lastButtonPress = 0;

void setup() {
    Serial.begin(115200);
    pinMode(BTN_PIN, INPUT_PULLUP);
    display.begin();
    int size;
    widgetManager.setLayout(layouts.getLayout(currentLayoutIndex, size), size);
}

void loop() {
    // Handle button press for layout switching
    if (digitalRead(BTN_PIN) == LOW && millis() - lastButtonPress > 500) {
        currentLayoutIndex = (currentLayoutIndex + 1) % layouts.getLayoutCount();
        int size;
        widgetManager.setLayout(layouts.getLayout(currentLayoutIndex, size), size);
        lastButtonPress = millis();
    }

    widgetManager.update(millis());
    delay(100);
}
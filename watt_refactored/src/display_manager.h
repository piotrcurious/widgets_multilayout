#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Color theme definition
struct Theme {
    uint16_t bg;
    uint16_t text;
    uint16_t frame;
    uint16_t graph;
    uint16_t graphHighlight;
    uint16_t valueNormal;
    uint16_t valueHigh;
    uint16_t valueLow;
};

// Abstract display interface for hardware decoupling
class Display {
public:
    virtual ~Display() {}
    virtual void begin() = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    virtual void setTextColor(uint16_t color) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void print(const char* text) = 0;
    virtual void print(float value, int decimals) = 0;
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
};

// Adafruit ILI9341 implementation of the Display interface
class AdafruitDisplay : public Display {
public:
    AdafruitDisplay(uint8_t cs, uint8_t dc, uint8_t rst) : tft(cs, dc, rst) {}

    void begin() override { tft.begin(); }
    void fillScreen(uint16_t color) override { tft.fillScreen(color); }
    void setCursor(int16_t x, int16_t y) override { tft.setCursor(x, y); }
    void setTextColor(uint16_t color) override { tft.setTextColor(color); }
    void setTextSize(uint8_t size) override { tft.setTextSize(size); }
    void print(const char* text) override { tft.print(text); }
    void print(float value, int decimals) override { tft.print(value, decimals); }
    void drawPixel(int16_t x, int16_t y, uint16_t color) override { tft.drawPixel(x, y, color); }
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { tft.fillRect(x, y, w, h, color); }
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override { tft.drawRect(x, y, w, h, color); }

private:
    Adafruit_ILI9341 tft;
};

#endif // DISPLAY_MANAGER_H
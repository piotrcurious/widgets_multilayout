// config.h
#ifndef CONFIG_H
#define CONFIG_H

// Display configuration - uncomment one
#define USE_TFT
//#define USE_OLED

// Pin definitions
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4

// Display dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Update frequencies
#define WATTS_UPDATE_FREQ 100
#define VOLTS_UPDATE_FREQ 1000
#define GRAPH_UPDATE_FREQ 1000

// Data buffer sizes
#define GRAPH_HISTORY_SIZE 50

#endif

// color_theme.h
#ifndef COLOR_THEME_H
#define COLOR_THEME_H

#ifdef USE_TFT
#include <TFT_eSPI.h>
#define COLOR_BG TFT_BLACK
#define COLOR_TEXT TFT_WHITE
#define COLOR_VALUE_NORMAL TFT_GREEN
#define COLOR_VALUE_HIGH TFT_RED
#define COLOR_VALUE_LOW TFT_YELLOW
#define COLOR_GRAPH TFT_CYAN
#define COLOR_GRAPH_HIGHLIGHT TFT_RED
#define COLOR_GRAPH_GRID TFT_DARKGREY
#endif

#ifdef USE_OLED
#include <Adafruit_SSD1306.h>
#define COLOR_BG 0
#define COLOR_TEXT 1
#define COLOR_VALUE_NORMAL 1
#define COLOR_VALUE_HIGH 1
#define COLOR_VALUE_LOW 1
#define COLOR_GRAPH 1
#define COLOR_GRAPH_HIGHLIGHT 1
#define COLOR_GRAPH_GRID 1
#endif

#endif

// TFTVisualizer.hpp
#ifdef USE_TFT
#ifndef TFT_VISUALIZER_HPP
#define TFT_VISUALIZER_HPP

#include <TFT_eSPI.h>
#include <vector>
#include "config.h"
#include "color_theme.h"

class TFTVisualizer {
private:
    TFT_eSPI tft;
    
    // Data storage
    float watts = 0, volts = 0, amperes = 0, wattHours = 0;
    std::vector<float> wattsHistory;
    std::vector<float> wattHoursHistory;
    size_t historyIndex = 0;
    
    // Timing
    uint32_t lastWattsUpdate = 0;
    uint32_t lastVoltsUpdate = 0;
    uint32_t lastGraphUpdate = 0;
    uint32_t lastWhCalculationTime = 0;
    
    // Layout state
    uint8_t currentLayout = 0;
    
    void drawValueWithLabel(int16_t x, int16_t y, const char* label, float value) {
        tft.setCursor(x, y);
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(2);
        tft.print(label);
        tft.print(": ");
        
        uint16_t valueColor = COLOR_VALUE_NORMAL;
        if (value > 400) valueColor = COLOR_VALUE_HIGH;
        else if (value < 100) valueColor = COLOR_VALUE_LOW;
        
        tft.setTextColor(valueColor);
        tft.print(value, 2);
    }
    
    void drawGraph(int16_t x, int16_t y, int16_t w, int16_t h, const std::vector<float>& history, float maxValue) {
        // Draw grid
        for (int i = 0; i < w; i += 20) {
            for (int j = 0; j < h; j += 10) {
                tft.drawPixel(x + i, y + j, COLOR_GRAPH_GRID);
            }
        }
        
        // Draw data
        for (size_t i = 0; i < history.size() - 1; i++) {
            size_t idx = (historyIndex + i) % history.size();
            int16_t x1 = x + i * (w / GRAPH_HISTORY_SIZE);
            int16_t y1 = y + h - (history[idx] * h / maxValue);
            
            uint16_t color = COLOR_GRAPH;
            if (history[idx] > maxValue * 0.8) {
                color = COLOR_GRAPH_HIGHLIGHT;
            }
            
            tft.drawPixel(x1, y1, color);
        }
    }
    
    void updateWattHours() {
        uint32_t currentTime = millis();
        float deltaTime = (currentTime - lastWhCalculationTime) / 3600000.0;
        wattHours += watts * deltaTime;
        lastWhCalculationTime = currentTime;
    }

public:
    TFTVisualizer() : wattsHistory(GRAPH_HISTORY_SIZE, 0), wattHoursHistory(GRAPH_HISTORY_SIZE, 0) {}
    
    void begin() {
        tft.init();
        tft.setRotation(3);
        tft.fillScreen(COLOR_BG);
        lastWhCalculationTime = millis();
    }
    
    void updateData(float _watts, float _volts, float _amperes) {
        watts = _watts;
        volts = _volts;
        amperes = _amperes;
        
        // Update histories
        uint32_t currentTime = millis();
        if (currentTime - lastGraphUpdate >= GRAPH_UPDATE_FREQ) {
            wattsHistory[historyIndex] = watts;
            wattHoursHistory[historyIndex] = wattHours;
            historyIndex = (historyIndex + 1) % GRAPH_HISTORY_SIZE;
            lastGraphUpdate = currentTime;
        }
        
        updateWattHours();
    }
    
    void drawLayout() {
        tft.fillScreen(COLOR_BG);
        
        switch (currentLayout) {
            case 0: // Watts, Volts, Watts Graph
                drawValueWithLabel(10, 10, "Watts", watts);
                drawValueWithLabel(10, 40, "Volts", volts);
                drawGraph(10, 100, 220, 50, wattsHistory, 500);
                break;
                
            case 1: // Watts, Volts, Amperes
                drawValueWithLabel(10, 10, "Watts", watts);
                drawValueWithLabel(10, 40, "Volts", volts);
                drawValueWithLabel(10, 70, "Amperes", amperes);
                break;
                
            case 2: // Watts, Watt Hours, WH Graph
                drawValueWithLabel(10, 10, "Watts", watts);
                drawValueWithLabel(10, 40, "Watt Hours", wattHours);
                drawGraph(10, 100, 220, 50, wattHoursHistory, wattHours * 1.2);
                break;
        }
    }
    
    void checkLayoutChange() {
        uint32_t currentTime = millis();
        if (currentTime > 20000 && currentTime < 40000) {
            currentLayout = 1;
        } else if (currentTime > 40000 && currentTime < 60000) {
            currentLayout = 2;
        } else {
            currentLayout = 0;
        }
    }
};

#endif
#endif

// Main program (watt_meter.ino)
#include "config.h"

#ifdef USE_TFT
#include "TFTVisualizer.hpp"
TFTVisualizer viz;
#endif

#ifdef USE_OLED
#include "OLEDVisualizer.hpp"
OLEDVisualizer viz;
#endif

// Simulated sensor data
float watts = 0, volts = 0, amperes = 0;

void setup() {
    viz.begin();
}

void loop() {
    // Simulate sensor readings
    watts = (sin(millis() / 1000.0) + 1) * 250;
    volts = 220 + sin(millis() / 2000.0) * 10;
    amperes = watts / volts;
    
    // Update visualizer
    viz.updateData(watts, volts, amperes);
    viz.checkLayoutChange();
    viz.drawLayout();
}

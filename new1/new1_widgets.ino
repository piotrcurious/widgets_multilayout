// ... [Previous code remains the same until the ProgressBar class] ...

// Float Display widget
class FloatDisplay : public Widget {
private:
    float value;
    uint8_t precision;
    char format[16];
    char label[32];
    bool showLabel;
    
public:
    FloatDisplay(int16_t x, int16_t y, int16_t w, int16_t h, 
                uint8_t precision = 2, const char* label = nullptr)
        : Widget(x, y, w, h), value(0.0f), precision(precision) {
        snprintf(format, sizeof(format), "%%.%df", precision);
        if (label) {
            strncpy(this->label, label, sizeof(this->label) - 1);
            this->label[sizeof(this->label) - 1] = '\0';
            showLabel = true;
        } else {
            showLabel = false;
        }
    }
    
    void setValue(float newValue) {
        if (abs(newValue - value) > pow(10, -precision)) {
            value = newValue;
            dirty = true;
        }
    }
    
    void draw(Adafruit_SSD1306& display) override {
        display.setTextColor(WHITE);
        
        char valueStr[16];
        snprintf(valueStr, sizeof(valueStr), format, value);
        
        if (showLabel) {
            // Draw label
            display.setCursor(x, y);
            display.print(label);
            display.print(": ");
            
            // Draw value right-aligned
            int16_t valueWidth = strlen(valueStr) * 6;
            display.setCursor(x + width - valueWidth, y);
        } else {
            // Center the value if no label
            int16_t valueWidth = strlen(valueStr) * 6;
            display.setCursor(x + (width - valueWidth) / 2, y + (height - 8) / 2);
        }
        
        display.print(valueStr);
        
        if (focused) {
            display.drawRect(x, y, width, height, WHITE);
        }
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// Function Plotter widget
class FunctionPlotter : public Widget {
public:
    typedef std::function<float(float)> PlotFunction;
    
private:
    PlotFunction function;
    float xMin, xMax, yMin, yMax;
    uint16_t numPoints;
    bool autoScale;
    static const uint16_t MAX_POINTS = 128;
    
    struct Point {
        float x, y;
    };
    Point points[MAX_POINTS];
    
public:
    FunctionPlotter(int16_t x, int16_t y, int16_t w, int16_t h,
                   PlotFunction func, float xMin = -5.0f, float xMax = 5.0f)
        : Widget(x, y, w, h), function(func),
          xMin(xMin), xMax(xMax), yMin(0), yMax(0),
          numPoints(min(w, (int16_t)MAX_POINTS)),
          autoScale(true) {
        calculatePoints();
    }
    
    void setFunction(PlotFunction func) {
        function = func;
        calculatePoints();
        dirty = true;
    }
    
    void setXRange(float min, float max) {
        xMin = min;
        xMax = max;
        calculatePoints();
        dirty = true;
    }
    
    void setYRange(float min, float max) {
        yMin = min;
        yMax = max;
        autoScale = false;
        calculatePoints();
        dirty = true;
    }
    
    void enableAutoScale(bool enable = true) {
        autoScale = enable;
        calculatePoints();
        dirty = true;
    }
    
private:
    void calculatePoints() {
        if (!function) return;
        
        // Calculate points
        float xStep = (xMax - xMin) / (numPoints - 1);
        
        // First pass: calculate Y range if auto-scaling
        if (autoScale) {
            yMin = INFINITY;
            yMax = -INFINITY;
            
            for (uint16_t i = 0; i < numPoints; i++) {
                float x = xMin + i * xStep;
                float y = function(x);
                
                yMin = min(yMin, y);
                yMax = max(yMax, y);
            }
            
            // Add margin to Y range
            float yMargin = (yMax - yMin) * 0.1f;
            yMin -= yMargin;
            yMax += yMargin;
        }
        
        // Second pass: store normalized points
        for (uint16_t i = 0; i < numPoints; i++) {
            points[i].x = xMin + i * xStep;
            points[i].y = function(points[i].x);
        }
    }
    
    int16_t mapToPixelX(float x) const {
        return x + (width - 1) * (x - xMin) / (xMax - xMin);
    }
    
    int16_t mapToPixelY(float y) const {
        return y + height - 1 - (height - 1) * (y - yMin) / (yMax - yMin);
    }
    
public:
    void draw(Adafruit_SSD1306& display) override {
        // Draw border
        display.drawRect(x, y, width, height, WHITE);
        
        // Draw axes if they're within the plot range
        if (yMin <= 0 && yMax >= 0) {
            int16_t yAxis = mapToPixelY(0);
            display.drawFastHLine(x, y + yAxis, width, WHITE);
        }
        
        if (xMin <= 0 && xMax >= 0) {
            int16_t xAxis = mapToPixelX(0);
            display.drawFastVLine(x + xAxis, y, height, WHITE);
        }
        
        // Draw function
        for (uint16_t i = 1; i < numPoints; i++) {
            int16_t x1 = mapToPixelX(points[i-1].x);
            int16_t y1 = mapToPixelY(points[i-1].y);
            int16_t x2 = mapToPixelX(points[i].x);
            int16_t y2 = mapToPixelY(points[i].y);
            
            display.drawLine(x + x1, y + y1, x + x2, y + y2, WHITE);
        }
        
        if (focused) {
            // Draw focus indicator
            for (int16_t i = 0; i < 4; i++) {
                display.drawPixel(x + i, y + i, WHITE);
                display.drawPixel(x + width - 1 - i, y + i, WHITE);
                display.drawPixel(x + i, y + height - 1 - i, WHITE);
                display.drawPixel(x + width - 1 - i, y + height - 1 - i, WHITE);
            }
        }
    }
    
    void handleInput(const InputEvent& event) override {}
    void update() override {}
};

// ... [Rest of the previous code remains the same] ...

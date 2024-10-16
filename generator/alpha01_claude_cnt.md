 Certainly! I'll complete the project by implementing the remaining components. Let's go through each of the missing pieces:

1. src/lib/code-generator.js:



<antArtifact identifier="code-generator" type="application/vnd.ant.code" language="javascript" title="src/lib/code-generator.js">
const ejs = require('ejs');
const path = require('path');
const fs = require('fs').promises;

class CodeGenerator {
  constructor(config) {
    this.config = config;
    this.templateDir = path.join(__dirname, '..', 'templates');
  }

  async generateCode() {
    const mainCode = await this.generateFromTemplate('main.cpp.ejs');
    const widgetCode = await this.generateFromTemplate('Widget.h.ejs');
    const displayCode = await this.generateFromTemplate('Display.h.ejs');

    return {
      'main.cpp': mainCode,
      'Widget.h': widgetCode,
      'Display.h': displayCode
    };
  }

  async generateFromTemplate(templateName) {
    const templatePath = path.join(this.templateDir, templateName);
    const templateContent = await fs.readFile(templatePath, 'utf-8');
    return ejs.render(templateContent, {
      config: this.config,
      widgets: this.config.widgets,
      project: this.config.project
    });
  }
}

module.exports = CodeGenerator;
</antArtifact>

2. src/widgets/text-widget.js:



<antArtifact identifier="text-widget" type="application/vnd.ant.code" language="javascript" title="src/widgets/text-widget.js">
class TextWidget {
  constructor(config) {
    this.name = config.name;
    this.x = config.x;
    this.y = config.y;
    this.width = config.width;
    this.height = config.height;
    this.processCallback = config.processCallback;
    this.displayCallback = config.displayCallback;
  }

  generateSetup() {
    return `
  ${this.name}.setProcessCallback([](Widget* w) {
    ${this.processCallbackCode}
  });
  ${this.name}.setDisplayCallback([](Widget* w, Display* d) {
    ${this.displayCallbackCode}
  });`;
  }

  generateDeclaration() {
    return `Widget ${this.name}(${this.x}, ${this.y}, ${this.width}, ${this.height});`;
  }

  generateProcessCall() {
    return `${this.name}.process();`;
  }

  generateDisplayCall() {
    return `${this.name}.display(&display);`;
  }
}

module.exports = TextWidget;
</antArtifact>

3. src/widgets/graph-widget.js:



<antArtifact identifier="graph-widget" type="application/vnd.ant.code" language="javascript" title="src/widgets/graph-widget.js">
class GraphWidget {
  constructor(config) {
    this.name = config.name;
    this.x = config.x;
    this.y = config.y;
    this.width = config.width;
    this.height = config.height;
    this.processCallback = config.processCallback;
    this.displayCallback = config.displayCallback;
    this.dataPoints = config.dataPoints || 100;
  }

  generateSetup() {
    return `
  ${this.name}.setProcessCallback([](Widget* w) {
    ${this.processCallbackCode}
  });
  ${this.name}.setDisplayCallback([](Widget* w, Display* d) {
    ${this.displayCallbackCode}
  });
  ${this.name}.setDataPoints(${this.dataPoints});`;
  }

  generateDeclaration() {
    return `GraphWidget ${this.name}(${this.x}, ${this.y}, ${this.width}, ${this.height});`;
  }

  generateProcessCall() {
    return `${this.name}.process();`;
  }

  generateDisplayCall() {
    return `${this.name}.display(&display);`;
  }
}

module.exports = GraphWidget;
</antArtifact>

4. src/widgets/gauge-widget.js:



<antArtifact identifier="gauge-widget" type="application/vnd.ant.code" language="javascript" title="src/widgets/gauge-widget.js">
class GaugeWidget {
  constructor(config) {
    this.name = config.name;
    this.x = config.x;
    this.y = config.y;
    this.width = config.width;
    this.height = config.height;
    this.processCallback = config.processCallback;
    this.displayCallback = config.displayCallback;
    this.minValue = config.minValue || 0;
    this.maxValue = config.maxValue || 100;
  }

  generateSetup() {
    return `
  ${this.name}.setProcessCallback([](Widget* w) {
    ${this.processCallbackCode}
  });
  ${this.name}.setDisplayCallback([](Widget* w, Display* d) {
    ${this.displayCallbackCode}
  });
  ${this.name}.setRange(${this.minValue}, ${this.maxValue});`;
  }

  generateDeclaration() {
    return `GaugeWidget ${this.name}(${this.x}, ${this.y}, ${this.width}, ${this.height});`;
  }

  generateProcessCall() {
    return `${this.name}.process();`;
  }

  generateDisplayCall() {
    return `${this.name}.display(&display);`;
  }
}

module.exports = GaugeWidget;
</antArtifact>

5. Update the template files:



<antArtifact identifier="main-cpp-template" type="application/vnd.ant.code" language="cpp" title="src/templates/main.cpp.ejs">
#include <Arduino.h>
#include "Display.h"
#include "Widget.h"

<% widgets.forEach(widget => { %>
<%- widget.generateDeclaration() %>
<% }); %>

Display display;

void setup() {
  Serial.begin(115200);
  display.begin();

  <% widgets.forEach(widget => { %>
  <%- widget.generateSetup() %>
  <% }); %>
}

void loop() {
  <% widgets.forEach(widget => { %>
  <%- widget.generateProcessCall() %>
  <% }); %>

  display.clear();
  <% widgets.forEach(widget => { %>
  <%- widget.generateDisplayCall() %>
  <% }); %>
  display.update();

  delay(100);  // Adjust as needed
}
</antArtifact>

<antArtifact identifier="widget-h-template" type="application/vnd.ant.code" language="cpp" title="src/templates/Widget.h.ejs">
#ifndef WIDGET_H
#define WIDGET_H

#include <functional>
#include "Display.h"

class Widget {
public:
  Widget(int x, int y, int width, int height)
    : x(x), y(y), width(width), height(height) {}

  virtual void setProcessCallback(std::function<void(Widget*)> cb) { processCallback = cb; }
  virtual void setDisplayCallback(std::function<void(Widget*, Display*)> cb) { displayCallback = cb; }

  virtual void process() { if (processCallback) processCallback(this); }
  virtual void display(Display* d) { if (displayCallback) displayCallback(this, d); }

  int x, y, width, height;
  float value;

protected:
  std::function<void(Widget*)> processCallback;
  std::function<void(Widget*, Display*)> displayCallback;
};

class GraphWidget : public Widget {
public:
  GraphWidget(int x, int y, int width, int height)
    : Widget(x, y, width, height), dataPoints(100) {}

  void setDataPoints(int points) { dataPoints = points; }

private:
  int dataPoints;
};

class GaugeWidget : public Widget {
public:
  GaugeWidget(int x, int y, int width, int height)
    : Widget(x, y, width, height), minValue(0), maxValue(100) {}

  void setRange(float min, float max) { minValue = min; maxValue = max; }

private:
  float minValue, maxValue;
};

#endif // WIDGET_H
</antArtifact>

<antArtifact identifier="display-h-template" type="application/vnd.ant.code" language="cpp" title="src/templates/Display.h.ejs">
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_<%=project.display.type%>.h>

class Display {
public:
  Display() : tft(TFT_CS, TFT_DC, TFT_RST) {}

  void begin() {
    tft.init(<%=project.display.height%>, <%=project.display.width%>);
    tft.setRotation(<%=project.display.rotation || 0%>);
    tft.fillScreen(<%=project.display.backgroundColor || 'ST77XX_BLACK'%>);
  }

  void clear() {
    tft.fillScreen(<%=project.display.backgroundColor || 'ST77XX_BLACK'%>);
  }

  void update() {
    // No need to call update for most displays, but you might need this for some
  }

  void setCursor(int16_t x, int16_t y) {
    tft.setCursor(x, y);
  }

  void setTextColor(uint16_t color) {
    tft.setTextColor(color);
  }

  void setTextSize(uint8_t size) {
    tft.setTextSize(size);
  }

  void print(const String &text) {
    tft.print(text);
  }

  void print(float value, int decimals) {
    tft.print(value, decimals);
  }

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    tft.drawLine(x0, y0, x1, y1, color);
  }

  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    tft.fillRect(x, y, w, h, color);
  }

  void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    tft.drawCircle(x0, y0, r, color);
  }

  void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    tft.fillCircle(x0, y0, r, color);
  }

private:
  Adafruit_<%=project.display.type%> tft;
};

#endif // DISPLAY_H
</antArtifact>

6. Update the default configuration:



<antArtifact identifier="default-config" type="application/vnd.ant.code" language="json" title="configs/default-config.json">
{
  "project": {
    "name": "ESP32 Watt Meter",
    "display": {
      "type": "ST7789",
      "width": 240,
      "height": 240,
      "rotation": 2,
      "backgroundColor": "ST77XX_BLACK"
    }
  },
  "widgets": [
    {
      "type": "text",
      "name": "voltageDisplay",
      "x": 10,
      "y": 10,
      "width": 100,
      "height": 30,
      "processCallback": "voltageProcess",
      "displayCallback": "voltageDisplay"
    },
    {
      "type": "text",
      "name": "currentDisplay",
      "x": 10,
      "y": 50,
      "width": 100,
      "height": 30,
      "processCallback": "currentProcess",
      "displayCallback": "currentDisplay"
    },
    {
      "type": "text",
      "name": "wattDisplay",
      "x": 10,
      "y": 90,
      "width": 100,
      "height": 30,
      "processCallback": "wattProcess",
      "displayCallback": "wattDisplay"
    },
    {
      "type": "graph",
      "name": "wattGraph",
      "x": 120,
      "y": 10,
      "width": 110,
      "height": 110,
      "processCallback": "graphProcess",
      "displayCallback": "graphDisplay",
      "dataPoints": 100
    },
    {
      "type": "gauge",
      "name": "wattGauge",
      "x": 10,
      "y": 130,
      "width": 100,
      "height": 100,
      "processCallback": "gaugeProcess",
      "displayCallback": "gaugeDisplay",
      "minValue": 0,
      "maxValue": 1000
    }
  ]
}
</antArtifact>

7. Create callback files:



<antArtifact identifier="voltage-callbacks" type="application/vnd.ant.code" language="javascript" title="callbacks/voltage-callbacks.js">
module.exports = {
  voltageProcess: `
    float voltage = analogRead(VOLTAGE_PIN) * (3.3 / 4095.0) * VOLTAGE_DIVIDER_RATIO;
    w->value = voltage;
  `,
  voltageDisplay: `
    d->setCursor(w->x, w->y);
    d->setTextColor(ST77XX_WHITE);
    d->setTextSize(2);
    d->print("Voltage: ");
    d->print(w->value, 2);
    d->print("V");
  `
};
</antArtifact>

<antArtifact identifier="current-callbacks" type="application/vnd.ant.code" language="javascript" title="callbacks/current-callbacks.js">
module.exports = {
  currentProcess: `
    float current = analogRead(CURRENT_PIN) * (3.3 / 4095.0) / CURRENT_SENSOR_SENSITIVITY;
    w->value = current;
  `,
  currentDisplay: `
    d->setCursor(w->x, w->y);
    d->setTextColor(ST77XX_WHITE);
    d->setTextSize(2);
    d->print("Current: ");
    d->print(w->value, 2);
    d->print("A");
  `
};
</antArtifact>


To improve the system so that each widget's class definition and callback logic are contained in individual HTML files, we can modify the approach to:

1. Include widget class definitions directly in the HTML files.


2. Update the Python transpiler to:

Extract widget class definitions.

Merge all widget classes and callbacks into a Widgets.h file that will be included in the final Arduino code.




Example HTML Files with Class Definitions

Each HTML file will now contain not just widget parameters and callbacks but also class definitions specific to that widget.

text_widget.html

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Text Widget</title>
</head>
<body>
    <!-- Widget Class Definition -->
    <classDefinition>
    class TextWidget : public Widget {
    private:
        const char* label;
        std::function<float()> dataSource;
        float value;

    public:
        TextWidget(Display* _display, int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency, const char* _label, std::function<float()> _dataSource)
            : Widget(_display, _x, _y, _w, _h, _processFrequency), label(_label), dataSource(_dataSource), value(0) {}

        void displayWidget() override {
            display->setCursor(x, y);
            display->setTextColor(COLOR_TEXT);
            display->setTextSize(2);
            display->fillRect(x, y, width, height, COLOR_BG);
            display->print(label);
            display->print(": ");
            display->print(value, 2);
        }

    protected:
        void processLogic() override {
            value = dataSource();
        }
    };
    </classDefinition>

    <!-- Widget Parameters -->
    <widget type="text">
        <parameters>
            <x>10</x>
            <y>20</y>
            <width>100</width>
            <height>30</height>
            <refreshRate>1000</refreshRate>
            <label>Voltage</label>
        </parameters>

        <!-- Callback Function -->
        <callback>
            void onVoltageCallback() {
                Serial.println("Voltage widget callback");
                // Add logic to update voltage display
            }
        </callback>
    </widget>
</body>
</html>

button_widget.html

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Button Widget</title>
</head>
<body>
    <!-- Widget Class Definition -->
    <classDefinition>
    class ButtonWidget : public Widget {
    private:
        const char* label;

    public:
        ButtonWidget(Display* _display, int16_t _x, int16_t _y, int16_t _w, int16_t _h, uint16_t _processFrequency, const char* _label)
            : Widget(_display, _x, _y, _w, _h, _processFrequency), label(_label) {}

        void displayWidget() override {
            display->setCursor(x, y);
            display->setTextColor(COLOR_TEXT);
            display->setTextSize(2);
            display->fillRect(x, y, width, height, COLOR_BG);
            display->print(label);
        }

        void onPress() {
            // Handle button press
        }
    };
    </classDefinition>

    <!-- Widget Parameters -->
    <widget type="button">
        <parameters>
            <x>20</x>
            <y>40</y>
            <width>150</width>
            <height>40</height>
            <label>Start</label>
        </parameters>

        <!-- Callback Function -->
        <callback>
            void onStartButtonCallback() {
                Serial.println("Start button pressed");
                // Add logic to start an action
            }
        </callback>
    </widget>
</body>
</html>

Python Code Update

The Python code will now handle the following:

1. Parse widget class definitions from each HTML file.


2. Extract widget parameters, callback logic, and class definitions.


3. Combine all class definitions into a single Widgets.h file.



Here’s how the updated Python code looks:

import os
from bs4 import BeautifulSoup

# Directory containing widget HTML files
widgets_directory = "./widgets"
output_header_file = "./src/Widgets.h"

# Function to extract widget class definitions and callback code
def parse_widget_html(file_path):
    with open(file_path, "r") as file:
        soup = BeautifulSoup(file, "html.parser")
        
        # Extract class definition
        class_definition = soup.find("classdefinition").get_text()
        
        # Extract callback code
        callback_code = soup.find("callback").get_text()

        return class_definition, callback_code

# Function to generate the Widgets.h file
def generate_widgets_header(widgets_dir, output_file):
    with open(output_file, "w") as output:
        # Write the header guard
        output.write("#ifndef WIDGETS_H\n")
        output.write("#define WIDGETS_H\n\n")
        output.write('#include "Display.h"\n\n')

        # Parse each widget HTML file
        for file_name in os.listdir(widgets_dir):
            if file_name.endswith(".html"):
                file_path = os.path.join(widgets_dir, file_name)
                class_def, callback = parse_widget_html(file_path)

                # Write the class definition and callback to the .h file
                output.write(class_def + "\n\n")
                output.write(callback + "\n\n")

        # End the header guard
        output.write("#endif // WIDGETS_H\n")

# Run the script
generate_widgets_header(widgets_directory, output_header_file)

Explanation of the Updated Python Code:

Class Definition Extraction: The script uses BeautifulSoup to parse the HTML files and extract widget class definitions (from the <classdefinition> tag) and callback functions (from the <callback> tag).

Combining into a Header File: Each class definition and callback code is combined into a single Widgets.h file, which is then included in your main project.


Directory Structure:

Here’s the updated directory structure with everything combined into a single Widgets.h file.

project_root/
├── src/
│   ├── LayoutManager.h
│   ├── Widgets.h         <-- Generated by the Python script
│   └── ...
├── widgets/
│   ├── text_widget.html
│   ├── button_widget.html
│   ├── graph_widget.html
│   └── ...
├── main.ino
└── code_generator.py     <-- Updated Python script

Conclusion:

Each widget’s class definition and callback logic are now encapsulated in its respective HTML file.

The Python script compiles all widget definitions and callbacks into a Widgets.h file, which will be used in your Arduino project.

This structure keeps the widget logic modular and easily modifiable while automatically generating the necessary code for your project.



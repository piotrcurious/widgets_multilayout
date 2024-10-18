To complete the project, we’ll generate a C++ Arduino code structure with separate files for widget management, layout handling, and the main program. This structure will make it modular, allowing easier management of widget types and layouts. Here’s how we can organize it:

Final Folder Structure:

/project-root/
    /widgets/                   # Directory containing widget HTML files
    /layout/                    # Directory containing layout HTML files
    /generated-code/            # Output directory for generated Arduino code
    /src/                       # Source code directory for supporting files
        WidgetBase.h            # Base widget class
        Widgets.h               # Derived widget classes
        LayoutManager.h         # Layout manager handling layout files
    main.ino                    # Main Arduino program
    code_generator.py           # Python script for generating C++ code

Step 1: Refactoring the Python Code for Modular Generation

We will refactor the Python code so that it generates files separately for widget classes, the layout manager, and the main Arduino program.

1.1 Widget Parsing and Code Generation (Refactored)

We’ll move the logic for parsing and generating code for each widget type into separate functions, so it’s easier to manage.

import os
from bs4 import BeautifulSoup

# Directory to store generated code
output_dir = "./generated-code"

def parse_widget_file(widget_file):
    with open(widget_file, 'r') as file:
        soup = BeautifulSoup(file, 'html.parser')
        
        widget = soup.find('img')
        style = widget['style']
        href = widget.get('href', '').replace('#', '')  # Extract callback ID
        
        callback_section = soup.find('section', id=href)
        callback_code = callback_section.text.strip() if callback_section else "null"
        
        widget_type = determine_widget_type(widget)
        
        return {
            'type': widget_type,
            'style': parse_style(style),
            'callback_code': callback_code
        }

def determine_widget_type(widget):
    if 'class' in widget.attrs:
        widget_class = widget['class'][0]
        if "graph" in widget_class:
            return "graph"
        elif "button" in widget_class:
            return "button"
        elif "image" in widget_class:
            return "image"
        elif "slider" in widget_class:
            return "slider"
    return "text"

def parse_style(style_str):
    styles = {}
    for item in style_str.split(';'):
        if item.strip():
            key, value = item.split(':')
            styles[key.strip()] = value.strip()
    return styles

def process_all_widgets(widgets_directory):
    widget_code_snippets = []
    
    for widget_file in os.listdir(widgets_directory):
        if widget_file.endswith(".html"):
            widget_path = os.path.join(widgets_directory, widget_file)
            widget_data = parse_widget_file(widget_path)

            # Extract widget attributes
            widget_type = widget_data['type']
            style = widget_data['style']
            callback_code = widget_data['callback_code']
            x = style.get('left', '0').replace('px', '')
            y = style.get('top', '0').replace('px', '')
            width = style.get('width', '100').replace('px', '')
            height = style.get('height', '30').replace('px', '')
            
            widget_code = generate_widget_code({
                'type': widget_type,
                'x': x,
                'y': y,
                'width': width,
                'height': height,
                'callback': callback_code
            })
            widget_code_snippets.append(widget_code)
    
    return widget_code_snippets

def generate_widget_code(widget_data):
    x = widget_data['x']
    y = widget_data['y']
    width = widget_data['width']
    height = widget_data['height']
    callback_code = widget_data['callback']
    widget_type = widget_data['type']
    
    label = f"Widget{widget_type}"

    if widget_type == "text":
        widget_code = f"""
        TextWidget {label}(display, {x}, {y}, {width}, {height}, 1000, "{label}", {callback_code});
        """
    elif widget_type == "graph":
        widget_code = f"""
        GraphWidget {label}(display, {x}, {y}, {width}, {height}, 1000, {callback_code});
        """
    elif widget_type == "button":
        widget_code = f"""
        ButtonWidget {label}(display, {x}, {y}, {width}, {height}, 1000, "{label}", {callback_code});
        """
    elif widget_type == "image":
        widget_code = f"""
        ImageWidget {label}(display, {x}, {y}, {width}, {height}, "{label}");
        """
    elif widget_type == "slider":
        widget_code = f"""
        SliderWidget {label}(display, {x}, {y}, {width}, {height}, 1000, {callback_code});
        """
    else:
        widget_code = "// Unknown widget type"
    
    return widget_code

1.2 Processing Layout Files

We’ll ensure layout files are processed separately, and widget references are resolved dynamically.

def process_layout_file(layout_file, widgets_code):
    with open(layout_file, 'r') as file:
        soup = BeautifulSoup(file, 'html.parser')
        
        layout_widgets = soup.find_all('img')
        layout_code_snippets = []
        
        for widget in layout_widgets:
            widget_id = widget['src'].replace(".png", "")
            for widget_code in widgets_code:
                if f"Widget{widget_id}" in widget_code:
                    layout_code_snippets.append(widget_code)
                    break
        
        return layout_code_snippets

1.3 Code Generator Execution

This section combines widget parsing and layout generation and writes the resulting code to different C++ files.

def generate_code_structure():
    widgets_directory = "./widgets"
    layouts_directory = "./layout"
    
    # Step 1: Process all widgets and generate widget code
    widgets_code = process_all_widgets(widgets_directory)

    # Step 2: Process layout files and integrate widget code
    layout_code = []
    for layout_file in os.listdir(layouts_directory):
        if layout_file.endswith(".html"):
            layout_path = os.path.join(layouts_directory, layout_file)
            layout_code.extend(process_layout_file(layout_path, widgets_code))
    
    # Step 3: Generate output C++ code files
    generate_main_ino(layout_code)
    generate_widget_header()
    generate_layout_manager()
    
def generate_main_ino(layout_code):
    with open(os.path.join(output_dir, "main.ino"), "w") as main_file:
        main_file.write("""
#include "src/Widgets.h"
#include "src/LayoutManager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// Initialize display
Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
    Serial.begin(115200);
    display.begin();
    display.setRotation(3);

    // Initialize layouts and widgets
""")
        main_file.write("\n".join(layout_code))
        main_file.write("""
}

void loop() {
    // Callbacks and UI updates happen here
}
""")

def generate_widget_header():
    with open(os.path.join(output_dir, "src/Widgets.h"), "w") as widget_header:
        widget_header.write("""
#ifndef WIDGETS_H
#define WIDGETS_H

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

class TextWidget {
public:
    TextWidget(Adafruit_ILI9341 &display, int x, int y, int width, int height, int refreshRate, const char *label, void (*callback)()) {
        // Widget initialization code
    }
};

class GraphWidget {
public:
    GraphWidget(Adafruit_ILI9341 &display, int x, int y, int width, int height, int refreshRate, void (*callback)()) {
        // Widget initialization code
    }
};

class ButtonWidget {
public:
    ButtonWidget(Adafruit_ILI9341 &display, int x, int y, int width, int height, int refreshRate, const char *label, void (*callback)()) {
        // Widget initialization code
    }
};

// Add more widget types as needed

#endif // WIDGETS_H
""")

def generate_layout_manager():
    with open(os.path.join(output_dir, "src/LayoutManager.h"), "w") as layout_manager:
        layout_manager.write("""
#ifndef LAYOUT_MANAGER_H
#define LAYOUT_MANAGER_H

class LayoutManager {
public:
    void loadLayout(int layoutNumber) {
        // Layout management logic based on layoutNumber
    }
};

#endif // LAYOUT_MANAGER_H
""")

Step 2: Code Output Files

When the code_generator.py is run, it will generate the following files:

main.ino: The main Arduino program that initializes the display, loads widgets, and handles the loop.

Widgets.h: Header file defining all widget types like TextWidget, GraphWidget, etc.

LayoutManager.h: Manages layout switching and widget updates.


Example main.ino

#include "src/Widgets.h"
#include "src/LayoutManager.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
    Serial.begin(115200);
    display.begin();
    display.setRotation(3);


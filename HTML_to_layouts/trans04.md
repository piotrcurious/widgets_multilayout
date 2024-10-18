To improve widget type management, we can source each widget definition from separate HTML files stored in a widgets directory. Each widget file will define its parameters (such as size, position, and styling) and associated callback logic. The Arduino code generator will then parse all these widget files and generate appropriate widget instances, making the system modular and easier to maintain.

Key Objectives:

1. Separate HTML files for each widget: Each file contains the widget’s attributes, such as dimensions, position, and callback logic.


2. Dynamically generate code for all widgets in the widgets directory.


3. Each widget file should define its own callback code, parameters, and type.



Step 1: Folder Structure

Let's assume we have the following folder structure:

/layout/
    layout1.html
    layout2.html
/widgets/
    button_widget.html
    graph_widget.html
    text_widget.html
    slider_widget.html

Each widget HTML file will describe a specific widget, and the layout HTML files will determine where those widgets are placed on the display.

Step 2: Example Widget HTML Files

Here's how you might define the widget HTML files inside the /widgets/ directory:

button_widget.html

<!DOCTYPE html>
<html>
<head>
    <style>
        left: 20px;
        top: 50px;
        width: 100px;
        height: 30px;
    </style>
</head>
<body>
    <img class="button" src="button_widget.png" href="#button_callback" />
    <section id="button_callback">
        void button_callback() {
            // Code to handle button press
            Serial.println("Button pressed");
        }
    </section>
</body>
</html>

graph_widget.html

<!DOCTYPE html>
<html>
<head>
    <style>
        left: 10px;
        top: 100px;
        width: 200px;
        height: 150px;
    </style>
</head>
<body>
    <img class="graph" src="graph_widget.png" href="#graph_callback" />
    <section id="graph_callback">
        void graph_callback() {
            // Code to update graph data
            Serial.println("Graph updated");
        }
    </section>
</body>
</html>

Step 3: Modify Code to Process Widget Files

The code generator will now search the widgets directory for each widget HTML file and extract parameters and callbacks for each widget type.

Here’s how we can extend the code:

Function to Parse Widget Files

import os
from bs4 import BeautifulSoup

def parse_widget_file(widget_file):
    with open(widget_file, 'r') as file:
        soup = BeautifulSoup(file, 'html.parser')
        
        # Extract widget parameters from the style attribute
        widget = soup.find('img')
        style = widget['style']
        href = widget.get('href', '').replace('#', '')  # Extract callback ID
        
        # Parse callback code
        callback_section = soup.find('section', id=href)
        callback_code = callback_section.text.strip() if callback_section else "null"
        
        # Extract widget type from the class attribute
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
    # Parses inline style attributes into a dictionary
    styles = {}
    for item in style_str.split(';'):
        if item.strip():
            key, value = item.split(':')
            styles[key.strip()] = value.strip()
    return styles

Function to Process All Widget Files

The following function will load all the widgets from the widgets directory and generate Arduino code for each one:

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
            
            # Generate widget code based on the type
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

Generate Widget Code Based on Type

We can modify the existing generate_widget_code to dynamically handle parameters and callbacks for different widget types:

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

Step 4: Process Layout Files and Widgets Together

Each layout file (e.g., layout1.html) will be processed to specify where and how widgets are placed. The layout will reference widgets by name (e.g., Widgetbutton_widget.png), and the widgets will be loaded from the widgets directory.

Here’s the function to handle layout files:

def process_layout_file(layout_file, widgets_code):
    with open(layout_file, 'r') as file:
        soup = BeautifulSoup(file, 'html.parser')
        
        layout_widgets = soup.find_all('img')
        layout_code_snippets = []
        
        for widget in layout_widgets:
            # Match the widget with the generated code
            widget_id = widget['src'].replace(".png", "")
            for widget_code in widgets_code:
                if f"Widget{widget_id}" in widget_code:
                    layout_code_snippets.append(widget_code)
                    break
        
        return layout_code_snippets

Step 5: Run the System

After everything is set up, we can combine the widget and layout processing:

# Process widgets from the widgets directory
widgets_directory = "./widgets"
widgets_code = process_all_widgets(widgets_directory)

# Process layout files and generate the final Arduino code
layouts_directory = "./layout"
for layout_file in os.listdir(layouts_directory):
    if layout_file.endswith(".html"):
        layout_path = os.path.join(layouts_directory, layout_file)
        layout_code = process_layout_file(layout_path, widgets_code)
        
        # Write or print the final code
        with open(f"generated_code_{layout_file.replace('.html', '')}.ino", "w") as code_file:
            code_file.write("\n".join(layout_code))

Summary of Enhancements:

Modular Widgets: Each widget is defined in its own HTML file with parameters and callback logic, making it easier to manage different widgets.

Layout and Widget Separation: Layout files determine widget placement, while widget files focus on the behavior and display specifics.

Dynamic Code Generation: The system dynamically generates Arduino code for all widgets and layouts, allowing easy updates by modifying individual widget files.


Would you like to explore additional improvements or test specific widget behaviors?


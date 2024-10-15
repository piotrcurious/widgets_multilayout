# widgets_multilayout
AI generated multi-layout widgets system with callbacks 
Here's a prompt that can generate Arduino ESP32 code for the system you described:


---

Generate an Arduino ESP32 program that implements a widget-based display system using the Adafruit GFX library as the backend. The system should support multiple sets of widgets, defined statically using structures, where each widget is updated via dual callbacks:

1. Process Callback: Runs at a specific frequency, defined per widget, to handle the process logic.


2. Display Callback: Renders the output from the process callback onto the display.



The system should support multiple layouts of statically defined widgets, which can be switched on demand. No window frames or buttons are to be used, but the system should support optional decorations and frames for each widget.

The program should also include:

A single global static color scheme for all widgets.

Dynamic parameters (variables) for each widget, like process frequency, display data, and position, while the structure definitions remain static.

Options to decorate each widget with borders, backgrounds, or text styles based on configuration.

Support for dynamic layout switching.


Requirements:

The layout and widgets should be statically defined, but each widget's data and parameters should be handled dynamically.

Use the Adafruit GFX library for rendering.

Implement dual callback functionality per widget (one for process logic and one for display updates).


The display system should be structured as follows:

Widget structure: Define each widget statically, including position, size, and callback functions.

Layouts: Support multiple widget layouts, which can be switched programmatically.

Color scheme: Only one global color scheme, applied uniformly across all layouts.


Generate code that represents the core functionality of this system.


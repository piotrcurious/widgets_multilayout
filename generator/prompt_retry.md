Here's a compact version of your request for retrying:


---

Prompt:

Generate a Haskell-based graphical designer and code generator for an Arduino ESP32 watt meter project, using the Adafruit GFX library. The system should dynamically generate Arduino code based on user-defined layouts and widgets, specified via text files or a configuration (JSON/YAML). The following are key requirements:

1. Widget and Layout System:

Widgets and layouts should be defined by the user in configuration files, not hardcoded.

Each widget has dual callbacks: a process callback for computation (e.g., watts, volts, etc.) and a display callback for rendering the widget.

The system should support background data processing, even if a widget isn't currently displayed.



2. Dynamic Code Generation:

The Haskell code should dynamically parse project settings and generate valid Arduino code (C++).

The generated Arduino code should use classes and wrapper functions to make swapping the display library easy.



3. Project Settings Management:

The tool should allow saving and loading project settings.

Separate callback function definitions should be allowed via text files.




Generate Haskell code that implements this system and provides example callback files for the watt meter project, including watt-hour integration and graph updates.


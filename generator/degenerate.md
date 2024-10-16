Thank you for clarifying! I see now that you're asking for a code generator written in Haskell that can produce the Arduino C++ code dynamically, based on user-defined layouts and widgets. This generator would take user input (such as a text file or project settings) and generate Arduino code for an ESP32-based watt meter with different layouts, while still allowing callbacks to be defined separately.

Let me improve the Haskell code generator to:

1. Dynamically generate Arduino code (C++) based on a user's project settings or a configuration file.


2. Allow the user to define widgets, layouts, and callbacks via configuration files.


3. Support the option to store and retrieve project settings (perhaps in JSON or YAML).


4. Implement the callbacks in separate files, so that they can be edited independently from the layout definitions.




---

Haskell Code Generator (Refined)

The improved Haskell code will:

Parse project settings from a configuration file.

Generate the required Arduino code (C++) based on these settings.

Separate callbacks into independent files.

Allow the system to save and load project settings.


{-# LANGUAGE OverloadedStrings #-}

import Data.Aeson (decode, encode, FromJSON, ToJSON)
import qualified Data.ByteString.Lazy as B
import System.Directory (doesFileExist)
import System.IO

-- Project structure definition
data Widget = Widget {
    widgetName :: String,
    widgetX :: Int,
    widgetY :: Int,
    widgetWidth :: Int,
    widgetHeight :: Int,
    widgetProcessCallback :: String,
    widgetDisplayCallback :: String
} deriving (Show, Read, Eq)

data Layout = Layout {
    layoutName :: String,
    layoutWidgets :: [Widget]
} deriving (Show, Read, Eq)

data Project = Project {
    projectName :: String,
    projectLayouts :: [Layout],
    projectGlobalColorScheme :: String
} deriving (Show, Read, Eq)

instance FromJSON Widget
instance ToJSON Widget
instance FromJSON Layout
instance ToJSON Layout
instance FromJSON Project
instance ToJSON Project

-- Default file paths
settingsFilePath :: FilePath
settingsFilePath = "project_settings.json"

-- Load project settings
loadProjectSettings :: IO (Maybe Project)
loadProjectSettings = do
    exists <- doesFileExist settingsFilePath
    if exists
      then do
        content <- B.readFile settingsFilePath
        return (decode content)
      else return Nothing

-- Save project settings
saveProjectSettings :: Project -> IO ()
saveProjectSettings project = B.writeFile settingsFilePath (encode project)

-- Generate Arduino Code
generateArduinoCode :: Project -> String
generateArduinoCode project =
    let header = unlines [
            "#include <Adafruit_GFX.h>",
            "#include <Adafruit_ILI9341.h>",
            "Adafruit_ILI9341 display = Adafruit_ILI9341(10, 9);",
            "void setup() {",
            "  display.begin();",
            "  display.fillScreen(0x0000); // Black background",
            "}"
          ]
        loopHeader = "void loop() {\n"
        loopFooter = "  delay(100);\n}\n"
        layoutsCode = concatMap generateLayoutCode (projectLayouts project)
    in header ++ loopHeader ++ layoutsCode ++ loopFooter

-- Generate Layout Code
generateLayoutCode :: Layout -> String
generateLayoutCode layout =
    let widgetCode = concatMap generateWidgetCode (layoutWidgets layout)
    in "  // Layout: " ++ layoutName layout ++ "\n" ++ widgetCode

-- Generate Widget Code
generateWidgetCode :: Widget -> String
generateWidgetCode widget =
    let processCode = "  " ++ widgetProcessCallback widget ++ "(millis());\n"
        displayCode = "  " ++ widgetDisplayCallback widget ++ "();\n"
    in "  // Widget: " ++ widgetName widget ++ "\n" ++ processCode ++ displayCode

-- Main function to either generate code or create a new project
main :: IO ()
main = do
    putStrLn "Loading project settings..."
    projectSettings <- loadProjectSettings
    case projectSettings of
        Just project -> do
            putStrLn "Project loaded. Generating code..."
            let arduinoCode = generateArduinoCode project
            writeFile (projectName project ++ ".ino") arduinoCode
            putStrLn ("Arduino code generated: " ++ projectName project ++ ".ino")
        Nothing -> do
            putStrLn "No project settings found. Creating a new project."
            let defaultProject = Project {
                projectName = "WattMeterProject",
                projectLayouts = [
                    Layout {
                        layoutName = "Layout1",
                        layoutWidgets = [
                            Widget "WattsWidget" 10 10 100 50 "watts_process" "watts_display",
                            Widget "VoltsWidget" 10 40 100 50 "volts_process" "volts_display"
                        ]
                    },
                    Layout {
                        layoutName = "Layout2",
                        layoutWidgets = [
                            Widget "WattsWidget" 10 10 100 50 "watts_process" "watts_display",
                            Widget "AmpsWidget" 10 40 100 50 "amps_process" "amps_display"
                        ]
                    }
                ],
                projectGlobalColorScheme = "0xFFFF"
            }
            saveProjectSettings defaultProject
            putStrLn "Default project created and saved."


---

Detailed Explanation

1. Project Settings & Widgets

The user defines their widgets (e.g., watts, volts) and their respective callbacks (watts_process, watts_display).

The Project structure contains layouts, each with a set of widgets. These are stored in a configuration file (project_settings.json).

The user can manually edit this JSON file to add new layouts or widgets, and their corresponding callbacks.


2. Arduino Code Generation

The Haskell code dynamically reads from the project configuration, and based on the defined widgets and layouts, it generates the corresponding Arduino C++ code.

Each widget has a processCallback and a displayCallback, which are inserted into the Arduino code.

The layout code is generated by iterating over each layout and inserting the necessary widget code into the loop() function.


3. File Handling

Loading and Saving Project Settings: The settings are saved in JSON format for easy modification.

Generated Callback Files: Callbacks (e.g., watts_process, volts_display) are defined separately and linked to the widgets. These callback function definitions can be provided in separate .cpp files.



---

Example Project Settings (JSON)

Here’s an example of what the user’s project settings file (project_settings.json) might look like:

{
  "projectName": "WattMeterProject",
  "projectLayouts": [
    {
      "layoutName": "Layout1",
      "layoutWidgets": [
        {
          "widgetName": "WattsWidget",
          "widgetX": 10,
          "widgetY": 10,
          "widgetWidth": 100,
          "widgetHeight": 50,
          "widgetProcessCallback": "watts_process",
          "widgetDisplayCallback": "watts_display"
        },
        {
          "widgetName": "VoltsWidget",
          "widgetX": 10,
          "widgetY": 40,
          "widgetWidth": 100,
          "widgetHeight": 50,
          "widgetProcessCallback": "volts_process",
          "widgetDisplayCallback": "volts_display"
        }
      ]
    },
    {
      "layoutName": "Layout2",
      "layoutWidgets": [
        {
          "widgetName": "WattsWidget",
          "widgetX": 10,
          "widgetY": 10,
          "widgetWidth": 100,
          "widgetHeight": 50,
          "widgetProcessCallback": "watts_process",
          "widgetDisplayCallback": "watts_display"
        },
        {
          "widgetName": "AmpsWidget",
          "widgetX": 10,
          "widgetY": 40,
          "widgetWidth": 100,
          "widgetHeight": 50,
          "widgetProcessCallback": "amps_process",
          "widgetDisplayCallback": "amps_display"
        }
      ]
    }
  ],
  "projectGlobalColorScheme": "0xFFFF"
}


---

Next Steps for the User

Edit the JSON configuration file to define their own layouts and widgets.

Generate Arduino C++ code by running the Haskell program (main).

Separate callback implementations can be stored in external .cpp files (e.g., callbacks.cpp).


This approach allows flexibility in defining projects, as the user doesn't need to hardcode layouts directly in the Haskell generator. They can modify the project settings and regenerate the Arduino code anytime.


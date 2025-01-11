// color_theme.h
// Maps functional UI element names to Spanish autumn-inspired color palette

#ifndef COLOR_THEME_H
#define COLOR_THEME_H

#include "palette_spain_otono.h"

// Background Colors
#define COLOR_BG                NEGRO_VOLCANICO    // Dark background for good contrast
#define COLOR_WIDGET_BG        GRIS_SIERRA        // Slightly lighter background for widgets

// Text Colors
#define COLOR_TEXT             AMARILLO_CAMPOS    // Main text color - golden wheat for readability
#define COLOR_TEXT_SECONDARY   CENIZA            // Secondary text - ash gray
#define COLOR_TEXT_HIGHLIGHT   NARANJA_SEVILLA   // Highlighted text - vibrant orange

// Graph Colors
#define COLOR_GRAPH            AZUL_ALHAMBRA     // Main graph line color
#define COLOR_GRAPH_GRID       GRIS_CAPITAL      // Graph grid lines
#define COLOR_GRAPH_AXIS       GRIS_CAMINO       // Graph axes
#define COLOR_GRAPH_HIGHLIGHT  ROJO_VERMELL      // Graph highlights/peaks

// Alert Colors
#define COLOR_WARNING          ROJO_TINTO        // Warning indicators
#define COLOR_SUCCESS          VERDE_BOSQUE      // Success indicators
#define COLOR_ERROR           ROJO_LAVA         // Error indicators

// Widget Border Colors
#define COLOR_BORDER          GRIS_MONTJUIC     // Widget borders
#define COLOR_BORDER_ACTIVE   AZUL_MEDITERRANEO // Active widget borders

// Value Indicators
#define COLOR_VALUE_NORMAL    VERDE_OLIVA       // Normal range values
#define COLOR_VALUE_HIGH      ROJO_CORDOBA      // High range values
#define COLOR_VALUE_LOW       AZUL_FISTERRE     // Low range values

// Notes:
// - Colors chosen for optimal contrast and readability
// - Dark background (NEGRO_VOLCANICO) provides good visibility for data
// - Text colors selected for clear hierarchy and readability
// - Graph colors chosen to be distinct from background and text
// - Alert colors follow conventional color meanings

#endif // COLOR_THEME_H

#ifndef EDITOR_TOOLS_H
#define EDITOR_TOOLS_H

#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include <stdbool.h>

typedef enum {
    TOOL_NONE,
    TOOL_ARROW,
    TOOL_RECTANGLE,
    TOOL_ELLIPSE,
    TOOL_TEXT,
    TOOL_FREEHAND
} ToolType;

// Font settings for text tool
typedef struct {
    char* family;        // Font family name
    double size;         // Font size in points
    bool is_bold;        // Bold style
    bool is_italic;      // Italic style
} FontSettings;

typedef struct {
    ToolType type;
    GdkRGBA color;
    double line_width;
    bool fill;
    FontSettings font;   // Font settings for text tool
} ToolSettings;

typedef struct {
    int x1, y1;
    int x2, y2;
} PointPair;

typedef struct {
    PointPair points[1000]; // For freehand drawing
    int point_count;
} FreehandPath;

typedef struct {
    ToolType type;
    ToolSettings settings;
    PointPair bounds;
    char* text; // For text tool
    FreehandPath path; // For freehand tool
} Annotation;

// Initialize tool settings
void tool_settings_init(ToolSettings* settings);

// Create a new annotation
Annotation* annotation_create(ToolType type, ToolSettings* settings);

// Draw an annotation
void annotation_draw(Annotation* annotation, cairo_t* cr);

// Free an annotation
void annotation_free(Annotation* annotation);

#endif // EDITOR_TOOLS_H 
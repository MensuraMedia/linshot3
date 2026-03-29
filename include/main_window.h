#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "screenshot_history.h"
#include "editor_tools.h"

typedef struct {
    GtkWidget* window;
    GtkWidget* toolbar;
    GtkWidget* canvas;
    GtkWidget* statusbar;
    GtkWidget* history_flow_box;  // Flow box for history thumbnails
    GtkStatusIcon* tray_icon;     // System tray icon
    ScreenshotHistory screenshot_history;
    bool minimize_to_tray;        // Whether to minimize to tray on close
} MainWindow;

typedef struct {
    MainWindow win;
    cairo_surface_t* current_image;
    ToolSettings current_tool;
    GList* annotations;       // Current annotations
    GList* undo_stack;       // Stack of removed annotations for undo
    bool drawing;
    PointPair start_point;
    Annotation* selected_text;  // Currently selected text annotation
    double drag_start_x;       // Starting point for text dragging
    double drag_start_y;
    bool capture_on_ready;     // Auto-capture after window is ready
    char* current_filename;    // Original capture filename (for save-as sequencing)
    int save_sequence;         // Next _N suffix for save
    // Marquee selection state
    bool has_marquee;          // Whether a marquee selection is active
    PointPair marquee_bounds;  // Marquee selection rectangle
    // Paste overlay state
    cairo_surface_t* paste_overlay;  // Pasted image floating on canvas
    double paste_x;            // Overlay position
    double paste_y;
    bool dragging_paste;       // Currently dragging the paste overlay
    double paste_drag_ox;      // Drag offset from overlay origin
    double paste_drag_oy;
} MainWindowData;

// Initialize and show the main window
bool main_window_init(MainWindow* win, int argc, char* argv[]);

// Clean up resources
void main_window_cleanup(MainWindow* win);

// Trigger a capture (callable from outside, e.g. from signal handler)
void main_window_trigger_capture(MainWindow* win);

#endif // MAIN_WINDOW_H 
#include "../include/capture_overlay.h"
#include "../include/crosshair_drawer.h"
#include <stdio.h>
#include <gdk/gdkx.h>  // For X11-specific window properties

static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data);
static gboolean on_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data);
static gboolean on_button_release(GtkWidget* widget, GdkEventButton* event, gpointer data);
static gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer data);
static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data);

bool capture_overlay_init(CaptureOverlay* overlay) {
    GdkScreen* screen;
    GdkVisual* visual;
    GdkDisplay* display;
    GdkMonitor* primary_monitor;
    GdkRectangle geometry;
    
    // Initialize screen capture but don't close it
    if (!capture_init()) {
        fprintf(stderr, "Failed to initialize screen capture\n");
        return false;
    }
    
    // Get the screen dimensions using modern GTK3 methods
    display = gdk_display_get_default();
    if (!display) {
        fprintf(stderr, "Failed to get default display\n");
        capture_cleanup();
        return false;
    }
    
    primary_monitor = gdk_display_get_primary_monitor(display);
    if (!primary_monitor) {
        fprintf(stderr, "Failed to get primary monitor\n");
        capture_cleanup();
        return false;
    }
    
    gdk_monitor_get_geometry(primary_monitor, &geometry);
    
    // Create an undecorated window
    overlay->window = gtk_window_new(GTK_WINDOW_POPUP);  // Use POPUP instead of TOPLEVEL
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(overlay->window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(overlay->window), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(overlay->window), TRUE);
    
    // Set up window geometry constraints
    GdkGeometry window_geometry;
    window_geometry.min_width = 1;
    window_geometry.min_height = 1;
    window_geometry.max_width = geometry.width;
    window_geometry.max_height = geometry.height;
    gtk_window_set_geometry_hints(GTK_WINDOW(overlay->window), NULL, &window_geometry, 
                                GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE);
    
    // Make the window transparent
    gtk_widget_set_app_paintable(overlay->window, TRUE);
    screen = gdk_display_get_default_screen(display);
    if (screen) {
        visual = gdk_screen_get_rgba_visual(screen);
        if (visual) {
            gtk_widget_set_visual(overlay->window, visual);
        }
    }
    
    // Set up window for mouse events
    gtk_widget_add_events(overlay->window, 
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_PRESS_MASK);
    
    // Create drawing area
    overlay->drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(overlay->window), overlay->drawing_area);
    
    // Connect signals
    g_signal_connect(overlay->drawing_area, "draw", G_CALLBACK(on_draw), overlay);
    g_signal_connect(overlay->window, "button-press-event", G_CALLBACK(on_button_press), overlay);
    g_signal_connect(overlay->window, "button-release-event", G_CALLBACK(on_button_release), overlay);
    g_signal_connect(overlay->window, "motion-notify-event", G_CALLBACK(on_motion_notify), overlay);
    g_signal_connect(overlay->window, "key-press-event", G_CALLBACK(on_key_press), overlay);
    
    // Initialize selection
    overlay->selecting = false;
    overlay->selection.x = 0;
    overlay->selection.y = 0;
    overlay->selection.width = 0;
    overlay->selection.height = 0;
    
    // Set window size to full screen
    gtk_window_move(GTK_WINDOW(overlay->window), geometry.x, geometry.y);
    gtk_window_resize(GTK_WINDOW(overlay->window), geometry.width, geometry.height);
    
    // Show window
    gtk_widget_show_all(overlay->window);
    
    // Capture the background after window is shown but before it's drawn
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    
    // Now capture the screen for background
    overlay->background = capture_screen(CAPTURE_FULLSCREEN, NULL);
    if (!overlay->background) {
        fprintf(stderr, "Failed to capture screen for overlay\n");
        gtk_widget_destroy(overlay->window);
        capture_cleanup();
        return false;
    }
    
    return true;
}

CaptureArea capture_overlay_get_selection(CaptureOverlay* overlay) {
    return overlay->selection;
}

void capture_overlay_cleanup(CaptureOverlay* overlay) {
    if (overlay->background) {
        cairo_surface_destroy(overlay->background);
        overlay->background = NULL;
    }
    
    if (overlay->window) {
        gtk_widget_destroy(overlay->window);
        overlay->window = NULL;
    }
}

static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    (void)widget;
    CaptureOverlay* overlay = (CaptureOverlay*)data;
    
    // Set identity matrix
    cairo_matrix_t matrix;
    cairo_matrix_init_identity(&matrix);
    cairo_set_matrix(cr, &matrix);
    
    // Draw background if it exists
    if (overlay->background) {
        cairo_set_source_surface(cr, overlay->background, 0, 0);
        cairo_paint(cr);
    }
    
    // Draw semi-transparent overlay only outside selection
    if (overlay->selecting) {
        int x = MIN(overlay->start_x, overlay->start_x + overlay->selection.width);
        int y = MIN(overlay->start_y, overlay->start_y + overlay->selection.height);
        int width = abs(overlay->selection.width);
        int height = abs(overlay->selection.height);
        
        // First, fill everything with semi-transparent black
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
        cairo_paint(cr);
        
        // Clear the selection area to show background
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_rectangle(cr, x, y, width, height);
        cairo_fill(cr);
        
        // Reset operator
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        
        // Draw selection border with white dashed lines
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, 1.0);
        
        // Set dashed line pattern
        double dashes[] = {4.0, 4.0};
        cairo_set_dash(cr, dashes, 2, 0.0);
        
        cairo_rectangle(cr, x, y, width, height);
        cairo_stroke(cr);
        
        // Reset dash pattern
        cairo_set_dash(cr, NULL, 0, 0.0);
        
        // Draw selection dimensions near the cursor
        char dimensions[32];
        snprintf(dimensions, sizeof(dimensions), "%dx%d", width, height);
        
        // Position the text near the selection rectangle
        cairo_text_extents_t extents;
        cairo_text_extents(cr, dimensions, &extents);
        double text_x = x + width - extents.width - 10;
        double text_y = y - 10;
        
        // Ensure text is visible by drawing a dark background
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.7);
        cairo_rectangle(cr, text_x - 5, text_y - extents.height - 5, extents.width + 10, extents.height + 10);
        cairo_fill(cr);
        
        // Draw the text in white
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, dimensions);
    } else {
        // If not selecting, just show semi-transparent overlay
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
        cairo_paint(cr);
    }
    
    // Draw the crosshair at the current mouse position
    GdkRGBA crosshair_color = {1.0, 0.0, 0.0, 1.0}; // Red, fully opaque
    int crosshair_size = 12;
    crosshair_drawer_draw(cr, overlay->mouse_x, overlay->mouse_y, crosshair_size, &crosshair_color);
    
    return FALSE;
}

static gboolean on_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    (void)widget;
    CaptureOverlay* overlay = (CaptureOverlay*)data;
    
    if (event->button == 1) { // Left mouse button
        overlay->selecting = true;
        overlay->start_x = (int)event->x;
        overlay->start_y = (int)event->y;
        overlay->selection.x = (int)event->x;
        overlay->selection.y = (int)event->y;
        overlay->selection.width = 0;
        overlay->selection.height = 0;
    }
    
    return TRUE;
}

static gboolean on_button_release(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    (void)widget;
    CaptureOverlay* overlay = (CaptureOverlay*)data;
    
    if (event->button == 1) { // Left mouse button
        overlay->selecting = false;
        
        // Normalize selection (ensure positive width and height)
        if (overlay->selection.width < 0) {
            overlay->selection.x += overlay->selection.width;
            overlay->selection.width = -overlay->selection.width;
        }
        
        if (overlay->selection.height < 0) {
            overlay->selection.y += overlay->selection.height;
            overlay->selection.height = -overlay->selection.height;
        }
        
        // If selection is too small, consider it as a cancelled selection
        if (overlay->selection.width < 5 || overlay->selection.height < 5) {
            overlay->selection.width = 0;
            overlay->selection.height = 0;
        }
        
        // Hide the window immediately to prevent it from appearing in the screenshot
        gtk_widget_hide(overlay->window);
        
        // Process events to ensure window is hidden
        while (gtk_events_pending()) {
            gtk_main_iteration();
        }
        
        // Small delay to ensure window is fully hidden
        g_usleep(100000);  // 100ms delay
        
        // Exit the GTK main loop to proceed with capture
        gtk_main_quit();
    }
    
    return TRUE;
}

static gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    CaptureOverlay* overlay = (CaptureOverlay*)data;
    overlay->mouse_x = (int)event->x;
    overlay->mouse_y = (int)event->y;
    gtk_widget_queue_draw(widget); // Redraw to update crosshair
    
    if (overlay->selecting) {
        // Update selection dimensions
        overlay->selection.width = (int)event->x - overlay->start_x;
        overlay->selection.height = (int)event->y - overlay->start_y;
        
        // Redraw to show the updated selection
        gtk_widget_queue_draw(overlay->drawing_area);
    }
    
    return TRUE;
}

static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    (void)widget;
    CaptureOverlay* overlay = (CaptureOverlay*)data;
    
    if (event->keyval == GDK_KEY_Escape) {
        // Cancel selection
        overlay->selection.width = 0;
        overlay->selection.height = 0;
        gtk_main_quit();
    } else if (event->keyval == GDK_KEY_Return) {
        // Accept selection and close overlay
        gtk_main_quit();
    }
    
    return TRUE;
} 
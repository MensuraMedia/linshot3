#include "../include/main_window.h"
#include "../include/screen_capture.h"
#include "../include/capture_overlay.h"
#include "../include/editor_tools.h"
#include "../include/utils.h"
#include <glib.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <glib/gstdio.h>
#include <gdk/gdkx.h>
#include <X11/keysym.h>

typedef enum {
    FILENAME_LINSHOT_NUMBER = 0,
    FILENAME_SCREENSHOT_NUMBER,
    FILENAME_LINSHOT_TIMESTAMP,
    FILENAME_SCREENSHOT_TIMESTAMP
} FilenameFormat;

typedef enum {
    SHORTCUT_NONE = 0,
    SHORTCUT_PRINTSCREEN,
    SHORTCUT_CTRL_PRINTSCREEN,
    SHORTCUT_SHIFT_PRINTSCREEN,
    SHORTCUT_CTRL_SHIFT_S,
    SHORTCUT_CTRL_ALT_S
} ShortcutKey;

typedef struct {
    char* screenshot_path;
    FilenameFormat filename_format;
    int auto_number;  // For auto-numbering format
    bool start_with_os;  // New: Start with OS option
    ShortcutKey shortcut_key;  // New: Shortcut key option
} Settings;

// Forward declarations
static void toggle_autostart(bool enable);
static GtkWidget* create_history_item_widget(ScreenshotEntry* entry, MainWindow* win);
static void on_history_item_clicked(GtkWidget* widget, GdkEventButton* event, gpointer data);
static void on_browse_clicked(GtkWidget* widget, gpointer data);
static void create_settings_page(MainWindow* win, GtkWidget* notebook);
static void on_settings_changed(GtkWidget* widget, gpointer data);
static void register_shortcut_key(MainWindow* win, ShortcutKey key);
static GdkFilterReturn key_filter_func(GdkXEvent* xevent, GdkEvent* event, gpointer data);
static void save_image_with_annotations(MainWindow* win, cairo_surface_t* surface, GList* annotations, const char* filename);

// Preprocessing function to validate GTK objects
static gboolean validate_gtk_object(GtkWidget* widget, const char* context) {
    if (!widget) {
        g_warning("%s: Widget is NULL", context);
        return FALSE;
    }
    
    if (!GTK_IS_WIDGET(widget)) {
        g_warning("%s: Widget is not a valid GTK widget", context);
        return FALSE;
    }
    
    if (!G_IS_OBJECT(widget)) {
        g_warning("%s: Widget is not a valid GObject", context);
        return FALSE;
    }
    
    return TRUE;
}

// Safe wrapper for g_object_get_data
static gpointer safe_get_data(GtkWidget* widget, const char* key, const char* context) {
    if (!validate_gtk_object(widget, context)) {
        return NULL;
    }
    return g_object_get_data(G_OBJECT(widget), key);
}

// Safe wrapper for g_object_set_data
static void safe_set_data(GtkWidget* widget, const char* key, gpointer data, const char* context) {
    if (!validate_gtk_object(widget, context)) {
        return;
    }
    g_object_set_data(G_OBJECT(widget), key, data);
}

// Safe wrapper for g_object_set_data_full
static void safe_set_data_full(GtkWidget* widget, const char* key, gpointer data, GDestroyNotify destroy_func, const char* context) {
    if (!validate_gtk_object(widget, context)) {
        if (destroy_func && data) {
            destroy_func(data);
        }
        return;
    }
    g_object_set_data_full(G_OBJECT(widget), key, data, destroy_func);
}

static char* get_config_file_path(void) {
    const char* config_dir = g_get_user_config_dir();
    char* linshot_dir = g_build_filename(config_dir, "linshot", NULL);
    g_mkdir_with_parents(linshot_dir, 0755);
    char* config_file = g_build_filename(linshot_dir, "settings.conf", NULL);
    g_free(linshot_dir);
    return config_file;
}

static void load_settings(Settings* settings) {
    // Set default values first
    settings->screenshot_path = g_strdup(g_get_user_special_dir(G_USER_DIRECTORY_PICTURES));
    settings->filename_format = FILENAME_LINSHOT_TIMESTAMP;
    settings->auto_number = 1;
    settings->start_with_os = false;
    settings->shortcut_key = SHORTCUT_PRINTSCREEN;
    
    // Try to load from config file
    char* config_file = get_config_file_path();
    GKeyFile* key_file = g_key_file_new();
    
    if (g_key_file_load_from_file(key_file, config_file, G_KEY_FILE_NONE, NULL)) {
        // Load screenshot path
        char* path = g_key_file_get_string(key_file, "Settings", "screenshot_path", NULL);
        if (path) {
            g_free(settings->screenshot_path);
            settings->screenshot_path = path;
        }
        
        // Load filename format
        settings->filename_format = g_key_file_get_integer(key_file, "Settings", "filename_format", NULL);
        
        // Load auto number
        settings->auto_number = g_key_file_get_integer(key_file, "Settings", "auto_number", NULL);
        
        // Load start with OS
        settings->start_with_os = g_key_file_get_boolean(key_file, "Settings", "start_with_os", NULL);
        
        // Load shortcut key
        settings->shortcut_key = g_key_file_get_integer(key_file, "Settings", "shortcut_key", NULL);
    }
    
    g_key_file_free(key_file);
    g_free(config_file);
}

static void save_settings(Settings* settings) {
    char* config_file = get_config_file_path();
    GKeyFile* key_file = g_key_file_new();
    
    // Save all settings
    g_key_file_set_string(key_file, "Settings", "screenshot_path", settings->screenshot_path);
    g_key_file_set_integer(key_file, "Settings", "filename_format", settings->filename_format);
    g_key_file_set_integer(key_file, "Settings", "auto_number", settings->auto_number);
    g_key_file_set_boolean(key_file, "Settings", "start_with_os", settings->start_with_os);
    g_key_file_set_integer(key_file, "Settings", "shortcut_key", settings->shortcut_key);
    
    // Save to file
    GError* error = NULL;
    if (!g_key_file_save_to_file(key_file, config_file, &error)) {
        g_warning("Failed to save settings: %s", error->message);
        g_error_free(error);
    }
    
    g_key_file_free(key_file);
    g_free(config_file);
}

static void on_browse_clicked(GtkWidget* widget, gpointer data) {
    (void)widget;
    GtkWidget* entry = GTK_WIDGET(data);
    
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Select Screenshot Directory",
        NULL,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Select", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Set current folder if entry has a path
    const char* current_path = gtk_entry_get_text(GTK_ENTRY(entry));
    if (current_path && *current_path) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), current_path);
    }
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* folder = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(entry), folder);
        g_free(folder);
    }
    
    gtk_widget_destroy(dialog);
}

static char* generate_screenshot_filename(MainWindow* win) {
    Settings* settings = safe_get_data(win->window, "settings", "generate_screenshot_filename");
    
    char* filename = NULL;
    char timestamp[20];
    static int number = 1;  // Static counter for auto-numbering
    
    switch (settings->filename_format) {
        case FILENAME_LINSHOT_NUMBER:
            filename = g_strdup_printf("%s/LinShot_%04d.png", settings->screenshot_path, number++);
            break;
        case FILENAME_SCREENSHOT_NUMBER:
            filename = g_strdup_printf("%s/Screenshot_%04d.png", settings->screenshot_path, number++);
            break;
        case FILENAME_LINSHOT_TIMESTAMP: {
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
            filename = g_strdup_printf("%s/LinShot_%s.png", settings->screenshot_path, timestamp);
            break;
        }
        case FILENAME_SCREENSHOT_TIMESTAMP: {
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_info);
            filename = g_strdup_printf("%s/Screenshot_%s.png", settings->screenshot_path, timestamp);
            break;
        }
    }
    
    return filename;
}

static void rgb_data_destroy(guchar* pixels, gpointer data) {
    (void)data;  // Unused parameter
    g_free(pixels);
}

static void copy_to_clipboard(MainWindow* win, cairo_surface_t* surface, GList* annotations) {
    if (!surface) return;
    
    // Create a new surface to hold both the image and annotations
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    cairo_surface_t* combined_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    
    // Create Cairo context for the new surface
    cairo_t* cr = cairo_create(combined_surface);
    
    // Draw the original image
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    
    // Draw all annotations
    GList* iter;
    for (iter = annotations; iter != NULL; iter = iter->next) {
        Annotation* annotation = (Annotation*)iter->data;
        annotation_draw(annotation, cr);
    }
    
    // Ensure all drawing operations are complete
    cairo_surface_flush(combined_surface);
    
    // Get surface data and stride
    unsigned char* surface_data = cairo_image_surface_get_data(combined_surface);
    int stride = cairo_image_surface_get_stride(combined_surface);
    
    // Create a new buffer for RGB data conversion
    guchar* rgb_data = g_malloc(width * height * 4);
    if (!rgb_data) {
        cairo_destroy(cr);
        cairo_surface_destroy(combined_surface);
        return;
    }
    
    // Convert ARGB to RGBA (Cairo uses ARGB32, GdkPixbuf uses RGBA)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int src_idx = y * stride + x * 4;
            int dst_idx = (y * width + x) * 4;
            
            // Cairo (ARGB) -> GdkPixbuf (RGBA)
            guchar alpha = surface_data[src_idx + 3];
            guchar red = surface_data[src_idx + 2];
            guchar green = surface_data[src_idx + 1];
            guchar blue = surface_data[src_idx + 0];
            
            rgb_data[dst_idx + 0] = red;
            rgb_data[dst_idx + 1] = green;
            rgb_data[dst_idx + 2] = blue;
            rgb_data[dst_idx + 3] = alpha;
        }
    }
    
    // Create pixbuf from converted data
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_data(
        rgb_data,
        GDK_COLORSPACE_RGB,
        TRUE,  // has alpha
        8,     // bits per sample
        width,
        height,
        width * 4,  // rowstride for packed RGB data
        rgb_data_destroy,  // Use our type-safe destroy function
        NULL
    );
    
    // Get the clipboard and set the image
    GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    
    // Create a copy of the pixbuf that will persist after we free our local resources
    GdkPixbuf* clipboard_pixbuf = gdk_pixbuf_copy(pixbuf);
    gtk_clipboard_set_image(clipboard, clipboard_pixbuf);
    g_object_unref(clipboard_pixbuf);
    
    // Clean up
    g_object_unref(pixbuf);
    cairo_destroy(cr);
    cairo_surface_destroy(combined_surface);
    
    gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Image with annotations copied to clipboard");
}

static cairo_surface_t* add_border_to_surface(cairo_surface_t* surface, double border_width, double r, double g, double b) {
    if (!surface) return NULL;
    
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    
    // Create a new surface with space for the border
    cairo_surface_t* bordered_surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32,
        width + 2 * border_width,
        height + 2 * border_width
    );
    
    cairo_t* cr = cairo_create(bordered_surface);
    
    // Clear the surface
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    // Draw the original image in the center
    cairo_set_source_surface(cr, surface, border_width, border_width);
    cairo_paint(cr);
    
    // Draw the border
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, border_width);
    cairo_rectangle(cr, 
                   border_width/2, border_width/2, 
                   width + border_width, height + border_width);
    cairo_stroke(cr);
    
    cairo_destroy(cr);
    return bordered_surface;
}

static void on_capture_button_clicked(GtkWidget* widget, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_capture_button_clicked");
    gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Capturing screen...");
    
    // Show capture overlay
    CaptureOverlay overlay = {0};
    if (!capture_overlay_init(&overlay)) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Failed to initialize capture overlay");
        return;
    }
    
    // Run the overlay until user makes a selection
    gtk_main();
    
    // Get the selected area
    CaptureArea area = capture_overlay_get_selection(&overlay);
    capture_overlay_cleanup(&overlay);
    
    if (area.width == 0 || area.height == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Capture cancelled");
        return;
    }
    
    // Initialize screen capture
    if (!capture_init()) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Failed to initialize screen capture");
        return;
    }
    
    // Capture selected area
    cairo_surface_t* surface = capture_screen(CAPTURE_AREA, &area);
    if (!surface) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Failed to capture screen");
        capture_cleanup();
        return;
    }
    
    // Clean up screen capture
    capture_cleanup();
    
    // Add border to the captured image
    cairo_surface_t* bordered_surface = add_border_to_surface(surface, 3.0, 0.0, 0.0, 0.0);
    cairo_surface_destroy(surface);  // Free the original surface
    
    if (!bordered_surface) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Failed to add border to image");
        return;
    }
    
    // Generate filename and save immediately
    char* filename = generate_screenshot_filename(win);
    
    // Save the raw screenshot without annotations
    cairo_status_t status = cairo_surface_write_to_png(bordered_surface, filename);
    if (status != CAIRO_STATUS_SUCCESS) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Failed to save screenshot");
        g_free(filename);
        cairo_surface_destroy(bordered_surface);
        return;
    }
    
    // Add to history
    screenshot_history_add(&win->screenshot_history, filename);
    
    // Update history view
    GList* entries = screenshot_history_get_sorted(&win->screenshot_history);
    
    // Clear existing history items
    GList* children = gtk_container_get_children(GTK_CONTAINER(win->history_flow_box));
    for (GList* iter = children; iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
    
    // Add updated history items
    for (GList* iter = entries; iter != NULL; iter = iter->next) {
        ScreenshotEntry* entry = (ScreenshotEntry*)iter->data;
        GtkWidget* item_widget = create_history_item_widget(entry, win);
        gtk_flow_box_insert(GTK_FLOW_BOX(win->history_flow_box), item_widget, -1);
    }
    gtk_widget_show_all(win->history_flow_box);
    
    // Update window data
    if (win_data->current_image) {
        cairo_surface_destroy(win_data->current_image);
    }
    win_data->current_image = bordered_surface;
    
    // Clear existing annotations
    g_list_free_full(win_data->annotations, (GDestroyNotify)annotation_free);
    win_data->annotations = NULL;
    
    // Copy to clipboard
    copy_to_clipboard(win, bordered_surface, NULL);
    
    // Redraw canvas
    gtk_widget_queue_draw(win->canvas);
    
    g_free(filename);
    gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Screenshot saved and copied to clipboard");
}

static void on_copy_button_clicked(GtkWidget* widget, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_copy_button_clicked");
    
    if (!win_data->current_image) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "No image to copy");
        return;
    }
    
    copy_to_clipboard(win, win_data->current_image, win_data->annotations);
}

static void on_tool_button_clicked(GtkWidget* widget, gpointer data) {
    MainWindow* win = (MainWindow*)data;
    if (!win || !win->window) {
        return;
    }
    
    // Remove active class from all buttons
    GtkWidget* buttons_container = gtk_widget_get_parent(widget);
    if (!buttons_container) {
        return;
    }
    
    GList* children = gtk_container_get_children(GTK_CONTAINER(buttons_container));
    for (GList* iter = children; iter != NULL; iter = iter->next) {
        GtkWidget* button = GTK_WIDGET(iter->data);
        GtkStyleContext* context = gtk_widget_get_style_context(button);
        gtk_style_context_remove_class(context, "active");
    }
    g_list_free(children);
    
    // Add active class to clicked button
    GtkStyleContext* context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_class(context, "active");
    
    int tool_id = GPOINTER_TO_INT(safe_get_data(widget, "tool-id", "on_tool_button_clicked"));
    
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_tool_button_clicked");
    if (win_data) {
        win_data->current_tool.type = tool_id;
        
        const char* tool_names[] = {
            "None", "Arrow", "Rectangle", "Ellipse", "Text", "Freehand"
        };
        char status[50];
        snprintf(status, sizeof(status), "Selected tool: %s", tool_names[tool_id]);
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, status);
    }
}

static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_draw");
    
    if (!win_data) {
        return FALSE;
    }
    
    // Get widget allocation
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    
    // Set up Cairo context
    cairo_save(cr);
    
    // Clear the background with theme color
    cairo_set_source_rgb(cr, 0.176, 0.176, 0.176);  // #2d2d2d
    cairo_rectangle(cr, 0, 0, allocation.width, allocation.height);
    cairo_fill(cr);
    
    if (win_data->current_image) {
        // Get image dimensions
        int width = cairo_image_surface_get_width(win_data->current_image);
        int height = cairo_image_surface_get_height(win_data->current_image);
        
        // Set canvas size to match image size if needed
        if (gtk_widget_get_allocated_width(win->canvas) != width ||
            gtk_widget_get_allocated_height(win->canvas) != height) {
            gtk_widget_set_size_request(win->canvas, width, height);
        }
        
        // Create a new surface for the image
        cairo_surface_t* image_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
        cairo_t* image_cr = cairo_create(image_surface);
        
        // Draw the image
        cairo_set_source_surface(image_cr, win_data->current_image, 0, 0);
        cairo_paint(image_cr);
        
        // Draw annotations on the image surface
        GList* iter;
        for (iter = win_data->annotations; iter != NULL; iter = iter->next) {
            Annotation* annotation = (Annotation*)iter->data;
            annotation_draw(annotation, image_cr);
        }
        
        // Draw current annotation if drawing
        if (win_data->drawing) {
            Annotation* current = annotation_create(win_data->current_tool.type, &win_data->current_tool);
            if (current) {
                current->bounds = win_data->start_point;
                annotation_draw(current, image_cr);
                annotation_free(current);
            }
        }
        
        // Draw the combined image and annotations to the widget
        cairo_set_source_surface(cr, image_surface, 0, 0);
        cairo_paint(cr);
        
        // Clean up
        cairo_destroy(image_cr);
        cairo_surface_destroy(image_surface);
    }
    
    cairo_restore(cr);
    return TRUE;
}

static void show_text_dialog(MainWindow* win, MainWindowData* win_data, double x, double y) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Enter Text",
        GTK_WINDOW(win->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel",
        GTK_RESPONSE_CANCEL,
        "_OK",
        GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    // Create text entry
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    
    // Set dialog properties
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
    gtk_widget_show_all(dialog);
    
    // Run dialog
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        const char* text = gtk_entry_get_text(GTK_ENTRY(entry));
        if (text && *text) {  // If text is not empty
            // Create text annotation
            Annotation* annotation = annotation_create(TOOL_TEXT, &win_data->current_tool);
            if (annotation) {
                annotation->text = g_strdup(text);
                annotation->bounds.x1 = x;
                annotation->bounds.y1 = y;
                win_data->annotations = g_list_append(win_data->annotations, annotation);
                gtk_widget_queue_draw(win->canvas);
            }
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Helper function to find text annotation at coordinates
static Annotation* find_text_at_coords(MainWindowData* win_data, double x, double y) {
    GList* iter;
    for (iter = win_data->annotations; iter != NULL; iter = iter->next) {
        Annotation* annotation = (Annotation*)iter->data;
        if (annotation->type == TOOL_TEXT) {
            // Check if coordinates are within text bounds
            if (x >= annotation->bounds.x1 && x <= annotation->bounds.x2 &&
                y >= annotation->bounds.y1 && y <= annotation->bounds.y2) {
                return annotation;
            }
        }
    }
    return NULL;
}

static gboolean on_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_button_press");
    
    if (event->button == 1) {  // Left mouse button
        // First, check if we're clicking on an existing text annotation
        Annotation* text_annotation = find_text_at_coords(win_data, event->x, event->y);
        if (text_annotation) {
            win_data->selected_text = text_annotation;
            win_data->drag_start_x = event->x - text_annotation->bounds.x1;
            win_data->drag_start_y = event->y - text_annotation->bounds.y1;
            gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Text selected - drag to move");
            return TRUE;
        }
        
        // If not clicking text, handle normal tool operations
        if (win_data->current_tool.type == TOOL_TEXT) {
            // Deselect any selected text when creating new text
            win_data->selected_text = NULL;
            show_text_dialog(win, win_data, event->x, event->y);
        } else if (win_data->current_tool.type != TOOL_NONE) {
            win_data->selected_text = NULL;  // Deselect text when using other tools
            win_data->drawing = true;
            win_data->start_point.x1 = event->x;
            win_data->start_point.y1 = event->y;
            win_data->start_point.x2 = event->x;
            win_data->start_point.y2 = event->y;
        }
    }
    
    return TRUE;
}

static gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_motion_notify");
    
    if (win_data->selected_text) {
        // Update text position while dragging
        double new_x = event->x - win_data->drag_start_x;
        double new_y = event->y - win_data->drag_start_y;
        
        // Update text bounds
        double width = win_data->selected_text->bounds.x2 - win_data->selected_text->bounds.x1;
        double height = win_data->selected_text->bounds.y2 - win_data->selected_text->bounds.y1;
        win_data->selected_text->bounds.x1 = new_x;
        win_data->selected_text->bounds.y1 = new_y;
        win_data->selected_text->bounds.x2 = new_x + width;
        win_data->selected_text->bounds.y2 = new_y + height;
        
        gtk_widget_queue_draw(win->canvas);
    } else if (win_data->drawing) {
        win_data->start_point.x2 = event->x;
        win_data->start_point.y2 = event->y;
        gtk_widget_queue_draw(win->canvas);
    }
    
    return TRUE;
}

static gboolean on_button_release(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_button_release");
    
    if (event->button == 1) {
        if (win_data->selected_text) {
            // Finish moving text
            win_data->selected_text = NULL;
            gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Text moved");
        } else if (win_data->drawing) {
            win_data->drawing = false;
            
            // Create and add new annotation
            Annotation* annotation = annotation_create(win_data->current_tool.type, &win_data->current_tool);
            if (annotation) {
                annotation->bounds = win_data->start_point;
                win_data->annotations = g_list_append(win_data->annotations, annotation);
            }
            
            gtk_widget_queue_draw(win->canvas);
        }
    }
    
    return TRUE;
}

static void undo_last_annotation(MainWindowData* win_data) {
    if (!win_data->annotations) {
        return;  // Nothing to undo
    }
    
    // Get the last annotation
    GList* last = g_list_last(win_data->annotations);
    Annotation* annotation = last->data;
    
    // Remove it from the current list
    win_data->annotations = g_list_delete_link(win_data->annotations, last);
    
    // Add it to the undo stack
    win_data->undo_stack = g_list_append(win_data->undo_stack, annotation);
    
    // Redraw canvas
    gtk_widget_queue_draw(win_data->win.canvas);
}

static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_key_press");
    
    // Check for Ctrl+Z
    if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_z) {
        undo_last_annotation(win_data);
        return TRUE;  // Event handled
    }
    
    return FALSE;  // Event not handled
}

static void on_save_button_clicked(GtkWidget* widget, gpointer data) {
    (void)widget;
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_save_button_clicked");
    
    if (!win_data->current_image) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "No image to save");
        return;
    }
    
    // Generate default filename using the dedicated function
    char* default_filename = generate_screenshot_filename(win);
    
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Save Screenshot",
        GTK_WINDOW(win->window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Save", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), default_filename);
    g_free(default_filename);
    
    // Add file filters
    GtkFileFilter* filter_all = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_all, "All Supported Formats");
    gtk_file_filter_add_pattern(filter_all, "*.*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_all);
    
    GtkFileFilter* filter_jpg = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_jpg, "JPEG Images (*.jpg, *.jpeg)");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpg");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_jpg);
    
    GtkFileFilter* filter_png = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_png, "PNG Images (*.png)");
    gtk_file_filter_add_pattern(filter_png, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_png);
    
    GtkFileFilter* filter_gif = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_gif, "GIF Images (*.gif)");
    gtk_file_filter_add_pattern(filter_gif, "*.gif");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_gif);
    
    GtkFileFilter* filter_svg = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_svg, "SVG Images (*.svg)");
    gtk_file_filter_add_pattern(filter_svg, "*.svg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_svg);
    
    GtkFileFilter* filter_tiff = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_tiff, "TIFF Images (*.tif, *.tiff)");
    gtk_file_filter_add_pattern(filter_tiff, "*.tif");
    gtk_file_filter_add_pattern(filter_tiff, "*.tiff");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_tiff);
    
    GtkFileFilter* filter_webp = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_webp, "WebP Images (*.webp)");
    gtk_file_filter_add_pattern(filter_webp, "*.webp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_webp);
    
    GtkFileFilter* filter_bmp = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_bmp, "BMP Images (*.bmp)");
    gtk_file_filter_add_pattern(filter_bmp, "*.bmp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_bmp);
    
    GtkFileFilter* filter_heic = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_heic, "HEIC/HEIF Images (*.heic, *.heif)");
    gtk_file_filter_add_pattern(filter_heic, "*.heic");
    gtk_file_filter_add_pattern(filter_heic, "*.heif");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_heic);
    
    GtkFileFilter* filter_raw = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_raw, "RAW Images (*.raw)");
    gtk_file_filter_add_pattern(filter_raw, "*.raw");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_raw);
    
    GtkFileFilter* filter_ico = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_ico, "ICO Images (*.ico)");
    gtk_file_filter_add_pattern(filter_ico, "*.ico");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_ico);
    
    GtkFileFilter* filter_psd = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_psd, "PSD Images (*.psd)");
    gtk_file_filter_add_pattern(filter_psd, "*.psd");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_psd);
    
    GtkFileFilter* filter_eps = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_eps, "EPS Images (*.eps)");
    gtk_file_filter_add_pattern(filter_eps, "*.eps");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_eps);
    
    GtkFileFilter* filter_ai = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_ai, "AI Images (*.ai)");
    gtk_file_filter_add_pattern(filter_ai, "*.ai");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_ai);
    
    GtkFileFilter* filter_avif = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_avif, "AVIF Images (*.avif)");
    gtk_file_filter_add_pattern(filter_avif, "*.avif");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_avif);
    
    GtkFileFilter* filter_cr2 = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_cr2, "CR2/CR3 Images (*.cr2, *.cr3)");
    gtk_file_filter_add_pattern(filter_cr2, "*.cr2");
    gtk_file_filter_add_pattern(filter_cr2, "*.cr3");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_cr2);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        save_image_with_annotations(win, win_data->current_image, win_data->annotations, filename);
        g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
}

static void save_image_with_annotations(MainWindow* win, cairo_surface_t* surface, GList* annotations, const char* filename) {
    if (!surface || !filename) return;
    
    // Create a new surface with annotations
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    cairo_surface_t* combined_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    
    // Create Cairo context for the new surface
    cairo_t* cr = cairo_create(combined_surface);
    
    // Draw the original image
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    
    // Draw all annotations
    GList* iter;
    for (iter = annotations; iter != NULL; iter = iter->next) {
        Annotation* annotation = (Annotation*)iter->data;
        annotation_draw(annotation, cr);
    }
    
    // Ensure all drawing operations are complete
    cairo_surface_flush(combined_surface);
    
    // Get the file extension
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        cairo_destroy(cr);
        cairo_surface_destroy(combined_surface);
        return;
    }
    ext++; // Skip the dot
    
    // Create a pixbuf from the surface
    GdkPixbuf* pixbuf = gdk_pixbuf_get_from_surface(combined_surface, 0, 0, width, height);
    if (!pixbuf) {
        cairo_destroy(cr);
        cairo_surface_destroy(combined_surface);
        return;
    }
    
    // Save based on file extension
    GError* error = NULL;
    if (g_ascii_strcasecmp(ext, "jpg") == 0 || g_ascii_strcasecmp(ext, "jpeg") == 0) {
        gdk_pixbuf_save(pixbuf, filename, "jpeg", &error, "quality", "100", NULL);
    } else if (g_ascii_strcasecmp(ext, "png") == 0) {
        gdk_pixbuf_save(pixbuf, filename, "png", &error, "compression", "9", NULL);
    } else if (g_ascii_strcasecmp(ext, "gif") == 0) {
        gdk_pixbuf_save(pixbuf, filename, "gif", &error, NULL);
    } else if (g_ascii_strcasecmp(ext, "svg") == 0) {
        // Note: SVG requires special handling as it's a vector format
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "SVG format not supported for screenshots");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "SVG format not supported for screenshots");
    } else if (g_ascii_strcasecmp(ext, "tiff") == 0 || g_ascii_strcasecmp(ext, "tif") == 0) {
        gdk_pixbuf_save(pixbuf, filename, "tiff", &error, NULL);
    } else if (g_ascii_strcasecmp(ext, "webp") == 0) {
        gdk_pixbuf_save(pixbuf, filename, "webp", &error, "quality", "100", NULL);
    } else if (g_ascii_strcasecmp(ext, "bmp") == 0) {
        gdk_pixbuf_save(pixbuf, filename, "bmp", &error, NULL);
    } else if (g_ascii_strcasecmp(ext, "heic") == 0 || g_ascii_strcasecmp(ext, "heif") == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "HEIC/HEIF format not supported");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "HEIC/HEIF format not supported");
    } else if (g_ascii_strcasecmp(ext, "raw") == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "RAW format not supported");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "RAW format not supported");
    } else if (g_ascii_strcasecmp(ext, "ico") == 0) {
        gdk_pixbuf_save(pixbuf, filename, "ico", &error, NULL);
    } else if (g_ascii_strcasecmp(ext, "psd") == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "PSD format not supported");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "PSD format not supported");
    } else if (g_ascii_strcasecmp(ext, "eps") == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "EPS format not supported");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "EPS format not supported");
    } else if (g_ascii_strcasecmp(ext, "ai") == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "AI format not supported");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "AI format not supported");
    } else if (g_ascii_strcasecmp(ext, "avif") == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "AVIF format not supported");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "AVIF format not supported");
    } else if (g_ascii_strcasecmp(ext, "cr2") == 0 || g_ascii_strcasecmp(ext, "cr3") == 0) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "CR2/CR3 format not supported");
        error = g_error_new(G_FILE_ERROR, G_FILE_ERROR_FAILED, "CR2/CR3 format not supported");
    }
    
    if (error) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to save image: %s", error->message);
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, error_msg);
        g_error_free(error);
    } else {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Image saved successfully");
        
        // Add to history
        screenshot_history_add(&win->screenshot_history, filename);
        
        // Clear existing history items
        GList* children = gtk_container_get_children(GTK_CONTAINER(win->history_flow_box));
        for (GList* iter = children; iter != NULL; iter = iter->next) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);
        
        // Add updated history items
        GList* entries = screenshot_history_get_sorted(&win->screenshot_history);
        for (GList* iter = entries; iter != NULL; iter = iter->next) {
            ScreenshotEntry* entry = (ScreenshotEntry*)iter->data;
            GtkWidget* item_widget = create_history_item_widget(entry, win);
            gtk_flow_box_insert(GTK_FLOW_BOX(win->history_flow_box), item_widget, -1);
        }
        
        gtk_widget_show_all(win->history_flow_box);
    }
    
    // Clean up
    g_object_unref(pixbuf);
    cairo_destroy(cr);
    cairo_surface_destroy(combined_surface);
}

static void on_history_item_clicked(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    (void)event;  // Unused parameter
    MainWindow* win = (MainWindow*)data;
    MainWindowData* win_data = safe_get_data(win->window, "window-data", "on_history_item_clicked");
    const char* filepath = safe_get_data(widget, "filepath", "on_history_item_clicked");
    
    // Load the image
    cairo_surface_t* surface = cairo_image_surface_create_from_png(filepath);
    if (!surface) {
        gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Failed to load image");
        return;
    }
    
    // Clean up existing image and annotations
    if (win_data->current_image) {
        cairo_surface_destroy(win_data->current_image);
    }
    g_list_free_full(win_data->annotations, (GDestroyNotify)annotation_free);
    
    // Set the new image
    win_data->current_image = surface;
    win_data->annotations = NULL;
    
    // Switch to screenshot tab (it's the first tab)
    GtkWidget* notebook = gtk_widget_get_ancestor(win->canvas, GTK_TYPE_NOTEBOOK);
    if (notebook) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
    }
    
    // Redraw canvas
    gtk_widget_queue_draw(win->canvas);
    gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Loaded image from history");
}

static GtkWidget* create_history_item_widget(ScreenshotEntry* entry, MainWindow* win) {
    // Create thumbnail
    GtkWidget* image = gtk_image_new_from_pixbuf(entry->thumbnail);
    gtk_widget_set_size_request(image, 200, 200);
    
    // Make the image clickable
    GtkWidget* event_box = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(event_box), image);
    
    // Store the filepath and window data
    safe_set_data_full(event_box, "filepath", g_strdup(entry->filepath), g_free, "create_history_item_widget");
    
    // Connect click event
    g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_history_item_clicked), win);
    
    return event_box;
}

static void create_settings_page(MainWindow* win, GtkWidget* notebook) {
    Settings* settings = safe_get_data(win->window, "settings", "create_settings_page");
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(vbox, 10);
    gtk_widget_set_margin_end(vbox, 10);
    gtk_widget_set_margin_top(vbox, 10);
    gtk_widget_set_margin_bottom(vbox, 10);

    // Screenshot Path Frame
    GtkWidget* path_frame = gtk_frame_new("Screenshot Path");
    GtkWidget* path_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(path_box), 10);
    
    GtkWidget* path_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(path_entry), settings->screenshot_path);
    safe_set_data(path_entry, "settings", settings, "create_settings_page");
    safe_set_data(path_entry, "window", win, "create_settings_page");
    g_signal_connect(path_entry, "changed", G_CALLBACK(on_settings_changed), NULL);
    
    GtkWidget* browse_button = gtk_button_new_with_label("Browse");
    g_signal_connect(browse_button, "clicked", G_CALLBACK(on_browse_clicked), path_entry);
    
    gtk_box_pack_start(GTK_BOX(path_box), path_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(path_box), browse_button, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(path_frame), path_box);
    gtk_box_pack_start(GTK_BOX(vbox), path_frame, FALSE, FALSE, 0);

    // Filename Format Frame
    GtkWidget* format_frame = gtk_frame_new("Filename Format");
    GtkWidget* format_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(format_box), 10);
    
    const char* format_labels[] = {
        "Screenshot_%Y%m%d_%H%M%S",
        "LinShot_%Y%m%d_%H%M%S",
        "Screenshot_%d_%H%M",
        "LinShot_%d_%H%M"
    };
    
    GtkWidget* format_radio = NULL;
    for (int i = 0; i < 4; i++) {
        GtkWidget* radio = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(format_radio), format_labels[i]);
        format_radio = radio;
        
        safe_set_data(radio, "settings", settings, "create_settings_page");
        safe_set_data(radio, "window", win, "create_settings_page");
        safe_set_data(radio, "format", GINT_TO_POINTER(i), "create_settings_page");
        
        if (settings->filename_format == (FilenameFormat)i) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
        }
        
        g_signal_connect(radio, "toggled", G_CALLBACK(on_settings_changed), NULL);
        gtk_box_pack_start(GTK_BOX(format_box), radio, FALSE, FALSE, 0);
    }
    
    gtk_container_add(GTK_CONTAINER(format_frame), format_box);
    gtk_box_pack_start(GTK_BOX(vbox), format_frame, FALSE, FALSE, 0);

    // Startup Options Frame
    GtkWidget* startup_frame = gtk_frame_new("Startup Options");
    GtkWidget* startup_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(startup_box), 10);
    
    GtkWidget* autostart_check = gtk_check_button_new_with_label("Start with OS");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(autostart_check), settings->start_with_os);
    safe_set_data(autostart_check, "settings", settings, "create_settings_page");
    safe_set_data(autostart_check, "window", win, "create_settings_page");
    g_signal_connect(autostart_check, "toggled", G_CALLBACK(on_settings_changed), NULL);
    
    gtk_box_pack_start(GTK_BOX(startup_box), autostart_check, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(startup_frame), startup_box);
    gtk_box_pack_start(GTK_BOX(vbox), startup_frame, FALSE, FALSE, 0);

    // Shortcut Key Frame
    GtkWidget* shortcut_frame = gtk_frame_new("Shortcut Key");
    GtkWidget* shortcut_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(shortcut_box), 10);
    
    const char* shortcut_labels[] = {
        "Print Screen",
        "Ctrl + Print Screen",
        "Alt + Print Screen",
        "Shift + Print Screen",
        "Ctrl + Shift + Print Screen"
    };
    
    GtkWidget* shortcut_radio = NULL;
    for (int i = 0; i < 5; i++) {
        GtkWidget* radio = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(shortcut_radio), shortcut_labels[i]);
        shortcut_radio = radio;
        
        safe_set_data(radio, "settings", settings, "create_settings_page");
        safe_set_data(radio, "window", win, "create_settings_page");
        safe_set_data(radio, "shortcut", GINT_TO_POINTER(i), "create_settings_page");
        
        if (settings->shortcut_key == (ShortcutKey)i) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), TRUE);
        }
        
        g_signal_connect(radio, "toggled", G_CALLBACK(on_settings_changed), NULL);
        gtk_box_pack_start(GTK_BOX(shortcut_box), radio, FALSE, FALSE, 0);
    }
    
    gtk_container_add(GTK_CONTAINER(shortcut_frame), shortcut_box);
    gtk_box_pack_start(GTK_BOX(vbox), shortcut_frame, FALSE, FALSE, 0);

    // Add the vbox to the notebook
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, gtk_label_new("Settings"));
}

static void register_shortcut_key(MainWindow* win, ShortcutKey key) {
    GdkDisplay* display = gdk_display_get_default();
    GdkScreen* screen = gdk_display_get_default_screen(display);
    GdkWindow* root = gdk_screen_get_root_window(screen);
    
    // First ungrab any existing shortcuts
    GdkSeat* seat = gdk_display_get_default_seat(display);
    gdk_seat_ungrab(seat);
    
    // Register the new shortcut
    switch (key) {
        case SHORTCUT_PRINTSCREEN:
            gdk_window_add_filter(root, (GdkFilterFunc)key_filter_func, win);
            break;
        case SHORTCUT_CTRL_PRINTSCREEN:
            gdk_window_add_filter(root, (GdkFilterFunc)key_filter_func, win);
            break;
        case SHORTCUT_SHIFT_PRINTSCREEN:
            gdk_window_add_filter(root, (GdkFilterFunc)key_filter_func, win);
            break;
        case SHORTCUT_CTRL_SHIFT_S:
            gdk_window_add_filter(root, (GdkFilterFunc)key_filter_func, win);
            break;
        case SHORTCUT_CTRL_ALT_S:
            gdk_window_add_filter(root, (GdkFilterFunc)key_filter_func, win);
            break;
        default:
            break;
    }
}

static void destroy_widget(gpointer data, gpointer user_data) {
    (void)user_data;
    if (GTK_IS_WIDGET(data)) {
        gtk_widget_destroy(GTK_WIDGET(data));
    }
}

static void on_settings_changed(GtkWidget* widget, gpointer data) {
    (void)data;  // Mark unused parameter as used
    Settings* settings = safe_get_data(widget, "settings", "on_settings_changed");
    MainWindow* win = safe_get_data(widget, "window", "on_settings_changed");
    
    if (!settings || !win) {
        g_warning("Settings or window pointer not found in widget data");
        return;
    }

    // Handle path entry changes
    if (GTK_IS_ENTRY(widget)) {
        const char* new_path = gtk_entry_get_text(GTK_ENTRY(widget));
        if (g_strcmp0(settings->screenshot_path, new_path) != 0) {
            g_free(settings->screenshot_path);
            settings->screenshot_path = g_strdup(new_path);
            
            // Update screenshot history path and reload
            screenshot_history_set_path(&win->screenshot_history, new_path);
            screenshot_history_load(&win->screenshot_history);
            
            // Clear and update history items in the view
            GtkFlowBox* history_box = GTK_FLOW_BOX(win->history_flow_box);
            GList* children = gtk_container_get_children(GTK_CONTAINER(history_box));
            g_list_foreach(children, destroy_widget, NULL);
            g_list_free(children);
            
            GList* entries = screenshot_history_get_sorted(&win->screenshot_history);
            for (GList* l = entries; l != NULL; l = l->next) {
                ScreenshotEntry* entry = l->data;
                GtkWidget* item_widget = create_history_item_widget(entry, win);
                gtk_flow_box_insert(GTK_FLOW_BOX(history_box), item_widget, -1);
            }
        }
    }
    // Handle autostart checkbox changes
    else if (GTK_IS_CHECK_BUTTON(widget)) {
        settings->start_with_os = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
        toggle_autostart(settings->start_with_os);
    }
    // Handle radio button changes (filename format and shortcut keys)
    else if (GTK_IS_RADIO_BUTTON(widget) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        gpointer format_data = safe_get_data(widget, "format", "on_settings_changed");
        gpointer shortcut_data = safe_get_data(widget, "shortcut", "on_settings_changed");
        
        if (format_data) {
            settings->filename_format = (FilenameFormat)GPOINTER_TO_INT(format_data);
        }
        else if (shortcut_data) {
            int new_shortcut = GPOINTER_TO_INT(shortcut_data);
            if (settings->shortcut_key != (ShortcutKey)new_shortcut) {
                settings->shortcut_key = (ShortcutKey)new_shortcut;
                register_shortcut_key(win, settings->shortcut_key);
            }
        }
    }
    
    save_settings(settings);
}

static void toggle_autostart(bool enable) {
    char* autostart_dir = g_build_filename(g_get_user_config_dir(), "autostart", NULL);
    char* desktop_file = g_build_filename(autostart_dir, "linshot.desktop", NULL);
    
    if (enable) {
        // Create autostart directory if it doesn't exist
        g_mkdir_with_parents(autostart_dir, 0755);
        
        // Create desktop entry file
        FILE* file = fopen(desktop_file, "w");
        if (file) {
            fprintf(file, "[Desktop Entry]\n");
            fprintf(file, "Type=Application\n");
            fprintf(file, "Name=LinShot\n");
            fprintf(file, "Exec=%s\n", g_get_current_dir());
            fprintf(file, "Hidden=false\n");
            fprintf(file, "NoDisplay=false\n");
            fprintf(file, "X-GNOME-Autostart-enabled=true\n");
            fclose(file);
        }
    } else {
        // Remove desktop entry file
        g_unlink(desktop_file);
    }
    
    g_free(desktop_file);
    g_free(autostart_dir);
}

static GdkFilterReturn key_filter_func(GdkXEvent* xevent, GdkEvent* event, gpointer data) {
    (void)event;
    MainWindow* win = (MainWindow*)data;
    XEvent* xe = (XEvent*)xevent;
    
    if (xe->type == KeyPress) {
        XKeyEvent* key_event = (XKeyEvent*)xe;
        KeySym key_sym = XLookupKeysym(key_event, 0);
        unsigned int modifiers = key_event->state;
        Settings* settings = safe_get_data(win->window, "settings", "key_filter_func");
        
        // Check if the key combination matches the configured shortcut
        switch (settings->shortcut_key) {
            case SHORTCUT_PRINTSCREEN:
                if (key_sym == XK_Print && modifiers == 0) {
                    on_capture_button_clicked(NULL, win);
                    return GDK_FILTER_REMOVE;
                }
                break;
            case SHORTCUT_CTRL_PRINTSCREEN:
                if (key_sym == XK_Print && (modifiers & ControlMask)) {
                    on_capture_button_clicked(NULL, win);
                    return GDK_FILTER_REMOVE;
                }
                break;
            case SHORTCUT_SHIFT_PRINTSCREEN:
                if (key_sym == XK_Print && (modifiers & ShiftMask)) {
                    on_capture_button_clicked(NULL, win);
                    return GDK_FILTER_REMOVE;
                }
                break;
            case SHORTCUT_CTRL_SHIFT_S:
                if (key_sym == XK_s && (modifiers & (ControlMask | ShiftMask))) {
                    on_capture_button_clicked(NULL, win);
                    return GDK_FILTER_REMOVE;
                }
                break;
            case SHORTCUT_CTRL_ALT_S:
                if (key_sym == XK_s && (modifiers & (ControlMask | Mod1Mask))) {
                    on_capture_button_clicked(NULL, win);
                    return GDK_FILTER_REMOVE;
                }
                break;
            default:
                break;
        }
    }
    
    return GDK_FILTER_CONTINUE;
}

bool main_window_init(MainWindow* win, int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    // Initialize screenshot history
    screenshot_history_init(&win->screenshot_history);
    screenshot_history_load(&win->screenshot_history);
    
    // Create main window
    win->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!validate_gtk_object(win->window, "main_window_init")) {
        return false;
    }
    
    // Initialize other MainWindow fields
    win->toolbar = NULL;
    win->canvas = NULL;
    win->statusbar = NULL;
    win->history_flow_box = NULL;
    
    gtk_window_set_title(GTK_WINDOW(win->window), "LinShot");
    gtk_window_set_default_size(GTK_WINDOW(win->window), 800, 600);
    
    // Connect destroy signal
    g_signal_connect(win->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    // Initialize window data
    MainWindowData* data = g_new0(MainWindowData, 1);
    if (!data) {
        gtk_widget_destroy(win->window);
        return false;
    }
    
    // Initialize the window data structure
    data->win = *win;  // Now safe to copy since win is fully initialized
    data->current_image = NULL;
    tool_settings_init(&data->current_tool);
    data->annotations = NULL;
    data->undo_stack = NULL;
    data->drawing = false;
    data->selected_text = NULL;
    data->drag_start_x = 0;
    data->drag_start_y = 0;
    
    // Set window data using safe wrapper
    safe_set_data_full(win->window, "window-data", data, g_free, "main_window_init");
    
    // Add key press event handling
    gtk_widget_add_events(win->window, GDK_KEY_PRESS_MASK);
    g_signal_connect(win->window, "key-press-event", G_CALLBACK(on_key_press), win);
    
    // Initialize settings
    Settings* settings = g_new0(Settings, 1);
    load_settings(settings);
    safe_set_data_full(win->window, "settings", settings, g_free, "main_window_init");
    
    // Create main horizontal box
    GtkWidget* main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(win->window), main_hbox);
    
    // Create sidebar container
    GtkWidget* sidebar_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(sidebar_container, 110, -1);
    gtk_widget_set_hexpand(sidebar_container, FALSE);
    gtk_widget_set_vexpand(sidebar_container, TRUE);
    
    // Style the sidebar container
    GtkCssProvider* css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider,
        "box.sidebar { background-color: #2d2d2d; }"
        "button.sidebar-button { "
        "   background: none; "
        "   color: #ffffff; "
        "   border: none; "
        "   border-radius: 0; "
        "   padding: 10px 8px 10px 15px; "
        "   margin: 0; "
        "   font-size: 13px; "
        "   min-width: 110px; "
        "   min-height: 0; "
        "   outline: none; "
        "   box-shadow: none; "
        "}"
        "button.sidebar-button:focus { outline: none; box-shadow: none; }"
        "button.sidebar-button:hover { background-color: #3d3d3d; }"
        "button.sidebar-button:active, button.sidebar-button.active { background-color: #4d4d4d; }"
        "label.footer { color: #888888; font-size: 13px; padding: 10px 8px 10px 15px; }"
        "box.content-area { background-color: #2d2d2d; }"
        "drawing-area { background-color: #2d2d2d; }",
        -1, NULL);
    
    GtkStyleContext* sidebar_context = gtk_widget_get_style_context(sidebar_container);
    gtk_style_context_add_provider(sidebar_context,
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_class(sidebar_context, "sidebar");
    
    // Create buttons container
    GtkWidget* buttons_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(sidebar_container), buttons_container, TRUE, TRUE, 0);
    
    // Create buttons with minimal labels
    const char* button_labels[] = {
        "Shot", "Arrow", "Box", "Circle", "Text", "Copy", "Save"
    };
    
    for (int i = 0; i < 7; i++) {
        GtkWidget* button = gtk_button_new_with_label(button_labels[i]);
        gtk_widget_set_hexpand(button, TRUE);
        
        GtkWidget* label = gtk_bin_get_child(GTK_BIN(button));
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        
        // Style the button
        GtkStyleContext* button_context = gtk_widget_get_style_context(button);
        gtk_style_context_add_provider(button_context,
            GTK_STYLE_PROVIDER(css_provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        gtk_style_context_add_class(button_context, "sidebar-button");
        
        // Connect signals based on button type
        if (i == 0) { // ScreenShot button
            g_signal_connect(button, "clicked", G_CALLBACK(on_capture_button_clicked), win);
        } else if (i == 5) { // Copy button
            g_signal_connect(button, "clicked", G_CALLBACK(on_copy_button_clicked), win);
        } else if (i == 6) { // Save button
            g_signal_connect(button, "clicked", G_CALLBACK(on_save_button_clicked), win);
        } else { // Tool buttons
            safe_set_data(button, "tool-id", GINT_TO_POINTER(i), "main_window_init");
            g_signal_connect(button, "clicked", G_CALLBACK(on_tool_button_clicked), win);
        }
        
        gtk_box_pack_start(GTK_BOX(buttons_container), button, FALSE, FALSE, 0);
    }
    
    // Add footer label
    GtkWidget* footer_label = gtk_label_new("SilverMax");
    gtk_widget_set_halign(footer_label, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(footer_label, TRUE);
    GtkStyleContext* footer_context = gtk_widget_get_style_context(footer_label);
    gtk_style_context_add_provider(footer_context,
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_class(footer_context, "footer");
    gtk_box_pack_end(GTK_BOX(sidebar_container), footer_label, FALSE, FALSE, 0);
    
    // Add sidebar to main container
    gtk_box_pack_start(GTK_BOX(main_hbox), sidebar_container, FALSE, FALSE, 0);
    
    // Create content area
    GtkWidget* content_area = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(main_hbox), content_area, TRUE, TRUE, 0);
    
    // Apply content area style
    GtkStyleContext* content_context = gtk_widget_get_style_context(content_area);
    gtk_style_context_add_provider(content_context,
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_class(content_context, "content-area");
    
    // Create notebook for tabs
    GtkWidget* notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(content_area), notebook, TRUE, TRUE, 0);
    
    // Create current screenshot page
    GtkWidget* screenshot_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* screenshot_label = gtk_label_new("Screenshot");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), screenshot_page, screenshot_label);
    
    // Create scrolled window for the canvas
    GtkWidget* canvas_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(canvas_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(canvas_scroll, TRUE);
    gtk_widget_set_vexpand(canvas_scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(screenshot_page), canvas_scroll, TRUE, TRUE, 0);
    
    // Create canvas for displaying captures
    win->canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(win->canvas, 400, 300);  // Set initial size
    gtk_widget_add_events(win->canvas,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);
    
    // Apply drawing area style
    GtkStyleContext* canvas_context = gtk_widget_get_style_context(win->canvas);
    gtk_style_context_add_provider(canvas_context,
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_class(canvas_context, "drawing-area");
    
    // Connect canvas signals
    g_signal_connect(win->canvas, "draw", G_CALLBACK(on_draw), win);
    g_signal_connect(win->canvas, "button-press-event", G_CALLBACK(on_button_press), win);
    g_signal_connect(win->canvas, "button-release-event", G_CALLBACK(on_button_release), win);
    g_signal_connect(win->canvas, "motion-notify-event", G_CALLBACK(on_motion_notify), win);
    
    // Add canvas to scrolled window
    gtk_container_add(GTK_CONTAINER(canvas_scroll), win->canvas);
    
    // Create history page
    GtkWidget* history_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* history_label = gtk_label_new("History");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), history_page, history_label);
    
    // Create scrolled window for history
    GtkWidget* history_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(history_scroll),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(history_scroll, TRUE);
    gtk_widget_set_hexpand(history_scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(history_page), history_scroll);
    
    // Create flow box for history thumbnails
    GtkWidget* flow_box = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(flow_box), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(flow_box), TRUE);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(flow_box), 2);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(flow_box), 5);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(flow_box), 5);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(flow_box), 5);
    gtk_widget_set_margin_start(flow_box, 5);
    gtk_widget_set_margin_end(flow_box, 5);
    gtk_widget_set_margin_top(flow_box, 5);
    gtk_widget_set_margin_bottom(flow_box, 5);
    gtk_container_add(GTK_CONTAINER(history_scroll), flow_box);
    
    // Create statusbar
    win->statusbar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(content_area), win->statusbar, FALSE, FALSE, 0);
    gtk_statusbar_push(GTK_STATUSBAR(win->statusbar), 0, "Ready");
    
    // Update window data with canvas and statusbar
    data->win.canvas = win->canvas;
    data->win.statusbar = win->statusbar;
    
    // Store flow box in window data for history updates
    win->history_flow_box = flow_box;
    
    // Create settings tab
    create_settings_page(win, notebook);
    
    // Show all widgets
    gtk_widget_show_all(win->window);
    
    return true;
}

void main_window_cleanup(MainWindow* win) {
    if (!win) {
        return;
    }
    
    // Clean up window data first
    if (win->window && GTK_IS_WIDGET(win->window)) {
        MainWindowData* data = safe_get_data(win->window, "window-data", "main_window_cleanup");
        if (data) {
            if (data->current_image) {
                cairo_surface_destroy(data->current_image);
                data->current_image = NULL;
            }
            
            // Free both annotations list and undo stack
            g_list_free_full(data->annotations, (GDestroyNotify)annotation_free);
            g_list_free_full(data->undo_stack, (GDestroyNotify)annotation_free);
            data->annotations = NULL;
            data->undo_stack = NULL;
            
            // Remove the data from the window before freeing
            safe_set_data(win->window, "window-data", NULL, "main_window_cleanup");
        }
    }
    
    // Clean up history
    screenshot_history_cleanup(&win->screenshot_history);
    
    // Clean up widgets
    if (win->window && GTK_IS_WIDGET(win->window)) {
        gtk_widget_destroy(win->window);
        win->window = NULL;
    }
} 
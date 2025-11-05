#ifndef CURSOR_MANAGER_H
#define CURSOR_MANAGER_H

#include <gtk/gtk.h>

// Set the cursor for a widget using a CSS cursor name (e.g., "crosshair").
// Falls back to default if the name is not available.
void cursor_manager_set(GtkWidget *widget, const char *cursor_name);

// Reset the cursor for a widget to the default pointer.
void cursor_manager_reset(GtkWidget *widget);

#endif // CURSOR_MANAGER_H 
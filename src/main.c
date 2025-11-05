#include "../include/main_window.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    MainWindow win = {0};
    
    if (!main_window_init(&win, argc, argv)) {
        fprintf(stderr, "Failed to initialize main window\n");
        return 1;
    }
    
    gtk_main();
    
    main_window_cleanup(&win);
    return 0;
} 
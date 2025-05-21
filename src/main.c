#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <ext2fs/ext2_fs.h>
#include "analyzer.h"
#include "editor.h"
#include "ui.h"
#include "utils.h"
#define _POSIX_C_SOURCE 200809L

void print_usage(const char *program_name) {
    printf("Usage: %s <device>\n", program_name);
    printf("\n");
    printf("Examples:\n");
    printf("  %s /dev/sda1           # Open interactive UI for /dev/sda1\n", program_name);
}

int main(int argc, char *argv[]) {
    char *device_path = NULL;

    if (argc != 2) {
        fprintf(stderr, "Error: Device path not specified\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    device_path = argv[1];

    fs_info_t *fs_info = analyzer_init(device_path);
    if (!fs_info) {
        fprintf(stderr, "Error: Failed to initialize filesystem analyzer\n");
        return EXIT_FAILURE;
    }

    // Initialize ncurses
    initscr();
    start_color();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    refresh();

    // Set up color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLUE);    // Header
    init_pair(2, COLOR_BLACK, COLOR_WHITE);   // Status bar
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Highlight
    init_pair(4, COLOR_RED, COLOR_BLACK);     // Error/Warning
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);  // Selected item
    init_pair(6, COLOR_CYAN, COLOR_BLACK);    // Field names
    init_pair(7, COLOR_WHITE, COLOR_RED);     // Modified data

    ui_context_t *ui_ctx = ui_init(fs_info);
    if (!ui_ctx) {
        endwin();
        fprintf(stderr, "Error: Failed to initialize UI\n");
        analyzer_cleanup(fs_info);
        return EXIT_FAILURE;
    }

    // Start main UI loop
    ui_main_loop(ui_ctx);

    ui_cleanup(ui_ctx);
    analyzer_cleanup(fs_info);
    
    // Proper ncurses cleanup
    curs_set(1);  // Make cursor visible again
    echo();       // Enable echo
    endwin();     // End ncurses

    return EXIT_SUCCESS;
}

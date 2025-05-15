#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ncurses.h>
#include <ext2fs/ext2_fs.h>
#include "analyzer.h"
#include "editor.h"
#include "ui.h"
#include "utils.h"
#define _POSIX_C_SOURCE 200809L
/**
 * Print usage information
 * 
 * @param program_name Name of the program
 */
void print_usage(const char *program_name) {
    printf("Usage: %s [options] <device>\n", program_name);
    printf("Options:\n");
    printf("  -h, --help            Display this help message\n");
    printf("  -r, --readonly        Open device in read-only mode\n");
    printf("  -a, --analyze         Print filesystem analysis and exit\n");
    printf("  -b, --block NUM       Open specific block in editor\n");
    printf("  -i, --inode NUM       Open specific inode in editor\n");
    printf("  -s, --superblock      Open superblock in editor\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s /dev/sda1           # Open interactive UI for /dev/sda1\n", program_name);
    printf("  %s -a /dev/sda1        # Print filesystem analysis for /dev/sda1\n", program_name);
    printf("  %s -b 2 /dev/sda1      # Open block 2 in editor\n", program_name);
}

int main(int argc, char *argv[]) {
    int opt;
    bool readonly_mode = false;
    bool analyze_only = false;
    bool open_superblock = false;
    int block_to_open = -1;
    int inode_to_open = -1;
    char *device_path = NULL;

    struct option long_options[] = {
        {"help",      no_argument,       0, 'h'},
        {"readonly",  no_argument,       0, 'r'},
        {"analyze",   no_argument,       0, 'a'},
        {"block",     required_argument, 0, 'b'},
        {"inode",     required_argument, 0, 'i'},
        {"superblock", no_argument,      0, 's'},
        {0, 0, 0, 0}
    };

    // Parse command line options
    while ((opt = getopt_long(argc, argv, "hrab:i:s", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case 'r':
                readonly_mode = true;
                break;
            case 'a':
                analyze_only = true;
                break;
            case 'b':
                block_to_open = atoi(optarg);
                break;
            case 'i':
                inode_to_open = atoi(optarg);
                break;
            case 's':
                open_superblock = true;
                break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Check if device path was provided
    if (optind < argc) {
        device_path = argv[optind];
    } else {
        fprintf(stderr, "Error: No device specified\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Initialize filesystem analyzer
    fs_info_t *fs_info = analyzer_init(device_path);
    if (!fs_info) {
        fprintf(stderr, "Error: Failed to initialize filesystem analyzer\n");
        return EXIT_FAILURE;
    }

    // Handle analyze-only mode
    if (analyze_only) {
        analyze_filesystem(fs_info);
        analyzer_cleanup(fs_info);
        return EXIT_SUCCESS;
    }

    // Initialize ncurses
    initscr();
    start_color();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    refresh();

    // Initialize color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLUE);    // Header
    init_pair(2, COLOR_BLACK, COLOR_WHITE);   // Status bar
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Highlight
    init_pair(4, COLOR_RED, COLOR_BLACK);     // Error/Warning
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);  // Selected item
    init_pair(6, COLOR_CYAN, COLOR_BLACK);    // Field names
    init_pair(7, COLOR_WHITE, COLOR_RED);     // Modified data

    // Initialize UI
    ui_context_t *ui_ctx = ui_init(fs_info);
    if (!ui_ctx) {
        endwin();
        fprintf(stderr, "Error: Failed to initialize UI\n");
        analyzer_cleanup(fs_info);
        return EXIT_FAILURE;
    }

    // Handle direct opening of specific structures
    if (open_superblock) {
        ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
        editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_SUPERBLOCK, 0);
    } else if (block_to_open >= 0) {
        ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
        editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_BLOCK, block_to_open);
    } else if (inode_to_open >= 0) {
        ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
        editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_INODE, inode_to_open);
    }

    // Main UI loop
    ui_main_loop(ui_ctx);

    // Cleanup
    ui_cleanup(ui_ctx);
    analyzer_cleanup(fs_info);
    endwin();

    return EXIT_SUCCESS;
}

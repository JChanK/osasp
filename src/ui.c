#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <menu.h>
#include <form.h>
#include <panel.h>
#include "ui.h"
#include "analyzer.h"
#include "editor.h"
#include "utils.h"
#define _POSIX_C_SOURCE 200809L

#define MAIN_MENU_ITEMS 5
#define MAX_STATUS_MSG 256

static const char *main_menu_items[] = {
    "Filesystem Analysis",
    "Block Browser",
    "Inode Browser",
    "Binary Editor",
    "Exit"
};

ui_context_t *ui_init(fs_info_t *fs_info) {
    ui_context_t *ui_ctx = (ui_context_t *)malloc(sizeof(ui_context_t));
    if (!ui_ctx) {
        return NULL;
    }

    memset(ui_ctx, 0, sizeof(ui_context_t));
    ui_ctx->fs_info = fs_info;
    ui_ctx->current_mode = UI_MODE_MENU;
    ui_ctx->current_block = 1;
    ui_ctx->current_inode = 1;
    ui_ctx->current_group = 0;

    // Initialize main window
    ui_ctx->main_win = newwin(LINES - 3, COLS, 0, 0);
    if (!ui_ctx->main_win) {
        free(ui_ctx);
        return NULL;
    }

    // Initialize status window
    ui_ctx->status_win = newwin(1, COLS, LINES - 2, 0);
    if (!ui_ctx->status_win) {
        delwin(ui_ctx->main_win);
        free(ui_ctx);
        return NULL;
    }

    // Initialize help window
    ui_ctx->help_win = newwin(1, COLS, LINES - 1, 0);
    if (!ui_ctx->help_win) {
        delwin(ui_ctx->status_win);
        delwin(ui_ctx->main_win);
        free(ui_ctx);
        return NULL;
    }

    // Initialize editor context
    ui_ctx->editor_ctx = editor_init(fs_info);
    if (!ui_ctx->editor_ctx) {
        delwin(ui_ctx->help_win);
        delwin(ui_ctx->status_win);
        delwin(ui_ctx->main_win);
        free(ui_ctx);
        return NULL;
    }

    keypad(ui_ctx->main_win, TRUE);
    keypad(ui_ctx->status_win, TRUE);
    keypad(ui_ctx->help_win, TRUE);

    return ui_ctx;
}

void ui_cleanup(ui_context_t *ui_ctx) {
    if (ui_ctx) {
        if (ui_ctx->editor_ctx) {
            editor_cleanup(ui_ctx->editor_ctx);
        }
        if (ui_ctx->help_win) {
            delwin(ui_ctx->help_win);
        }
        if (ui_ctx->status_win) {
            delwin(ui_ctx->status_win);
        }
        if (ui_ctx->main_win) {
            delwin(ui_ctx->main_win);
        }
        free(ui_ctx);
    }
}

void ui_set_mode(ui_context_t *ui_ctx, ui_mode_t mode) {
    if (!ui_ctx) return;
    ui_ctx->current_mode = mode;
}

void ui_display_fs_info(ui_context_t *ui_ctx) {
    if (!ui_ctx || !ui_ctx->fs_info) return;

    werase(ui_ctx->main_win);
    
    char buffer[1024];
    char fs_type[32];
    get_fs_type_string(ui_ctx->fs_info, fs_type, sizeof(fs_type));
    
    // Print basic filesystem info
    wattron(ui_ctx->main_win, A_BOLD);
    mvwprintw(ui_ctx->main_win, 1, 2, "Filesystem Information");
    wattroff(ui_ctx->main_win, A_BOLD);
    
    mvwprintw(ui_ctx->main_win, 3, 2, "Device: %s", ui_ctx->fs_info->device_path);
    mvwprintw(ui_ctx->main_win, 4, 2, "Type: %s", fs_type);
    
    format_value(ui_ctx->fs_info->block_size, buffer, sizeof(buffer), true);
    mvwprintw(ui_ctx->main_win, 5, 2, "Block size: %s", buffer);
    
    uint64_t total_size = (uint64_t)ui_ctx->fs_info->sb.s_blocks_count * ui_ctx->fs_info->block_size;
    format_value(total_size, buffer, sizeof(buffer), true);
    mvwprintw(ui_ctx->main_win, 6, 2, "Total size: %s", buffer);
    
    uint64_t free_size = (uint64_t)ui_ctx->fs_info->sb.s_free_blocks_count * ui_ctx->fs_info->block_size;
    format_value(free_size, buffer, sizeof(buffer), true);
    mvwprintw(ui_ctx->main_win, 7, 2, "Free space: %s", buffer);
    
    float used_percent = 100.0f * (1.0f - (float)ui_ctx->fs_info->sb.s_free_blocks_count / 
                                  ui_ctx->fs_info->sb.s_blocks_count);
    mvwprintw(ui_ctx->main_win, 8, 2, "Used space: %.1f%%", used_percent);
    
    // Print superblock info
    wattron(ui_ctx->main_win, A_BOLD);
    mvwprintw(ui_ctx->main_win, 10, 2, "Superblock Information");
    wattroff(ui_ctx->main_win, A_BOLD);
    
    superblock_to_string(&ui_ctx->fs_info->sb, buffer, sizeof(buffer));
    char *line = strtok(buffer, "\n");
    int row = 12;
    while (line && row < LINES - 5) {
        mvwprintw(ui_ctx->main_win, row++, 2, "%s", line);
        line = strtok(NULL, "\n");
    }
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_block_browser(ui_context_t *ui_ctx) {
    if (!ui_ctx || !ui_ctx->fs_info) return;

    werase(ui_ctx->main_win);
    
    // Print header
    wattron(ui_ctx->main_win, A_BOLD);
    mvwprintw(ui_ctx->main_win, 1, 2, "Block Browser");
    wattroff(ui_ctx->main_win, A_BOLD);
    
    // Print current block info
    mvwprintw(ui_ctx->main_win, 3, 2, "Current Block: %d", ui_ctx->current_block);
    mvwprintw(ui_ctx->main_win, 4, 2, "Status: %s", 
              is_block_allocated(ui_ctx->fs_info, ui_ctx->current_block) ? "Allocated" : "Free");
    
    // Open the block in editor for display
    editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_BLOCK, ui_ctx->current_block);
    editor_render(ui_ctx->editor_ctx);
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_inode_browser(ui_context_t *ui_ctx) {
    if (!ui_ctx || !ui_ctx->fs_info) return;

    werase(ui_ctx->main_win);
    
    // Print header
    wattron(ui_ctx->main_win, A_BOLD);
    mvwprintw(ui_ctx->main_win, 1, 2, "Inode Browser");
    wattroff(ui_ctx->main_win, A_BOLD);
    
    // Print current inode info
    mvwprintw(ui_ctx->main_win, 3, 2, "Current Inode: %d", ui_ctx->current_inode);
    mvwprintw(ui_ctx->main_win, 4, 2, "Status: %s", 
              is_inode_allocated(ui_ctx->fs_info, ui_ctx->current_inode) ? "Allocated" : "Free");
    
    // Read inode data
    struct ext2_inode inode;
    if (read_inode(ui_ctx->fs_info, ui_ctx->current_inode, &inode) == 0) {
        char buffer[1024];
        inode_to_string(&inode, buffer, sizeof(buffer));
        
        char *line = strtok(buffer, "\n");
        int row = 6;
        while (line && row < LINES - 5) {
            mvwprintw(ui_ctx->main_win, row++, 2, "%s", line);
            line = strtok(NULL, "\n");
        }
    } else {
        mvwprintw(ui_ctx->main_win, 6, 2, "Failed to read inode %d", ui_ctx->current_inode);
    }
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_binary_editor(ui_context_t *ui_ctx) {
    if (!ui_ctx || !ui_ctx->editor_ctx) return;
    editor_render(ui_ctx->editor_ctx);
}

void ui_display_status(ui_context_t *ui_ctx, const char *message) {
    if (!ui_ctx || !ui_ctx->status_win) return;
    
    werase(ui_ctx->status_win);
    wattron(ui_ctx->status_win, COLOR_PAIR(2));
    
    char status_msg[MAX_STATUS_MSG];
    snprintf(status_msg, sizeof(status_msg), "Status: %s", message);
    mvwprintw(ui_ctx->status_win, 0, 0, "%-*s", COLS - 1, status_msg);
    
    wattroff(ui_ctx->status_win, COLOR_PAIR(2));
    wrefresh(ui_ctx->status_win);
}

void ui_display_help(ui_context_t *ui_ctx) {
    if (!ui_ctx || !ui_ctx->help_win) return;
    
    werase(ui_ctx->help_win);
    wattron(ui_ctx->help_win, COLOR_PAIR(1));
    
    const char *help_text = "";
    switch (ui_ctx->current_mode) {
        case UI_MODE_MENU:
            help_text = "UP/DOWN:Navigate  ENTER:Select  Q:Quit";
            break;
        case UI_MODE_ANALYZER:
            help_text = "Q:Return to menu";
            break;
        case UI_MODE_BLOCK_BROWSER:
            help_text = "+/-:Change block  E:Edit block  Q:Return to menu";
            break;
        case UI_MODE_INODE_BROWSER:
            help_text = "+/-:Change inode  E:Edit inode  Q:Return to menu";
            break;
        case UI_MODE_BINARY_EDITOR:
            help_text = "Arrows:Move  TAB:Toggle edit  F:Field highlight  S:Save  Q:Quit";
            break;
    }
    
    mvwprintw(ui_ctx->help_win, 0, 0, "%-*s", COLS - 1, help_text);
    wattroff(ui_ctx->help_win, COLOR_PAIR(1));
    wrefresh(ui_ctx->help_win);
}

void ui_show_error(ui_context_t *ui_ctx, const char *message) {
    if (!ui_ctx || !message) return;
    
    WINDOW *err_win = newwin(5, COLS / 2, LINES / 2 - 2, COLS / 4);
    if (!err_win) return;
    
    wattron(err_win, COLOR_PAIR(4) | A_BOLD);
    box(err_win, 0, 0);
    mvwprintw(err_win, 1, 2, "Error");
    wattroff(err_win, COLOR_PAIR(4) | A_BOLD);
    
    wattron(err_win, COLOR_PAIR(4));
    mvwprintw(err_win, 2, 2, "%s", message);
    wattroff(err_win, COLOR_PAIR(4));
    
    mvwprintw(err_win, 4, 2, "Press any key to continue...");
    wrefresh(err_win);
    
    wgetch(err_win);
    delwin(err_win);
    
    touchwin(ui_ctx->main_win);
    touchwin(ui_ctx->status_win);
    touchwin(ui_ctx->help_win);
    wrefresh(ui_ctx->main_win);
    wrefresh(ui_ctx->status_win);
    wrefresh(ui_ctx->help_win);
}

bool ui_prompt(ui_context_t *ui_ctx, const char *prompt, char *buffer, size_t max_len) {
    if (!ui_ctx || !prompt || !buffer || max_len <= 0) return false;
    
    WINDOW *prompt_win = newwin(3, COLS / 2, LINES / 2 - 1, COLS / 4);
    if (!prompt_win) return false;
    
    wattron(prompt_win, COLOR_PAIR(5) | A_BOLD);
    box(prompt_win, 0, 0);
    mvwprintw(prompt_win, 0, 2, " %s ", prompt);
    wattroff(prompt_win, COLOR_PAIR(5) | A_BOLD);
    
    echo();
    curs_set(1);
    mvwgetnstr(prompt_win, 1, 2, buffer, max_len - 1);
    curs_set(0);
    noecho();
    
    delwin(prompt_win);
    
    touchwin(ui_ctx->main_win);
    touchwin(ui_ctx->status_win);
    touchwin(ui_ctx->help_win);
    wrefresh(ui_ctx->main_win);
    wrefresh(ui_ctx->status_win);
    wrefresh(ui_ctx->help_win);
    
    return buffer[0] != '\0';
}

void ui_main_loop(ui_context_t *ui_ctx) {
    if (!ui_ctx) return;
    
    int selected_item = 0;
    bool running = true;
    
    while (running) {
        switch (ui_ctx->current_mode) {
            case UI_MODE_MENU: {
                werase(ui_ctx->main_win);
                
                wattron(ui_ctx->main_win, A_BOLD);
                mvwprintw(ui_ctx->main_win, 1, COLS / 2 - 10, "EXT2/3/4 Filesystem Editor");
                wattroff(ui_ctx->main_win, A_BOLD);
                
                for (int i = 0; i < MAIN_MENU_ITEMS; i++) {
                    if (i == selected_item) {
                        wattron(ui_ctx->main_win, COLOR_PAIR(5));
                    }
                    mvwprintw(ui_ctx->main_win, 5 + i * 2, COLS / 2 - 8, "%s", main_menu_items[i]);
                    if (i == selected_item) {
                        wattroff(ui_ctx->main_win, COLOR_PAIR(5));
                    }
                }
                
                ui_display_status(ui_ctx, "Use arrow keys to navigate, Enter to select");
                ui_display_help(ui_ctx);
                wrefresh(ui_ctx->main_win);
                
                int ch = wgetch(ui_ctx->main_win);
                switch (ch) {
                    case KEY_UP:
                        selected_item = (selected_item - 1 + MAIN_MENU_ITEMS) % MAIN_MENU_ITEMS;
                        break;
                    case KEY_DOWN:
                        selected_item = (selected_item + 1) % MAIN_MENU_ITEMS;
                        break;
                    case '\n':
                    case KEY_ENTER:
                        switch (selected_item) {
                            case 0: // Filesystem Analysis
                                ui_set_mode(ui_ctx, UI_MODE_ANALYZER);
                                break;
                            case 1: // Block Browser
                                ui_set_mode(ui_ctx, UI_MODE_BLOCK_BROWSER);
                                break;
                            case 2: // Inode Browser
                                ui_set_mode(ui_ctx, UI_MODE_INODE_BROWSER);
                                break;
                            case 3: // Binary Editor
                                ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
                                break;
                            case 4: // Exit
                                running = false;
                                break;
                        }
                        break;
                    case 'q':
                    case 'Q':
                        running = false;
                        break;
                }
                break;
            }
            case UI_MODE_ANALYZER:
                ui_display_fs_info(ui_ctx);
                ui_display_status(ui_ctx, "Viewing filesystem information");
                ui_display_help(ui_ctx);
                
                if (wgetch(ui_ctx->main_win)) {
                    ui_set_mode(ui_ctx, UI_MODE_MENU);
                }
                break;
            case UI_MODE_BLOCK_BROWSER:
                ui_display_block_browser(ui_ctx);
                ui_display_status(ui_ctx, "Browsing blocks");
                ui_display_help(ui_ctx);
                
                switch (wgetch(ui_ctx->main_win)) {
                    case '+':
                    case '=':
                        ui_ctx->current_block++;
                        if (ui_ctx->current_block >= ui_ctx->fs_info->sb.s_blocks_count) {
                            ui_ctx->current_block = ui_ctx->fs_info->sb.s_blocks_count - 1;
                        }
                        break;
                    case '-':
                        ui_ctx->current_block--;
                        if (ui_ctx->current_block < 1) {
                            ui_ctx->current_block = 1;
                        }
                        break;
                    case 'e':
                    case 'E':
                        editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_BLOCK, ui_ctx->current_block);
                        ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
                        break;
                    case 'q':
                    case 'Q':
                        ui_set_mode(ui_ctx, UI_MODE_MENU);
                        break;
                }
                break;
            case UI_MODE_INODE_BROWSER:
                ui_display_inode_browser(ui_ctx);
                ui_display_status(ui_ctx, "Browsing inodes");
                ui_display_help(ui_ctx);
                
                switch (wgetch(ui_ctx->main_win)) {
                    case '+':
                    case '=':
                        ui_ctx->current_inode++;
                        if (ui_ctx->current_inode >= ui_ctx->fs_info->sb.s_inodes_count) {
                            ui_ctx->current_inode = ui_ctx->fs_info->sb.s_inodes_count - 1;
                        }
                        break;
                    case '-':
                        ui_ctx->current_inode--;
                        if (ui_ctx->current_inode < 1) {
                            ui_ctx->current_inode = 1;
                        }
                        break;
                    case 'e':
                    case 'E':
                        editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_INODE, ui_ctx->current_inode);
                        ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
                        break;
                    case 'q':
                    case 'Q':
                        ui_set_mode(ui_ctx, UI_MODE_MENU);
                        break;
                }
                break;
            case UI_MODE_BINARY_EDITOR:
                running = editor_handle_key(ui_ctx->editor_ctx, wgetch(ui_ctx->main_win));
                if (running && ui_ctx->editor_ctx->should_exit) {
                    ui_set_mode(ui_ctx, UI_MODE_MENU);
                    ui_ctx->editor_ctx->should_exit = false;
                }
                ui_display_status(ui_ctx, "Binary editor mode");
                ui_display_help(ui_ctx);
                break;
        }
    }
}

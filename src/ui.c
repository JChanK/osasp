#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <ncurses.h>
#include "ui.h"
#include "utils.h"
#include "editor.h"

static bool ui_handle_binary_editor_input(ui_context_t *ui_ctx, int key);
static void ui_display_menu(ui_context_t *ui_ctx);
static bool ui_handle_menu_input(ui_context_t *ui_ctx, int key);
static bool ui_handle_analyzer_input(ui_context_t *ui_ctx, int key);
static bool ui_handle_block_browser_input(ui_context_t *ui_ctx, int key);
static bool ui_handle_inode_browser_input(ui_context_t *ui_ctx, int key);

ui_context_t *ui_init(fs_info_t *fs_info) {
    ui_context_t *ui_ctx = (ui_context_t *)malloc(sizeof(ui_context_t));
    if (!ui_ctx) {
        return NULL;
    }

    ui_ctx->fs_info = fs_info;
    ui_ctx->current_mode = UI_MODE_MENU;
    ui_ctx->current_block = 0;
    ui_ctx->current_inode = 1; 
    ui_ctx->current_group = 0;

    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    ui_ctx->main_win = newwin(max_y - 2, max_x, 0, 0);
    ui_ctx->status_win = newwin(1, max_x, max_y - 2, 0);
    ui_ctx->help_win = newwin(1, max_x, max_y - 1, 0);

    wbkgd(ui_ctx->status_win, COLOR_PAIR(2));
    wbkgd(ui_ctx->help_win, COLOR_PAIR(1));

    ui_ctx->editor_ctx = editor_init(fs_info);
    if (!ui_ctx->editor_ctx) {
        ui_cleanup(ui_ctx);
        return NULL;
    }

    return ui_ctx;
}

void ui_cleanup(ui_context_t *ui_ctx) {
    if (!ui_ctx) {
        return;
    }

    if (ui_ctx->editor_ctx) {
        editor_cleanup(ui_ctx->editor_ctx);
    }

    if (ui_ctx->help_win) {
        delwin(ui_ctx->help_win);
        ui_ctx->help_win = NULL;
    }
    if (ui_ctx->status_win) {
        delwin(ui_ctx->status_win);
        ui_ctx->status_win = NULL;
    }
    if (ui_ctx->main_win) {
        delwin(ui_ctx->main_win);
        ui_ctx->main_win = NULL;
    }

    free(ui_ctx);
}

static void ui_display_status(ui_context_t *ui_ctx, const char *message, ...) {
    int max_y, max_x;
    getmaxyx(ui_ctx->status_win, max_y, max_x);
    (void)max_y; (void)max_x;
    
    werase(ui_ctx->status_win);
    wbkgd(ui_ctx->status_win, COLOR_PAIR(2));
    
    va_list args;
    va_start(args, message);
    vw_printw(ui_ctx->status_win, message, args);
    va_end(args);
    
    wrefresh(ui_ctx->status_win);
}

static void ui_display_detailed_help(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);
    
    int y = 0;
    mvwprintw(ui_ctx->main_win, y++, 0, "EXT Filesystem Editor Help");
    mvwprintw(ui_ctx->main_win, y++, 0, "=========================");
    y++;
    
    mvwprintw(ui_ctx->main_win, y++, 0, "General Navigation:");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - Arrow keys: Move cursor");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - ESC: Return to previous menu");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - Q: Quit the program");
    y++;
    
    mvwprintw(ui_ctx->main_win, y++, 0, "Editable Structures:");
    mvwprintw(ui_ctx->main_win, y++, 0, "  1. Superblock (Block 1)");
    mvwprintw(ui_ctx->main_win, y++, 0, "  2. Group Descriptors (Block 2)");
    mvwprintw(ui_ctx->main_win, y++, 0, "  3. Block Bitmaps (Per group)");
    mvwprintw(ui_ctx->main_win, y++, 0, "  4. Inode Bitmaps (Per group)");
    mvwprintw(ui_ctx->main_win, y++, 0, "  5. Inode Table (Per group)");
    mvwprintw(ui_ctx->main_win, y++, 0, "  6. Data Blocks");
    y++;
    
    mvwprintw(ui_ctx->main_win, y++, 0, "Editing Instructions:");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - TAB: Toggle edit mode");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - In edit mode, type hex digits (0-9, A-F)");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - Each key press modifies a nibble (4 bits)");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - Cursor moves automatically after edit");
    y++;
    
    mvwprintw(ui_ctx->main_win, y++, 0, "Saving Changes:");
    mvwprintw(ui_ctx->main_win, y++, 0, "  1. Make your changes in the editor");
    mvwprintw(ui_ctx->main_win, y++, 0, "  2. Press S to save changes");
    mvwprintw(ui_ctx->main_win, y++, 0, "  3. Changes are written immediately to disk");
    mvwprintw(ui_ctx->main_win, y++, 0, "  - WARNING: Changes can corrupt filesystem!");
    y++;
    
    mvwprintw(ui_ctx->main_win, y++, 0, "Press any key to return...");
    
    wrefresh(ui_ctx->main_win);
    getch(); 
}

void ui_display_help(ui_context_t *ui_ctx) {
    int max_y, max_x;
    getmaxyx(ui_ctx->help_win, max_y, max_x);
    (void)max_y; (void)max_x;
    
    werase(ui_ctx->help_win);
    wbkgd(ui_ctx->help_win, COLOR_PAIR(1));
    
    switch (ui_ctx->current_mode) {
        case UI_MODE_MENU:
            mvwprintw(ui_ctx->help_win, 0, 0, "F1:Help | 1:Analyzer | 2:Block Browser | 3:Inode Browser | Q:Quit");
            break;
        case UI_MODE_ANALYZER:
            mvwprintw(ui_ctx->help_win, 0, 0, "F1:Help | ESC:Back | G:Group | Q:Quit");
            break;
        case UI_MODE_BLOCK_BROWSER:
            mvwprintw(ui_ctx->help_win, 0, 0, "F1:Help | ESC:Back | ARROWS:Navigate | E:Edit Block | G:Go to Block | Q:Quit");
            break;
        case UI_MODE_INODE_BROWSER:
            mvwprintw(ui_ctx->help_win, 0, 0, "F1:Help | ESC:Back | ARROWS:Navigate | E:Edit Inode | G:Go to Inode | Q:Quit");
            break;
        case UI_MODE_BINARY_EDITOR:
            mvwprintw(ui_ctx->help_win, 0, 0, "F1:Help | ESC:Back | ARROWS:Move | TAB:Edit Mode | S:Save | Q:Quit");
            break;
    }
    
    wrefresh(ui_ctx->help_win);
}

void ui_show_error(ui_context_t *ui_ctx, const char *message) {
    ui_display_status(ui_ctx, "%s", message);
    wgetch(ui_ctx->status_win); 
}

bool ui_prompt(ui_context_t *ui_ctx, const char *prompt, char *buffer, size_t max_len) {
    ui_display_status(ui_ctx, "%s", prompt);
    
    echo();
    curs_set(1);
    
    wgetnstr(ui_ctx->status_win, buffer, max_len - 1);
    
    noecho();
    curs_set(0);
    
    if (buffer[0] == '\0') {
        ui_display_status(ui_ctx, "");
        return false;
    }
    
    ui_display_status(ui_ctx, "");
    return true;
}

void ui_set_mode(ui_context_t *ui_ctx, ui_mode_t mode) {
    ui_ctx->current_mode = mode;
    werase(ui_ctx->main_win);
    
    switch (mode) {
        case UI_MODE_MENU:
            ui_display_status(ui_ctx, "EXT Filesystem Analyzer - %s", ui_ctx->fs_info->device_path);
            break;
        case UI_MODE_ANALYZER:
            ui_display_status(ui_ctx, "Filesystem Analyzer - %s", ui_ctx->fs_info->device_path);
            break;
        case UI_MODE_BLOCK_BROWSER:
            ui_display_status(ui_ctx, "Block Browser - Block %d", ui_ctx->current_block);
            break;
        case UI_MODE_INODE_BROWSER:
            ui_display_status(ui_ctx, "Inode Browser - Inode %d", ui_ctx->current_inode);
            break;
        case UI_MODE_BINARY_EDITOR:
            ui_display_status(ui_ctx, "Binary Editor");
            break;
    }
    
    ui_display_help(ui_ctx);
}

static void ui_display_menu(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);
    
    int max_y, max_x;
    getmaxyx(ui_ctx->main_win, max_y, max_x);
    (void)max_y;
    
    char fs_type[32];
    get_fs_type_string(ui_ctx->fs_info, fs_type, sizeof(fs_type));
    
    // Центрируем каждую строку отдельно
    int y = 2;
    const char *title = "EXT Filesystem Analyzer and Editor";
    mvwprintw(ui_ctx->main_win, y++, (max_x - strlen(title)) / 2, title);
    
    const char *divider = "=================================";
    mvwprintw(ui_ctx->main_win, y++, (max_x - strlen(divider)) / 2, divider);
    
    y++;
    char device_info[256];
    snprintf(device_info, sizeof(device_info), "Device: %s", ui_ctx->fs_info->device_path);
    mvwprintw(ui_ctx->main_win, y++, (max_x - strlen(device_info)) / 2, device_info);
    
    char type_info[256];
    snprintf(type_info, sizeof(type_info), "Type: %s", fs_type);
    mvwprintw(ui_ctx->main_win, y++, (max_x - strlen(type_info)) / 2, type_info);
    
    y++;
    const char *menu_items[] = {
        "1. Filesystem Analyzer",
        "2. Block Browser",
        "3. Inode Browser",
        "4. Edit Superblock",
        "",
        "Q. Quit"
    };
    
    // Находим самую длинную строку меню для центрирования
    size_t max_len = 0;
    for (size_t i = 0; i < sizeof(menu_items)/sizeof(menu_items[0]); i++) {
        size_t len = strlen(menu_items[i]);
        if (len > max_len) max_len = len;
    }
    
    // Выводим пункты меню
    for (size_t i = 0; i < sizeof(menu_items)/sizeof(menu_items[0]); i++) {
        if (menu_items[i][0] == '\0') {
            y++; // Пустая строка
            continue;
        }
        mvwprintw(ui_ctx->main_win, y++, (max_x - max_len) / 2, menu_items[i]);
    }
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_fs_info(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);
    
    const struct ext2_super_block *sb = &ui_ctx->fs_info->sb;
    
    int y = 0;
    
    mvwprintw(ui_ctx->main_win, y++, 0, "Filesystem Information:");
    mvwprintw(ui_ctx->main_win, y++, 0, "======================");
    
    char fs_type[32];
    get_fs_type_string(ui_ctx->fs_info, fs_type, sizeof(fs_type));
    
    y++;
    mvwprintw(ui_ctx->main_win, y++, 2, "Filesystem Type: %s", fs_type);
    
    char size_str[32];
    format_value(sb->s_blocks_count * ui_ctx->fs_info->block_size, size_str, sizeof(size_str), true);
    mvwprintw(ui_ctx->main_win, y++, 2, "Filesystem Size: %s", size_str);
    
    mvwprintw(ui_ctx->main_win, y++, 2, "Block Size: %u bytes", ui_ctx->fs_info->block_size);
    mvwprintw(ui_ctx->main_win, y++, 2, "Inode Size: %u bytes", sb->s_inode_size);
    
    mvwprintw(ui_ctx->main_win, y++, 2, "Blocks Count: %u", sb->s_blocks_count);
    mvwprintw(ui_ctx->main_win, y++, 2, "Free Blocks: %u", sb->s_free_blocks_count);
    
    mvwprintw(ui_ctx->main_win, y++, 2, "Inodes Count: %u", sb->s_inodes_count);
    mvwprintw(ui_ctx->main_win, y++, 2, "Free Inodes: %u", sb->s_free_inodes_count);
    
    mvwprintw(ui_ctx->main_win, y++, 2, "Block Groups: %u", ui_ctx->fs_info->groups_count);
    mvwprintw(ui_ctx->main_win, y++, 2, "Blocks Per Group: %u", ui_ctx->fs_info->blocks_per_group);
    mvwprintw(ui_ctx->main_win, y++, 2, "Inodes Per Group: %u", ui_ctx->fs_info->inodes_per_group);
    
    y++;
    mvwprintw(ui_ctx->main_win, y++, 0, "Block Group #%d Information:", ui_ctx->current_group);
    mvwprintw(ui_ctx->main_win, y++, 0, "=============================");
    
    if ((uint32_t)ui_ctx->current_group < ui_ctx->fs_info->groups_count) {
        struct ext2_group_desc *gd = &ui_ctx->fs_info->group_desc[ui_ctx->current_group];
        
        mvwprintw(ui_ctx->main_win, y++, 2, "Block Bitmap: %u", gd->bg_block_bitmap);
        mvwprintw(ui_ctx->main_win, y++, 2, "Inode Bitmap: %u", gd->bg_inode_bitmap);
        mvwprintw(ui_ctx->main_win, y++, 2, "Inode Table: %u", gd->bg_inode_table);
        mvwprintw(ui_ctx->main_win, y++, 2, "Free Blocks Count: %u", gd->bg_free_blocks_count);
        mvwprintw(ui_ctx->main_win, y++, 2, "Free Inodes Count: %u", gd->bg_free_inodes_count);
        mvwprintw(ui_ctx->main_win, y++, 2, "Used Directories Count: %u", gd->bg_used_dirs_count);
    } else {
        mvwprintw(ui_ctx->main_win, y++, 2, "Invalid block group number");
    }
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_block_browser(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);
    
    mvwprintw(ui_ctx->main_win, 0, 0, "Block Browser - Block %d of %d", 
              ui_ctx->current_block, ui_ctx->fs_info->sb.s_blocks_count - 1);
    mvwprintw(ui_ctx->main_win, 1, 0, "===============================");
    
    bool is_allocated = is_block_allocated(ui_ctx->fs_info, ui_ctx->current_block);
    
    char block_type[64] = "Regular Data Block";
    
    if (ui_ctx->current_block == 0) {
        strcpy(block_type, "Reserved (Block 0)");
    } else if (ui_ctx->current_block == 1) {
        strcpy(block_type, "Superblock");
    } else if ((uint32_t)ui_ctx->current_block >= 1 && (uint32_t)ui_ctx->current_block < 1 + ui_ctx->fs_info->groups_count) {
        strcpy(block_type, "Group Descriptor");
    } else {
        for (uint32_t i = 0; i < ui_ctx->fs_info->groups_count; i++) {
            struct ext2_group_desc *gd = &ui_ctx->fs_info->group_desc[i];
            
            if ((uint32_t)ui_ctx->current_block == gd->bg_block_bitmap) {
                snprintf(block_type, sizeof(block_type), "Block Bitmap (Group %u)", i);
                break;
            } else if ((uint32_t)ui_ctx->current_block == gd->bg_inode_bitmap) {
                snprintf(block_type, sizeof(block_type), "Inode Bitmap (Group %u)", i);
                break;
            } else if ((uint32_t)ui_ctx->current_block >= gd->bg_inode_table && 
                       (uint32_t)ui_ctx->current_block < gd->bg_inode_table + 
                       (ui_ctx->fs_info->inodes_per_group * ui_ctx->fs_info->sb.s_inode_size + ui_ctx->fs_info->block_size - 1) / 
                        ui_ctx->fs_info->block_size) {
                snprintf(block_type, sizeof(block_type), "Inode Table (Group %u)", i);
                break;
            }
        }
    }
    
    mvwprintw(ui_ctx->main_win, 3, 0, "Block Status: %s", is_allocated ? "Allocated" : "Free");
    mvwprintw(ui_ctx->main_win, 4, 0, "Block Type: %s", block_type);
    mvwprintw(ui_ctx->main_win, 5, 0, "Block Size: %u bytes", ui_ctx->fs_info->block_size);

    uint32_t block_group = ui_ctx->current_block / ui_ctx->fs_info->blocks_per_group;
    uint32_t block_in_group = ui_ctx->current_block % ui_ctx->fs_info->blocks_per_group;
    
    mvwprintw(ui_ctx->main_win, 6, 0, "Block Group: %u", block_group);
    mvwprintw(ui_ctx->main_win, 7, 0, "Block in Group: %u", block_in_group);
    
    uint8_t *block_data = malloc(ui_ctx->fs_info->block_size);
    if (block_data) {
        if (read_block(ui_ctx->fs_info, ui_ctx->current_block, block_data) == 0) {
            mvwprintw(ui_ctx->main_win, 9, 0, "Block Data (first 256 bytes):");
            
            int max_rows = 10;
            int bytes_per_row = 16;
            uint32_t rows = (uint32_t)ui_ctx->fs_info->block_size < (uint32_t)(max_rows * bytes_per_row) ? 
                       ui_ctx->fs_info->block_size / bytes_per_row : (uint32_t)max_rows;
            
            for (int i = 0; (uint32_t)i < rows; i++) {
                mvwprintw(ui_ctx->main_win, 10 + i, 0, "%04X: ", i * bytes_per_row);
                
                for (int j = 0; j < bytes_per_row; j++) {
                    int offset = i * bytes_per_row + j;
                    if ((uint32_t)offset < ui_ctx->fs_info->block_size) {
                        wprintw(ui_ctx->main_win, "%02X ", block_data[offset]);
                    } else {
                        wprintw(ui_ctx->main_win, "   ");
                    }
                }
                
                wprintw(ui_ctx->main_win, " | ");
                
                for (int j = 0; j < bytes_per_row; j++) {
                    int offset = i * bytes_per_row + j;
                    if ((uint32_t)offset < ui_ctx->fs_info->block_size) {
                        char c = block_data[offset];
                        wprintw(ui_ctx->main_win, "%c", isprint(c) ? c : '.');
                    } else {
                        wprintw(ui_ctx->main_win, " ");
                    }
                }
            }
        } else {
            mvwprintw(ui_ctx->main_win, 9, 0, "Error reading block data");
        }
        
        free(block_data);
    } else {
        mvwprintw(ui_ctx->main_win, 9, 0, "Memory allocation error");
    }
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_inode_browser(ui_context_t *ui_ctx) {
    werase(ui_ctx->main_win);
    
    mvwprintw(ui_ctx->main_win, 0, 0, "Inode Browser - Inode %d of %d", 
              ui_ctx->current_inode, ui_ctx->fs_info->sb.s_inodes_count - 1);
    mvwprintw(ui_ctx->main_win, 1, 0, "===============================");
    
    bool is_allocated = is_inode_allocated(ui_ctx->fs_info, ui_ctx->current_inode);
    
    mvwprintw(ui_ctx->main_win, 3, 0, "Inode Status: %s", is_allocated ? "Allocated" : "Free");
    
    struct ext2_inode inode;
    if (read_inode(ui_ctx->fs_info, ui_ctx->current_inode, &inode) == 0) {
        mvwprintw(ui_ctx->main_win, 4, 0, "Mode: 0%o", inode.i_mode);
        
        char file_type[32] = "Unknown";
        if (S_ISREG(inode.i_mode)) strcpy(file_type, "Regular File");
        else if (S_ISDIR(inode.i_mode)) strcpy(file_type, "Directory");
        else if (S_ISLNK(inode.i_mode)) strcpy(file_type, "Symbolic Link");
        else if (S_ISCHR(inode.i_mode)) strcpy(file_type, "Character Device");
        else if (S_ISBLK(inode.i_mode)) strcpy(file_type, "Block Device");
        else if (S_ISFIFO(inode.i_mode)) strcpy(file_type, "FIFO");
        else if (S_ISSOCK(inode.i_mode)) strcpy(file_type, "Socket");
        
        mvwprintw(ui_ctx->main_win, 5, 0, "File Type: %s", file_type);
        
        char size_str[32];
        format_value(inode.i_size, size_str, sizeof(size_str), true);
        mvwprintw(ui_ctx->main_win, 6, 0, "Size: %s", size_str);
        
        mvwprintw(ui_ctx->main_win, 7, 0, "Links: %u", inode.i_links_count);
        mvwprintw(ui_ctx->main_win, 8, 0, "UID: %u", inode.i_uid);
        mvwprintw(ui_ctx->main_win, 9, 0, "GID: %u", inode.i_gid);
        
        char atime_buf[32], mtime_buf[32], ctime_buf[32];
        time_t atime = inode.i_atime;
        time_t mtime = inode.i_mtime;
        time_t ctime = inode.i_ctime;
        
        struct tm *atime_tm = localtime(&atime);
        struct tm *mtime_tm = localtime(&mtime);
        struct tm *ctime_tm = localtime(&ctime);
        
        strftime(atime_buf, sizeof(atime_buf), "%Y-%m-%d %H:%M:%S", atime_tm);
        strftime(mtime_buf, sizeof(mtime_buf), "%Y-%m-%d %H:%M:%S", mtime_tm);
        strftime(ctime_buf, sizeof(ctime_buf), "%Y-%m-%d %H:%M:%S", ctime_tm);
        
        mvwprintw(ui_ctx->main_win, 10, 0, "Access Time: %s", atime_buf);
        mvwprintw(ui_ctx->main_win, 11, 0, "Modify Time: %s", mtime_buf);
        mvwprintw(ui_ctx->main_win, 12, 0, "Change Time: %s", ctime_buf);
        
        mvwprintw(ui_ctx->main_win, 14, 0, "Direct Blocks:");
        for (int i = 0; i < 12 && i < 8; i++) {
            mvwprintw(ui_ctx->main_win, 15 + i, 2, "[%d]: %u", i, inode.i_block[i]);
        }
        
        mvwprintw(ui_ctx->main_win, 23, 0, "Indirect Blocks:");
        mvwprintw(ui_ctx->main_win, 24, 2, "Single: %u", inode.i_block[12]);
        mvwprintw(ui_ctx->main_win, 25, 2, "Double: %u", inode.i_block[13]);
        mvwprintw(ui_ctx->main_win, 26, 2, "Triple: %u", inode.i_block[14]);
    } else {
        mvwprintw(ui_ctx->main_win, 4, 0, "Error reading inode");
    }
    
    wrefresh(ui_ctx->main_win);
}

void ui_display_binary_editor(ui_context_t *ui_ctx) {
    editor_render(ui_ctx->editor_ctx);
}

static bool ui_handle_binary_editor_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27: // ESC
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case 'q':
        case 'Q':
            return false;
        default:
            return editor_handle_key(ui_ctx->editor_ctx, key);
    }
}

void ui_main_loop(ui_context_t *ui_ctx) {
    bool running = true;
    
    ui_set_mode(ui_ctx, UI_MODE_MENU);
    
    while (running) {
        switch (ui_ctx->current_mode) {
            case UI_MODE_MENU:
                ui_display_menu(ui_ctx);
                break;
            case UI_MODE_ANALYZER:
                ui_display_fs_info(ui_ctx);
                break;
            case UI_MODE_BLOCK_BROWSER:
                ui_display_block_browser(ui_ctx);
                break;
            case UI_MODE_INODE_BROWSER:
                ui_display_inode_browser(ui_ctx);
                break;
            case UI_MODE_BINARY_EDITOR:
                ui_display_binary_editor(ui_ctx);
                break;
        }
        
        ui_display_help(ui_ctx);
        
        int key = getch();
        
        if (key == KEY_F(1)) {
            ui_display_detailed_help(ui_ctx);
            continue; 
        }
        
        switch (ui_ctx->current_mode) {
            case UI_MODE_MENU:
                running = ui_handle_menu_input(ui_ctx, key);
                break;
            case UI_MODE_ANALYZER:
                running = ui_handle_analyzer_input(ui_ctx, key);
                break;
            case UI_MODE_BLOCK_BROWSER:
                running = ui_handle_block_browser_input(ui_ctx, key);
                break;
            case UI_MODE_INODE_BROWSER:
                running = ui_handle_inode_browser_input(ui_ctx, key);
                break;
            case UI_MODE_BINARY_EDITOR:
                running = ui_handle_binary_editor_input(ui_ctx, key);
                break;
        }
    }
}

static bool ui_handle_menu_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case '1':
            ui_set_mode(ui_ctx, UI_MODE_ANALYZER);
            return true;
        case '2':
            ui_set_mode(ui_ctx, UI_MODE_BLOCK_BROWSER);
            return true;
        case '3':
            ui_set_mode(ui_ctx, UI_MODE_INODE_BROWSER);
            return true;
        case '4':
            ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
            editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_SUPERBLOCK, 0);
            return true;
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}

static bool ui_handle_analyzer_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27: // ESC
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case 'g': case 'G': {
            char buffer[32];
            if (ui_prompt(ui_ctx, "Enter group number: ", buffer, sizeof(buffer))) {
                uint32_t group;
                if (sscanf(buffer, "%u", &group) == 1 && group < ui_ctx->fs_info->groups_count) {
                    ui_ctx->current_group = (int)group;
                }
            }
            return true;
        }
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}

static bool ui_handle_block_browser_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27: // ESC
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case KEY_LEFT:
            if (ui_ctx->current_block > 0) {
                ui_ctx->current_block--;
                ui_display_block_browser(ui_ctx);
                ui_display_status(ui_ctx, "Block Browser - Block %d", ui_ctx->current_block);
            }
            return true;
        case KEY_RIGHT:
            if ((uint32_t)ui_ctx->current_block < ui_ctx->fs_info->sb.s_blocks_count - 1) {
                ui_ctx->current_block++;
                ui_display_block_browser(ui_ctx);
                ui_display_status(ui_ctx, "Block Browser - Block %d", ui_ctx->current_block);
            }
            return true;
        case KEY_UP:
            if (ui_ctx->current_block >= 10) {
                ui_ctx->current_block -= 10;
                ui_display_block_browser(ui_ctx);
                ui_display_status(ui_ctx, "Block Browser - Block %d", ui_ctx->current_block);
            }
            return true;
        case KEY_DOWN:
            if ((uint32_t)ui_ctx->current_block + 10 < ui_ctx->fs_info->sb.s_blocks_count) {
                ui_ctx->current_block += 10;
                ui_display_block_browser(ui_ctx);
                ui_display_status(ui_ctx, "Block Browser - Block %d", ui_ctx->current_block);
            }
            return true;
        case 'e':
        case 'E':
            ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
            editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_BLOCK, ui_ctx->current_block);
            return true;
        case 'g':
        case 'G': {
            char buffer[32];
            if (ui_prompt(ui_ctx, "Enter block number: ", buffer, sizeof(buffer))) {
                int block = atoi(buffer);
                if ( (uint32_t)block < ui_ctx->fs_info->sb.s_blocks_count) {
                    ui_ctx->current_block = block;
                    ui_display_block_browser(ui_ctx);
                    ui_display_status(ui_ctx, "Block Browser - Block %d", ui_ctx->current_block);
                } else {
                    ui_show_error(ui_ctx, "Invalid block number");
                }
            }
            return true;
        }
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}

static bool ui_handle_inode_browser_input(ui_context_t *ui_ctx, int key) {
    switch (key) {
        case 27: // ESC
            ui_set_mode(ui_ctx, UI_MODE_MENU);
            return true;
        case KEY_LEFT:
            if (ui_ctx->current_inode > 1) {
                ui_ctx->current_inode--;
                ui_display_inode_browser(ui_ctx);
                ui_display_status(ui_ctx, "Inode Browser - Inode %d", ui_ctx->current_inode);
            }
            return true;
        case KEY_RIGHT:
            if ((uint32_t)ui_ctx->current_inode < ui_ctx->fs_info->sb.s_inodes_count - 1) {
                ui_ctx->current_inode++;
                ui_display_inode_browser(ui_ctx);
                ui_display_status(ui_ctx, "Inode Browser - Inode %d", ui_ctx->current_inode);
            }
            return true;
        case KEY_UP:
            if (ui_ctx->current_inode > 10) {
                ui_ctx->current_inode -= 10;
                ui_display_inode_browser(ui_ctx);
                ui_display_status(ui_ctx, "Inode Browser - Inode %d", ui_ctx->current_inode);
            }
            return true;
        case KEY_DOWN:
            if ((uint32_t)ui_ctx->current_inode + 10 < ui_ctx->fs_info->sb.s_inodes_count) {
                ui_ctx->current_inode += 10;
                ui_display_inode_browser(ui_ctx);
                ui_display_status(ui_ctx, "Inode Browser - Inode %d", ui_ctx->current_inode);
            }
            return true;
        case 'e':
        case 'E':
            ui_set_mode(ui_ctx, UI_MODE_BINARY_EDITOR);
            editor_open_structure(ui_ctx->editor_ctx, STRUCTURE_INODE, ui_ctx->current_inode);
            return true;
        case 'g':
        case 'G': {
            char buffer[32];
            if (ui_prompt(ui_ctx, "Enter inode number: ", buffer, sizeof(buffer))) {
                int inode = atoi(buffer);
                if ((uint32_t)inode > 0 && (uint32_t)inode < ui_ctx->fs_info->sb.s_inodes_count) {
                    ui_ctx->current_inode = inode;
                    ui_display_inode_browser(ui_ctx);
                    ui_display_status(ui_ctx, "Inode Browser - Inode %d", ui_ctx->current_inode);
                } else {
                    ui_show_error(ui_ctx, "Invalid inode number");
                }
            }
            return true;
        }
        case 'q':
        case 'Q':
            return false;
        default:
            return true;
    }
}

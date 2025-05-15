#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <ncurses.h>
#include <ext2fs/ext2_fs.h>

#include <unistd.h>

#include "editor.h"
#include "analyzer.h"
#include "utils.h"
#include "ui.h"

#define _POSIX_C_SOURCE 200809L
#define BYTES_PER_ROW 16
#define VIEW_ROWS 16

editor_context_t *editor_init(fs_info_t *fs_info) {
    editor_context_t *ctx = (editor_context_t *)malloc(sizeof(editor_context_t));
    if (!ctx) {
        return NULL;
    }

    memset(ctx, 0, sizeof(editor_context_t));
    ctx->fs_info = fs_info;
    ctx->bytes_per_row = BYTES_PER_ROW;
    ctx->view_rows = VIEW_ROWS;
    ctx->buffer_size = ctx->bytes_per_row * ctx->view_rows;
    ctx->buffer = (uint8_t *)malloc(ctx->buffer_size);
    
    if (!ctx->buffer) {
        free(ctx);
        return NULL;
    }

    return ctx;
}

void editor_cleanup(editor_context_t *ctx) {
    if (ctx) {
        if (ctx->buffer) {
            free(ctx->buffer);
        }
        free(ctx);
    }
}

int editor_open_structure(editor_context_t *ctx, structure_type_t type, uint32_t id) {
    if (!ctx || !ctx->fs_info) {
        return -1;
    }

    uint32_t block_num = 0;
    size_t structure_size = 0;

    switch (type) {
        case STRUCTURE_SUPERBLOCK:
            block_num = 1; // Superblock is at block 1 (offset 1024)
            structure_size = sizeof(struct ext2_super_block);
            break;
        case STRUCTURE_GROUP_DESC: {
            uint32_t group = id;
            if (group >= ctx->fs_info->groups_count) {
                return -1;
            }
            block_num = ctx->fs_info->group_desc[group].bg_block_bitmap;
            structure_size = ctx->fs_info->block_size;
            break;
        }
        case STRUCTURE_INODE: {
            if (id <= 0 || id > ctx->fs_info->sb.s_inodes_count) {
                return -1;
            }
            uint32_t group = (id - 1) / ctx->fs_info->inodes_per_group;
            uint32_t index = (id - 1) % ctx->fs_info->inodes_per_group;
            block_num = ctx->fs_info->group_desc[group].bg_inode_table + 
                       (index * ctx->fs_info->sb.s_inode_size) / ctx->fs_info->block_size;
            structure_size = ctx->fs_info->sb.s_inode_size;
            break;
        }
        case STRUCTURE_BLOCK:
            if (id <= 0 || id >= ctx->fs_info->sb.s_blocks_count) {
                return -1;
            }
            block_num = id;
            structure_size = ctx->fs_info->block_size;
            break;
        case STRUCTURE_BLOCK_BITMAP: {
            uint32_t group = id;
            if (group >= ctx->fs_info->groups_count) {
                return -1;
            }
            block_num = ctx->fs_info->group_desc[group].bg_block_bitmap;
            structure_size = ctx->fs_info->block_size;
            break;
        }
        case STRUCTURE_INODE_BITMAP: {
            uint32_t group = id;
            if (group >= ctx->fs_info->groups_count) {
                return -1;
            }
            block_num = ctx->fs_info->group_desc[group].bg_inode_bitmap;
            structure_size = ctx->fs_info->block_size;
            break;
        }
        default:
            return -1;
    }

    ctx->current_offset = (uint64_t)block_num * ctx->fs_info->block_size;
    ctx->current_structure = type;
    ctx->current_id = id;

    // Read the data into buffer
    if (pread(ctx->fs_info->fd, ctx->buffer, ctx->buffer_size, ctx->current_offset) != ctx->buffer_size) {
        return -1;
    }

    ctx->cursor_x = 0;
    ctx->cursor_y = 0;
    ctx->editing_mode = false;

    return 0;
}

int editor_save_changes(editor_context_t *ctx) {
    if (!ctx || !ctx->fs_info) {
        return -1;
    }

    if (pwrite(ctx->fs_info->fd, ctx->buffer, ctx->buffer_size, ctx->current_offset) != ctx->buffer_size) {
        return -1;
    }

    return 0;
}

void editor_move_cursor(editor_context_t *ctx, int dx, int dy) {
    if (!ctx) return;

    int new_x = (int)ctx->cursor_x + dx;
    int new_y = (int)ctx->cursor_y + dy;

    if (new_x >= 0 && new_x < (int)ctx->bytes_per_row) {
        ctx->cursor_x = (uint32_t)new_x;
    }

    if (new_y >= 0 && new_y < (int)ctx->view_rows) {
        ctx->cursor_y = (uint32_t)new_y;
    }
}

void editor_set_byte(editor_context_t *ctx, uint8_t value) {
    if (!ctx || !ctx->buffer) return;

    size_t offset = ctx->cursor_y * ctx->bytes_per_row + ctx->cursor_x;
    if (offset < ctx->buffer_size) {
        ctx->buffer[offset] = value;
    }
}

void editor_toggle_field_highlight(editor_context_t *ctx) {
    if (!ctx) return;
    ctx->field_highlight = !ctx->field_highlight;
}

void editor_render(editor_context_t *ctx) {
    if (!ctx || !ctx->buffer) return;

    clear();
    
    // Print header
    char title[128];
    switch (ctx->current_structure) {
        case STRUCTURE_SUPERBLOCK:
            snprintf(title, sizeof(title), "Superblock (Offset: 0x%lx)", ctx->current_offset);
            break;
        case STRUCTURE_GROUP_DESC:
            snprintf(title, sizeof(title), "Group Descriptor %u (Offset: 0x%lx)", 
                    ctx->current_id, ctx->current_offset);
            break;
        case STRUCTURE_INODE:
            snprintf(title, sizeof(title), "Inode %u (Offset: 0x%lx)", 
                    ctx->current_id, ctx->current_offset);
            break;
        case STRUCTURE_BLOCK:
            snprintf(title, sizeof(title), "Block %u (Offset: 0x%lx)", 
                    ctx->current_id, ctx->current_offset);
            break;
        case STRUCTURE_BLOCK_BITMAP:
            snprintf(title, sizeof(title), "Block Bitmap for Group %u (Offset: 0x%lx)", 
                    ctx->current_id, ctx->current_offset);
            break;
        case STRUCTURE_INODE_BITMAP:
            snprintf(title, sizeof(title), "Inode Bitmap for Group %u (Offset: 0x%lx)", 
                    ctx->current_id, ctx->current_offset);
            break;
        default:
            snprintf(title, sizeof(title), "Binary Editor (Offset: 0x%lx)", ctx->current_offset);
    }
    
    attron(A_BOLD);
    mvprintw(0, 0, "%s", title);
    attroff(A_BOLD);
    
    // Print hex dump
    for (int row = 0; row < ctx->view_rows; row++) {
        // Print offset
        mvprintw(row + 2, 0, "%08lx:", ctx->current_offset + row * ctx->bytes_per_row);
        
        // Print hex values
        for (int col = 0; col < ctx->bytes_per_row; col++) {
            size_t offset = row * ctx->bytes_per_row + col;
            if (offset >= ctx->buffer_size) {
                break;
            }
            
            // Highlight current cursor position
            if (row == ctx->cursor_y && col == ctx->cursor_x) {
                attron(A_REVERSE);
            }
            
            // Highlight structure fields if enabled
            if (ctx->field_highlight) {
                // Superblock field highlighting
                if (ctx->current_structure == STRUCTURE_SUPERBLOCK && 
                    offset < sizeof(struct ext2_super_block)) {
                    attron(COLOR_PAIR(6));
                }
                // Inode field highlighting
                else if (ctx->current_structure == STRUCTURE_INODE && 
                         offset < sizeof(struct ext2_inode)) {
                    attron(COLOR_PAIR(6));
                }
            }
            
            mvprintw(row + 2, 9 + col * 3, "%02x", ctx->buffer[offset]);
            
            if (row == ctx->cursor_y && col == ctx->cursor_x) {
                attroff(A_REVERSE);
            }
            
            if (ctx->field_highlight) {
                attroff(COLOR_PAIR(6));
            }
        }
        
        // Print ASCII representation
        mvprintw(row + 2, 9 + ctx->bytes_per_row * 3 + 2, "|");
        for (int col = 0; col < ctx->bytes_per_row; col++) {
            size_t offset = row * ctx->bytes_per_row + col;
            if (offset >= ctx->buffer_size) {
                break;
            }
            
            char c = isprint(ctx->buffer[offset]) ? ctx->buffer[offset] : '.';
            mvaddch(row + 2, 9 + ctx->bytes_per_row * 3 + 3 + col, c);
        }
        mvprintw(row + 2, 9 + ctx->bytes_per_row * 3 + 3 + ctx->bytes_per_row, "|");
    }
    
    // Print status line
    attron(COLOR_PAIR(2));
    mvprintw(ctx->view_rows + 3, 0, "Cursor: %02d:%02d | Offset: 0x%08lx | Value: 0x%02x '%c' | %s", 
            ctx->cursor_y, ctx->cursor_x,
            ctx->current_offset + ctx->cursor_y * ctx->bytes_per_row + ctx->cursor_x,
            ctx->buffer[ctx->cursor_y * ctx->bytes_per_row + ctx->cursor_x],
            isprint(ctx->buffer[ctx->cursor_y * ctx->bytes_per_row + ctx->cursor_x]) ? 
                ctx->buffer[ctx->cursor_y * ctx->bytes_per_row + ctx->cursor_x] : '.',
            ctx->editing_mode ? "EDIT" : "VIEW");
    attroff(COLOR_PAIR(2));
    
    // Print help line
    attron(COLOR_PAIR(1));
    mvprintw(ctx->view_rows + 4, 0, 
             "Arrows:Move  TAB:Toggle edit  F:Field highlight  S:Save  Q:Quit");
    attroff(COLOR_PAIR(1));
    
    refresh();
}

bool editor_handle_key(editor_context_t *ctx, int key) {
    if (!ctx) return false;

    switch (key) {
        case KEY_LEFT:
            editor_move_cursor(ctx, -1, 0);
            break;
        case KEY_RIGHT:
            editor_move_cursor(ctx, 1, 0);
            break;
        case KEY_UP:
            editor_move_cursor(ctx, 0, -1);
            break;
        case KEY_DOWN:
            editor_move_cursor(ctx, 0, 1);
            break;
        case KEY_PPAGE: {
            // Page up - move to previous block
            if (ctx->current_offset >= ctx->fs_info->block_size) {
                ctx->current_offset -= ctx->fs_info->block_size;
                pread(ctx->fs_info->fd, ctx->buffer, ctx->buffer_size, ctx->current_offset);
            }
            break;
        }
        case KEY_NPAGE: {
            // Page down - move to next block
            ctx->current_offset += ctx->fs_info->block_size;
            pread(ctx->fs_info->fd, ctx->buffer, ctx->buffer_size, ctx->current_offset);
            break;
        }
        case '\t':
            ctx->editing_mode = !ctx->editing_mode;
            break;
        case 'f':
        case 'F':
            editor_toggle_field_highlight(ctx);
            break;
        case 's':
        case 'S':
            editor_save_changes(ctx);
            break;
        case 'q':
        case 'Q':
            return false;
        default:
            if (ctx->editing_mode && isxdigit(key)) {
                // Handle hex input for editing
                static uint8_t nibble = 0;
                static bool high_nibble = true;
                uint8_t value = 0;
                
                if (key >= '0' && key <= '9') {
                    value = key - '0';
                } else if (key >= 'a' && key <= 'f') {
                    value = key - 'a' + 10;
                } else if (key >= 'A' && key <= 'F') {
                    value = key - 'A' + 10;
                }
                
                if (high_nibble) {
                    nibble = value << 4;
                    high_nibble = false;
                } else {
                    nibble |= value;
                    high_nibble = true;
                    editor_set_byte(ctx, nibble);
                    editor_move_cursor(ctx, 1, 0);
                }
            }
    }
    
    return true;
}

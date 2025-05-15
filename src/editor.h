#ifndef EDITOR_H
#define EDITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h> 
#include "analyzer.h"

#define _POSIX_C_SOURCE 200809L

/**
 * Structure type to edit
 */
typedef enum {
    STRUCTURE_SUPERBLOCK,       // Superblock
    STRUCTURE_GROUP_DESC,       // Group descriptor
    STRUCTURE_INODE,            // Inode
    STRUCTURE_BLOCK,            // Data block
    STRUCTURE_BLOCK_BITMAP,     // Block bitmap
    STRUCTURE_INODE_BITMAP      // Inode bitmap
} structure_type_t;
/**
 * Binary editor context
 */
typedef struct {
    fs_info_t *fs_info;         // Filesystem information
    uint64_t current_offset;    // Current offset in the file/device
    uint8_t *buffer;            // Buffer for the current view
    size_t buffer_size;         // Size of the buffer
    uint32_t cursor_x;          // Cursor X position (column)
    uint32_t cursor_y;          // Cursor Y position (row)
    uint32_t bytes_per_row;     // Number of bytes displayed per row
    uint32_t view_rows;         // Number of rows in the view
    bool editing_mode;          // Whether in editing mode
    bool field_highlight;       // Whether to highlight structure fields
    structure_type_t current_structure; // Current structure being edited
    uint32_t current_id;        // ID of current structure (block/inode number)
    bool should_exit;           // Flag to indicate if editor should exit
} editor_context_t;

/**
 * Initialize binary editor
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @return Pointer to editor_context_t structure or NULL on error
 */
editor_context_t *editor_init(fs_info_t *fs_info);

/**
 * Close and cleanup binary editor
 * 
 * @param ctx Pointer to editor_context_t structure
 */
void editor_cleanup(editor_context_t *ctx);

/**
 * Open a structure for editing
 * 
 * @param ctx Pointer to editor_context_t structure
 * @param type Type of structure to edit
 * @param id Identifier of the structure (block number, inode number, etc.)
 * @return 0 on success, -1 on error
 */
int editor_open_structure(editor_context_t *ctx, structure_type_t type, uint32_t id);

/**
 * Save changes to the filesystem
 * 
 * @param ctx Pointer to editor_context_t structure
 * @return 0 on success, -1 on error
 */
int editor_save_changes(editor_context_t *ctx);

/**
 * Move cursor in the editor
 * 
 * @param ctx Pointer to editor_context_t structure
 * @param dx Change in X position
 * @param dy Change in Y position
 */
void editor_move_cursor(editor_context_t *ctx, int dx, int dy);

/**
 * Set byte value at cursor position
 * 
 * @param ctx Pointer to editor_context_t structure
 * @param value New byte value
 */
void editor_set_byte(editor_context_t *ctx, uint8_t value);

/**
 * Toggle field highlighting for structure fields
 * 
 * @param ctx Pointer to editor_context_t structure
 */
void editor_toggle_field_highlight(editor_context_t *ctx);

/**
 * Render binary editor
 * 
 * @param ctx Pointer to editor_context_t structure
 */
void editor_render(editor_context_t *ctx);

/**
 * Handle key press in the editor
 * 
 * @param ctx Pointer to editor_context_t structure
 * @param key The key code
 * @return true if editor should continue, false to exit
 */
bool editor_handle_key(editor_context_t *ctx, int key);

#endif /* EDITOR_H */

#ifndef UI_H
#define UI_H
#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include "analyzer.h"
#include "editor.h"

/**
 * Main UI mode
 */
typedef enum {
    UI_MODE_MENU,               // Main menu
    UI_MODE_ANALYZER,           // Filesystem analyzer
    UI_MODE_BLOCK_BROWSER,      // Block browser
    UI_MODE_INODE_BROWSER,      // Inode browser
    UI_MODE_BINARY_EDITOR       // Binary editor
} ui_mode_t;

/**
 * UI context
 */
typedef struct {
    WINDOW *main_win;           // Main window
    WINDOW *status_win;         // Status bar window
    WINDOW *help_win;           // Help window
    ui_mode_t current_mode;     // Current UI mode
    fs_info_t *fs_info;         // Filesystem information
    editor_context_t *editor_ctx; // Editor context
    int current_block;          // Current block number (for block browser)
    int current_inode;          // Current inode number (for inode browser)
    int current_group;          // Current block group
} ui_context_t;

/**
 * Initialize UI
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @return Pointer to ui_context_t structure or NULL on error
 */
ui_context_t *ui_init(fs_info_t *fs_info);

/**
 * Close and cleanup UI
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 */
void ui_cleanup(ui_context_t *ui_ctx);

/**
 * Main UI loop
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 */
void ui_main_loop(ui_context_t *ui_ctx);

/**
 * Set UI mode
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 * @param mode Mode to set
 */
void ui_set_mode(ui_context_t *ui_ctx, ui_mode_t mode);

/**
 * Display filesystem info in analyzer mode
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 */
void ui_display_fs_info(ui_context_t *ui_ctx);

/**
 * Display block browser
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 */
void ui_display_block_browser(ui_context_t *ui_ctx);

/**
 * Display inode browser
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 */
void ui_display_inode_browser(ui_context_t *ui_ctx);

/**
 * Display binary editor
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 */
void ui_display_binary_editor(ui_context_t *ui_ctx);

/**
 * Display status bar
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 * @param message Status message to display
 */
void ui_display_status(ui_context_t *ui_ctx, const char *message);

/**
 * Display help window
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 */
void ui_display_help(ui_context_t *ui_ctx);

/**
 * Show error message
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 * @param message Error message to display
 */
void ui_show_error(ui_context_t *ui_ctx, const char *message);

/**
 * Prompt user for input
 * 
 * @param ui_ctx Pointer to ui_context_t structure
 * @param prompt Prompt message
 * @param buffer Buffer to store input
 * @param max_len Maximum length of input
 * @return true if input was provided, false if canceled
 */
bool ui_prompt(ui_context_t *ui_ctx, const char *prompt, char *buffer, size_t max_len);

#endif /* UI_H */

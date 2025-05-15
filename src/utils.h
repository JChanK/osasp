#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ext2fs/ext2_fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "analyzer.h"

/**
 * Convert value to human-readable string
 * 
 * @param value Value to convert
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 * @param is_size Whether value represents size (for B/KB/MB/GB formatting)
 */
void format_value(uint64_t value, char *buffer, size_t buffer_size, bool is_size);

/**
 * Check if bit is set in bitmap
 * 
 * @param bitmap Bitmap
 * @param bit_num Bit number
 * @return true if bit is set, false otherwise
 */
bool check_bitmap_bit(const unsigned char *bitmap, uint32_t bit_num);

/**
 * Set bit in bitmap
 * 
 * @param bitmap Bitmap
 * @param bit_num Bit number
 */
void set_bitmap_bit(unsigned char *bitmap, uint32_t bit_num);

/**
 * Clear bit in bitmap
 * 
 * @param bitmap Bitmap
 * @param bit_num Bit number
 */
void clear_bitmap_bit(unsigned char *bitmap, uint32_t bit_num);

/**
 * Convert struct ext2_super_block to string with field descriptions
 * 
 * @param sb Pointer to superblock
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void superblock_to_string(const struct ext2_super_block *sb, char *buffer, size_t buffer_size);

/**
 * Convert struct ext2_group_desc to string with field descriptions
 * 
 * @param gd Pointer to group descriptor
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void group_desc_to_string(const struct ext2_group_desc *gd, char *buffer, size_t buffer_size);

/**
 * Convert struct ext2_inode to string with field descriptions
 * 
 * @param inode Pointer to inode
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void inode_to_string(const struct ext2_inode *inode, char *buffer, size_t buffer_size);

/**
 * Get filesystem type string (ext2/ext3/ext4)
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param buffer Buffer to store string
 * @param buffer_size Size of buffer
 */
void get_fs_type_string(const fs_info_t *fs_info, char *buffer, size_t buffer_size);

/**
 * Log message to file (for debugging)
 * 
 * @param format Format string
 * @param ... Arguments
 */
void log_message(const char *format, ...);

#endif /* UTILS_H */

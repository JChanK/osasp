#ifndef ANALYZER_H
#define ANALYZER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ext2fs/ext2_fs.h>
#define _POSIX_C_SOURCE 200809L

/**
 * Structure to hold filesystem information
 */
typedef struct {
    int fd;                         // File descriptor for device
    char *device_path;              // Path to the device
    struct ext2_super_block sb;     // Superblock data
    uint32_t block_size;            // Block size in bytes
    uint32_t inodes_per_group;      // Number of inodes per group
    uint32_t blocks_per_group;      // Number of blocks per group
    uint32_t groups_count;          // Number of block groups
    struct ext2_group_desc *group_desc; // Group descriptors
    bool is_ext4;                   // Whether filesystem is ext4
} fs_info_t;

/**
 * Initialize filesystem analyzer
 * 
 * @param device_path Path to the device to analyze
 * @return Pointer to fs_info_t structure or NULL on error
 */
fs_info_t *analyzer_init(const char *device_path);

/**
 * Close and cleanup filesystem analyzer
 * 
 * @param fs_info Pointer to fs_info_t structure
 */
void analyzer_cleanup(fs_info_t *fs_info);

/**
 * Read specific block from filesystem
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param block_num Block number to read
 * @param buffer Buffer to read into
 * @return 0 on success, -1 on error
 */
int read_block(fs_info_t *fs_info, uint32_t block_num, void *buffer);

/**
 * Write specific block to filesystem
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param block_num Block number to write
 * @param buffer Buffer with data to write
 * @return 0 on success, -1 on error
 */
int write_block(fs_info_t *fs_info, uint32_t block_num, void *buffer);

/**
 * Check if block is allocated
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param block_num Block number to check
 * @return true if allocated, false if free
 */
bool is_block_allocated(fs_info_t *fs_info, uint32_t block_num);

/**
 * Check if inode is allocated
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param inode_num Inode number to check
 * @return true if allocated, false if free
 */
bool is_inode_allocated(fs_info_t *fs_info, uint32_t inode_num);

/**
 * Read inode from filesystem
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param inode_num Inode number to read
 * @param inode Buffer to read inode into
 * @return 0 on success, -1 on error
 */
int read_inode(fs_info_t *fs_info, uint32_t inode_num, struct ext2_inode *inode);

/**
 * Write inode to filesystem
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param inode_num Inode number to write
 * @param inode Inode data to write
 * @return 0 on success, -1 on error
 */
int write_inode(fs_info_t *fs_info, uint32_t inode_num, struct ext2_inode *inode);

/**
 * Get block bitmap for a block group
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param group_num Block group number
 * @param bitmap Buffer to read bitmap into (must be at least blocks_per_group/8 bytes)
 * @return 0 on success, -1 on error
 */
int get_block_bitmap(fs_info_t *fs_info, uint32_t group_num, unsigned char *bitmap);

/**
 * Get inode bitmap for a block group
 * 
 * @param fs_info Pointer to fs_info_t structure
 * @param group_num Block group number
 * @param bitmap Buffer to read bitmap into (must be at least inodes_per_group/8 bytes)
 * @return 0 on success, -1 on error
 */
int get_inode_bitmap(fs_info_t *fs_info, uint32_t group_num, unsigned char *bitmap);

/**
 * Analyze and print filesystem information
 * 
 * @param fs_info Pointer to fs_info_t structure
 */
void analyze_filesystem(fs_info_t *fs_info);

#endif /* ANALYZER_H */

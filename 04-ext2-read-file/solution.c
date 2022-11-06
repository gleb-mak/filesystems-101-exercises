#include <solution.h>
#include <errno.h>
#include <ext2fs/ext2_fs.h>
#include <unistd.h>
#include <stdlib.h>

int WriteDataFromBlock(int img, unsigned int block_size, off_t offset, unsigned int* file_size, int out) {
		uint8_t* buff = (uint8_t* )malloc(block_size);
		unsigned int size = (*file_size > block_size) ? block_size : *file_size;
		if (pread(img, buff, size, offset) == -1) {
			free(buff);
			return -1;
		}
		if (write(out, buff, size) == -1) {
			free(buff);
			return -1;
		}
		*file_size = (*file_size > block_size) ? *file_size - block_size : 0;
		free(buff);
		return 0;
}

int WriteIndirectBlock(int img, unsigned int block_size, off_t offset, unsigned int* file_size, int out) {
	uint32_t* block_list = (uint32_t* )malloc(block_size);
	if (pread(img, block_list, block_size, offset) == -1) {
		free(block_list);
		return -1;
	}
	while (*file_size > 0) {
		for (unsigned int count = 0; count < block_size / 4; ++count) {
			if (block_list[count] == 0)
				break;
			if (WriteDataFromBlock(img, block_size, block_list[count] * block_size, file_size, out) != 0) {
				free(block_list);
				return -1;
			}
		}
	}
	free(block_list);
	return 0;
}

int WriteDoubleBlock(int img, unsigned int block_size, off_t offset, unsigned int* file_size, int out) {
	uint32_t* list_of_list = (uint32_t* )malloc(block_size);
/*	for (size_t count = 0; count < block_size / sizeof(uint32_t); ++count) 
		list_of_list[count] = 0;*/
	if (pread(img, list_of_list, block_size, offset) == -1) {
        free(list_of_list);
        return -1;
    }
	while (*file_size > 0) {
		for (unsigned int count = 0; count < block_size / 4; ++count) {
			if (list_of_list[count] == 0)
				break;
			if (WriteIndirectBlock(img, block_size, list_of_list[count] * block_size, file_size, out) != 0) {
				free(list_of_list);
				return -1;
			}
		}
	}
    free(list_of_list);
	return 0;
}

int dump_file(int img, int inode_nr, int out)
{
	(void) img;
	(void) inode_nr;
	(void) out;
	struct ext2_super_block superblock;
	if (pread(img, &superblock, sizeof(struct ext2_super_block), 1024) == -1)
		return -errno;
	unsigned int inodes_per_group = superblock.s_inodes_per_group;
	unsigned int block_group_num = (inode_nr - 1) / inodes_per_group;
	unsigned int block_size = 1024 << superblock.s_log_block_size;
	struct ext2_group_desc group_desc;
	off_t group_offset = block_size * superblock.s_first_data_block + block_size + block_group_num * sizeof(struct ext2_group_desc);
	if (pread(img, &group_desc, sizeof(struct ext2_group_desc), group_offset) == -1)
		return -errno;
	
	unsigned int inode_index = (inode_nr - 1) % inodes_per_group;
	unsigned int inode_size = superblock.s_inode_size;
	struct ext2_inode inode;
	off_t inode_offset = group_desc.bg_inode_table * block_size + inode_index * inode_size;
	if (pread(img, &inode, inode_size, inode_offset) == -1) 
		return -errno;
	unsigned int file_size = inode.i_size;
	for (unsigned int block_count = 0; block_count < EXT2_N_BLOCKS; ++block_count) {
		if (inode.i_block[block_count] == 0)
			break;
		off_t offset = inode.i_block[block_count] * block_size;
		if (block_count < EXT2_NDIR_BLOCKS) {
			if (WriteDataFromBlock(img, block_size, offset, &file_size, out) != 0)
				return -errno;
		}
		else if (block_count == EXT2_IND_BLOCK) {
			if (WriteIndirectBlock(img, block_size, offset, &file_size, out) != 0)
				return -errno;
		}
		else if (block_count == EXT2_DIND_BLOCK) {
			if (WriteDoubleBlock(img, block_size, offset, &file_size, out) != 0)
				return -errno;
		}
	}
	/* implement me */
	return 0;
}

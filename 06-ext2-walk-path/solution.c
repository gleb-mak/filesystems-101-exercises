#include <stdio.h>
#include <string.h>
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
		for (unsigned int count = 0; count < block_size / sizeof(uint32_t); ++count) {
			if (block_list[count] == 0)
				break;
			if (WriteDataFromBlock(img, block_size, block_list[count] * block_size, file_size, out) != 0) {
				free(block_list);
				return -1;
			}
		}
	free(block_list);
	return 0;
}

int WriteDoubleBlock(int img, unsigned int block_size, off_t offset, unsigned int* file_size, int out) {
	uint32_t* list_of_list = (uint32_t* )malloc(block_size);
	if (pread(img, list_of_list, block_size, offset) == -1) {
        free(list_of_list);
        return -1;
    }
		for (unsigned int count = 0; count < block_size / sizeof(uint32_t); ++count) {
			if (list_of_list[count] == 0)
				break;
			if (WriteIndirectBlock(img, block_size, list_of_list[count] * block_size, file_size, out) != 0) {
				free(list_of_list);
				return -1;
			}
		}
    free(list_of_list);
	return 0;
}

int WriteFile(int img, int inode_nr, int out)
{
	struct ext2_super_block superblock;
	if (pread(img, &superblock, sizeof(struct ext2_super_block), 1024) == -1)
		return -errno;
	unsigned int inodes_per_group = superblock.s_inodes_per_group;
	unsigned int block_group_num = (inode_nr - 1) / inodes_per_group;
	unsigned int block_size = 1024 << superblock.s_log_block_size;
	struct ext2_group_desc group_desc;
	off_t group_offset = 1024 + block_size * superblock.s_blocks_per_group * block_group_num + block_size; //1024 in begining, block_size * superblock.s_blocks_per_group - size of block_group, block_size - size of superblock in needed group block
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


__le32 GetInodeFromBlock(int img, unsigned int block_size, off_t offset, unsigned int* file_size, char* name) {
		uint8_t* buff = (uint8_t* )malloc(block_size);
		int size = (*file_size > block_size) ? block_size : *file_size;
		if (pread(img, buff, size, offset) == -1) {
			free(buff);
			return -1;
		}
		int len = 0;
		while (size > 0) {
			struct ext2_dir_entry_2* dir = (struct ext2_dir_entry_2*)(buff + len);
			if (dir->inode == 0)
				break;
			if (!strcmp(name, dir->name)) {
				free(buff);
				return dir->inode;
			}
			size -= dir->rec_len;
			len += dir->rec_len;
		}
		*file_size = (*file_size > block_size) ? *file_size - block_size : 0;
		free(buff);
		return 0;
}

__le32 GetInodeFromIndirectBlock(int img, unsigned int block_size, off_t offset, unsigned int* file_size, char* name) {
	uint32_t* block_list = (uint32_t* )malloc(block_size);
	if (pread(img, block_list, block_size, offset) == -1) {
		free(block_list);
		return -1;
	}
	for (unsigned int count = 0; count < block_size / sizeof(uint32_t); ++count) {
		if (block_list[count] == 0) {
			errno = ENOENT;
			free(block_list);
			return -1;
		}
		__le32 ret = GetInodeFromBlock(img, block_size, block_list[count] * block_size, file_size, name);
		switch (ret) {
			case 0:
				break;
			case -1:
				free(block_list);
				return -1;
			default:
				return ret;
		}
	}
	free(block_list);
	return 0;
}

__le32 GetInodeFromDoubleBlock(int img, unsigned int block_size, off_t offset, unsigned int* file_size, char* name) {
	uint32_t* list_of_list = (uint32_t* )malloc(block_size);
	if (pread(img, list_of_list, block_size, offset) == -1) {
        free(list_of_list);
        return -1;
    }
	for (unsigned int count = 0; count < block_size / sizeof(uint32_t); ++count) {
		if (list_of_list[count] == 0) {
			errno = ENOENT;
			free(list_of_list);
			return -1;
		}
		__le32 ret = GetInodeFromIndirectBlock(img, block_size, list_of_list[count] * block_size, file_size, name);
		switch (ret) {
			case 0:
				break;
			case -1:
				free(list_of_list);
				return -1;
			default:
				return ret;
		}
	}
    free(list_of_list);
	return 0;
}

int GetNextInode(int img, int inode_nr, char* name)
{
	struct ext2_super_block superblock;
	if (pread(img, &superblock, sizeof(struct ext2_super_block), 1024) == -1)
		return -errno;
	unsigned int inodes_per_group = superblock.s_inodes_per_group;
	unsigned int block_group_num = (inode_nr - 1) / inodes_per_group;
	unsigned int block_size = 1024 << superblock.s_log_block_size;
	struct ext2_group_desc group_desc;
	off_t group_offset = 1024 + block_size * superblock.s_blocks_per_group * block_group_num + block_size; //1024 in begining, block_size * superblock.s_blocks_per_group - size of block_group, block_size - size of superblock in needed group block
	if (pread(img, &group_desc, sizeof(struct ext2_group_desc), group_offset) == -1)
		return -1;

	unsigned int inode_index = (inode_nr - 1) % inodes_per_group;
	unsigned int inode_size = superblock.s_inode_size;
	struct ext2_inode inode;
	off_t inode_offset = group_desc.bg_inode_table * block_size + inode_index * inode_size;
	if (pread(img, &inode, inode_size, inode_offset) == -1) 
		return -1;
	unsigned int file_size = inode.i_size;
	for (unsigned int block_count = 0; block_count < EXT2_N_BLOCKS; ++block_count) {
		if (inode.i_block[block_count] == 0) {
			errno = ENOENT;
			return -1;
		}
		off_t offset = inode.i_block[block_count] * block_size;
		int ret = 0;
		if (block_count < EXT2_NDIR_BLOCKS) {
			ret = GetInodeFromBlock(img, block_size, offset, &file_size, name);
		}
		else if (block_count == EXT2_IND_BLOCK) {
			ret = GetInodeFromIndirectBlock(img, block_size, offset, &file_size, name);
		}
		else if (block_count == EXT2_DIND_BLOCK) {
			ret = GetInodeFromDoubleBlock(img, block_size, offset, &file_size, name);
		}
		switch (ret) {
			case -1:
				return -1;
			case 0:
				break;
			default:
				return ret;
		}
	}
	return 0;
}


int dump_file(int img, const char *path, int out) {
	(void) img;
	(void) path;
	(void) out;
	char* path_cpy = (char *)malloc((strlen(path) + 1) * sizeof(char));
	strcpy(path_cpy, path);
	char* cur_name = strtok(path_cpy, "/");
	int cur_parent_inode = 2; //root
	while (cur_name != NULL) {
		cur_parent_inode = GetNextInode(img, cur_parent_inode, cur_name);
		switch (cur_parent_inode) {
			case -1:
				free(path_cpy);
				return -errno;
			case 0:
				free(path_cpy);
				return -ENOENT;
		}
		cur_name = strtok(NULL, "/");
		//printf("%s ", cur_name);
	}
	int file_inode = cur_parent_inode;
	WriteFile(img, file_inode, out);
	free(path_cpy);
	return 0;
}

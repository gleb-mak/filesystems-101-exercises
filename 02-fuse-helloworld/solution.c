#include <solution.h>

#include <errno.h>
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>


static int hello_getattr(const char *path, struct stat *st, struct fuse_file_info *info) {
	info = info;
	memset(st, 0, sizeof(struct stat));
	st->st_mode = S_IFREG | 0400;
	st->st_nlink = 1;
	st->st_uid = fuse_get_context()->uid;
	st->st_gid = fuse_get_context()->gid;
	st->st_atime = time(NULL);
	st->st_mtime = time(NULL);
	if (!strcmp(path, "/")) {
		st->st_nlink = 2;
		st->st_mode = S_IFDIR | 0755;
	}
	return 0;
}

static int hello_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	path = path; buf = buf; size = size; offset = offset; info = info;
	return -EROFS;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info, enum fuse_readdir_flags flags) {
	offset = offset; info = info; flags = flags;
	filler(buf, ".", NULL, 0, 0);
	filler(buf, "..", NULL, 0, 0);
	if (!strcmp(path, "/")) {
		filler(buf, "hello", NULL, 0, 0);
	}
	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *info) {
	info = info;
	if (strcmp(path, "/"))
		return -1;
	char data[20] = { '\0' };
	sprintf(data, "hello, %d\n", fuse_get_context()->pid);
	size_t len = strlen(data);
	if (offset >= len)
		return 0;
	len = (len - (size_t)offset < size) ? len - (size_t)offset : size;
	memset(buf, data + offset, len);
	return len;
}

static const struct fuse_operations hellofs_ops = {
	.getattr = hello_getattr,
    .readdir = hello_readdir,
    .read = hello_read,
	.write = hello_write,
};

int helloworld(const char *mntp) {
	char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
	return fuse_main(3, argv, &hellofs_ops, NULL);
}

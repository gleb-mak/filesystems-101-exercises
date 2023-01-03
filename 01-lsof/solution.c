#include "solution.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool IsNumber(const char* name) {
	int count = 0;
    while(name[count] != '\0') {
        if(!isdigit(name[count])) {
            return false;
        }
		count++;
    }
    return true;
}

char* GetNextPID(DIR* dirp) {
    char* name = NULL;
    struct dirent* dir_info;
    while((dir_info = readdir(dirp))) {
		if (!errno) {
			report_error("/proc/", errno);
//			exit(1);
		}
        if(IsNumber(dir_info->d_name)) {
            name = dir_info->d_name;
            break;
        }
    }
    return name;
}

void ReportUsingFiles(const char* pid_str) {
	char fd_path[PATH_MAX] = { '\0' };
	sprintf(fd_path, "/proc/%s/fd", pid_str);
	DIR* fd = opendir(fd_path);
	if (NULL == fd) {
		report_error(fd_path, errno);
//		exit(1);
	}
	struct dirent* dir_info;
	while ((dir_info = readdir(fd))) {
		if (!errno) {
			report_error(fd_path, errno);
			continue;
//			exit(1);
		}
		if ((!IsNumber(dir_info->d_name)) || (dir_info->d_type != DT_LNK))
			continue;
		char symlink_path[PATH_MAX] = { '\0' };
		char tmp_file[PATH_MAX] = { '\0' };
		sprintf(symlink_path, "/proc/%s/fd/%s", pid_str, dir_info->d_name);
		ssize_t nbytes = readlink(symlink_path, tmp_file, PATH_MAX);
		if (nbytes == -1) {
			report_error(symlink_path, errno);
			continue;
//			exit(1);
		}
		size_t len = strlen(tmp_file) + 1;
		char* file_path = (char* )malloc(len * sizeof(char));
		memcpy(file_path, tmp_file, len);
		report_file(file_path);
		free(file_path);
	}
	closedir(fd);
}

void lsof(void) {
    char* pid_str;
    DIR* dp = opendir("/proc");
    if (NULL == dp)
        report_error("/proc", errno);
    while ((pid_str = GetNextPID(dp)))
		ReportUsingFiles(pid_str);
    closedir(dp);
}


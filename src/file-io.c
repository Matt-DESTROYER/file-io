#if defined(_WIN32) || defined(_WIN64)
	#define __WINDOWS 1
#else
	#define __WINDOWS 0
#endif

#if !__WINDOWS
	#define _POSIX_C_SOURCE 200112L
	#define _FILE_OFFSET_BITS 64
#endif

#include "file-io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __WINDOWS
	#include <windows.h>
	#include <direct.h>
	#include <io.h>

	#define ACCESS _access
	#define GETCWD _getcwd

	#define F_OK 0
	#ifndef PATH_MAX
		#define PATH_MAX MAX_PATH
	#endif

	#define DIR_SEP '\\'
	#define DIR_SEP_STR "\\"
#else
	#include <sys/stat.h>
	#include <unistd.h>
	#include <limits.h>

	#define ACCESS access
	#define GETCWD getcwd

	#define DIR_SEP '/'
	#define DIR_SEP_STR "/"
#endif

bool file_exists(const char* file) {
#if __WINDOWS
	DWORD attributes = GetFileAttributesA(file);
	return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat file_stat;
	if (stat(file, &file_stat) != 0)
		return false;
	return S_ISREG(file_stat.st_mode);
#endif
}

bool directory_exists(const char* directory) {
#if __WINDOWS
	DWORD attributes = GetFileAttributesA(directory);
	return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat dir_stat;
	if (stat(directory, &dir_stat) != 0)
		return false;
	return S_ISDIR(dir_stat.st_mode);
#endif
}

bool path_exists(const char* path) {
#if __WINDOWS
	DWORD attributes = GetFileAttributesA(path);
	return attributes != INVALID_FILE_ATTRIBUTES;
#else
	struct stat path_stat;
	return stat(path, &path_stat) == 0;
#endif
}

bool directory_create(const char* path) {
#if __WINDOWS
	return CreateDirectoryA(path, NULL) != 0;
#else
	return mkdir(path, 0777) == 0;
#endif
}

file_t file_open(const char* file, FileOptions_t options) {
#if __WINDOWS
	file_t fp = NULL;
	errno_t err = 1;

	switch (options) {
		case READ:
			err = fopen_s(&fp, file, "r");
			break;
		case READ_BINARY:
			err = fopen_s(&fp, file, "rb");
			break;
		case READ_WRITE:
			err = fopen_s(&fp, file, "r+");
			break;
		case READ_WRITE_BINARY:
			err = fopen_s(&fp, file, "r+b");
			break;
		case WRITE:
			err = fopen_s(&fp, file, "w");
			break;
		case WRITE_BINARY:
			err = fopen_s(&fp, file, "wb");
			break;
		case WRITE_READ:
			err = fopen_s(&fp, file, "w+");
			break;
		case WRITE_READ_BINARY:
			err = fopen_s(&fp, file, "w+b");
			break;
		case APPEND:
			err = fopen_s(&fp, file, "a");
			break;
		case APPEND_BINARY:
			err = fopen_s(&fp, file, "ab");
			break;
		case APPEND_READ:
			err = fopen_s(&fp, file, "a+");
			break;
		case APPEND_READ_BINARY:
			err = fopen_s(&fp, file, "a+b");
			break;
		default:
			return NULL;
	}
	if (err == 0)
		return fp;
	return NULL;
#else
	switch (options) {
		case READ:
			return fopen(file, "r");
		case READ_BINARY:
			return fopen(file, "rb");
		case READ_WRITE:
			return fopen(file, "r+");
		case READ_WRITE_BINARY:
			return fopen(file, "r+b");
		case WRITE:
			return fopen(file, "w");
		case WRITE_BINARY:
			return fopen(file, "wb");
		case WRITE_READ:
			return fopen(file, "w+");
		case WRITE_READ_BINARY:
			return fopen(file, "w+b");
		case APPEND:
			return fopen(file, "a");
		case APPEND_BINARY:
			return fopen(file, "ab");
		case APPEND_READ:
			return fopen(file, "a+");
		case APPEND_READ_BINARY:
			return fopen(file, "a+b");
		default:
			return NULL;
	}
#endif
}

void file_close(file_t file) {
	fclose(file);
}

size_t file_read(file_t file, char** out) {
	if (file == NULL)
		return 0;

#if __WINDOWS
	_fseeki64(file, 0, SEEK_END);
	long long int _file_size = _ftelli64(file);
#else
	fseeko(file, 0, SEEK_END);
	off_t _file_size = ftello(file);
#endif
	rewind(file);
	if (_file_size < 1)
		return 0;

	size_t file_size = (size_t)_file_size;

	char* buffer = malloc(sizeof(char) * (file_size + 1));
	if (buffer == NULL)
		return 0;

	size_t read_size = fread(buffer, sizeof(char), file_size, file);

	buffer[read_size] = '\0';
	if (read_size < file_size) {
		char* new_buffer = realloc(buffer, sizeof(char) * (read_size + 1));
		if (new_buffer != NULL) {
			buffer = new_buffer;
		}
	}

	*out = buffer;

	return read_size;
}

void file_write(file_t file, const char* buffer, size_t size) {
	if (file == NULL)
		return;

	fwrite(buffer, sizeof(char), size, file);
}

int file_io_internal_int_min(int x, int y) {
	return x < y ? x : y;
}

// (End not inclusive)
char* file_io_internal_bounded_strdup(const char* string, size_t start, size_t end) {
	if (string == NULL || start == end)
		return NULL;

	int reverse = 0;
	if (start > end) {
		reverse = 1;

		size_t temp = start;
		start = end;
		end = temp;
	}

	size_t new_length = end - start;
	char* new_string = malloc(sizeof(char) * (new_length + 1));
	if (new_string == NULL)
		return NULL;

	memcpy(new_string, string + start, sizeof(char) * new_length);
	new_string[new_length] = '\0';

	if (reverse == 1) {
		for (size_t i = 0; i < new_length / 2; i++) {
			size_t end_idx = new_length - i - 1;
			char temp = new_string[i];
			new_string[i] = new_string[end_idx];
			new_string[end_idx] = temp;
		}
	}

	return new_string;
}

char* file_root_by_file(const char* file) {
	if (file == NULL)
		return NULL;

	char current_dir[PATH_MAX];

	if (GETCWD(current_dir, sizeof(current_dir)) == NULL)
		return NULL;

	char file_path[PATH_MAX];
	while (true) {
		snprintf(file_path, file_io_internal_int_min(sizeof(file_path), PATH_MAX), "%s" DIR_SEP_STR "%s", current_dir, file);

		if (ACCESS(file_path, F_OK) == 0)
			return file_io_internal_(current_dir, 0, strlen(current_dir));

		if (strcmp(current_dir, "/") == 0)
			break;

		char* last_slash = strrchr(current_dir, DIR_SEP);
		if (last_slash == NULL)
			break;

		// last_slash == current_dir would indicate POSIX root
		if (last_slash == current_dir)
			strcpy(current_dir, "/");
		else
			*last_slash = '\0';
	}

	return NULL;
}


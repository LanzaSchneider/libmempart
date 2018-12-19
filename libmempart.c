/* ===================================================
 * libmempart.c
 * Author: Lanza Schneider
 * ===================================================
*/
#include "libmempart.h"
#include <string.h>

#include <stdlib.h>

/* Create and dispose */
mempart_t* mempart_create(uint32_t size) {
	mempart_t* partition = (mempart_t*)libmempart_malloc(sizeof(mempart_t));
	if (!partition) return NULL;
	
	mempart_file_node_t* newnode = (mempart_file_node_t*)libmempart_malloc(sizeof(mempart_file_node_t));
	if (!newnode) {
		libmempart_free(partition);
		return NULL;
	}
	newnode->name[0] = '\0';
	newnode->file = NULL;
	newnode->prev = NULL;
	newnode->next = NULL;
	
	partition->size = size;
	partition->used_count = 0;
	partition->first_node = newnode;
	return partition;
}

void mempart_release(mempart_t* mempart) {
	mempart_file_node_t* node = (mempart_file_node_t*)mempart->first_node;
	while (node) {
		if (node->file) {
			if (node->file->buffer) {
				libmempart_free(node->file->buffer);
			}
			libmempart_free(node->file);
		}
		mempart_file_node_t* delete_node = node;
		node = node->next;
		libmempart_free(delete_node);
	}
	free(mempart);
}

/* Load and Save */
mempart_t* mempart_load(const void* data, uint32_t size) {
	if (size < sizeof(uint32_t) * 3) return NULL;
	uint32_t* data32 = (uint32_t*)data;
	mempart_t* mempart = mempart_create(data32[0]);
	if (!mempart) return NULL;
	mempart->used_count = data32[1];
	void* start = data + sizeof(uint32_t) * 3;
	uint8_t* offset = start;
	uint32_t i = 0;
	for (; i < data32[2]; i++) {
		char filename[MEMPART_FILENAME_MAX_CHAR];
		size_t j = 0;
		while (offset[j]) {
			filename[j] = offset[j];
			j++;
		}
		filename[j] = '\0';
		offset += j + 1;
		mempart_file_t* file = NULL;
		if (mempart_file_open(mempart, filename, "rb", &file)) continue;
		if (*(uint32_t*)(offset)) mempart_file_write(file, offset + sizeof(uint32_t), *(uint32_t*)(offset), 1);
		offset += *(uint32_t*)(offset);
		offset += sizeof(uint32_t);
	}
	return mempart;
}

uint32_t mempart_dump_size(mempart_t* mempart) {
	uint32_t offset = sizeof(uint32_t) * 3;
	uint32_t i = 0;
	mempart_file_node_t* node = (mempart_file_node_t*)mempart->first_node->next;
	while (node) {
		offset += strlen(node->name) + 1;
		offset += sizeof(uint32_t);
		if (node->file) {
			offset += node->file->size;
		}
		node = node->next;
	}
	return offset;
}

uint32_t mempart_dump(mempart_t* mempart, void* buf) {
	uint8_t* buffer = (uint8_t*)buf;
	uint32_t* buffer32 = (uint32_t*)buffer;
	uint32_t offset = sizeof(uint32_t) * 3;
	uint32_t i = 0;
	buffer32[0] = mempart->size;
	buffer32[1] = mempart->used_count;
	buffer32[2] = 0;
	mempart_file_node_t* node = (mempart_file_node_t*)mempart->first_node->next;
	while (node) {
		strcpy(buffer + offset, node->name);
		offset += strlen(node->name);
		buffer[offset] = '\0';
		offset += 1;
		
		if (node->file) {
			*(uint32_t*)(buffer + offset) = node->file->size;
			offset += sizeof(uint32_t);
		} else {
			*(uint32_t*)(buffer + offset) = 0;
			offset += sizeof(uint32_t);
		}
		
		if (node->file) {
			memcpy(buffer + offset, node->file->buffer, node->file->size);
			offset += node->file->size;
		}
		
		buffer32[2] += 1;
		node = node->next;
	}
	return 0;
}

/* File utils. */
int mempart_file_open(mempart_t* mempart, const char* filename, const char* mode, mempart_file_t** file) {
	if (strlen(filename) >= MEMPART_FILENAME_MAX_CHAR) return MEMFILE_OPEN_ERR_NAME_TOOLONG;
	mempart_file_node_t* node = (mempart_file_node_t*)mempart->first_node;
	while (node) {
		if (!node->next) {
			mempart_file_node_t* newnode = (mempart_file_node_t*)libmempart_malloc(sizeof(mempart_file_node_t));
			if (!newnode) return MEMFILE_OPEN_ERR_NOMEMORY;
			strcpy(newnode->name, filename);
			newnode->file = NULL;
			newnode->prev = node;
			newnode->next = NULL;
			node = node->next = newnode;
		}
		if (!strcmp(filename, node->name)) {
			if (node->file) {
				*file = node->file;
				return 0;
			} else {
				mempart_file_t* newfile = (mempart_file_t*)libmempart_malloc(sizeof(mempart_file_t));
				if (!newfile) return MEMFILE_OPEN_ERR_NOMEMORY;
				newfile->partition = mempart;
				newfile->buffer = NULL;
				newfile->size = 0;
				newfile->cursor = 0;
				newfile->flags = MEMFILE_FLAGS_READABLE | MEMFILE_FLAGS_WRITABLE;
				*file = node->file = newfile;
				return 0;
			}
		}
		node = node->next;
	}
	return MEMFILE_OPEN_ERR_UNKNOWN;
}

int mempart_file_exist(mempart_t* mempart, const char* filename) {
	if (strlen(filename) >= MEMPART_FILENAME_MAX_CHAR) return MEMFILE_OPEN_ERR_NAME_TOOLONG;
	mempart_file_node_t* node = (mempart_file_node_t*)mempart->first_node;
	while (node) {
		if (!strcmp(filename, node->name)) {
			return 1;
		}
		node = node->next;
	}
	return 0;
}

int mempart_file_delete(mempart_t* mempart, const char* filename) {
	if (strlen(filename) >= MEMPART_FILENAME_MAX_CHAR) return MEMFILE_OPEN_ERR_NAME_TOOLONG;
	mempart_file_node_t* node = (mempart_file_node_t*)mempart->first_node;
	while (node) {
		if (!strcmp(filename, node->name)) {
			if (node->file) {
				mempart->used_count -= node->file->size;
				if (node->file->buffer) {
					libmempart_free(node->file->buffer);
				}
				libmempart_free(node->file);
			}
			if (node->prev) {
				node->prev->next = node->next;
			}
			if (node->next) {
				node->next->prev = node->prev;
			}
			libmempart_free(node);
			return 0;
		}
		node = node->next;
	}
	return MEMFILE_OPEN_ERR_NOEXIST;
}

int mempart_file_rename(mempart_t* mempart, const char* filename, const char* newname) {
	if (strlen(filename) >= MEMPART_FILENAME_MAX_CHAR) return MEMFILE_OPEN_ERR_NAME_TOOLONG;
	mempart_file_node_t* node = (mempart_file_node_t*)mempart->first_node;
	while (node) {
		if (!strcmp(filename, node->name)) {
			strcpy(node->name, newname);
			return 0;
		}
		node = node->next;
	}
	return MEMFILE_OPEN_ERR_NOEXIST;
}

// Memory file operators.
int mempart_file_seek(mempart_file_t* file, long offset, int whence) {
	int idxEnd = file->size - 1;
	switch (whence) {
	case SEEK_SET: {
		if (offset >= 0 && offset <= idxEnd) {
			file->cursor = offset;
			return 0;
		}
	} break;
	case SEEK_CUR: {
		if ((offset + file->cursor) >= 0 && (offset + file->cursor) <= idxEnd) {
			file->cursor += offset;
			return 0;
		}
	} break;
	case SEEK_END: {
		if ((offset + idxEnd) >= 0 && (offset + idxEnd) <= idxEnd) {
			file->cursor = idxEnd + offset + 1;
			return 0;
		}
	} break;
	}
	return -1;
}

long mempart_file_tell(mempart_file_t* file) {
	return file->cursor;
}

void mempart_file_rewind(mempart_file_t* file) {
	file->cursor = 0;
}

uint32_t mempart_file_read(mempart_file_t* file, void* buffer, uint32_t size, uint32_t count) {
	if (!(file->flags & MEMFILE_FLAGS_READABLE)) return 0;
	if (size > (file->size - file->cursor)) {
		return 0;
	}
		
	uint32_t read_count = 0;
	while ((read_count < count) && ((read_count > 0 ? read_count : 1) * size < (file->size - file->cursor))) {
		read_count += 1;
	}
		
	memcpy(buffer, file->buffer + file->cursor, read_count * size);
	file->cursor += read_count * size;
		
	return read_count;
}

uint32_t mempart_file_write(mempart_file_t* file, const void* buffer, uint32_t size, uint32_t count) {
	if (!(file->flags & MEMFILE_FLAGS_WRITABLE)) return 0;
	
	uint32_t write_count = 0;
	while ((write_count < count) && ((write_count > 0 ? write_count : 1) * size < (file->partition->size - file->partition->used_count))) {
		write_count++;
	}
	
	if (!write_count) {
		return 0;
	}
	
	if (file->buffer) {
		file->buffer = libmempart_realloc(file->buffer, file->size + write_count * size);
	} else {
		file->buffer = libmempart_malloc(file->size + write_count * size);
	}
	
	memcpy(file->buffer + file->cursor, buffer, write_count * size);
	file->cursor += write_count * size;
	file->size += write_count * size;
	file->partition->used_count += write_count * size;
	return write_count;
}

/* ===================================================
 * The library of memory partition.
 * Author: Lanza Schneider
 * ===================================================
*/
#ifndef LIBMEMPART_HEADER
#define LIBMEMPART_HEADER

#include <stdint.h>
#include <stdio.h>

#define libmempart_malloc(n)			malloc(n)
#define libmempart_free(n)				free(n)
#define libmempart_realloc(p, n)		realloc(p, n)

#define MEMPART_FILENAME_MAX_CHAR		16

typedef struct mempart_file_t mempart_file_t;
typedef struct mempart_file_node_t mempart_file_node_t;

/* ---------------------------------------------------
 * Memory Partition
 * ---------------------------------------------------
*/
typedef struct mempart_t{
	size_t size;						// The byte size of partition.
	size_t used_count;					// The byte size of used spaces.
	mempart_file_node_t* first_node;	// The first file node.
} mempart_t;

/* Create and dispose */
// Create a new memory partition. return NULL if failed.
mempart_t* mempart_create(size_t size);
// Release a memory partition.
void mempart_release(mempart_t* mempart);

/* Load and Save */
// Load a memory partition from a memory buffer, return NULL if failed.
mempart_t* mempart_load(const void* data, size_t size);
// Get the size of the memory dump buffer.
size_t mempart_dump_size(mempart_t* mempart);
// Dump a memory partition, return 0 if failed.
size_t mempart_dump(mempart_t* mempart, void* buffer);

/* File utils. */
// Open a memory file in partition, return a non-zero value if failed.
#define MEMFILE_OPEN_ERR_NOMEMORY		-1
#define MEMFILE_OPEN_ERR_MODE_ERROR		-2
#define MEMFILE_OPEN_ERR_UNKNOWN		-10
#define MEMFILE_OPEN_ERR_NOEXIST		-19
int mempart_file_open(mempart_t* mempart, const char* filename, const char* mode, mempart_file_t** file);
// Find a file, return a non-zero value if file exists.
int mempart_file_exist(mempart_t* mempart, const char* filename);
// Delete a file, return a non-zero value if failed.
int mempart_file_delete(mempart_t* mempart, const char* filename);
// Rename a file, return a non-zero value if failed.
int mempart_file_rename(mempart_t* mempart, const char* filename, const char* newname);

/* ---------------------------------------------------
 * Memory Partition Node
 * ---------------------------------------------------
*/
typedef struct mempart_file_node_t {
	char name[MEMPART_FILENAME_MAX_CHAR];	// The name of file.
	mempart_file_t* file;
	mempart_file_node_t* prev;
	mempart_file_node_t* next;
} mempart_file_node_t;

/* ---------------------------------------------------
 * Memory Partition File
 * ---------------------------------------------------
*/

#define MEMFILE_FLAGS_READABLE			0x0001
#define MEMFILE_FLAGS_WRITABLE			0x0010

typedef struct mempart_file_t{
	mempart_t* partition;				// The partition that this file belong to.
	void* buffer;						// The contents of this file.
	size_t size;						// The size of this file.
	size_t cursor;						// The cursor of this file.
	uint32_t flags;						// The file flags.
} mempart_file_t;

// Memory file operators.
int mempart_file_seek(mempart_file_t* file, long offset, int whence);
long mempart_file_tell(mempart_file_t* file);
void mempart_file_rewind(mempart_file_t* file);
size_t mempart_file_read(mempart_file_t* file, void* buffer, size_t size, size_t count);
size_t mempart_file_write(mempart_file_t* file, const void* buffer, size_t size, size_t count);

#endif
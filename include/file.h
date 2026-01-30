#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "./heap.h"
/*
 
 * most databases store data via normal files. when i say normal i mean files that the operating system can recognize and handle. its not a special file that the os cannot handle.
 
 * we are done with implementing a basic version of the heap page structure for fixed records
 * next step is to organize those pages within these files, called the 'heap' files.
 * the reason they are called 'heap' files is because they can be placed anywhere within the file
*/

typedef struct {
	int num_pages;
	int next_free_page;
} FileHeader;

typedef struct {
	FileHeader header;
	FILE *file_ptr;
} HeapFile;


typedef struct {
    int page_id;
    int slot_idx;
} RecordId;

HeapFile * create_file(const char *filename);
HeapFile * open_file(const char *filename);
GrainResult close_file(HeapFile *file);

// Page-level I/O operations
GrainResult hf_read_page(HeapFile *hf, int page_id, HeapPage *page_buffer);
GrainResult hf_write_page(HeapFile *hf, int page_id, const HeapPage *page_buffer);
#endif

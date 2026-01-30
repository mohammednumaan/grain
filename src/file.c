#include "../include/file.h"
#include "../include/heap.h"
#include <stdio.h>
#include <stdlib.h>

int write_file_header(HeapFile *file){
	CHECK_RET_GRAIN_NULL(file);
	
	if (fseek(file->file_ptr, 0, SEEK_SET) != 0){
		return GRAIN_FILE_ERROR;
	}

	int writeRes = fwrite(&file->header, sizeof(FileHeader), 1, file->file_ptr);
	if (writeRes != 1){
		return GRAIN_FILE_ERROR;
	}
	
	fflush(file->file_ptr);
	return GRAIN_OK;
}

HeapFile * create_file(const char *filename){
	CHECK_RET_NULL(filename);

	FILE *file_ptr = fopen(filename, "wb+");
	CHECK_RET_NULL(file_ptr);

	HeapFile *heap_file = (HeapFile *)malloc(sizeof(HeapFile));
	if (heap_file == NULL){
		fclose(file_ptr);
		return NULL;
	}

	heap_file->file_ptr = file_ptr;
	heap_file->header.num_pages = 0;
	heap_file->header.next_free_page = -1;

	// now that i have a heap file, i need to write fill in the first few bytes
	// with metadata about the file, i.e FileHeader
	if (write_file_header(heap_file)!= GRAIN_OK){
		fclose(file_ptr);
		free(heap_file);
		return NULL;
	}

	return heap_file;
}

HeapFile * open_file(const char *filename){
	CHECK_RET_NULL(filename);

	FILE *file_ptr = fopen(filename, "rb+");
	CHECK_RET_NULL(file_ptr);

	HeapFile *heap_file = (HeapFile *)malloc(sizeof(HeapFile));
	if (heap_file == NULL){
		fclose(file_ptr);
		return NULL;
	}

	heap_file->file_ptr = file_ptr;
	if (fread(&heap_file->header, sizeof(FileHeader), 1, file_ptr) != 1){
		fclose(file_ptr);
		free(heap_file);
		return NULL;
	}

	return heap_file;
}

GrainResult close_file(HeapFile *file){
	CHECK_RET_GRAIN_NULL(file);

	if (file->file_ptr != NULL){
		// todo: i need to flush the data?
		fclose(file->file_ptr);
	}

	free(file);
	return GRAIN_OK;
}

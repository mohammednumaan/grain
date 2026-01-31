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

GrainResult read_page(HeapFile *hf, HeapPage *hp, int page_id){
	CHECK_RET_GRAIN_NULL(hf);
	CHECK_RET_GRAIN_NULL(hp);

	int offset =  sizeof(FileHeader) + (page_id * PAGE_SIZE);
	if (fseek(hf->file_ptr, offset, SEEK_SET) != 0){
		return GRAIN_FILE_ERROR;
	}

	if (fread(hp, PAGE_SIZE, 1, hf->file_ptr) != 1){
		return GRAIN_FILE_ERROR;
	}

	return GRAIN_OK;
}


GrainResult write_page(HeapFile *file, HeapPage *hp){
	CHECK_RET_GRAIN_NULL(file);
	CHECK_RET_GRAIN_NULL(hp);

	// i simply add the page to end of the file
	// to find the end, we need to calculate an offset that is:
	// (HEADER_SIZE + (NUM_PAGES * PAGE_SIZE))
	int offset = sizeof(FileHeader) + (file->header.num_pages * PAGE_SIZE); 
	if (fseek(file->file_ptr, offset, SEEK_SET) != 0){
		return GRAIN_FILE_ERROR;
	}

	int writeRes = fwrite(hp, PAGE_SIZE, 1, file->file_ptr);
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


// TODO: need to add free_page list support
int hf_alloc_page(HeapFile *hf){
	CHECK_RET_GRAIN_NULL(hf);

	if (hf->header.next_free_page == -1){
		// this means that 	there are no free pages, so we need to 
		// create new page and add it to the end of the file 
		int new_page_id = hf->header.next_page_idx;
		hf->header.next_page_idx++;

		HeapPage heap_page;
		init_page(&heap_page, new_page_id);

        if (write_page(hf, &heap_page) != GRAIN_OK){
			return GRAIN_FILE_ERROR;
		}

		hf->header.num_pages++;

		int writeRes = write_file_header(hf);
		if (writeRes != GRAIN_OK){
			return writeRes;
		}
		return new_page_id;
	}

	// if we have a free page in the free-page list
	// we allocate a new page in that free space instead of appending at the end
	int reused_page_id = hf->header.next_free_page;
	HeapPage heap_page;

	GrainResult readRes = read_page(hf, &heap_page, reused_page_id);
	if (readRes != GRAIN_OK){
		return GRAIN_FILE_ERROR;
	}

	init_page(&heap_page, reused_page_id);
	if (write_page(hf, &heap_page) != GRAIN_OK){
		return GRAIN_FILE_ERROR;
	}

	int writeRes = write_file_header(hf);
	if (writeRes != GRAIN_OK){
		return writeRes;
	}
	return reused_page_id;
	return 0;

}

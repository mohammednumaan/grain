#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "heap.h"

typedef struct {
    int32_t num_pages;
    int32_t next_page_idx;
    int32_t first_free_page;
} FileHeader;

typedef struct {
    FileHeader header;
    FILE *file_ptr;
} HeapFile;

typedef struct {
    int32_t page_id;
    int32_t slot_idx;
} RecordId;

HeapFile *create_file(const char *filename);
HeapFile *open_file(const char *filename);
GrainResult close_file(HeapFile *file);
GrainResult write_file_header(HeapFile *hf);

GrainResult hf_alloc_page(HeapFile *hf, int32_t *page_id);
GrainResult write_page(HeapFile *hf, HeapPage *hp);
GrainResult read_page(HeapFile *hf, HeapPage *hp, int32_t page_id);

GrainResult hf_insert_record(HeapFile *hf, Record *rec);
GrainResult hf_scan_next(HeapFile *hf, RecordId *rid, Record *rec);
GrainResult hf_update_record(HeapFile *hf, RecordId rid, Record *rec);
GrainResult hf_delete_record(HeapFile *hf, RecordId rid);

#endif

#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define PAGE_SIZE 8192
#define RECORD_SIZE 64
#define MAX_SLOTS ((PAGE_SIZE - sizeof(PageHeader)) / RECORD_SIZE)
#define FREE_SLOT_END -1

#define CHECK_RET_NULL(ptr) if ((ptr) == NULL) return NULL;
#define CHECK_RET_BOOL(ptr) if ((ptr) == NULL) return false;
#define CHECK_RET_INT(ptr) if ((ptr) == NULL) return -1;
#define CHECK_RET_GRAIN_NULL(ptr) if ((ptr) == NULL) return GRAIN_NULL_PTR;
#define CHECK_RET_GRAIN_INVALID(ptr) if ((ptr) == NULL) return GRAIN_INVALID_SLOT;

typedef enum {
    GRAIN_OK              =  0,
    GRAIN_END             =  1,
    GRAIN_NULL_PTR        = -1,
    GRAIN_INVALID_SLOT    = -2,
    GRAIN_RECORD_NOT_FOUND= -3,
    GRAIN_PAGE_FULL       = -4,
    GRAIN_INVALID_PAGE_ID = -5,
    GRAIN_FILE_OPEN_FAILED= -6,
    GRAIN_FILE_READ_FAILED= -7,
    GRAIN_FILE_WRITE_FAILED=-8,
    GRAIN_FILE_SEEK_FAILED= -9,
    GRAIN_CORRUPT_HEADER  = -10
} GrainResult;

typedef struct {
    int32_t id;
    char name[32];
    int32_t age;
    char email[24];
} Record;

typedef struct {
    int32_t next_free_slot;
} FreeSlot;

typedef struct {
    int32_t page_id;
    int32_t num_slots;
    int32_t next_slot_idx;
    int32_t first_free_slot;
    int32_t next_free_page;
} PageHeader;

typedef struct {
    PageHeader header;
    char storage[PAGE_SIZE - sizeof(PageHeader)];
} HeapPage;

void *get_slot(HeapPage *page, int32_t slot_idx);
bool has_free_space(HeapPage *page);
bool is_in_free_list(HeapPage *page, int32_t slot_idx);

HeapPage *init_page(HeapPage *page, int32_t page_id);
int32_t insert_record(HeapPage *page, Record *record);
GrainResult delete_record(HeapPage *page, int32_t slot_idx);
GrainResult update_record(HeapPage *page, int32_t slot_idx, Record *new_record);
Record *get_record(HeapPage *page, int32_t slot_idx);

#endif

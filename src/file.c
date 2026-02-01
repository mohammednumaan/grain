#include "../include/file.h"
#include "../include/heap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static FileHeader *read_file_header(HeapFile *hf) {
    CHECK_RET_NULL(hf);
    if (fseek(hf->file_ptr, 0, SEEK_SET) != 0) {
        return NULL;
    }
    if (fread(&hf->header, sizeof(FileHeader), 1, hf->file_ptr) != 1) {
        return NULL;
    }
    return &hf->header;
}

GrainResult write_file_header(HeapFile *hf) {
    CHECK_RET_GRAIN_NULL(hf);
    if (fseek(hf->file_ptr, 0, SEEK_SET) != 0) {
        return GRAIN_FILE_SEEK_FAILED;
    }
    if (fwrite(&hf->header, sizeof(FileHeader), 1, hf->file_ptr) != 1) {
        return GRAIN_FILE_WRITE_FAILED;
    }
    fflush(hf->file_ptr);
    return GRAIN_OK;
}

GrainResult read_page(HeapFile *hf, HeapPage *hp, int32_t page_id) {
    CHECK_RET_GRAIN_NULL(hf);
    CHECK_RET_GRAIN_NULL(hp);
    if (page_id < 0 || page_id >= hf->header.num_pages) {
        return GRAIN_INVALID_PAGE_ID;
    }
    long offset = sizeof(FileHeader) + ((long)page_id * PAGE_SIZE);
    if (fseek(hf->file_ptr, offset, SEEK_SET) != 0) {
        return GRAIN_FILE_SEEK_FAILED;
    }
    if (fread(hp, PAGE_SIZE, 1, hf->file_ptr) != 1) {
        return GRAIN_FILE_READ_FAILED;
    }
    return GRAIN_OK;
}

GrainResult write_page(HeapFile *hf, HeapPage *hp) {
    CHECK_RET_GRAIN_NULL(hf);
    CHECK_RET_GRAIN_NULL(hp);
    long offset = sizeof(FileHeader) + ((long)hp->header.page_id * PAGE_SIZE);
    if (fseek(hf->file_ptr, offset, SEEK_SET) != 0) {
        return GRAIN_FILE_SEEK_FAILED;
    }
    if (fwrite(hp, PAGE_SIZE, 1, hf->file_ptr) != 1) {
        return GRAIN_FILE_WRITE_FAILED;
    }
    fflush(hf->file_ptr);
    return GRAIN_OK;
}

HeapFile *create_file(const char *filename) {
    CHECK_RET_NULL(filename);
    FILE *file_ptr = fopen(filename, "wb+");
    CHECK_RET_NULL(file_ptr);

    HeapFile *heap_file = (HeapFile *)malloc(sizeof(HeapFile));
    if (heap_file == NULL) {
        fclose(file_ptr);
        return NULL;
    }

    heap_file->file_ptr = file_ptr;
    heap_file->header.num_pages = 0;
    heap_file->header.next_page_idx = 0;
    heap_file->header.first_free_page = -1;

    if (write_file_header(heap_file) != GRAIN_OK) {
        fclose(file_ptr);
        free(heap_file);
        return NULL;
    }

    return heap_file;
}

static bool validate_header(FileHeader *header) {
    if (header->num_pages < 0) return false;
    if (header->next_page_idx < 0) return false;
    if (header->first_free_page < -1) return false;
    if (header->next_page_idx < header->num_pages) return false;
    return true;
}

HeapFile *open_file(const char *filename) {
    CHECK_RET_NULL(filename);
    FILE *file_ptr = fopen(filename, "rb+");
    CHECK_RET_NULL(file_ptr);

    HeapFile *heap_file = (HeapFile *)malloc(sizeof(HeapFile));
    if (heap_file == NULL) {
        fclose(file_ptr);
        return NULL;
    }

    heap_file->file_ptr = file_ptr;
    if (fread(&heap_file->header, sizeof(FileHeader), 1, file_ptr) != 1) {
        fclose(file_ptr);
        free(heap_file);
        return NULL;
    }

    if (!validate_header(&heap_file->header)) {
        fclose(file_ptr);
        free(heap_file);
        return NULL;
    }

    return heap_file;
}

GrainResult close_file(HeapFile *hf) {
    CHECK_RET_GRAIN_NULL(hf);
    if (hf->file_ptr != NULL) {
        fclose(hf->file_ptr);
        hf->file_ptr = NULL;
    }
    free(hf);
    return GRAIN_OK;
}

GrainResult hf_alloc_page(HeapFile *hf, int32_t *page_id) {
    CHECK_RET_GRAIN_NULL(hf);
    CHECK_RET_GRAIN_NULL(page_id);

    int32_t new_page_id = hf->header.next_page_idx;
    hf->header.next_page_idx++;

    HeapPage heap_page;
    init_page(&heap_page, new_page_id);

    heap_page.header.next_free_page = hf->header.first_free_page;
    hf->header.first_free_page = new_page_id;

    GrainResult res = write_page(hf, &heap_page);
    if (res != GRAIN_OK) {
        return res;
    }

    hf->header.num_pages++;

    res = write_file_header(hf);
    if (res != GRAIN_OK) {
        return res;
    }

    *page_id = new_page_id;
    return GRAIN_OK;
}

GrainResult hf_insert_record(HeapFile *hf, Record *rec) {
    CHECK_RET_GRAIN_NULL(hf);
    CHECK_RET_GRAIN_NULL(rec);

    int32_t page_id;
    HeapPage page;

    if (hf->header.first_free_page != -1) {
        page_id = hf->header.first_free_page;
        GrainResult res = read_page(hf, &page, page_id);
        if (res != GRAIN_OK) {
            return res;
        }
    } else {
        GrainResult res = hf_alloc_page(hf, &page_id);
        if (res != GRAIN_OK) {
            return res;
        }
        res = read_page(hf, &page, page_id);
        if (res != GRAIN_OK) {
            return res;
        }
    }

    int32_t slot = insert_record(&page, rec);
    if (slot == -1) {
        return GRAIN_PAGE_FULL;
    }

    if (!has_free_space(&page)) {
        hf->header.first_free_page = page.header.next_free_page;
        page.header.next_free_page = -1;
        GrainResult res = write_file_header(hf);
        if (res != GRAIN_OK) {
            return res;
        }
    }

    GrainResult res = write_page(hf, &page);
    if (res != GRAIN_OK) {
        return res;
    }

    return GRAIN_OK;
}

GrainResult hf_scan_next(HeapFile *hf, RecordId *rid, Record *rec) {
    CHECK_RET_GRAIN_NULL(hf);
    CHECK_RET_GRAIN_NULL(rid);
    CHECK_RET_GRAIN_NULL(rec);

    HeapPage page;
    int32_t currPage = rid->page_id;
    int32_t nextSlot = rid->slot_idx + 1;

    while (currPage < hf->header.num_pages) {
        GrainResult res = read_page(hf, &page, currPage);
        if (res != GRAIN_OK) {
            return res;
        }

        while (nextSlot < page.header.next_slot_idx) {
            Record *found = get_record(&page, nextSlot);
            if (found != NULL) {
                *rec = *found;
                rid->page_id = currPage;
                rid->slot_idx = nextSlot;
                return GRAIN_OK;
            }
            nextSlot++;
        }

        currPage++;
        nextSlot = 0;
    }

    return GRAIN_END;
}

GrainResult hf_update_record(HeapFile *hf, RecordId rid, Record *rec) {
    CHECK_RET_GRAIN_NULL(hf);
    CHECK_RET_GRAIN_NULL(rec);

    HeapPage page;
    GrainResult res = read_page(hf, &page, rid.page_id);
    if (res != GRAIN_OK) {
        return res;
    }

    res = update_record(&page, rid.slot_idx, rec);
    if (res != GRAIN_OK) {
        return res;
    }

    res = write_page(hf, &page);
    if (res != GRAIN_OK) {
        return res;
    }

    return GRAIN_OK;
}

GrainResult hf_delete_record(HeapFile *hf, RecordId rid) {
    CHECK_RET_GRAIN_NULL(hf);

    HeapPage page;
    GrainResult res = read_page(hf, &page, rid.page_id);
    if (res != GRAIN_OK) {
        return res;
    }

    bool was_full = !has_free_space(&page);

    res = delete_record(&page, rid.slot_idx);
    if (res != GRAIN_OK) {
        return res;
    }

    if (was_full) {
        page.header.next_free_page = hf->header.first_free_page;
        hf->header.first_free_page = rid.page_id;
        res = write_file_header(hf);
        if (res != GRAIN_OK) {
            return res;
        }
    }

    res = write_page(hf, &page);
    if (res != GRAIN_OK) {
        return res;
    }

    return GRAIN_OK;
}

#include "../include/heap.h"
#include <string.h>

static inline bool slot_in_range(const HeapPage *page, int slot_idx) {
    if (page == NULL) return false;
    return slot_idx >= 0 && slot_idx < page->header.next_slot_idx;
}

HeapPage * init_page(HeapPage *page, int page_id){
	page->header.page_id = page_id;
	page->header.num_slots = 0;
	page->header.next_slot_idx = 0;
	page->header.first_free_slot = FREE_SLOT_END;
}

void * get_slot(HeapPage *page, int slot_idx){
    CHECK_RET_NULL(page);

	// this prevents clients from accessing slots that is unallocated
    if (!slot_in_range(page, slot_idx)){
		return NULL;
	}
    return page->storage + (slot_idx * RECORD_SIZE);
}

bool is_in_free_list(HeapPage *page, int slot_idx){
    CHECK_RET_BOOL(page);
    if (page->header.first_free_slot == FREE_SLOT_END){
        return false;
    }

    int curr = page->header.first_free_slot;
    while (curr != FREE_SLOT_END){
        if (curr == slot_idx) return true;
        FreeSlot *slot = (FreeSlot *)get_slot(page, curr);
        CHECK_RET_BOOL(slot);

        curr = slot->next_free_slot;
    }

    return false;
}

int insert_record(HeapPage *page, Record* record){
    CHECK_RET_INT(page);
    CHECK_RET_INT(record);

	if (page->header.first_free_slot == FREE_SLOT_END &&
		page->header.next_slot_idx >= MAX_SLOTS){
		return -1;
	}

	int slot_idx;
    if (page->header.first_free_slot != FREE_SLOT_END){
        slot_idx = page->header.first_free_slot;
        FreeSlot *slot = (FreeSlot *) get_slot(page, slot_idx);
        CHECK_RET_INT(slot);
        page->header.first_free_slot = slot->next_free_slot;
    }
	else {
		slot_idx = page->header.next_slot_idx;
		page->header.next_slot_idx++;
	}

	// now that we have determined where to write this record to
	// we can start writing it using memcpy
	void *dest = get_slot(page, slot_idx);
	memcpy(dest, record, RECORD_SIZE);
	page->header.num_slots++;
	return slot_idx;
}

GrainResult delete_record(HeapPage *page, int slot_idx){

    CHECK_RET_GRAIN_NULL(page);
    if (!slot_in_range(page, slot_idx)){
        return GRAIN_INVALID_SLOT;
    }

	if (is_in_free_list(page, slot_idx)){
		return GRAIN_INVALID_SLOT;
	};
    FreeSlot *slot = (FreeSlot *)get_slot(page, slot_idx);
    CHECK_RET_GRAIN_INVALID(slot);

	slot->next_free_slot = page->header.first_free_slot;
	page->header.first_free_slot = slot_idx;
	page->header.num_slots--;
	return GRAIN_OK;
	
}

GrainResult update_record(HeapPage *page, int slot_idx, Record *new_record){

    CHECK_RET_GRAIN_NULL(page);
    CHECK_RET_GRAIN_NULL(new_record);
    if (!slot_in_range(page, slot_idx)) {
        return GRAIN_INVALID_SLOT;
    }

    Record *record = get_record(page, slot_idx);
    CHECK_RET_GRAIN_NULL(record);

	strcpy(record->name, new_record->name);
	strcpy(record->email, new_record->email);
	record->age = new_record->age;
	return GRAIN_OK;
}

Record * get_record(HeapPage *page, int slot_idx){
    CHECK_RET_NULL(page);
    if (!slot_in_range(page, slot_idx)) return NULL;

    if (is_in_free_list(page, slot_idx)) return NULL;

    Record *record = (Record *)get_slot(page, slot_idx);
    CHECK_RET_NULL(record);
    return record;
}

bool has_free_space(HeapPage *page){
	CHECK_PTR_RET(page, false);
	if (page->header.first_free_slot != FREE_SLOT_END){
		return true;
	}
	return page->header.next_slot_idx < MAX_SLOTS;
}

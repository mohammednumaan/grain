#include "../include/heap.h"
#include <string.h>


void * get_slot(HeapPage *page, int slot_idx){
	if (slot_idx > page->header.num_slots) return NULL;
	return page->storage + (slot_idx * RECORD_SIZE);
}

int insert_record(HeapPage *page, Record* record){

	int slot_idx;
	if (page->header.first_free_slot != -1){
		slot_idx = page->header.first_free_slot;
		FreeSlot *slot = (FreeSlot *) get_slot(page, slot_idx);
		page->header.first_free_slot = slot->next_free_slot;
	}

	else {
		if (page->header.num_slots >= MAX_SLOTS){
			return -1;
		}
		slot_idx = page->header.num_slots;
		page->header.num_slots++;
	}

	// now that we have determined where to write this record to
	// we can start writing it using memcpy
	void *dest = get_slot(page, slot_idx);
	memcpy(dest, record, RECORD_SIZE);
	return slot_idx;
}

void delete_record(HeapPage *page, int slot_idx){
	if (slot_idx > page->header.num_slots) return;

	FreeSlot *slot = (FreeSlot *)get_slot(page, slot_idx);
	slot->next_free_slot = page->header.first_free_slot;
	page->header.first_free_slot = slot_idx;
}

#include "../include/heap.h"
#include <string.h>

int insert_record(HeapPage *page, const char *record){

	size_t recordSize = strlen(record);
	int availableSize = page->header.available_size;

	// if the record to insert is larger than the available size
	// we cannot store the record in this page.
	if (recordSize > availableSize){
		return -1;
	}

	// this initial implementation assumes that page->num_records
	// is the start of the next free space. so basically we keep inserting
	// records at the end of the free space in the page.
	
	int recordId = page->header.num_records/recordSize; 
	// copies record to page->data (first el address) + num_records
	// which gives the new starting address to start placing this data.
	memcpy(page->data + page->header.num_records, record, recordSize);

	// now we need to update the metadata of this page
	page->header.num_records += recordSize;
	page->header.available_size -= recordSize;
	return recordId;
}

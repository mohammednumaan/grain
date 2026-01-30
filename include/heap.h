#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>
#include <stdbool.h>

/*
 * grain uses a "heap" file to ogranize data. a heap file is simply a file where data is stored in a random order.
 * each "heap" file is partitioned into multiple pages. we can call these pages "heap" pages. there pages are fixed-size and contains
 the actual data.
 
 this is the initial phase of the project so i'll be focusing on the following:
 > implementing a heap file and basic operations associated with it.
 > implementing a heap page for *fixed-length* records.


 here are some questions i need to answer:
 > how are pages stored in the file?
 > how are records stored in the record?
 > how are fields stored in the record? 
 	* differs for fixed-length (or) variable-length records.



 fixed-length records:
   so each record is of fixed-length meaning they only contain attributes which take char, int, boolean and other fixed-size values.
   each page will have something called a page header to store information about the current page. following the page header is just a bunch of
   records and free space.

   the structure can be visualized as:
   +-------------+
   | page header |
   +-------------+
   | record 01   |
   +-------------+
   | record 02   |
   +-------------+
   | record n    |
   +-------------+
   | free space  |
   +-------------+

   each page is 8KB in size, which is 8192 Bytes
   so, the amount of data we can store is (PAGE_SIZE - sizeof(HEADER))

	inserts:
	- one way to handle inserts is to just keep writing records continously 
	- but sometimes, a record might not entirely fit within this page and only a portion of it does. in this case a part of the record will be in one page and the other in an nother page which required 2 page access to read.
	- to overcome this limitation, we only add as many records that can fit within a single page. so i need to write an if-check to decide to place or not.

	- another problem is, when we delete a record, we open up a free space. one way to fill this space is to just shift all the records, but is very inefficient. so we need a way to "mark" free spaces and add records to available free spaces on insert.

	- we can do this by using a "free-space" list. it is basically a linked list that stored which block is free.
*/

#define PAGE_SIZE 8192
#define RECORD_SIZE 64
#define MAX_SLOTS ((PAGE_SIZE - sizeof(PageHeader)) / RECORD_SIZE)
#define CHECK_PTR_RET(ptr, ret_val) if ((ptr) == NULL) return (ret_val);
#define CHECK_PTR_VOID(ptr) if ((ptr) == NULL) return;

#define FREE_SLOT_END -1

#define CHECK_RET_NULL(ptr) if ((ptr) == NULL) return NULL;
#define CHECK_RET_BOOL(ptr) if ((ptr) == NULL) return false;
#define CHECK_RET_INT(ptr) if ((ptr) == NULL) return -1;
#define CHECK_RET_GRAIN_NULL(ptr) if ((ptr) == NULL) return GRAIN_NULL_PTR;
#define CHECK_RET_GRAIN_INVALID(ptr) if ((ptr) == NULL) return GRAIN_INVALID_SLOT;

typedef enum {
	GRAIN_OK = 0,
	GRAIN_NULL_PTR,
	GRAIN_INVALID_SLOT,
	GRAIN_FILE_ERROR
} GrainResult;

typedef struct {
	int id;
	char name[32];
	int age;
	char email[24];
} Record;

typedef struct {
	int next_free_slot;
} FreeSlot;

typedef struct {
	int page_id;
	int num_slots;
	int next_slot_idx;
	int first_free_slot;
} PageHeader;

typedef struct {
	PageHeader header;
	char storage[PAGE_SIZE - sizeof(PageHeader)];
} HeapPage;


// slot related functions
void * get_slot(HeapPage *page, int slot_idx);
bool has_free_space(HeapPage *page);
bool is_in_free_list(HeapPage *page, int slot_idx);

// page related functions
HeapPage * init_page(HeapPage *page, int page_id);
int insert_record(HeapPage *page, Record* record);
GrainResult delete_record(HeapPage *page, int slot_idx);
GrainResult update_record(HeapPage *page, int slot_idx, Record* new_record);
Record * get_record(HeapPage *page, int slot_idx);
#endif

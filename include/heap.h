#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>

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
*/


#define PAGE_SIZE 8192 
typedef struct {
	int page_id;
	int num_records;
	int available_size;
} PageHeader;

typedef struct {
	PageHeader header;
	char data[PAGE_SIZE - sizeof(PageHeader)];
} HeapPage;

int insert_record(HeapPage *page, const char *record); 
#endif


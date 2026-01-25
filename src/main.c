#include <stdio.h>
#include "../include/heap.h"

int main(){
	printf("grain - an experimental heap storage layer.\n");

	PageHeader header = {1, 0, 10};
	HeapPage page = {header};

	insert_record(&page, "hello");
	insert_record(&page, "world");

	printf("num_record = %d: \n", page.header.num_records/5);
	printf("space available = %d: \n", page.header.available_size);

	for (int i = 0; i < PAGE_SIZE - sizeof(PageHeader); i++){
		printf("%c", page.data[i]);
	}
	return 0;
}

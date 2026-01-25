#include <stdio.h>
#include <string.h>
#include "../include/heap.h"

int main() {

	/*
	 * note: these examples were written by claude, im too lazy for this :|
	*/
    HeapPage page;
    page.header.page_id = 0;
    page.header.num_slots = 0;
    page.header.first_free_slot = -1;  // -1 means no free slots

    printf("=== Grain Heap Page Demo ===\n\n");
    printf("Page size: %d bytes\n", PAGE_SIZE);
    printf("Record size: %d bytes\n", RECORD_SIZE);
    printf("Max slots per page: %lu\n\n", MAX_SLOTS);

    // Create some sample records
    Record r1 = {1, "Alice", 25, "alice@example.com"};
    Record r2 = {2, "Bob", 30, "bob@example.com"};
    Record r3 = {3, "Charlie", 35, "charlie@example.com"};

    // Insert records into the page
    printf("--- Inserting Records ---\n");
    int slot1 = insert_record(&page, &r1);
    printf("Inserted record 1 (Alice) at slot %d\n", slot1);

    int slot2 = insert_record(&page, &r2);
    printf("Inserted record 2 (Bob) at slot %d\n", slot2);

    int slot3 = insert_record(&page, &r3);
    printf("Inserted record 3 (Charlie) at slot %d\n", slot3);

    printf("\nPage now has %d slots used\n", page.header.num_slots);

    // Read back a record using get_slot
    printf("\n--- Reading Records ---\n");
    Record *retrieved = (Record *)get_slot(&page, slot2);
    if (retrieved) {
        printf("Record at slot %d: id=%d, name=%s, age=%d, email=%s\n",
               slot2, retrieved->id, retrieved->name, retrieved->age, retrieved->email);
    }

    // Delete a record
    printf("\n--- Deleting Record ---\n");
    printf("Deleting record at slot %d (Bob)\n", slot2);
    delete_record(&page, slot2);
    printf("First free slot is now: %d\n", page.header.first_free_slot);

    // Insert a new record - should reuse the deleted slot
    printf("\n--- Inserting After Delete ---\n");
    Record r4 = {4, "Diana", 28, "diana@example.com"};
    int slot4 = insert_record(&page, &r4);
    printf("Inserted record 4 (Diana) at slot %d (reused deleted slot)\n", slot4);

    // Verify the new record
    Record *diana = (Record *)get_slot(&page, slot4);
    if (diana) {
        printf("Record at slot %d: id=%d, name=%s, age=%d, email=%s\n",
               slot4, diana->id, diana->name, diana->age, diana->email);
    }

    // Insert another record when freelist is empty - should append to end
    printf("\n--- Inserting When Freelist Empty ---\n");
    printf("Current first_free_slot: %d (freelist is empty)\n", page.header.first_free_slot);
    Record r5 = {5, "Eve", 32, "eve@example.com"};
    int slot5 = insert_record(&page, &r5);
    printf("Inserted record 5 (Eve) at slot %d (appended to end)\n", slot5);

    // Add a bunch more records
    printf("\n--- Adding More Records ---\n");
    Record r6 = {6, "Frank", 40, "frank@example.com"};
    Record r7 = {7, "Grace", 29, "grace@example.com"};
    Record r8 = {8, "Henry", 33, "henry@example.com"};
    Record r9 = {9, "Ivy", 27, "ivy@example.com"};
    Record r10 = {10, "Jack", 31, "jack@example.com"};

    int slot6 = insert_record(&page, &r6);
    int slot7 = insert_record(&page, &r7);
    int slot8 = insert_record(&page, &r8);
    int slot9 = insert_record(&page, &r9);
    int slot10 = insert_record(&page, &r10);

    printf("Inserted Frank at slot %d\n", slot6);
    printf("Inserted Grace at slot %d\n", slot7);
    printf("Inserted Henry at slot %d\n", slot8);
    printf("Inserted Ivy at slot %d\n", slot9);
    printf("Inserted Jack at slot %d\n", slot10);
    printf("Total slots: %d\n", page.header.num_slots);

    // Delete multiple records to build up the freelist
    printf("\n--- Deleting Multiple Records ---\n");
    printf("Deleting slots: 2 (Charlie), 5 (Grace), 7 (Ivy), 4 (Frank)\n");
    delete_record(&page, 2);  // Charlie
    delete_record(&page, 5);  // Grace
    delete_record(&page, 7);  // Ivy
    delete_record(&page, 4);  // Frank

    // Walk the freelist and display it
    printf("\n--- Freelist State ---\n");
    printf("Walking the freelist:\n");
    int current = page.header.first_free_slot;
    int count = 0;
    printf("HEAD -> ");
    while (current != -1) {
        printf("[slot %d]", current);
        FreeSlot *fs = (FreeSlot *)get_slot(&page, current);
        current = fs->next_free_slot;
        if (current != -1) {
            printf(" -> ");
        }
        count++;
    }
    printf(" -> NULL\n");
    printf("Total free slots in freelist: %d\n", count);

    printf("\n--- Final Page State ---\n");
    printf("Total slots used: %d\n", page.header.num_slots);
    printf("First free slot: %d\n", page.header.first_free_slot);

    return 0;
}

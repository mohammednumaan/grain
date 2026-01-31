#include <check.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/heap.h"

START_TEST(test_insert_records)
{
    HeapPage page;
    init_page(&page, 0);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Alice");
    strcpy(rec.email, "alice@test.com");

    int slot = insert_record(&page, &rec);
    ck_assert_int_eq(slot, 0);

    Record *retrieved = get_record(&page, slot);
    ck_assert_ptr_nonnull(retrieved);
    ck_assert_int_eq(retrieved->id, rec.id);
    ck_assert_int_eq(retrieved->age, rec.age);
    ck_assert_str_eq(retrieved->name, rec.name);
    ck_assert_str_eq(retrieved->email, rec.email);
    ck_assert_int_eq(page.header.num_slots, 1);

    for (int i = 1; i < 5; i++) {
        Record r = {.id = i, .age = 20 + i};
        snprintf(r.name, sizeof(r.name), "User%d", i);
        snprintf(r.email, sizeof(r.email), "user%d@test.com", i);
        int s = insert_record(&page, &r);
        ck_assert_int_eq(s, i);
    }
    ck_assert_int_eq(page.header.num_slots, 5);
}
END_TEST

START_TEST(test_insert_null_params)
{
    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Test");
    strcpy(rec.email, "test@test.com");

    int slot = insert_record(NULL, &rec);
    ck_assert_int_eq(slot, -1);

    HeapPage page;
    init_page(&page, 0);
    slot = insert_record(&page, NULL);
    ck_assert_int_eq(slot, -1);
}
END_TEST

START_TEST(test_insert_page_full)
{
    HeapPage page;
    init_page(&page, 0);

    for (int i = 0; i < MAX_SLOTS; i++) {
        Record rec = {.id = i, .age = 20 + i};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "user%d@test.com", i);
        int slot = insert_record(&page, &rec);
        ck_assert_int_eq(slot, i);
    }
    ck_assert_int_eq(page.header.num_slots, MAX_SLOTS);

    Record rec = {.id = 999, .age = 99};
    strcpy(rec.name, "Overflow");
    strcpy(rec.email, "overflow@test.com");

    int slot = insert_record(&page, &rec);
    ck_assert_int_eq(slot, -1);
}
END_TEST

START_TEST(test_delete_records)
{
    HeapPage page;
    init_page(&page, 5);

    for (int i = 0; i < 5; i++) {
        Record rec = {.id = i, .age = 20 + i};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "user%d@test.com", i);
        insert_record(&page, &rec);
    }

    GrainResult result = delete_record(&page, 2);
    ck_assert_int_eq(result, GRAIN_OK);
    ck_assert_int_eq(page.header.num_slots, 4);
    ck_assert_ptr_null(get_record(&page, 2));

    result = delete_record(&page, 4);
    ck_assert_int_eq(result, GRAIN_OK);
    ck_assert_int_eq(page.header.num_slots, 3);

    result = delete_record(&page, 0);
    ck_assert_int_eq(result, GRAIN_OK);
    ck_assert_int_eq(page.header.num_slots, 2);
}
END_TEST

START_TEST(test_delete_invalid_slot)
{
    HeapPage page;
    init_page(&page, 2);

    GrainResult result = delete_record(&page, 5);
    ck_assert_int_eq(result, GRAIN_INVALID_SLOT);

    result = delete_record(&page, -1);
    ck_assert_int_eq(result, GRAIN_INVALID_SLOT);

    result = delete_record(&page, 100);
    ck_assert_int_eq(result, GRAIN_INVALID_SLOT);

    result = delete_record(NULL, 0);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);
}
END_TEST

START_TEST(test_double_delete)
{
    HeapPage page;
    init_page(&page, 11);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Alice");
    strcpy(rec.email, "alice@test.com");

    insert_record(&page, &rec);

    GrainResult result = delete_record(&page, 0);
    ck_assert_int_eq(result, GRAIN_OK);

    result = delete_record(&page, 0);
    ck_assert_int_eq(result, GRAIN_INVALID_SLOT);
}
END_TEST

START_TEST(test_update_record)
{
    HeapPage page;
    init_page(&page, 3);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Alice");
    strcpy(rec.email, "alice@test.com");
    insert_record(&page, &rec);

    Record new_rec = {.id = 2, .age = 30};
    strcpy(new_rec.name, "Bob");
    strcpy(new_rec.email, "bob@test.com");

    GrainResult result = update_record(&page, 0, &new_rec);
    ck_assert_int_eq(result, GRAIN_OK);

    Record *updated = get_record(&page, 0);
    ck_assert_ptr_nonnull(updated);
    ck_assert_int_eq(updated->age, 30);
    ck_assert_str_eq(updated->name, "Bob");
    ck_assert_str_eq(updated->email, "bob@test.com");
}
END_TEST

START_TEST(test_update_null_invalid)
{
    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Test");
    strcpy(rec.email, "test@test.com");

    GrainResult result = update_record(NULL, 0, &rec);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);

    HeapPage page;
    init_page(&page, 0);
    insert_record(&page, &rec);

    result = update_record(&page, 0, NULL);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);

    result = update_record(&page, 5, &rec);
    ck_assert_int_eq(result, GRAIN_INVALID_SLOT);
}
END_TEST

START_TEST(test_free_list_linked_list)
{
    HeapPage page;
    init_page(&page, 0);

    for (int i = 0; i < 5; i++) {
        Record rec = {.id = i, .age = 20 + i};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "user%d@test.com", i);
        insert_record(&page, &rec);
    }

    delete_record(&page, 2);
    ck_assert_int_eq(page.header.first_free_slot, 2);
    FreeSlot *slot2 = (FreeSlot *)get_slot(&page, 2);
    ck_assert_int_eq(slot2->next_free_slot, FREE_SLOT_END);

    delete_record(&page, 4);
    ck_assert_int_eq(page.header.first_free_slot, 4);
    FreeSlot *slot4 = (FreeSlot *)get_slot(&page, 4);
    ck_assert_int_eq(slot4->next_free_slot, 2);

    delete_record(&page, 0);
    ck_assert_int_eq(page.header.first_free_slot, 0);
    FreeSlot *slot0 = (FreeSlot *)get_slot(&page, 0);
    ck_assert_int_eq(slot0->next_free_slot, 4);

    ck_assert(is_in_free_list(&page, 0) == true);
    ck_assert(is_in_free_list(&page, 2) == true);
    ck_assert(is_in_free_list(&page, 4) == true);
    ck_assert(is_in_free_list(&page, 1) == false);
    ck_assert(is_in_free_list(&page, 3) == false);

    ck_assert_int_eq(page.header.num_slots, 2);

    int slot = insert_record(&page, &(Record){.id = 100, .age = 50});
    ck_assert_int_eq(slot, 0);
    ck_assert_int_eq(page.header.first_free_slot, 4);
    ck_assert(is_in_free_list(&page, 0) == false);
    ck_assert_int_eq(page.header.num_slots, 3);

    slot = insert_record(&page, &(Record){.id = 101, .age = 51});
    ck_assert_int_eq(slot, 4);
    ck_assert_int_eq(page.header.first_free_slot, 2);
    ck_assert(is_in_free_list(&page, 4) == false);
    ck_assert_int_eq(page.header.num_slots, 4);

    slot = insert_record(&page, &(Record){.id = 102, .age = 52});
    ck_assert_int_eq(slot, 2);
    ck_assert_int_eq(page.header.first_free_slot, FREE_SLOT_END);
    ck_assert(is_in_free_list(&page, 2) == false);
    ck_assert_int_eq(page.header.num_slots, 5);
}
END_TEST

START_TEST(test_get_record)
{
    HeapPage page;
    init_page(&page, 0);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Alice");
    strcpy(rec.email, "alice@test.com");

    insert_record(&page, &rec);
    delete_record(&page, 0);

    Record *retrieved = get_record(&page, 0);
    ck_assert_ptr_null(retrieved);

    retrieved = get_record(&page, 5);
    ck_assert_ptr_null(retrieved);

    retrieved = get_record(&page, -1);
    ck_assert_ptr_null(retrieved);

    retrieved = get_record(NULL, 0);
    ck_assert_ptr_null(retrieved);
}
END_TEST

START_TEST(test_has_free_space)
{
    HeapPage page;
    init_page(&page, 0);
    ck_assert(has_free_space(&page) == true);

    for (int i = 0; i < MAX_SLOTS; i++) {
        Record rec = {.id = i, .age = 20 + i};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "user%d@test.com", i);
        insert_record(&page, &rec);
    }
    ck_assert(has_free_space(&page) == false);

    delete_record(&page, 0);
    ck_assert(has_free_space(&page) == true);
}
END_TEST

START_TEST(test_page_initialization)
{
    HeapPage page;
    init_page(&page, 42);

    ck_assert_int_eq(page.header.page_id, 42);
    ck_assert_int_eq(page.header.num_slots, 0);
    ck_assert_int_eq(page.header.next_slot_idx, 0);
    ck_assert_int_eq(page.header.first_free_slot, FREE_SLOT_END);
}
END_TEST

static Suite *heap_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Heap Storage Tests");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_insert_records);
    tcase_add_test(tc_core, test_insert_null_params);
    tcase_add_test(tc_core, test_insert_page_full);

    tcase_add_test(tc_core, test_delete_records);
    tcase_add_test(tc_core, test_delete_invalid_slot);
    tcase_add_test(tc_core, test_double_delete);

    tcase_add_test(tc_core, test_update_record);
    tcase_add_test(tc_core, test_update_null_invalid);

    tcase_add_test(tc_core, test_free_list_linked_list);

    tcase_add_test(tc_core, test_get_record);

    tcase_add_test(tc_core, test_has_free_space);

    tcase_add_test(tc_core, test_page_initialization);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = heap_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}

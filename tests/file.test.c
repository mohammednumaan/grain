#include <check.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/file.h"

static const char *test_file = "hf_test.bin";

static void cleanup(void)
{
    remove(test_file);
}

static void create_corrupted_file(const char *filename)
{
    FILE *f = fopen(filename, "wb");
    ck_assert_ptr_nonnull(f);
    fwrite("corrupt", 7, 1, f);
    fclose(f);
}

static void create_truncated_file(const char *filename)
{
    FILE *f = fopen(filename, "wb");
    ck_assert_ptr_nonnull(f);
    FileHeader header = {1, 2, 3};
    fwrite(&header, sizeof(header) / 2, 1, f);
    fclose(f);
}

static void create_invalid_header_file(const char *filename, FileHeader *header)
{
    FILE *f = fopen(filename, "wb");
    ck_assert_ptr_nonnull(f);
    fwrite(header, sizeof(FileHeader), 1, f);
    fclose(f);
}

START_TEST(test_create_file_creates_on_disk)
{
    cleanup();
    ck_assert_int_eq(access(test_file, F_OK), -1);

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);
    ck_assert_int_eq(access(test_file, F_OK), 0);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_create_file_header_initialized)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);
    ck_assert_int_eq(hf->header.num_pages, 0);
    ck_assert_int_eq(hf->header.next_page_idx, 0);
    ck_assert_int_eq(hf->header.first_free_page, -1);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_create_file_header_persisted)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);
    close_file(hf);

    FILE *f = fopen(test_file, "rb");
    ck_assert_ptr_nonnull(f);

    FileHeader raw_header;
    size_t read_count = fread(&raw_header, sizeof(FileHeader), 1, f);
    fclose(f);

    ck_assert_int_eq(read_count, 1);
    ck_assert_int_eq(raw_header.num_pages, 0);
    ck_assert_int_eq(raw_header.next_page_idx, 0);
    ck_assert_int_eq(raw_header.first_free_page, -1);

    cleanup();
}
END_TEST

START_TEST(test_create_file_null_filename)
{
    HeapFile *hf = create_file(NULL);
    ck_assert_ptr_null(hf);
}
END_TEST

START_TEST(test_create_file_returns_valid_heapfile)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);
    ck_assert_ptr_nonnull(hf->file_ptr);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_open_file_reads_header)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_ge(page_id, 0);

    int expected_num_pages = hf->header.num_pages;
    int expected_next_page_idx = hf->header.next_page_idx;
    int expected_first_free_page = hf->header.first_free_page;
    close_file(hf);

    HeapFile *hf2 = open_file(test_file);
    ck_assert_ptr_nonnull(hf2);
    ck_assert_int_eq(hf2->header.num_pages, expected_num_pages);
    ck_assert_int_eq(hf2->header.next_page_idx, expected_next_page_idx);
    ck_assert_int_eq(hf2->header.first_free_page, expected_first_free_page);

    close_file(hf2);
    cleanup();
}
END_TEST

START_TEST(test_open_file_null_filename)
{
    HeapFile *hf = open_file(NULL);
    ck_assert_ptr_null(hf);
}
END_TEST

START_TEST(test_open_file_nonexistent)
{
    cleanup();
    HeapFile *hf = open_file("nonexistent_file_12345.dat");
    ck_assert_ptr_null(hf);
}
END_TEST

START_TEST(test_open_file_corrupted)
{
    cleanup();
    create_corrupted_file(test_file);

    HeapFile *hf = open_file(test_file);
    ck_assert_ptr_null(hf);

    cleanup();
}
END_TEST

START_TEST(test_open_file_truncated)
{
    cleanup();
    create_truncated_file(test_file);

    HeapFile *hf = open_file(test_file);
    ck_assert_ptr_null(hf);

    cleanup();
}
END_TEST

START_TEST(test_open_file_invalid_header_negative_pages)
{
    cleanup();

    FileHeader invalid_header = {
        .num_pages = -1,
        .next_page_idx = 0,
        .first_free_page = -1
    };
    create_invalid_header_file(test_file, &invalid_header);

    HeapFile *hf = open_file(test_file);
    ck_assert_ptr_null(hf);

    cleanup();
}
END_TEST

START_TEST(test_open_file_invalid_header_bad_free_page)
{
    cleanup();

    FileHeader invalid_header = {
        .num_pages = 0,
        .next_page_idx = 0,
        .first_free_page = -2
    };
    create_invalid_header_file(test_file, &invalid_header);

    HeapFile *hf = open_file(test_file);
    ck_assert_ptr_null(hf);

    cleanup();
}
END_TEST

START_TEST(test_close_file_returns_ok)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    GrainResult result = close_file(hf);
    ck_assert_int_eq(result, GRAIN_OK);

    cleanup();
}
END_TEST

START_TEST(test_close_file_null_ptr)
{
    GrainResult result = close_file(NULL);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);
}
END_TEST

START_TEST(test_close_file_flushes_data)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_ge(page_id, 0);

    HeapPage page;
    GrainResult result = read_page(hf, &page, page_id);
    ck_assert_int_eq(result, GRAIN_OK);

    Record rec = {.id = 123, .age = 30};
    strcpy(rec.name, "FlushTest");
    strcpy(rec.email, "flush@test.com");
    insert_record(&page, &rec);

    result = write_page(hf, &page);
    ck_assert_int_eq(result, GRAIN_OK);
    close_file(hf);

    HeapFile *hf2 = open_file(test_file);
    ck_assert_ptr_nonnull(hf2);

    HeapPage read_page_data;
    result = read_page(hf2, &read_page_data, page_id);
    ck_assert_int_eq(result, GRAIN_OK);

    Record *retrieved = get_record(&read_page_data, 0);
    ck_assert_ptr_nonnull(retrieved);
    ck_assert_int_eq(retrieved->id, 123);
    ck_assert_int_eq(retrieved->age, 30);
    ck_assert_str_eq(retrieved->name, "FlushTest");
    ck_assert_str_eq(retrieved->email, "flush@test.com");

    close_file(hf2);
    cleanup();
}
END_TEST

START_TEST(test_write_page_success)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_ge(page_id, 0);

    HeapPage page;
    GrainResult result = read_page(hf, &page, page_id);
    ck_assert_int_eq(result, GRAIN_OK);

    result = write_page(hf, &page);
    ck_assert_int_eq(result, GRAIN_OK);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_write_page_null_heapfile)
{
    HeapPage page;
    init_page(&page, 0);

    GrainResult result = write_page(NULL, &page);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);
}
END_TEST

START_TEST(test_write_page_null_heappage)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    GrainResult result = write_page(hf, NULL);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_read_page_success)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_ge(page_id, 0);

    HeapPage write_page_data;
    GrainResult result = read_page(hf, &write_page_data, page_id);
    ck_assert_int_eq(result, GRAIN_OK);

    result = write_page(hf, &write_page_data);
    ck_assert_int_eq(result, GRAIN_OK);

    HeapPage read_page_data;
    result = read_page(hf, &read_page_data, page_id);
    ck_assert_int_eq(result, GRAIN_OK);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_read_page_null_heapfile)
{
    HeapPage page;
    GrainResult result = read_page(NULL, &page, 0);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);
}
END_TEST

START_TEST(test_read_page_null_heappage)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_ge(page_id, 0);

    GrainResult result = read_page(hf, NULL, page_id);
    ck_assert_int_eq(result, GRAIN_NULL_PTR);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_read_page_invalid_page_id_negative)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_ge(page_id, 0);

    HeapPage page;
    GrainResult result = read_page(hf, &page, -1);
    ck_assert_int_eq(result, GRAIN_INVALID_PAGE_ID);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_read_page_invalid_page_id_out_of_bounds)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_eq(page_id, 0);

    HeapPage page;
    GrainResult result = read_page(hf, &page, 999);
    ck_assert_int_eq(result, GRAIN_INVALID_PAGE_ID);

    result = read_page(hf, &page, 1);
    ck_assert_int_eq(result, GRAIN_INVALID_PAGE_ID);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_read_write_data_integrity)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int page_id;
    ck_assert_int_eq(hf_alloc_page(hf, &page_id), GRAIN_OK);
    ck_assert_int_ge(page_id, 0);

    HeapPage write_page_data;
    GrainResult result = read_page(hf, &write_page_data, page_id);
    ck_assert_int_eq(result, GRAIN_OK);

    Record rec = {.id = 42, .age = 25};
    strcpy(rec.name, "TestUser");
    strcpy(rec.email, "test@example.com");

    int slot = insert_record(&write_page_data, &rec);
    ck_assert_int_ge(slot, 0);

    result = write_page(hf, &write_page_data);
    ck_assert_int_eq(result, GRAIN_OK);

    HeapPage read_page_data;
    result = read_page(hf, &read_page_data, page_id);
    ck_assert_int_eq(result, GRAIN_OK);

    Record *retrieved = get_record(&read_page_data, slot);
    ck_assert_ptr_nonnull(retrieved);
    ck_assert_int_eq(retrieved->id, 42);
    ck_assert_int_eq(retrieved->age, 25);
    ck_assert_str_eq(retrieved->name, "TestUser");
    ck_assert_str_eq(retrieved->email, "test@example.com");

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_read_write_multiple_pages)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record records[3] = {
        {.id = 1, .age = 20},
        {.id = 2, .age = 30},
        {.id = 3, .age = 40}
    };
    strcpy(records[0].name, "User1");
    strcpy(records[0].email, "user1@test.com");
    strcpy(records[1].name, "User2");
    strcpy(records[1].email, "user2@test.com");
    strcpy(records[2].name, "User3");
    strcpy(records[2].email, "user3@test.com");

    int page_ids[3];

    for (int i = 0; i < 3; i++) {
        ck_assert_int_eq(hf_alloc_page(hf, &page_ids[i]), GRAIN_OK);
        ck_assert_int_ge(page_ids[i], 0);

        HeapPage page;
        GrainResult result = read_page(hf, &page, page_ids[i]);
        ck_assert_int_eq(result, GRAIN_OK);

        insert_record(&page, &records[i]);

        result = write_page(hf, &page);
        ck_assert_int_eq(result, GRAIN_OK);
    }

    for (int i = 0; i < 3; i++) {
        HeapPage read_page_data;
        GrainResult result = read_page(hf, &read_page_data, page_ids[i]);
        ck_assert_int_eq(result, GRAIN_OK);

        Record *retrieved = get_record(&read_page_data, 0);
        ck_assert_ptr_nonnull(retrieved);
        ck_assert_int_eq(retrieved->id, records[i].id);
        ck_assert_int_eq(retrieved->age, records[i].age);
        ck_assert_str_eq(retrieved->name, records[i].name);
        ck_assert_str_eq(retrieved->email, records[i].email);
    }

    close_file(hf);
    cleanup();
}
END_TEST

// ============== hf_insert_record tests ==============

START_TEST(test_hf_insert_record_success)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "TestUser");
    strcpy(rec.email, "test@example.com");

    GrainResult result = hf_insert_record(hf, &rec);
    ck_assert_int_eq(result, GRAIN_OK);
    ck_assert_int_eq(hf->header.num_pages, 1);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_insert_record_null_params)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record rec = {.id = 1, .age = 25};

    ck_assert_int_eq(hf_insert_record(NULL, &rec), GRAIN_NULL_PTR);
    ck_assert_int_eq(hf_insert_record(hf, NULL), GRAIN_NULL_PTR);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_insert_record_multiple)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    for (int i = 0; i < 10; i++) {
        Record rec = {.id = i, .age = 20 + i};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "user%d@test.com", i);

        GrainResult result = hf_insert_record(hf, &rec);
        ck_assert_int_eq(result, GRAIN_OK);
    }

    ck_assert_int_eq(hf->header.num_pages, 1);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_insert_record_allocates_new_page)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    for (int i = 0; i < MAX_SLOTS + 5; i++) {
        Record rec = {.id = i, .age = 20};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "u%d@t.com", i);

        GrainResult result = hf_insert_record(hf, &rec);
        ck_assert_int_eq(result, GRAIN_OK);
    }

    ck_assert_int_eq(hf->header.num_pages, 2);

    close_file(hf);
    cleanup();
}
END_TEST

// ============== hf_scan_next tests ==============

START_TEST(test_hf_scan_next_empty_file)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record rec;

    GrainResult result = hf_scan_next(hf, &rid, &rec);
    ck_assert_int_eq(result, GRAIN_END);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_scan_next_single_record)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record insert_rec = {.id = 42, .age = 30};
    strcpy(insert_rec.name, "ScanTest");
    strcpy(insert_rec.email, "scan@test.com");

    ck_assert_int_eq(hf_insert_record(hf, &insert_rec), GRAIN_OK);

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record found_rec;

    GrainResult result = hf_scan_next(hf, &rid, &found_rec);
    ck_assert_int_eq(result, GRAIN_OK);
    ck_assert_int_eq(found_rec.id, 42);
    ck_assert_int_eq(found_rec.age, 30);
    ck_assert_str_eq(found_rec.name, "ScanTest");
    ck_assert_str_eq(found_rec.email, "scan@test.com");
    ck_assert_int_eq(rid.page_id, 0);
    ck_assert_int_eq(rid.slot_idx, 0);

    result = hf_scan_next(hf, &rid, &found_rec);
    ck_assert_int_eq(result, GRAIN_END);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_scan_next_multiple_records)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    for (int i = 0; i < 5; i++) {
        Record rec = {.id = i, .age = 20 + i};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "user%d@test.com", i);
        ck_assert_int_eq(hf_insert_record(hf, &rec), GRAIN_OK);
    }

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record found_rec;
    int count = 0;

    while (hf_scan_next(hf, &rid, &found_rec) == GRAIN_OK) {
        ck_assert_int_eq(found_rec.id, count);
        ck_assert_int_eq(found_rec.age, 20 + count);
        count++;
    }

    ck_assert_int_eq(count, 5);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_scan_next_across_pages)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    int total_records = MAX_SLOTS + 10;
    for (int i = 0; i < total_records; i++) {
        Record rec = {.id = i, .age = i % 100};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "u%d@t.com", i);
        ck_assert_int_eq(hf_insert_record(hf, &rec), GRAIN_OK);
    }

    ck_assert_int_ge(hf->header.num_pages, 2);

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record found_rec;
    int count = 0;

    while (hf_scan_next(hf, &rid, &found_rec) == GRAIN_OK) {
        ck_assert_int_eq(found_rec.id, count);
        count++;
    }

    ck_assert_int_eq(count, total_records);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_scan_next_null_params)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record rec;

    ck_assert_int_eq(hf_scan_next(NULL, &rid, &rec), GRAIN_NULL_PTR);
    ck_assert_int_eq(hf_scan_next(hf, NULL, &rec), GRAIN_NULL_PTR);
    ck_assert_int_eq(hf_scan_next(hf, &rid, NULL), GRAIN_NULL_PTR);

    close_file(hf);
    cleanup();
}
END_TEST

// ============== hf_update_record tests ==============

START_TEST(test_hf_update_record_success)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Original");
    strcpy(rec.email, "original@test.com");
    ck_assert_int_eq(hf_insert_record(hf, &rec), GRAIN_OK);

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record found_rec;
    ck_assert_int_eq(hf_scan_next(hf, &rid, &found_rec), GRAIN_OK);

    Record updated = {.id = 1, .age = 30};
    strcpy(updated.name, "Updated");
    strcpy(updated.email, "updated@test.com");

    GrainResult result = hf_update_record(hf, rid, &updated);
    ck_assert_int_eq(result, GRAIN_OK);

    RecordId rid2 = {.page_id = 0, .slot_idx = -1};
    Record verify_rec;
    ck_assert_int_eq(hf_scan_next(hf, &rid2, &verify_rec), GRAIN_OK);
    ck_assert_int_eq(verify_rec.id, 1);
    ck_assert_int_eq(verify_rec.age, 30);
    ck_assert_str_eq(verify_rec.name, "Updated");
    ck_assert_str_eq(verify_rec.email, "updated@test.com");

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_update_record_null_params)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    RecordId rid = {.page_id = 0, .slot_idx = 0};
    Record rec = {.id = 1, .age = 25};

    ck_assert_int_eq(hf_update_record(NULL, rid, &rec), GRAIN_NULL_PTR);
    ck_assert_int_eq(hf_update_record(hf, rid, NULL), GRAIN_NULL_PTR);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_update_record_invalid_page)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    RecordId rid = {.page_id = 999, .slot_idx = 0};
    Record rec = {.id = 1, .age = 25};

    GrainResult result = hf_update_record(hf, rid, &rec);
    ck_assert_int_eq(result, GRAIN_INVALID_PAGE_ID);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_update_record_invalid_slot)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Test");
    strcpy(rec.email, "test@test.com");
    ck_assert_int_eq(hf_insert_record(hf, &rec), GRAIN_OK);

    RecordId rid = {.page_id = 0, .slot_idx = 999};
    Record updated = {.id = 1, .age = 30};

    GrainResult result = hf_update_record(hf, rid, &updated);
    ck_assert_int_eq(result, GRAIN_INVALID_SLOT);

    close_file(hf);
    cleanup();
}
END_TEST

// ============== hf_delete_record tests ==============

START_TEST(test_hf_delete_record_success)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "ToDelete");
    strcpy(rec.email, "delete@test.com");
    ck_assert_int_eq(hf_insert_record(hf, &rec), GRAIN_OK);

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record found_rec;
    ck_assert_int_eq(hf_scan_next(hf, &rid, &found_rec), GRAIN_OK);

    GrainResult result = hf_delete_record(hf, rid);
    ck_assert_int_eq(result, GRAIN_OK);

    RecordId rid2 = {.page_id = 0, .slot_idx = -1};
    result = hf_scan_next(hf, &rid2, &found_rec);
    ck_assert_int_eq(result, GRAIN_END);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_delete_record_null_heapfile)
{
    RecordId rid = {.page_id = 0, .slot_idx = 0};
    ck_assert_int_eq(hf_delete_record(NULL, rid), GRAIN_NULL_PTR);
}
END_TEST

START_TEST(test_hf_delete_record_invalid_page)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    RecordId rid = {.page_id = 999, .slot_idx = 0};

    GrainResult result = hf_delete_record(hf, rid);
    ck_assert_int_eq(result, GRAIN_INVALID_PAGE_ID);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_delete_record_invalid_slot)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record rec = {.id = 1, .age = 25};
    strcpy(rec.name, "Test");
    strcpy(rec.email, "test@test.com");
    ck_assert_int_eq(hf_insert_record(hf, &rec), GRAIN_OK);

    RecordId rid = {.page_id = 0, .slot_idx = 999};

    GrainResult result = hf_delete_record(hf, rid);
    ck_assert_int_eq(result, GRAIN_INVALID_SLOT);

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_delete_record_reuses_slot)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    Record rec1 = {.id = 1, .age = 25};
    strcpy(rec1.name, "First");
    strcpy(rec1.email, "first@test.com");
    ck_assert_int_eq(hf_insert_record(hf, &rec1), GRAIN_OK);

    RecordId rid = {.page_id = 0, .slot_idx = -1};
    Record found_rec;
    ck_assert_int_eq(hf_scan_next(hf, &rid, &found_rec), GRAIN_OK);

    ck_assert_int_eq(hf_delete_record(hf, rid), GRAIN_OK);

    Record rec2 = {.id = 2, .age = 30};
    strcpy(rec2.name, "Second");
    strcpy(rec2.email, "second@test.com");
    ck_assert_int_eq(hf_insert_record(hf, &rec2), GRAIN_OK);

    ck_assert_int_eq(hf->header.num_pages, 1);

    RecordId rid2 = {.page_id = 0, .slot_idx = -1};
    ck_assert_int_eq(hf_scan_next(hf, &rid2, &found_rec), GRAIN_OK);
    ck_assert_int_eq(found_rec.id, 2);
    ck_assert_str_eq(found_rec.name, "Second");

    close_file(hf);
    cleanup();
}
END_TEST

START_TEST(test_hf_delete_record_page_rejoins_free_list)
{
    cleanup();

    HeapFile *hf = create_file(test_file);
    ck_assert_ptr_nonnull(hf);

    for (int i = 0; i < MAX_SLOTS; i++) {
        Record rec = {.id = i, .age = 20};
        snprintf(rec.name, sizeof(rec.name), "User%d", i);
        snprintf(rec.email, sizeof(rec.email), "u%d@t.com", i);
        ck_assert_int_eq(hf_insert_record(hf, &rec), GRAIN_OK);
    }

    ck_assert_int_eq(hf->header.first_free_page, -1);

    RecordId rid = {.page_id = 0, .slot_idx = 0};
    ck_assert_int_eq(hf_delete_record(hf, rid), GRAIN_OK);

    ck_assert_int_eq(hf->header.first_free_page, 0);

    close_file(hf);
    cleanup();
}
END_TEST

static Suite *file_suite(void)
{
    Suite *s;
    TCase *tc_create, *tc_open, *tc_close, *tc_readwrite;
    TCase *tc_insert, *tc_scan, *tc_update, *tc_delete;

    s = suite_create("File Tests");

    tc_create = tcase_create("Create");
    tcase_add_test(tc_create, test_create_file_creates_on_disk);
    tcase_add_test(tc_create, test_create_file_header_initialized);
    tcase_add_test(tc_create, test_create_file_header_persisted);
    tcase_add_test(tc_create, test_create_file_null_filename);
    tcase_add_test(tc_create, test_create_file_returns_valid_heapfile);
    suite_add_tcase(s, tc_create);

    tc_open = tcase_create("Open");
    tcase_add_test(tc_open, test_open_file_reads_header);
    tcase_add_test(tc_open, test_open_file_null_filename);
    tcase_add_test(tc_open, test_open_file_nonexistent);
    tcase_add_test(tc_open, test_open_file_corrupted);
    tcase_add_test(tc_open, test_open_file_truncated);
    tcase_add_test(tc_open, test_open_file_invalid_header_negative_pages);
    tcase_add_test(tc_open, test_open_file_invalid_header_bad_free_page);
    suite_add_tcase(s, tc_open);

    tc_close = tcase_create("Close");
    tcase_add_test(tc_close, test_close_file_returns_ok);
    tcase_add_test(tc_close, test_close_file_null_ptr);
    tcase_add_test(tc_close, test_close_file_flushes_data);
    suite_add_tcase(s, tc_close);

    tc_readwrite = tcase_create("ReadWrite");
    tcase_add_test(tc_readwrite, test_write_page_success);
    tcase_add_test(tc_readwrite, test_write_page_null_heapfile);
    tcase_add_test(tc_readwrite, test_write_page_null_heappage);
    tcase_add_test(tc_readwrite, test_read_page_success);
    tcase_add_test(tc_readwrite, test_read_page_null_heapfile);
    tcase_add_test(tc_readwrite, test_read_page_null_heappage);
    tcase_add_test(tc_readwrite, test_read_page_invalid_page_id_negative);
    tcase_add_test(tc_readwrite, test_read_page_invalid_page_id_out_of_bounds);
    tcase_add_test(tc_readwrite, test_read_write_data_integrity);
    tcase_add_test(tc_readwrite, test_read_write_multiple_pages);
    suite_add_tcase(s, tc_readwrite);

    tc_insert = tcase_create("Insert");
    tcase_add_test(tc_insert, test_hf_insert_record_success);
    tcase_add_test(tc_insert, test_hf_insert_record_null_params);
    tcase_add_test(tc_insert, test_hf_insert_record_multiple);
    tcase_add_test(tc_insert, test_hf_insert_record_allocates_new_page);
    suite_add_tcase(s, tc_insert);

    tc_scan = tcase_create("Scan");
    tcase_add_test(tc_scan, test_hf_scan_next_empty_file);
    tcase_add_test(tc_scan, test_hf_scan_next_single_record);
    tcase_add_test(tc_scan, test_hf_scan_next_multiple_records);
    tcase_add_test(tc_scan, test_hf_scan_next_across_pages);
    tcase_add_test(tc_scan, test_hf_scan_next_null_params);
    suite_add_tcase(s, tc_scan);

    tc_update = tcase_create("Update");
    tcase_add_test(tc_update, test_hf_update_record_success);
    tcase_add_test(tc_update, test_hf_update_record_null_params);
    tcase_add_test(tc_update, test_hf_update_record_invalid_page);
    tcase_add_test(tc_update, test_hf_update_record_invalid_slot);
    suite_add_tcase(s, tc_update);

    tc_delete = tcase_create("Delete");
    tcase_add_test(tc_delete, test_hf_delete_record_success);
    tcase_add_test(tc_delete, test_hf_delete_record_null_heapfile);
    tcase_add_test(tc_delete, test_hf_delete_record_invalid_page);
    tcase_add_test(tc_delete, test_hf_delete_record_invalid_slot);
    tcase_add_test(tc_delete, test_hf_delete_record_reuses_slot);
    tcase_add_test(tc_delete, test_hf_delete_record_page_rejoins_free_list);
    suite_add_tcase(s, tc_delete);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = file_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? 0 : 1;
}

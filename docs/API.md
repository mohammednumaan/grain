# Grain API Reference

A simple heap file storage engine in C.

---

## Architecture

```
+===========================================================================+
|                          HEAP FILE (example.bin)                          |
+===========================================================================+
|                                                                           |
|   +-----------------+                                                     |
|   |   FileHeader    |  12 bytes                                           |
|   +-----------------+                                                     |
|   | num_pages: 2    |                                                     |
|   | next_page_idx: 2|                                                     |
|   | first_free_page +-----+                                               |
|   +-----------------+     |                                               |
|                           v                                               |
|   +-----------------------------+     +-----------------------------+     |
|   |          PAGE 0             |     |          PAGE 1             |     |
|   |        (8192 bytes)         |     |        (8192 bytes)         |     |
|   +-----------------------------+     +-----------------------------+     |
|   | PageHeader      (20 bytes)  |     | PageHeader      (20 bytes)  |     |
|   | +-------------------------+ |     | +-------------------------+ |     |
|   | | page_id: 0              | |     | | page_id: 1              | |     |
|   | | num_slots: 5            | |     | | num_slots: 3            | |     |
|   | | next_slot_idx: 5        | |     | | next_slot_idx: 3        | |     |
|   | | first_free_slot: -1     | |     | | first_free_slot: -1     | |     |
|   | | next_free_page: 1 ------+-+---->| | next_free_page: -1      | |     |
|   | +-------------------------+ |     | +-------------------------+ |     |
|   +-----------------------------+     +-----------------------------+     |
|   | Storage       (8172 bytes)  |     | Storage       (8172 bytes)  |     |
|   | +--------+ +--------+       |     | +--------+ +--------+       |     |
|   | |Record 0| |Record 1| ...   |     | |Record 0| |Record 1| ...   |     |
|   | +--------+ +--------+       |     | +--------+ +--------+       |     |
|   +-----------------------------+     +-----------------------------+     |
|                                                                           |
+===========================================================================+
```

---

## Record Structure

```
+===================================================================+
|                       RECORD (64 bytes)                           |
+============+=======================+============+=================+
|     id     |         name          |    age     |      email      |
|   4 bytes  |       32 bytes        |   4 bytes  |    24 bytes     |
|  (int32_t) |       (char[])        |  (int32_t) |     (char[])    |
+============+=======================+============+=================+
```

---

## Free Space Management

### Free Page List

Pages with available slots are linked together:

```
FileHeader.first_free_page
             |
             v
        +--------+       +--------+       +--------+
        | Page 0 | ----> | Page 2 | ----> | Page 5 | ----> NULL
        +--------+       +--------+       +--------+
```

### Free Slot List

Deleted slots within a page form a linked list:

```
PageHeader.first_free_slot
             |
             v
        +--------+       +--------+       +--------+
        | Slot 3 | ----> | Slot 1 | ----> | Slot 7 | ----> NULL
        +--------+       +--------+       +--------+
```

New inserts reuse slots from the head (LIFO).

---

## Data Types

### Record

```c
typedef struct {
    int32_t id;
    char name[32];
    int32_t age;
    char email[24];
} Record;
```

### RecordId

```c
typedef struct {
    int32_t page_id;
    int32_t slot_idx;
} RecordId;
```

### GrainResult

| Value | Name                     | Description           |
|-------|--------------------------|-----------------------|
|   0   | `GRAIN_OK`               | Success               |
|   1   | `GRAIN_END`              | End of scan           |
|  -1   | `GRAIN_NULL_PTR`         | Null pointer          |
|  -2   | `GRAIN_INVALID_SLOT`     | Invalid slot index    |
|  -3   | `GRAIN_RECORD_NOT_FOUND` | Record not found      |
|  -4   | `GRAIN_PAGE_FULL`        | Page is full          |
|  -5   | `GRAIN_INVALID_PAGE_ID`  | Invalid page ID       |
|  -6   | `GRAIN_FILE_OPEN_FAILED` | Failed to open file   |
|  -7   | `GRAIN_FILE_READ_FAILED` | Failed to read file   |
|  -8   | `GRAIN_FILE_WRITE_FAILED`| Failed to write file  |
|  -9   | `GRAIN_FILE_SEEK_FAILED` | Failed to seek file   |
| -10   | `GRAIN_CORRUPT_HEADER`   | Corrupted header      |

---

## Constants

| Constant       | Value | Description                |
|----------------|-------|----------------------------|
| `PAGE_SIZE`    | 8192  | Page size in bytes (8KB)   |
| `RECORD_SIZE`  | 64    | Record size in bytes       |
| `MAX_SLOTS`    | 127   | Maximum records per page   |
| `FREE_SLOT_END`| -1    | End of free list marker    |

---

## File Operations

### create_file

```c
HeapFile *create_file(const char *filename);
```

Creates a new heap file. Returns `HeapFile*` on success, `NULL` on failure.

### open_file

```c
HeapFile *open_file(const char *filename);
```

Opens an existing heap file. Returns `HeapFile*` on success, `NULL` on failure.

### close_file

```c
GrainResult close_file(HeapFile *file);
```

Closes a heap file and frees resources. Returns `GRAIN_OK` on success.

---

## Record Operations

### hf_insert_record

```c
GrainResult hf_insert_record(HeapFile *hf, Record *rec);
```

Inserts a record. Allocates new pages automatically.

### hf_scan_next

```c
GrainResult hf_scan_next(HeapFile *hf, RecordId *rid, Record *rec);
```

Gets the next record. Initialize `rid` to `{0, -1}` to start scanning.
Returns `GRAIN_OK` if found, `GRAIN_END` if no more records.

### hf_update_record

```c
GrainResult hf_update_record(HeapFile *hf, RecordId rid, Record *rec);
```

Updates a record at the given location.

### hf_delete_record

```c
GrainResult hf_delete_record(HeapFile *hf, RecordId rid);
```

Deletes a record at the given location.

---

## File Layout

```
Offset      Content
----------- ------------------
0           FileHeader (12 bytes)
12          Page 0 (8192 bytes)
8204        Page 1 (8192 bytes)
16396       Page 2 (8192 bytes)
...         ...
```

**Formula:** `offset = 12 + (page_id * 8192)`

---

## Example

```c
#include "include/file.h"
#include "include/heap.h"

int main() {
    HeapFile *hf = create_file("test.bin");

    // Insert
    Record r = {.id = 1, .age = 25, .name = "Alice", .email = "alice@test.com"};
    hf_insert_record(hf, &r);

    // Scan
    RecordId rid = {0, -1};
    Record rec;
    while (hf_scan_next(hf, &rid, &rec) == GRAIN_OK) {
        printf("%d: %s\n", rec.id, rec.name);
    }

    // Update
    rid = (RecordId){0, -1};
    while (hf_scan_next(hf, &rid, &rec) == GRAIN_OK) {
        if (rec.id == 1) {
            rec.age = 26;
            hf_update_record(hf, rid, &rec);
            break;
        }
    }

    // Delete
    rid = (RecordId){0, -1};
    while (hf_scan_next(hf, &rid, &rec) == GRAIN_OK) {
        if (rec.id == 1) {
            hf_delete_record(hf, rid);
            break;
        }
    }

    close_file(hf);
    return 0;
}
```

---

## Building

```bash
make heap_test      # Build heap tests
make file_test      # Build file tests
make main           # Build demo

make run_heap_test  # Run heap tests
make run_file_test  # Run file tests
make run_main       # Run demo

make clean          # Clean build artifacts
```

---

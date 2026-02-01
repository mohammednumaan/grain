# grain

an experimental heap file storage engine for fixed-length records in C for learning database internals.

> **Note:** This is not production-ready. It was built as a learning project to explore how data is stored in databases 

## build

    make main           # build demo
    make run_main       # run demo
    make run_heap_test  # run heap tests
    make run_file_test  # run file tests

## example

```c
// create a file
HeapFile *hf = create_file("data.bin");

// insert records
Record alice = {1, "Alice", 25, "alice@example.com"};
Record bob = {2, "Bob", 30, "bob@example.com"};
hf_insert_record(hf, &alice);
hf_insert_record(hf, &bob);

// scan all records
RecordId rid = {-1, -1};
Record rec;
while (hf_scan_next(hf, &rid, &rec) == GRAIN_OK) {
    printf("[page %d, slot %d] %s\n", rid.page_id, rid.slot_idx, rec.name);
}

// update a record
rec.age = 31;
hf_update_record(hf, rid, &rec);

// delete a record
hf_delete_record(hf, rid);

// close the file
close_file(hf);
```

## docs

see [docs/API.md](docs/API.md) for the API reference.

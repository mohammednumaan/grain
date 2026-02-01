# design
this file is a complete documentation of me building `grain`. it includes all my learnings and decisions i've made to build this project.

# goal
a few weeks back, i read about `database storage organization` from a well known book called "Database System Concepts". so i thought, "why not try to build something to enforce these concepts?". the main goal of this project is to understand `disk-oriented` database systems and how they internally store data.

# understanding non-volatile storage
**non-volatile** storage is a type of computer memory that can retain information even after the power is turned off. it is mainly used for **long-term persistent** storage. this type of storage can be further divided into 2 categories:
1. **mechanical:** uses physical moving parts to perform disk operations (eg: HDD)
2. **electrical:** uses electrical signals to perform disk operations (eg: SSD)

these type of storage devices are **block-addressable**. meaning, data can be read or written only in fixed-size "blocks" and not in "bytes".
## questions
**why are hard disks slow compared to RAM (volatile-storage)?**
the reason disks are slow compared to RAM is because of how they are designed. typical `hdd's` contain many physical moving parts which can drop performance when trying to read or write data.  the delays associated with `hdd's` are:
    - **seek latency**: time it takes for the head to move to the correct sector.
    - **rotational latency**: time it takes to rotate the required sector to move under the head.

`ssd's` signficantly improve performance but is still slower than RAM. this is because the way they are designed is very different. moreover, `ssd's` cannot be directly accessed like RAM (RAM can be accessed directly via the address bus, but SSD's require PCI-bus). anything attached to the address bus is directly readable and writable via simple `load & store` instructions. anything attached the other way (in this case SSD) requires much more instructions to access. there is more info on why its slower, which i will explore later in my free time.

# sequential vs random access
one of the most fundamental questions when talking about disks is how data is accessed. it can be divided into 2 different ways:
1. sequential access (reads and writes)
2. random access (reads and writes)

## sequential access:
this data access pattern reads/writes data in continuous and linear fashion. the seek and rotational latency only occurs once because the the data to be read/write is adjacent to the previously accessed data.

## random access
this data access pattern read/writes data in a random order. the data to be read/write can be located anywhere on the disk, therefore there is a lot of seek and rotational latency.

for interactive visualizations, the best resource out there is: https://planetscale.com/blog/io-devices-and-latency.

# heap file organization
now that i understand how disks work, i can start thinking about how to organize data on disk. the simplest approach is called a **heap file**. a heap file is just a file where records are stored in no particular order. records are inserted wherever there is space.

## why heap files?
heap files are simple to implement and work well for:
- bulk inserts (just append to the end)
- full table scans (read everything sequentially)
- situations where you don't need fast lookups by key

the downside is that finding a specific record requires scanning the entire file. but that's okay for now - the goal is to understand the basics first.

# pages
instead of reading/writing individual records, databases work with **pages**. a page is a fixed-size block of data (commonly 4KB, 8KB, or 16KB). i chose **8KB (8192 bytes)** for grain.

## why pages?
1. **disk alignment**: disks read/write in blocks anyway, so aligning to page boundaries is efficient
2. **buffer management**: easier to cache fixed-size chunks in memory
3. **less I/O overhead**: reading one 8KB page is faster than reading 127 individual 64-byte records

## page structure
each page in grain has two parts:
1. **header**: metadata about the page (20 bytes)
2. **storage**: space for actual records (8172 bytes)

```
+------------------+
|   PageHeader     |  20 bytes
+------------------+
|                  |
|    Records       |  8172 bytes
|                  |
+------------------+
```

the header contains:
- `page_id`: unique identifier for this page
- `num_slots`: how many active records are stored
- `next_slot_idx`: high water mark (next slot to allocate)
- `first_free_slot`: head of deleted slots linked list
- `next_free_page`: link to next page with free space

# fixed-length records
for simplicity, grain uses **fixed-length records**. each record is exactly **64 bytes**:

```
+------+----------+------+----------+
|  id  |   name   | age  |  email   |
+------+----------+------+----------+
   4       32        4       24      = 64 bytes
```

## why fixed-length?
- simple offset calculation: `slot_address = storage + (slot_idx * 64)`
- no fragmentation issues
- predictable space usage

the tradeoff is wasted space if fields don't use their full allocation. but for learning purposes, this simplicity is worth it.

# free space management
one of the trickier parts of building a storage engine is managing free space. when records are deleted, we need a way to reuse that space.

## the problem
imagine we have 5 records and delete record 2:

```
before: [R0][R1][R2][R3][R4]
after:  [R0][R1][__][R3][R4]
```

we have a "hole" at slot 2. if we just keep appending new records at the end, we waste space.

## solution: free slot list
i use a **linked list** to track deleted slots. when a record is deleted, its slot is added to the front of the list. the slot's memory is reused to store a pointer to the next free slot.

```
first_free_slot = 2
slot 2 -> next_free_slot = -1 (end of list)
```

if we delete slot 4 next:

```
first_free_slot = 4
slot 4 -> next_free_slot = 2
slot 2 -> next_free_slot = -1
```

when inserting, we pop from the head of the list (LIFO). this is simple and efficient.

## free page list
the same concept applies at the file level. pages with available space are linked together:

```
first_free_page -> Page 0 -> Page 2 -> Page 5 -> NULL
```

when a page becomes full, it's removed from the list. when a slot is deleted from a full page, it rejoins the list.

# file layout
the heap file has a simple layout:

```
+------------------+  offset 0
|   FileHeader     |  12 bytes
+------------------+  offset 12
|     Page 0       |  8192 bytes
+------------------+  offset 8204
|     Page 1       |  8192 bytes
+------------------+  offset 16396
|     Page 2       |  8192 bytes
+------------------+
       ...
```

the file header contains:
- `num_pages`: total number of pages
- `next_page_idx`: next page id to allocate
- `first_free_page`: head of free page list

# design decisions

## decision 1: no auto-compaction
when records are deleted, empty slots remain in place. i don't compact the page or move records around. this keeps the implementation simple and avoids invalidating any external references to record locations.

## decision 2: no duplicate id checking
the storage layer doesn't enforce uniqueness on the `id` field. that's a higher-level concern. grain is just a storage engine - it stores and retrieves bytes.

## decision 3: update doesn't change id
when updating a record, the `id` field is preserved. only `name`, `age`, and `email` are modified. this prevents accidental key changes.

## decision 4: scan-based lookups
there's no index. to find a record by id, you scan all records. this is O(n) but keeps things simple. adding an index would be a good future improvement.

## decision 5: explicit error codes
i use an enum with simple values:
- `0` = success
- `1` = end of iteration
- negative values = errors

this makes error checking easy: `if (result < 0) { handle_error(); }`

# data type choices

## why int32_t?
i use `int32_t` from `<stdint.h>` instead of plain `int` for:
- **portability**: `int32_t` is guaranteed to be 4 bytes everywhere
- **explicitness**: clear about the size we're using
- **alignment**: predictable struct layout

## why char[32] and char[24]?
these sizes were chosen to:
1. make the total record size exactly 64 bytes (a power of 2)
2. avoid padding issues (both are divisible by 4)
3. provide reasonable space for names (31 chars + null) and emails (23 chars + null)

# what i learned
building grain taught me:

1. **pages are fundamental**: databases don't read individual records they read pages.

2. **free space management is tricky**: you need to track holes efficiently. linked lists are a simple solution.

3. **fixed-size structures are easier**: variable-length records add complexity (slotted pages, compaction, etc). starting with fixed-length was simple.

4. **error handling matters**: having a clear error code strategy from the start saves headaches later.

5. **tests are essential**: the 56 tests were written by claude, i was too lazy for it.

# future improvements
things i'd add if i continue this project:

1. **indexes**: a B+ tree index for O(log n) lookups by id
2. **variable-length records**: slotted page layout for strings of varying size
3. **buffer pool**: cache pages in memory instead of reading from disk every time

# conclusion
grain is a minimal but functional heap file storage engine. it handles the basics: creating files, inserting records, scanning, updating, and deleting. the code is clean, well-tested, and documented.

most importantly, building this helped me understand how database storage actually works under the hood. reading about pages and free lists in a textbook is one thing - implementing them is another.

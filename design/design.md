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

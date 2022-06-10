Virtual-Memory-Management
Authored by Laith Al-mouhtaseb
211353297

==Description==
This program is simulating memory management using paging,it works with 2 process and executable file or less,open a swap file and init a memory with 200 size.


==methods==
sim_mem: the constructor method init the tables and open files according to the number of the process, and reference every value to the object value.

~sim_mem: the destructor free the allocated memory and close the opened files.

load: received 2 args, process number and the address that you want to load, convert the address to physical address,
according to many situation,if the page not found in the physical memory(RAM), the method bring it from the correct location (logical memory,swap file or init a new page),
and at the end it returns the character according to the address that received and update the page table.

store: same like load but this method edit the address and add the value that received.

freeLoc: if there's no free location at the physical memory (RAM), the method put the page at swap file or override it if the page not dirty
and can bring it any time from executable file.

emptyLoc: this method return the index of the empty location at the memory and if the memory full it calls freeLoc method then return the free location index.

print_memory: print the main memory.

print_swap: print the swap file,every page in one line.

print_page_table: print the page table for each process.

==Program Files==
sim_mem.h,sim_mem.cpp

==How to compile==
compile: g++ -o main main.cpp sim_mem.cpp
run: ./main

==input==
files: exec1,exec2

==output==
organization memory.
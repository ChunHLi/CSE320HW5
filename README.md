# HW #5

## Part 1 ##
Possible inputs:

```
create
kill X
list
mem X
allocate X
read X Y
write X Y Z
exit
```

Allocate calls cse320_malloc() which takes three parameters:
1. unsigned long pt_1[64] - Page Table 1
2. unsigned long pt_2[64] - Page Table 2
3. char* myid - index of process to be allocated

cse320_virt_to_phys() takes two parameters:
1. unsigned long va - virtual address
2. int myid - index of process address is from


Virtual Addresses are printed in long form, not binary at the moment, might change if have time.

A pipe exists between main and mem **AND** additional pipes between main and threads. Threads communicate between the mem using the same mem as main.

## Part 2 ##

Cache is a global integer double array cache[4][2].
The outer array holds the four lines.
The inner array holds the physical address and value respectfully.


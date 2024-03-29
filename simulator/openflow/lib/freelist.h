// MAH: start
// This freelist data structure can be used to allocate numbers from a list of
// of numbers starting at 0 and ending at the specified size-1
// It was written for the controller to manage the available vport entries
// on a switch.
#ifndef FREELIST_H
#define FREELIST_H

typedef unsigned char byte_t;
//#ifndef NS3
//typedef unsigned int size_t;
//#endif

// The freelist is implemented as a bitmap that is dynamically allocated in
// BLOCK_SIZE byte chunks.
#define BLOCK_SIZE	1024


typedef struct freeList
{
	byte_t **byteArray;
	uint32_t size;		// entries in freeList
	uint32_t byteCnt;		// number of bytes allocated
	uint32_t firstFree;	// start search for next bit to allocate from nextIndx_
} freeList, *freeListPtr;


// create a new freelist
freeListPtr create_freelist(uint32_t size);

// returns true if there are free entries in the freelist
bool free_entries(freeListPtr list);

// returns the next free entry in the free list
// note: you must first check if there are available entries using free_entries()
uint32_t alloc_entry(freeListPtr list);

// frees the specified entry in the free list allowing it to be reallocated
void free_entry(freeListPtr list, uint32_t entry);

// destroy the freelist object
void destroy_freelist(freeListPtr list);

#endif
// MAH: end

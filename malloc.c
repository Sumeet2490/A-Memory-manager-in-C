#include <stdio.h>
#include <unistd.h>
#include <string.h>
//#include "malloc.h"

// a struct representing a memory block
struct memoryBlock {
	// metadata about memory block
	size_t size;		// 8 bytes
	struct memoryBlock *next;	// 8 bytes
//void *space;
};

// function declarations
void* malloc(size_t);
void free(void*);
void* calloc(size_t, size_t);
void* realloc(void*, size_t);
struct memoryBlock* mymemcpy(void*, const void*, size_t);
void fuse_adjacent_freeBlocks();
size_t round_to_next_power_of_two(unsigned int);
struct memoryBlock* get_bestFit_freeBlock(size_t size);
void print_freelist();

struct memoryBlock head = { 0, 0 };

/* This function allocates a block of memory of size bytes. */
void* malloc(size_t size) {

//	char msg_buf[100];

//	sprintf(msg_buf, "\t(%s) ", __func__);
//	write(2, msg_buf, strlen(msg_buf));

	//If size is zero or less, return NULL
	if (size <= 0) {
		return NULL;
	}

	struct memoryBlock *p = head.next;

	size = round_to_next_power_of_two(size + sizeof(struct memoryBlock));
//	sprintf(msg_buf, "Allocating %lu bytes...", size);
//	write(2, msg_buf, strlen(msg_buf));

	// when free-list is empty, No freelist traversal, just sbrk and return the pointer
	if (p == NULL) {
//		sprintf(msg_buf, "Free-list is empty, calling sbrk...");
//		write(2, msg_buf, strlen(msg_buf));

		p = (struct memoryBlock*) sbrk(size);
		p->size = size;
//		sprintf(msg_buf, "returned block (%p) to user.\n",
//				((char*) p) + sizeof(struct memoryBlock));
//		write(2, msg_buf, strlen(msg_buf));

		return ((char*) p) + sizeof(struct memoryBlock);
	}

	// traverse the free-list for a best-fit, if found return it
	p = get_bestFit_freeBlock(size);
	if (p != NULL) {
//		sprintf(msg_buf, "returned block (%p) to user.\n",
//				((char*) p) + sizeof(struct memoryBlock));
//		write(2, msg_buf, strlen(msg_buf));

		return ((char*) p) + sizeof(struct memoryBlock);
	}

	// reached only if best-fit not found, sbrk and return
//	sprintf(msg_buf, "No best-fit found, calling sbrk.\n");
//	write(2, msg_buf, strlen(msg_buf));

	p = (struct memoryBlock*) sbrk(size);
	p->size = size;
	return ((char*) p) + sizeof(struct memoryBlock);
}

/* This function frees a block of memory that had previously been allocated. */
void free(void *ptr) {

//	char msg_buf[100];

//	sprintf(msg_buf, "\n\t(%s) ", __func__);
//	write(2, msg_buf, strlen(msg_buf));

	// If ptr is NULL, this function does nothing, just RETURNs
	if (ptr == NULL) {
		return;
	}

	struct memoryBlock *to_free = (struct memoryBlock*) (((char*) ptr)
			- sizeof(struct memoryBlock));
//	sprintf(msg_buf, "Freeing %lu (%p) bytes...", to_free->size,
//			(void*) to_free);
//	write(2, msg_buf, strlen(msg_buf));

	struct memoryBlock *p = head.next;
	
	// if free-list is empty, insert and return
	if(p == NULL)
	{
		head.next = to_free;
		to_free->next = NULL;	
		return;
	}	

	// try to insert at the appropriate location, fuse and return
	for (p = head.next; p->next!= NULL; p = p->next) 
	{
		if((p < to_free) && (to_free < p->next))
		{
			to_free->next = p->next;
			p->next = to_free;
			fuse_adjacent_freeBlocks();
			return;
		}
	}

	// last resort - insert at the end of free-list
	p->next = to_free;
	to_free->next = NULL;	
	
	// fuse
	fuse_adjacent_freeBlocks();
}

/* This function allocates memory for an array of nmemb elements of size bytes each and returns a pointer to the allocated memory. */
void* calloc(size_t nmemb, size_t size) {

//	char msg_buf[100];

//	sprintf(msg_buf, "\t(%s) ", __func__);
//	write(2, msg_buf, strlen(msg_buf));

	//If nmemb or size is 0, then returns NULL
	if ((nmemb == 0) || (size == 0)) {
		return NULL;
	}

	size_t actualSize = nmemb * size;
	//sprintf(msg_buf,"Need to allocate %lu bytes...", roundToNextPowerOf2(actualSize));

	void *p = malloc(actualSize);

	// return memset(p, 0, size); alternate code below
	// zero the memory location
	char *d = (char*) p;
	for (size_t i = 0; i < size; i++) {
		d[i] = 0;
	}

	return p;
}

/* realloc() changes the size of the memory block pointed to by ptr to size bytes. */
void* realloc(void *ptr, size_t size) {

//	char msg_buf[100];

//	sprintf(msg_buf, "\t(%s) ", __func__);
//	write(2, msg_buf, strlen(msg_buf));

	size_t actulSize = round_to_next_power_of_two(
			size + sizeof(struct memoryBlock));

	// If ptr is NULL, then the call is equivalent to just calling malloc(size) for all values of size. EVEN for NEGATIVE? ASK TA, RESOLVED
	if (ptr == NULL) {
		return (malloc(size));
	}

	//if size is equal to zero, and ptr is not NULL, then
	if ((size == 0) && (ptr != NULL)) {
		free(ptr);
		return NULL;
	}

	//struct FreeList *p = ptr; // incorrect, overlap problem
	struct memoryBlock *p = (struct memoryBlock*) (((char*) ptr)
			- sizeof(struct memoryBlock)); //move the pointer back by sizeof(struct FreeList) to make it overlap properly

//	sprintf(msg_buf,
//			"Reallocating from %lu to %lu (user supplied size, IGNORE THIS)...",
//			p->size, size);
//	write(2, msg_buf, strlen(msg_buf));
//	sprintf(msg_buf, "Reallocating from %lu to %lu...", p->size, actulSize);
//	write(2, msg_buf, strlen(msg_buf));

	// if the new size is equal to the existing size of the block, then just return the ptr as is
	if (actulSize == p->size) {
//		sprintf(msg_buf, "Same size as present!\n");
//		write(2, msg_buf, strlen(msg_buf));
		return ptr;
	}

	// if the new size is less than the existing size of the block, split it.
	// can be split only if the resulting block is of size - power of 2, if not then we allocate a new block (after this IF)
	if (actulSize < p->size) {

		size_t size_difference = p->size - actulSize;
		if ((size_difference > sizeof(struct memoryBlock))
				&& (size_difference
						>= round_to_next_power_of_two(size_difference))) {

//			sprintf(msg_buf,
//					"Reducing block size from %lu to %lu by splitting.\n",
//					p->size, actulSize);
//			write(2, msg_buf, strlen(msg_buf));

			p->size = actulSize;

			struct memoryBlock *return_to_freelist =
					(struct memoryBlock*) (((char*) p) + p->size);
			return_to_freelist->size = size_difference;
			return_to_freelist =
					(struct memoryBlock*) (((char*) return_to_freelist)
							+ sizeof(struct memoryBlock));
			free(return_to_freelist);

			return ((char*) p) + sizeof(struct memoryBlock);
		}

	}

	// reached when neither of the cases were satisfied , and allocating a new block is the option left
	// Allocate a new block  to accommodate the new size, copy the contents to the new block and free the old block
//	sprintf(msg_buf, "Allocate a new block  to accommodate the new size\n");
//	write(2, msg_buf, strlen(msg_buf));
	struct memoryBlock *reallocedBlock = malloc(size);

	// If malloc returns NULL
	if (reallocedBlock == NULL) {
		return NULL;
	}

	// Copy contents from old location to the new one
	if (actulSize > p->size) {
		mymemcpy(reallocedBlock, (((char*) p) + sizeof(struct memoryBlock)),
				(p->size - sizeof(struct memoryBlock)));
		//sprintf(msg_buf, "(actulSize > p->size) Copied %lu bytes\n", (p->size - sizeof(struct memoryBlock)));
		//write(2, msg_buf, strlen(msg_buf));

	} else if (actulSize < p->size) {
		mymemcpy(reallocedBlock, (((char*) p) + sizeof(struct memoryBlock)),
				size);
		//sprintf(msg_buf, "(actulSize > p->size) Copied %lu bytes\n", size);
		//write(2, msg_buf, strlen(msg_buf));
	}

	// Free the old block
	free(ptr);
	return reallocedBlock;

}

struct memoryBlock* mymemcpy(void *dest, const void *src, size_t len) {

	char *d = (char*) dest;
	const char *s = (char*) src;

	for (size_t i = 0; i < len; i++) {
		d[i] = s[i];
	}
	return (struct memoryBlock*) d;
}

/* auxillary function to print the free list, meant for debug purpose only */
void print_freelist() {

	char msg_buf[100];

	sprintf(msg_buf, "\t(%s) ", __func__);
	write(2, msg_buf, strlen(msg_buf));

	struct memoryBlock *p = head.next;

	if (p == NULL) {
		sprintf(msg_buf, "Free list is empty!!\n");
		write(2, msg_buf, strlen(msg_buf));

		return;
	}

	sprintf(msg_buf, "Free-List: ");
	write(2, msg_buf, strlen(msg_buf));

	for (p = head.next; p != NULL; p = p->next) {
		//sprintf(msg_buf,"[%lu] --> ", p->size);
		sprintf(msg_buf, "[%lu](%p) --> ", p->size, (void*) p);
		write(2, msg_buf, strlen(msg_buf));

	}
	sprintf(msg_buf, " %p\n", (void*) p);
	write(2, msg_buf, strlen(msg_buf));

}

/* Merges two adjacent free blocks into a single, contiguous free block. */
void fuse_adjacent_freeBlocks() {

//	char msg_buf[100];

//	sprintf(msg_buf, "\n\t(%s) ", __func__);
//	write(2, msg_buf, strlen(msg_buf));

	struct memoryBlock *p;

	// merge until no more merges possible
	for (int i = 0; i < 2; i++) {
		for (p = head.next; p != NULL; p = p->next) {

			if (p->next != NULL) {

				if (((((char*) p) + p->size) == (char*) p->next)
						&& (p->size == p->next->size)) { // check contiguity & equality of size
//					sprintf(msg_buf, "fusing [%lu](%p) with [%lu](%p)\n",
//							p->size, (void*) p, p->next->size, (void*) p->next);
//					write(2, msg_buf, strlen(msg_buf));

					p->size = p->size + p->next->size; // update the size
					p->next = p->next->next; // merge
				}
			}
		}
	}
}

/* Given a size, returns the best-fit block from the free-list */
struct memoryBlock* get_bestFit_freeBlock(size_t size) {

//	char msg_buf[100];

	struct memoryBlock *bestFit_freeBlock = NULL;

	// set the minumum to the size difference with the first free block in the free-list
	size_t minimum = head.next->size - size;

//	sprintf(msg_buf, "==>minimum %lu - %lu = %lu\n\n", head.next->size, size,
//			minimum);
//	write(2, msg_buf, strlen(msg_buf));

	struct memoryBlock *p = head.next;
	struct memoryBlock *trail_p = &head;

	for (p = head.next; p != NULL; p = p->next) {
		if (p->size >= size) {
			// exact size best-fit
			if (p->size == size) {
//				sprintf(msg_buf, "Found best-fit...");
//				write(2, msg_buf, strlen(msg_buf));

				bestFit_freeBlock = p;
				trail_p->next = p->next; // unlink p
				return bestFit_freeBlock; // no further search needed, a kind of optimization.
			} else if ((p->size - size) <= minimum) {
				//sprintf(msg_buf, "==>checking %lu < %lu\n\n", p->size - size, minimum);
				//write(2, msg_buf, strlen(msg_buf));
				// check if OK to split
				if (((p->size - size) > sizeof(struct memoryBlock))
						&& ((p->size - size)
								>= round_to_next_power_of_two(p->size - size))) {
					minimum = p->size - size;
					bestFit_freeBlock = p;
				}
			}
		}
		trail_p = p; // trail p, useful while unlinking
	}

	// reached if best-fit found by splitting (i.e not by exact size match)
	if (bestFit_freeBlock != NULL) {
		bestFit_freeBlock->size = bestFit_freeBlock->size - size;
//		sprintf(msg_buf,
//				"Found best-fit...splitting block, leaves %lu on the freelist...",
//				bestFit_freeBlock->size);
//		write(2, msg_buf, strlen(msg_buf));

		bestFit_freeBlock = (struct memoryBlock*) (((char*) bestFit_freeBlock)
				+ bestFit_freeBlock->size);
		bestFit_freeBlock->size = size;
	}

	return bestFit_freeBlock;
}

// code for below function was taken from https://graphics.stanford.edu/~seander/bithacks.html as mentioned in the professor's e-mail
size_t round_to_next_power_of_two(unsigned int v) {

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;

	return v;
}

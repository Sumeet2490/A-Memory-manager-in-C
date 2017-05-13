# Memory-manager-in-C

The goal is to implement a simple version of malloc() and its associated functions, free(), calloc(), and realloc().

API

void *malloc(size_t size);
This function allocates a block of memory of size bytes. If size is zero, it returns NULL. The returned address must be aligned to a multiple of 8. Also, the memory returned from calloc() and realloc() must be aligned to 8.

void free(void *ptr);
This function frees a block of memory that had previously been allocated. If ptr is NULL, this function does nothing.

void *calloc(size_t nmemb, size_t size);
This function allocates memory for an array of nmemb elements of size bytes each and returns a pointer to the allocated memory. The memory is set to zero. If nmemb or size is 0, then this returns NULL.

void *realloc(void *ptr, size_t size);
realloc() changes the size of the memory block pointed to by ptr to size bytes. Existing contents will not be changed, however, a new memory block with a copy of the original contents may be returned. In any case, the contents will be unchanged to the minimum of the old and new sizes; and any newly allocated memory will be uninitialized.

If ptr is NULL, then the call is equivalent to just calling malloc(size) for all values of size; if size is equal to zero, and ptr is not NULL, then the call is equivalent to free(ptr). Unless ptr is NULL, it must have been returned by an earlier call to malloc(), calloc() or realloc(). If a new block had to be allocated to accommodate the new size, the old block should be freed.

Implementation

To obtain memory from the OS, use the sbrk() system call. This call increases the size of the heap by the given amount, and returns the previous limit. See the man page for details.

When free() is called, the memory cannot usually be returned to the OS, due to contiguity restrictions. So your implementation should simply put this memory on its own internal free set. When malloc() is called, your implementation should first check your internal free set. Only if the memory is not available there should your implementation call sbrk() to request another chunk.

To keep things relatively simple, all block sizes should be based on powers of 2. You should use a best fit strategy to allocate from your free set. In other words, return the smallest block that will satisfy the malloc() request. If the block is larger than needed, you must split the block if there is enough free space to create a block of the next smaller power of 2. However, you are also allowed to maintain a reasonable minimum size of a block. In other words, if the unused space available is smaller than a reasonable limit, you do not need to split the block.

Also, you must merge adjacent free blocks. In other words, if two free blocks are adjacent, they must be merged into a single, contiguous free block. Your merge code does not need to be efficient.

To help you get started, an example is here that uses sbrk(). Note that there may be library functions that you cannot call, because some library functions may internally call malloc(), leading to problems such as an infinite recursion.

Do not use any floating point functions that require linking with -lm.

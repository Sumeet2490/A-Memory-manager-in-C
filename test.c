/*
 * To call wrappers, define CS550_RENAME with
 *    -DCS550_RENAME
 *
 * To run this use:
 *
 *     ./a.out check iters n_blocks max_size
 *
 * The arguments are:
 *
 *     check
 *          Either 0 or 1. 0 means don't check for overwriting. Use 0 when testing performance.
 *
 *      iters
 *          Number of iterations to do.
 *
 *      n_blocks
 *          Maximum possible number of blocks to be allocated at any one time.
 *
 *      max_size
 *          Maximum size of a block.
 *
 */

#ifdef CS550_RENAME
    #define malloc cs550_malloc
    #define free cs550_free
    #define calloc cs550_calloc
    #define realloc cs550_realloc
    #define sbrk cs550_sbrk
#endif

// To get sbrk() from unistd.h if compiling in C99.
#define _XOPEN_SOURCE 500
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc.c"



/*
 * cs550_print() is a print function that supports %lu, %lx, %s, and %%, and will not call malloc().
 * 
 * cs550_dbg_print() is a macro that calls cs550_print(). If you do not define CS550_DBG_PRINT, then
 * nothing that uses this macro will print out.
 * 
 * cs550_assert() is like regular assert(), except that it won't call malloc().  You need to define CS550_ASSERT
 * to enable it.
 * 
*/



#ifdef CS550_DBG_PRINT
    #pragma GCC diagnostic push
    // #pragma GCC diagnostic ignored "-Wgnu"
    #define cs550_dbg_print(fmt, ...) \
        cs550_print("[%s, %s, %lu]: " fmt, __FILE__, __func__, (unsigned long) __LINE__, ##__VA_ARGS__);
    #pragma GCC diagnostic pop
#else
    #define cs550_dbg_print(fmt, ...) ((void) 0)
#endif



#ifdef CS550_ASSERT
    #define cs550_assert(c) \
        (c) ? (void) 0 : cs550_assert_fail(#c, __FILE__, __func__, __LINE__)
#else
    #define cs550_assert(c)  ((void) 0)
#endif



void cs550_assert_fail(const char *cond, const char *file_name, const char *func_name, int line_num);
void cs550_print(const char *fmt, ...);



// We don't want to call any standard functions in cs550_print() in actual use,
// since those might call malloc() and cs550_print() will likely be called from
// within malloc().  However, when debugging cs550_print() itself, we won't be
// calling it from within malloc(), in which case it's fine to call assert(),
// etc.  Thus, this wil make any use of assert() in cs550_print() go away,
// unless we are actually just testing cs550_print().  In that case, define
// CS550_PRINT_DEBUG.
#ifndef CS550_PRINT_DEBUG
#define cs550_print_assert(cond) ((void) 0)
#else
#define cs550_print_assert assert
#endif

static void
error() {
    const char *const m = "\ncs550_print(): Bad format.\n";
    (void) write(2, m, strlen(m));
}

// Convert ulong to decimal ASCII.  dtr means digit to the right.
static char *
ultoda_helper(char *buf, unsigned long ul, int dtr) {
    if (ul > 0) {
        buf = ultoda_helper(buf, ul/10, dtr + 1);
        *buf++ = '0' + ul%10;
        // Check if need a comma.
        if (dtr%3 == 0 && dtr > 0) {
            *buf++ = ',';
        }
    }
    return buf;
}
static void
ultoda(char *buf, unsigned long ul) {
    cs550_print_assert(buf != 0);
    if (ul == 0) {
        *buf++ = '0';
    } else {
        buf = ultoda_helper(buf, ul, 0);
    }
    *buf = '\0';
}

// Convert ulong to hex ASCII.
static char hex_digit_map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
// dtr means digit to the right.
static char *
ultoha_helper(char *buf, unsigned long ul, int dtr) {
    if (ul > 0) {
        buf = ultoha_helper(buf, ul/16, dtr + 1);
        *buf++ = hex_digit_map[ul%16];
        // Check if need a comma.  For hex, do it every 4.
        if (dtr%4 == 0 && dtr > 0) {
            *buf++ = ',';
        }
    }
    return buf;
}
static void
ultoha(char *buf, unsigned long ul) {
    cs550_print_assert(buf != 0);
    if (ul == 0) {
        *buf++ = '0';
    } else {
        buf = ultoha_helper(buf, ul, 0);
    }
    *buf = '\0';
}

// This is a tiny version of printf() that accepts only %lu, %lx, %s, and %%.
#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
void
cs550_print(const char *fmt, ...) {

    enum { ST_CHUNK = 1, ST_PERCENT, ST_L } state = ST_CHUNK;
    size_t chunk_begin = 0;
    int rv;

    va_list ap;
    va_start(ap, fmt);

    size_t i = 0;
    for (; fmt[i] != '\0'; i++) {
        int ch = fmt[i];
        switch (state) {
            case ST_CHUNK:
                {
                    if (ch == '%') {
                        state = ST_PERCENT;
                        rv = write(2, &fmt[chunk_begin], i - chunk_begin);
                        cs550_print_assert((unsigned) rv == i - chunk_begin);
                    }
                }
                break;
            case ST_PERCENT:
                {
                    if (ch == 's') {
                        const char *s = va_arg(ap, const char *);
                        rv = write(2, s, strlen(s));
                        cs550_print_assert((unsigned) rv == strlen(buf));
                        chunk_begin = i + 1;
                        state = ST_CHUNK;
                    } else if (ch == 'l') {
                        state = ST_L;
                    } else if (ch == '%') {
                        chunk_begin = i;
                        state = ST_CHUNK;
                    } else {
                        error();
                    }
                }
                break;
            case ST_L:
                {
                    if (ch == 'u') {
                        unsigned long ul = va_arg(ap, unsigned long);
                        char buf[100];
                        ultoda(buf, ul);
                        rv = write(2, buf, strlen(buf));
                        cs550_print_assert((unsigned) rv == strlen(buf));
                        chunk_begin = i + 1;
                        state = ST_CHUNK;
                    } else if (ch == 'x') {
                        unsigned long ul = va_arg(ap, unsigned long);
                        char buf[100];
                        ultoha(buf, ul);
                        rv = write(2, buf, strlen(buf));
                        cs550_print_assert((unsigned) rv == strlen(buf));
                        chunk_begin = i + 1;
                        state = ST_CHUNK;
                    } else {
                        error();
                    }
                }
                break;
            default:
                cs550_print_assert(0);
                abort();
                break;
        }
    }

    if (i - chunk_begin > 0) {
        rv = write(2, &fmt[chunk_begin], i - chunk_begin);
        cs550_print_assert((unsigned) rv == i - chunk_begin);
    }

    va_end(ap);
}
#pragma GCC diagnostic pop

void
cs550_assert_fail(const char *cond, const char *file_name, const char *func_name, int line_num) {
    cs550_print("Assertion \"%s\" failed in function %s, at line %lu in file %s.\n",
     cond, func_name, (unsigned long) line_num, file_name);
    abort();
}



void
check_mem(unsigned char *p, unsigned char v, size_t sz) {
    for (size_t i = 0; i < sz; i++) {
        if (*p != v) {
            cs550_print("Contents overwritten in block at 0x%lx at offset %lu: Should have been %lu, but was %lu.\n",
             p, i, (unsigned long) v, (unsigned long) *p);
            exit(1);
        }
    }
}

int
main(int argc, char **argv) {

    char *begin = sbrk(0);

    // Some basic tests.
    {
        void *vp1, *vp2;
        vp1 = malloc(10);
        free(vp1);
        vp1 = malloc(10);
        vp2 = malloc(30);
        free(vp1);
        free(vp2);
        vp2 = malloc(30);
        vp1 = malloc(10);
        free(vp1);
        free(vp2);
        free(malloc(100));
        free(malloc(48));
    }

    // Now a stress test.
    {
        if (argc != 5) {
            cs550_print("Wrong number of arguments.\n");
            exit(1);
        }

        // Flag as to whether or not check for overwriting blocks.
        int check = atoi(argv[1]);
        size_t n_iters = atol(argv[2]); // Number of iterations.
        size_t n_blocks = atol(argv[3]); // Number of blocks that may be outstanding.
        size_t max_sz = atol(argv[4]); // Maximum block size.

        if (check) {
            cs550_print("Running stress test with checking:\n");
        } else {
            cs550_print("Running stress test without checking:\n");
        }
        cs550_print("    %lu iterations, %lu blocks, %lu max block size\n",
         (unsigned long) n_iters, (unsigned long) n_blocks, (unsigned long) max_sz);

        // This is used to hold information about the block that was allocated,
        // so that we can test whether or not the malloc() implementation is
        // working.
        struct Block {
            // Addr of the block.  If null, it means that no block for this
            // slot is allocated.
            void *ptr;
            size_t sz; // Size of the block.
            unsigned char val; // A random value that was written into the block.
        };
        // At any one moment, there is a max of n_blocks that may be allocated,
        // so create n_blocks slots to hold info about the block.
        struct Block blocks[n_blocks];
        // Set all to null, to indicate that the slot is unused.
        for (size_t i = 0; i < n_blocks; i++) {
            blocks[i].ptr = 0;
        }
        // Now do n_iters iterations.  On each iteration, we pick a random
        // slot.  If the slot has a null pointer, that means that there is no
        // block allocated for tht slot.
        for (size_t k = 0; k < n_iters; k++) {
            struct Block *b = &blocks[rand()%n_blocks];
            // Block is null, allocate some memory.
            if (b->ptr == 0) {
                // Randomly pick using malloc() or calloc().
                if (rand()%2) {
                    b->sz = rand()%max_sz;
                    b->ptr = malloc(b->sz);
                } else {
                    size_t sz = rand()%max_sz;
                    size_t n_membs = rand()%100 + 1, memb_size = sz/n_membs;
                    b->ptr = calloc(n_membs, memb_size);
                    b->sz = n_membs*memb_size;
                }
                // Fill with a random value.
                b->val = rand()%256;
                memset(b->ptr, b->val, b->sz);
            // If the ptr is not null, then there is a block allocated.  We
            // randomly decide to either free it, or to realloc() it.
            } else {
                if (rand()%2) {
                    free(b->ptr);
                    b->ptr = 0;
                } else {
                    b->sz = rand()%max_sz + 1;
                    b->ptr = realloc(b->ptr, b->sz);
                    b->val = rand()%256;
                    memset(b->ptr, b->val, b->sz);
                }
            }

            // Check all outstanding blocks to make sure that there contents
            // have not been altered.
            if (check) {
                for (size_t i = 0; i < n_blocks; i++) {
                    struct Block *b = &blocks[i];
                    if (b->ptr != 0) {
                        check_mem(b->ptr, b->val, b->sz);
                    }
                }
            }
        }

        for (size_t i = 0; i < n_blocks; i++) {
            free(blocks[i].ptr);
        }
    }

    char *end = sbrk(0);
    cs550_print("Break usage is %lu\n", end - begin);
}

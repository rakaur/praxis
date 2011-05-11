/*  praxis: services for TSora IRC networks.
 *  src/balloc.c: Block memory allocator.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *  Copyright (c) 2002-2004 ircd-ratbox development team.
 *
 *  $Id$
 */

#include "praxis.h"
#include "dlink.h"
#include "ilog.h"
#include "imem.h"
#include "timer.h"
#include "balloc.h"

/* HP-UX apparently defines MAP_ANONYMOUS instead of MAP_ANON. */
#ifdef HAVE_MMAP
#ifdef MAP_ANONYMOUS
#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif
#endif
#endif

/* If we have mmap() but we don't have MAP_ANON we'll have to improvise. */
#ifdef HAVE_MMAP
#ifndef MAP_ANON
static int zero_fd = -1;
#endif
#endif

static void ballocGarbage(void *);

static DLinkList heap_lists;

/* _ballocFail()
 *     Logs an error message and exits.
 *
 * inputs     - message, file, line
 * outputs    - none
 */
static void
_ballocFail(const char *reason, const char *file, int line)
{
    ilog(L_INFO, "%s:%d: Block allocator failure: %s", file, line, reason);
    abort();
}

#define ballocFail(x) _ballocFail(x, __FILE__, __LINE__)

/* ballocInit()
 *     Initialises the block allocator.
 *
 * inputs     - none
 * outputs    - none
 */
void
ballocInit(void)
{
#ifdef HAVE_MMAP
#ifndef MAP_ANON
    zero_fd = open("/dev/zero", O_RDWR);

    if (zero_fd < 0)
        ballocFail("couldn't open /dev/zero");
#endif
#endif

    /* Garbage collection every five minutes. */
    timerAdd("ballocGarbage", ballocGarbage, NULL, 300);
}

/* balloc()
 *     Allocates a new block of memory.
 *
 * inputs     - size to allocate
 * outputs    - pointer to new memory or NULL on failure.
 */
void *
balloc(size_t size)
{
    void *memory;

#ifdef HAVE_MMAP
#ifdef MAP_ANON
    memory = mmap(NULL, size, (PROT_READ | PROT_WRITE),
                  (MAP_PRIVATE | MAP_ANON), -1, 0);
#else
    memory = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_PRIVATE,
                  zero_fd, 0);
#endif
    if (memory == MAP_FAILED)
        memory = NULL;
#else
    memory = malloc(size);
#endif
    return memory;
}

/* ballocFree()
 *     free()'s our memory.
 *
 * inputs     - pointer to memory, size to free
 * outputs    - none
 */
void
ballocFree(void *memory, size_t size)
{
#ifdef HAVE_MMAP
    munmap(memory, size);
#else
    free(memory);
#endif
}

/* ballocBlockCreate()
 *     Allocates a new Block for addition to a Heap.
 *
 * inputs     - Heap to add to
 * outputs    - 1 on success, 0 on failure
 */
uchar
ballocBlockCreate(Heap *heap_p)
{
    MemBlock *memblock_p;
    Block *block_p;
    ulong i;
    void *offset;

    iassert(heap_p != NULL);

    /* Set up initial data structure. */
    block_p = calloc(1, sizeof(Block));

    if (block_p == NULL)
        return 0;

    block_p->next = heap_p->block_p;
    block_p->alloc_size = ((heap_p->elems_per_block + 1) *
                           (heap_p->elem_size + sizeof(MemBlock)));

    block_p->elems = balloc(block_p->alloc_size);

    if (block_p->elems == NULL)
        return 0;

    offset = block_p->elems;

    /* Set up our MemBlocks now. */
    for (i = 0; i < heap_p->elems_per_block; i++)
    {
        void *data;

        memblock_p = offset;
        memblock_p->block_p = block_p;
#ifdef DEBUG_BALLOC
        memblock_p->magic = BALLOC_MAGIC;
#endif
        data = (void *)((size_t) offset + sizeof(MemBlock));

        dlinkAdd(data, &memblock_p->self, &block_p->free_list);

        offset = (uchar *)((uchar *)offset + heap_p->elem_size +
                           sizeof(MemBlock));
    }

    heap_p->blocks_allocated++;
    heap_p->elems_free += heap_p->elems_per_block;
    heap_p->block_p = block_p;

    return 1;
}

/* ballocHeapCreate()
 *     Creates a new Heap for Blocks.
 *
 * inputs     - element size, elements per Block
 * outputs    - pointer to Heap or NULL on failure
 */
Heap *
ballocHeapCreate(size_t elem_size, uint elems_per_block)
{
    Heap *heap_p;

    iassert(elem_size > 0);
    iassert(elems_per_block > 0);

    /* Set up our initial data structure. */
    heap_p = calloc(1, sizeof(Heap));

    if (heap_p == NULL)
        return NULL;

    /* Some systems want pointers that are multiples of void *. */
    if ((elem_size % sizeof(void *)) != 0)
    {
        elem_size += sizeof(void *);
        elem_size &= ~(sizeof(void *) - 1);
    }

    heap_p->elem_size = elem_size;
    heap_p->elems_per_block = elems_per_block;

    /* Now get some Blocks for it. */
    if (ballocBlockCreate(heap_p) == 0)
    {
        if (heap_p != NULL)
            free(heap_p);

        ballocFail("ballocBlockCreate() failed.");
    }

    if (heap_p == NULL)
        ballocFail("heap_p == NULL");

    dlinkAdd(heap_p, &heap_p->hlist, &heap_lists);

    return heap_p;
}

/* ballocHeapDestroy()
 *     Completely frees a Heap.
 *
 * inputs     - Heap to destroy
 * outputs    - 1 on success or 0 on failure
 */
uchar
ballocHeapDestroy(Heap *heap_p)
{
    Block *walker, *next;

    iassert(heap_p != NULL);

    for (walker = heap_p->block_p; walker != NULL; walker = next)
    {
        next = walker->next;

        ballocFree(walker->elems, walker->alloc_size);

        if (walker != NULL)
            free(walker);
    }

    dlinkDelete(&heap_p->hlist, &heap_lists);

    free(heap_p);

    return 1;
}

/* ballocHeapAlloc()
 *     Finds a structure that's free for the taking.
 *
 * inputs     - Heap to use for memory
 * outputs    - pointer to a structure or NULL on failure
 */
void *
ballocHeapAlloc(Heap *heap_p)
{
    Block *walker;
    DLinkNode *node_p;

    iassert(heap_p != NULL);

    /* Check to see if we have any free elements. */
    if (heap_p->elems_free == 0)
    {
        /* We're out of free elements.  Let's allocate a new Block. */
        if (ballocBlockCreate(heap_p) == 0)
        {
            /* We couldn't get a new Block.  Let's try garbage collection. */
            ballocHeapGarbage(heap_p);

            if (heap_p->elems_free == 0)
            {
                /* That didn't work either, so we're out of memory. */
                ballocFail("couldn't allocate a new Block");
            }
        }
    }

    /* Find a free element. */
    for (walker = heap_p->block_p; walker != NULL; walker = walker->next)
    {
        if (dlinkLength(&walker->free_list) > 0)
        {
            /* We have a free element. */
            heap_p->elems_free--;

            node_p = walker->free_list.head;
            dlinkNodeMove(node_p, &walker->free_list, &walker->used_list);

            iassert(node_p->data != NULL);

            memset(node_p->data, 0, heap_p->elem_size);

            return node_p->data;
        }
    }

    /* If we get here then we couldn't get a free element. */
    ballocFail("couldn't get a free element");

    return NULL;
}

/* ballocHeapFree()
 *     Returns an element to the free pool.
 *
 * inputs     - Heap containing element, element
 * outputs    - 1 on success, 0 on failure
 */
uchar
ballocHeapFree(Heap *heap_p, void *element)
{
    Block *block_p;
    MemBlock *memblock_p;

    iassert(heap_p != NULL);
    iassert(element != NULL);

    memblock_p = (void *)((size_t) element - sizeof(MemBlock));

#ifdef DEBUG_BALLOC
    if (memblock_p->magic != BALLOC_MAGIC)
        ballocFail("magics do not match");
#endif

    iassert(memblock_p->block_p != NULL);

    block_p = memblock_p->block_p;
    heap_p->elems_free++;

    dlinkNodeMove(&memblock_p->self, &block_p->used_list, &block_p->free_list);

    return 1;
}

/* ballocGarbage()
 *     Performs garbage collection on all Heaps.
 *
 * inputs     - Timer argument
 * outputs    - none
 */
static void
ballocGarbage(void *arg)
{
    DLinkNode *node_p;

    DLINK_FOREACH(node_p, heap_lists.head) ballocHeapGarbage(node_p->data);
}

/* ballocHeapGarbage()
 *     Performs garbage collection on specified Heap.
 *
 * inputs     - Heap to clean
 * outputs    - none
 */
void
ballocHeapGarbage(Heap *heap_p)
{
    Block *walker, *last;

    iassert(heap_p != NULL);

    /* Check for entirely free Blocks. */
    if ((heap_p->elems_free < heap_p->elems_per_block) ||
        (heap_p->blocks_allocated == 1))
        return;

    last = NULL;
    walker = heap_p->block_p;

    while (walker != NULL)
    {
        if (dlinkLength(&walker->free_list) == heap_p->elems_per_block)
        {
            ballocFree(walker->elems, walker->alloc_size);

            if (last != NULL)
            {
                last->next = walker->next;

                if (walker != NULL)
                    free(walker);

                walker = last->next;
            }
            else
            {
                heap_p->block_p = walker->next;

                if (walker != NULL)
                    free(walker);

                walker = heap_p->block_p;
            }

            heap_p->blocks_allocated--;
            heap_p->elems_free -= heap_p->elems_per_block;
        }
        else
        {
            last = walker;
            walker = walker->next;
        }
    }
}

/*  praxis: services for TSora IRC networks.
 *  include/balloc.h: Contains forward declarations for balloc.c.
 *
 *  Copyright (c) 2004 Eric Will <rakaur@malkier.net>
 *  Copyright (c) 2003-2004 shrike development team.
 *
 *  $Id$
 */

#ifndef INCLUDED_balloc_h
#define INCLUDED_balloc_h

#undef DEBUG_BALLOC

#ifdef DEBUG_BALLOC
#define BALLOC_MAGIC 0x3d3a3c3d
#endif

typedef struct Block Block;

struct Block
{
    size_t alloc_size;
    Block *next;
    void *elems;
    DLinkList free_list;
    DLinkList used_list;
};

typedef struct MemBlock MemBlock;

struct MemBlock
{
#ifdef DEBUG_BALLOC
    ulong magic;
#endif
    DLinkNode self;
    Block *block_p;
};

typedef struct Heap Heap;

struct Heap
{
    DLinkNode hlist;
    size_t elem_size;
    ulong elems_free;
    ulong elems_per_block;
    ulong blocks_allocated;
    Block *block_p;
};

void ballocInit(void);
void *balloc(size_t);
void ballocFree(void *, size_t);
uchar ballocBlockCreate(Heap *);
Heap *ballocHeapCreate(size_t, uint);
uchar ballocHeapDestroy(Heap *);
void *ballocHeapAlloc(Heap *);
uchar ballocHeapFree(Heap *, void *);
void ballocHeapGarbage(Heap *);

#endif /* INCLUDED_balloc_h */

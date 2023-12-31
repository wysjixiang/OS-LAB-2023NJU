#ifndef __PMM_H__
#define __PMM_H__

extern Area heap;
#define MEM_TABLE_SIZE 24

#define __BLOCK_START ((uint64_t)heap.start + MEM_TABLE_SIZE*(16*1024+16*1024 + 1*1024 + 4*1024 + 4*1024 + 128 + 2 + 4 + 2))

#define ALIGN(addr,num) ((((uint64_t)(addr)) % ((uint64_t)(num)) == 0) ? ((uint64_t)(addr)) : (((((uint64_t)(addr)) / (uint64_t)(num)) + 1) * ((uint64_t)(num))))

// ---------------
// after below macro process, we can get:
// HEAD_128_BLOCK  & TAIL_128_BLOCK
// and the address is aligned 

// // 128 Bytes x 16k
// MACRO_MEM_BLOCK(128)
// // 256 Bytes x 16k
// MACRO_MEM_BLOCK(256)
// // 1k Bytes x 1k
// MACRO_MEM_BLOCK(1k)
// // 4k Bytes x 4k
// MACRO_MEM_BLOCK(4k)
// // 8k Bytes x 4k
// MACRO_MEM_BLOCK(8k)
// // 64k Bytes x 128
// MACRO_MEM_BLOCK(64k)
// // 1M Bytes x 2
// MACRO_MEM_BLOCK(1m)
// // 4M Bytes x 4
// MACRO_MEM_BLOCK(4m)
// // 16M Bytes x 2
// MACRO_MEM_BLOCK(16m)

// #define HEAD_128_BLOCK     (ALIGN(__BLOCK_START,128))
// #define HEAD_256_BLOCK     (ALIGN(HEAD_128_BLOCK + 16*1024*128,256))
// #define HEAD_1k_BLOCK    (ALIGN(HEAD_256_BLOCK + 16*1024*256,1024))
// #define HEAD_4k_BLOCK    (ALIGN(HEAD_1k_BLOCK + 1*1024*1*1024,4*1024))
// #define HEAD_1m_BLOCK    (ALIGN(HEAD_4k_BLOCK + 16*1024*4*1024,1*1024*1024))
// #define HEAD_4m_BLOCK    (ALIGN(HEAD_1m_BLOCK + 4*1*1024*1024,4*1024*1024))
// #define HEAD_16m_BLOCK    (ALIGN(HEAD_4m_BLOCK + 4*4*1024*1024,16*1024*1024))
// #define HEAD_END_BLOCK    (ALIGN(HEAD_16m_BLOCK + 2*16*1024*1024,1))

uintptr_t HEAD_128_BLOCK     ;
uintptr_t HEAD_256_BLOCK     ;
uintptr_t HEAD_1k_BLOCK    ;
uintptr_t HEAD_4k_BLOCK    ;
uintptr_t HEAD_8k_BLOCK    ;
uintptr_t HEAD_64k_BLOCK    ;
uintptr_t HEAD_1m_BLOCK    ;
uintptr_t HEAD_4m_BLOCK    ;
uintptr_t HEAD_16m_BLOCK    ;
uintptr_t HEAD_END_BLOCK    ;


// handle the header structure
#define HEAD_128_MEM (heap.start)
#define HEAD_256_MEM (HEAD_128_MEM + 16*1024*MEM_TABLE_SIZE)
#define HEAD_1k_MEM (HEAD_256_MEM + 16*1024*MEM_TABLE_SIZE)
#define HEAD_4k_MEM (HEAD_1k_MEM + 1*1024*MEM_TABLE_SIZE)
#define HEAD_8k_MEM (HEAD_4k_MEM + 4*1024*MEM_TABLE_SIZE)
#define HEAD_64k_MEM (HEAD_8k_MEM + 4*1024*MEM_TABLE_SIZE)
#define HEAD_1m_MEM (HEAD_64k_MEM + 128*MEM_TABLE_SIZE)
#define HEAD_4m_MEM (HEAD_1m_MEM + 2*MEM_TABLE_SIZE)
#define HEAD_16m_MEM (HEAD_4m_MEM + 4*MEM_TABLE_SIZE)


// 24 Bytes
typedef struct mempool {
    void* addr;
    int used;
    struct mempool *next;
} mempool;

// 16 Bytes
typedef struct mempool_head {
    int room;
    int used;

    mempool *next;

} mempool_head;

#define MEMPOOL_HEAD(num,rom) \
    mempool_head mempool_head_##num = { \
        .room = rom, \
        .used = 0, \
        .next = NULL,  \
    }

// we can get mempool_head_128
MEMPOOL_HEAD(128,16*1024);
MEMPOOL_HEAD(256,16*1024);
MEMPOOL_HEAD(1k,1*1024);
MEMPOOL_HEAD(4k,4*1024);
MEMPOOL_HEAD(8k,4*1024);
MEMPOOL_HEAD(64k,128);
MEMPOOL_HEAD(1m,2);
MEMPOOL_HEAD(4m,4);
MEMPOOL_HEAD(16m,2);


#endif





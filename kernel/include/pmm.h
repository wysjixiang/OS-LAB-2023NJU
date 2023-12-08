#ifndef __PMM_H__
#define __PMM_H__

extern Area heap;


#define __BLOCK_START ((uint64_t)heap.start + 16*(16*1024+16*1024 + 1*1024 + 16*1024 + 2 + 4 + 2))
//  #define __BLOCK_START (0x10c4080)

#define ALIGN(addr,num) ((((uint64_t)(addr)) % ((uint64_t)(num)) == 0) ? ((uint64_t)(addr)) : (((((uint64_t)(addr)) / (uint64_t)(num)) + 1) * ((uint64_t)(num))))

#define HEAD_128_BLOCK     (ALIGN(__BLOCK_START,128))
#define HEAD_256_BLOCK     (ALIGN(HEAD_128_BLOCK + 16*1024*128,256))
#define HEAD_1k_BLOCK    (ALIGN(HEAD_256_BLOCK + 16*1024*256,1024))
#define HEAD_4k_BLOCK    (ALIGN(HEAD_1k_BLOCK + 1*1024*1*1024,4*1024))
#define HEAD_1m_BLOCK    (ALIGN(HEAD_4k_BLOCK + 16*1024*4*1024,1*1024*1024))
#define HEAD_4m_BLOCK    (ALIGN(HEAD_1m_BLOCK + 2*1*1024*1024,4*1024*1024))
#define HEAD_16m_BLOCK    (ALIGN(HEAD_4m_BLOCK + 4*4*1024*1024,16*1024*1024))
#define HEAD_END_BLOCK    (ALIGN(HEAD_16m_BLOCK + 2*16*1024*1024,1))

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
// // 4k Bytes x 16k
// MACRO_MEM_BLOCK(4k)
// // 1M Bytes x 2
// MACRO_MEM_BLOCK(1m)
// // 4M Bytes x 4
// MACRO_MEM_BLOCK(4m)
// // 16M Bytes x 2
// MACRO_MEM_BLOCK(16m)



// handle the header structure
#define HEAD_128_MEM (heap.start)
#define HEAD_256_MEM (HEAD_128_MEM + 16*1024*16)
#define HEAD_1k_MEM (HEAD_256_MEM + 16*1024*16)
#define HEAD_4k_MEM (HEAD_1k_MEM + 1*1024*16)
#define HEAD_1m_MEM (HEAD_4k_MEM + 16*1024*16)
#define HEAD_4m_MEM (HEAD_1m_MEM + 2*16)
#define HEAD_16m_MEM (HEAD_4m_MEM + 4*16)


// 16 Bytes
typedef struct mempool {
    void* addr;
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
MEMPOOL_HEAD(4k,16*1024);
MEMPOOL_HEAD(1m,2);
MEMPOOL_HEAD(4m,4);
MEMPOOL_HEAD(16m,2);


#endif





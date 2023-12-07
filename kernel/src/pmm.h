#ifndef __PMM_H__
#define __PMM_H__

extern Area heap;


#define __BLOCK_START (heap.start + 16*(16*1024+16*1024 + 1*1024 + 16*1024 + 2 + 4 + 2))

#define ALIGN(addr,num) ((uint64_t)addr % num == 0 ? addr : (((uint64_t)addr / num + 1) * num ))

// #define MACRO_HEAD_BLOCK(num, addr)
//     (#de##fine HEAD_##num##_BLOCK (addr))

// #define MACRO_TAIL_BLOCK(num,addr) 
//     (#define TAIL_##num##_BLOCK (addr))

// #define MACRO_HEAP(num,start,bytes) 
//     (#define _##num##_heap_start (start)) 
//     (#define _##num##_heap_end (start + bytes))

#define HEAD_128_BLOCK     (ALIGN(__BLOCK_START,128))
#define HEAD_256_BLOCK     (ALIGN(HEAD_128_BLOCK+16*1024*128,256))
#define HEAD_1k_BLOCK    (ALIGN(HEAD_256_BLOCK + 16*1024*256,1024))
#define HEAD_4k_BLOCK    (ALIGN(HEAD_1k_BLOCK + 1*1024*1*1024,4*1024))
#define HEAD_1m_BLOCK    (ALIGN(HEAD_4k_BLOCK + 16*1024*1*1024,1*1024*1024))
#define HEAD_4m_BLOCK    (ALIGN(HEAD_1m_BLOCK + 2*1024*1024,4*1024*1024))
#define HEAD_16k_BLOCK    (ALIGN(HEAD_4m_BLOCK + 4*1024*1024,16*1024*1024))

// // macro for mempool table
// MACRO_HEAP(128, ALIGN(__BLOCK_START,128), 16*1024*128)
// MACRO_HEAP(256, ALIGN(_128_heap_end,256), 16*1024*256)
// MACRO_HEAP(1k, ALIGN(_256_heap_end,1024), 1*1024*1*1024)
// MACRO_HEAP(4k, ALIGN(_1k_heap_end,(4*1024)), 16*1024*4*1024)
// MACRO_HEAP(1m, ALIGN(_4k_heap_end,(1024*1024)), 2*1*1024*1024)
// MACRO_HEAP(4m, ALIGN(_1m_heap_end,(4*1024*1024)), 4*4*1024*1024)
// MACRO_HEAP(16m, ALIGN(_4m_heap_end,(16*1024*1024)), 2*16*1024*1024)

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
#define HEAD_4m_MEM (HEAD_1k_MEM + 2*16)
#define HEAD_16m_MEM (HEAD_4k_MEM + 4*16)


// 16 Bytes
typedef struct mempool {
    void* addr;
    mempool *next;
} mempool;

// 16 Bytes
typedef struct mempool_head {
    int room;
    int used;

    mempool *next;

} mempool_head;

#define MEMPOOL_HEAD(num,room) \
    mempool_head mempool_head_##num { \
        .room = room, \
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





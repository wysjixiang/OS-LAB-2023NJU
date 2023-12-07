#include <common.h>
#include "pmm.h"

static void *kalloc(size_t size);
static void kfree(void *ptr);

// mempool init
static int mempool_init(void);
// mempool test
static void mempool_assert_test(void);
static void mempool_test(void);



static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

  if(mempool_init())
    printf("Mem init error\n");

  printf("128_block = %p\n", HEAD_128_BLOCK);
  printf("256_block = %p\n", HEAD_256_BLOCK);
  printf("1k_block = %p\n", HEAD_1k_BLOCK);
  printf("4k_block = %p\n", HEAD_4k_BLOCK);
  printf("1m_block = %p\n", HEAD_1m_BLOCK);
  printf("4m_block = %p\n", HEAD_4m_BLOCK);
  printf("16m_block = %p\n", HEAD_16m_BLOCK);
  printf("end_block = %p\n", HEAD_END_BLOCK);

// ----------
// assertion test for mempool range alignment
  mempool_assert_test();
}

static int mempool_init(void){
  mempool *head_128_mem = (mempool*) HEAD_128_MEM;
  mempool *head_256_mem = (mempool*) HEAD_256_MEM;
  mempool *head_1k_mem = (mempool*) HEAD_1k_MEM;
  mempool *head_4k_mem = (mempool*) HEAD_4k_MEM;
  mempool *head_1m_mem = (mempool*) HEAD_1m_MEM;
  mempool *head_4m_mem = (mempool*) HEAD_4m_MEM;
  mempool *head_16m_mem = (mempool*) HEAD_16m_MEM;

  mempool *ptr = head_128_mem;
  mempool *temp;
  void *addr = (void*)HEAD_128_BLOCK;
  for(int i = 0; i < 16*1024-1; i++){
    ptr->addr = addr;
    temp = ptr + 1;
    ptr->next = temp;
    ptr = temp;
    addr += 128;
  }
  ptr->addr = addr;
  ptr->next = NULL;

  ptr = head_256_mem;
  addr = (void*)HEAD_256_BLOCK;
  for(int i = 0; i < 16*1024-1; i++){
    ptr->addr = addr;
    temp = ptr + 1;
    ptr->next = temp;
    ptr = temp;
    addr += 256;
  }
  ptr->addr = addr;
  ptr->next = NULL;

  ptr = head_1k_mem;
  addr = (void*)HEAD_1k_BLOCK;
  for(int i = 0; i < 1*1024-1; i++){
    ptr->addr = addr;
    temp = ptr + 1;
    ptr->next = temp;
    ptr = temp;
    addr += 1024;
  }
  ptr->addr = addr;
  ptr->next = NULL;

  ptr = head_4k_mem;
  addr = (void*)HEAD_4k_BLOCK;
  for(int i = 0; i < 16*1024-1; i++){
    ptr->addr = addr;
    temp = ptr + 1;
    ptr->next = temp;
    ptr = temp;
    addr += 4*1024;
  }
  ptr->addr = addr;
  ptr->next = NULL;



  ptr = head_1m_mem;
  addr = (void*)HEAD_1m_BLOCK;
  for(int i = 0; i < 2-1; i++){
    ptr->addr = addr;
    temp = ptr + 1;
    ptr->next = temp;
    ptr = temp;
    addr += 1*1024*1024;
  }
  ptr->addr = addr;
  ptr->next = NULL;

  ptr = head_4m_mem;
  addr = (void*)HEAD_4m_BLOCK;
  for(int i = 0; i < 4-1; i++){
    ptr->addr = addr;
    temp = ptr + 1;
    ptr->next = temp;
    ptr = temp;
    addr += 4*1024*1024;
  }
  ptr->addr = addr;
  ptr->next = NULL;

  ptr = head_16m_mem;
  addr = (void*)HEAD_16m_BLOCK;
  for(int i = 0; i < 2-1; i++){
    ptr->addr = addr;
    temp = ptr + 1;
    ptr->next = temp;
    ptr = temp;
    addr += 16*1024*1024;
  }
  ptr->addr = addr;
  ptr->next = NULL;


  mempool_head_128.next = head_128_mem; 
  mempool_head_256.next = head_256_mem; 
  mempool_head_1k.next = head_1k_mem ;
  mempool_head_4k.next = head_4k_mem ;
  mempool_head_1m.next = head_1m_mem ;
  mempool_head_4m.next = head_4m_mem ;
  mempool_head_16m.next = head_16m_mem; 


  return 0;

}

#define MEMPOOL_KALLOC(num,temp,addr) do{ \
  temp = mempool_head_##num.next; \
  if(temp != NULL) { \
    mempool_head_##num.next = temp->next; \
    mempool_head_##num.used += 1; \
    addr = temp->addr; \
    return addr; \
  } else{ \
    printf("OOM!\n"); \
    return NULL; \
  } \
} while(0)

static void *kalloc(size_t size) {

  mempool *temp;
  static void *addr = NULL;
  if(size <= 128){
    MEMPOOL_KALLOC(128,temp,addr);
  } else if(size <= 256){
    MEMPOOL_KALLOC(256,temp,addr);
  } else if(size <= 1024){
    MEMPOOL_KALLOC(1k,temp,addr);

  } else if(size <= 4*1024){
    MEMPOOL_KALLOC(4k,temp,addr);

  } else if(size <= 1*1024*1024){
    MEMPOOL_KALLOC(1m,temp,addr);

  } else if(size <= 4*1024*1024){
    MEMPOOL_KALLOC(4m,temp,addr);

  } else if(size <= 16*1024*1024){
    MEMPOOL_KALLOC(16m,temp,addr);

  } else{
    printf("Kalloc mem size over maximun 16M, cannot meet the demand\n");
    return NULL;
  }

  return NULL;
}

static void kfree(void *ptr) {
}

#define MEM_KALLOC_TEST(num,addr,cnt,base,test_cnt) do{ \
  cnt = 0; \
  addr = heap.start; \
  while((addr = kalloc(num)) != NULL){ \
    printf("Kalloc addr= %p\n",addr); \
    assert(addr == (void*)(cnt*(num) + HEAD_##base##_BLOCK)); \
    cnt ++; \
  } \
  assert(cnt == test_cnt); \
} while(0)


static void mempool_test(void){
  void *addr;
  uint32_t cnt;
  MEM_KALLOC_TEST(128,addr,cnt,128,16*1024);
  MEM_KALLOC_TEST(256,addr,cnt,256,16*1024);
  MEM_KALLOC_TEST(1024,addr,cnt,1k,1*1024);
  MEM_KALLOC_TEST(4*1024,addr,cnt,4k,16*1024);
  MEM_KALLOC_TEST(1*1024*1024,addr,cnt,1m,2);
  MEM_KALLOC_TEST(4*1024*1024,addr,cnt,4m,4);
  MEM_KALLOC_TEST(16*1024*1024,addr,cnt,16m,2);

}


static void mempool_assert_test(void){
  assert(HEAD_128_BLOCK % 128 == 0);
  assert(HEAD_256_BLOCK % 256 == 0);
  assert(HEAD_1k_BLOCK % 1024 == 0);
  assert(HEAD_4k_BLOCK % (4*1024) == 0);
  assert(HEAD_1m_BLOCK % (1*1024*1024) == 0);
  assert(HEAD_4m_BLOCK % (4*1024*1024) == 0);
  assert(HEAD_16m_BLOCK % (16*1024*1024) == 0);
  assert(HEAD_256_BLOCK - __BLOCK_START    >= 16*1024*128); 
  assert(HEAD_1k_BLOCK - HEAD_256_BLOCK    >= 16*1024*256  ); 
  assert(HEAD_4k_BLOCK  -  HEAD_1k_BLOCK   >= 1*1024*1*1024  );
  assert(HEAD_1m_BLOCK  - HEAD_4k_BLOCK    >= 16*1024*4*1024  );
  assert(HEAD_4m_BLOCK  - HEAD_1m_BLOCK    >= 2*1024*1024  );
  assert(HEAD_16m_BLOCK - HEAD_4m_BLOCK    >= 4*4*1024*1024  ); 


  mempool_test();

}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};

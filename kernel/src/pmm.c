#include <common.h>
#include "pmm.h"

spinlock_t alloc_lock = {
  .value = 0,
};
spinlock_t free_lock = {
  .value = 0,
};
static void spin_lock(spinlock_t *lk) {
  while (1) {
    intptr_t value = atomic_xchg(&lk->value, 1);
    if (value == 0) {
      break;
    }
  }
}
static void spin_unlock(spinlock_t *lk) {
  atomic_xchg(&lk->value, 0);
}

static void *kalloc(size_t size);
static void kfree(void *ptr);

// mempool init
static int mempool_init(void);
// mempool test
static void mempool_assert_test(void);
static void mempool_test(void);

#ifdef __MEM_TEST
#define MEM_TEST mempool_test()
#else
#define MEM_TEST  // define nothing

#endif

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
// test for mempool 
  mempool_test();
}

#define MEM_ASSIGN(ptr,temp,base,cnt,num) do{ \
  ptr = head_##base##_mem; \
  addr = (void*)HEAD_##base##_BLOCK; \
  for(int i =0; i < cnt -1; i++){ \
    ptr->addr = addr; \
    temp = ptr + 1; \
    ptr->next = temp; \
    ptr = temp; \
    addr += num; \
  } \
  ptr->addr = addr; \
  ptr->next = NULL; \
} while(0)


static int mempool_init(void){
  mempool *head_128_mem = (mempool*) HEAD_128_MEM;
  mempool *head_256_mem = (mempool*) HEAD_256_MEM;
  mempool *head_1k_mem = (mempool*) HEAD_1k_MEM;
  mempool *head_4k_mem = (mempool*) HEAD_4k_MEM;
  mempool *head_1m_mem = (mempool*) HEAD_1m_MEM;
  mempool *head_4m_mem = (mempool*) HEAD_4m_MEM;
  mempool *head_16m_mem = (mempool*) HEAD_16m_MEM;

  mempool *ptr;
  mempool *temp;
  void *addr;
  MEM_ASSIGN(ptr,temp,128,16*1024,128);
  MEM_ASSIGN(ptr,temp,256,16*1024,256);
  MEM_ASSIGN(ptr,temp,1k,1*1024,1024);
  MEM_ASSIGN(ptr,temp,4k,16*1024,4*1024);
  MEM_ASSIGN(ptr,temp,1m,2,1*1024*1024);
  MEM_ASSIGN(ptr,temp,4m,4,4*1024*1024);
  MEM_ASSIGN(ptr,temp,16m,2,16*1024*1024);
  mempool_head_128.next = head_128_mem; 
  mempool_head_256.next = head_256_mem; 
  mempool_head_1k.next = head_1k_mem ;
  mempool_head_4k.next = head_4k_mem ;
  mempool_head_1m.next = head_1m_mem ;
  mempool_head_4m.next = head_4m_mem ;
  mempool_head_16m.next = head_16m_mem; 

  return 0;

}

#define MEMPOOL_KALLOC(num,temp,addr,mem_size) do{ \
  spin_lock(&allock_lock); \
  temp = mempool_head_##num.next; \
  if(temp != NULL) { \
    mempool_head_##num.next = temp->next; \
    mempool_head_##num.used += 1; \
    spin_unlock(&allock_lock); \
    addr = temp->addr; \
    temp->used = 1; \
    uint64_t *start = (uint64_t*)addr; \
    for(int j = 0; j < ((mem_size) >> 3); j++){ \
      *start++ = 0; \
    } \
    return addr; \
  } else{ \
    spin_unlock(&allock_lock); \
    printf("OOM!\n"); \
    return NULL; \
  } \
} while(0)

static void *kalloc(size_t size) {

  mempool *temp;
  static void *addr = NULL;
  if(size <= 128){
    MEMPOOL_KALLOC(128,temp,addr,128);
  } else if(size <= 256){
    MEMPOOL_KALLOC(256,temp,addr,256);
  } else if(size <= 1024){
    MEMPOOL_KALLOC(1k,temp,addr,1024);

  } else if(size <= 4*1024){
    MEMPOOL_KALLOC(4k,temp,addr,4*1024);

  } else if(size <= 1*1024*1024){
    MEMPOOL_KALLOC(1m,temp,addr,1*1024*1024);

  } else if(size <= 4*1024*1024){
    MEMPOOL_KALLOC(4m,temp,addr,4*1024*1024);

  } else if(size <= 16*1024*1024){
    MEMPOOL_KALLOC(16m,temp,addr,16*1024*1024);

  } else{
    printf("Kalloc mem size over maximun 16M, cannot meet the demand\n");
    return NULL;
  }

  return NULL;
}

#define MEM_FREE(ptr,mem_table,temp,dif,base,mem_size) do{ \
  dif = (uint64_t)(ptr - HEAD_##base##_BLOCK); \
  if( dif % (mem_size)){ \
    printf("The mem: %p you want free is unaligned! Align size:%d\n",ptr,(mem_size)); \
    assert(0); \
  } \
  dif = dif/(mem_size); \
  mem_table = (mempool*)(dif *(sizeof(mempool)) + HEAD_##base##_MEM); \
  if(mem_table->used != 1){ \
    printf("The mem:%p you want free is already released!\n",mem_table->addr); \
    assert(0); \
  } \
  spin_lock(&free_lock); \
  mem_table->used = 0; \
  temp = mempool_head_##base.next; \
  mempool_head_##base.next = mem_table; \
  mem_table->next = temp; \
  mempool_head_##base.used -= 1; \
  spin_unlock(&free_lock); \
  return; \
} while(0)

static void kfree(void *ptr) {
  
  uint64_t dif;
  mempool *mem_table;
  mempool *temp;

  if(ptr >= (void*)HEAD_128_BLOCK && ptr < (void*)HEAD_256_BLOCK){
    MEM_FREE(ptr,mem_table,temp,dif,128,128);

  } else if(ptr >= (void*)HEAD_256_BLOCK && ptr < (void*)HEAD_1k_BLOCK){
    MEM_FREE(ptr,mem_table,temp,dif,256,256);

  } else if(ptr >= (void*)HEAD_1k_BLOCK && ptr < (void*)HEAD_4k_BLOCK){
    MEM_FREE(ptr,mem_table,temp,dif,1k,1024);
    
  } else if(ptr >= (void*)HEAD_4k_BLOCK && ptr < (void*)HEAD_1m_BLOCK){
    MEM_FREE(ptr,mem_table,temp,dif,4k,4*1024);

  } else if(ptr >= (void*)HEAD_1m_BLOCK && ptr < (void*)HEAD_4m_BLOCK){
    MEM_FREE(ptr,mem_table,temp,dif,1m,1*1024*1024);

  } else if(ptr >= (void*)HEAD_4m_BLOCK && ptr < (void*)HEAD_16m_BLOCK){
    MEM_FREE(ptr,mem_table,temp,dif,4m,4*1024*1024);

  } else if(ptr >= (void*)HEAD_16m_BLOCK && ptr < (void*)HEAD_END_BLOCK){
    MEM_FREE(ptr,mem_table,temp,dif,16m,16*1024*1024);

  } else{
    printf("The ptr:%p you want free is illegal\n",ptr);
    assert(0);
  }

}

#define MEM_KALLOC_TEST(num,addr,cnt,base,test_cnt) do{ \
  cnt = 0; \
  addr = heap.start; \
  while((addr = kalloc(num)) != NULL){ \
    assert(addr == (void*)(cnt*(num) + HEAD_##base##_BLOCK)); \
    cnt ++; \
  } \
  assert(cnt == test_cnt); \
} while(0)


static void mempool_test(void){
  void *addr;
  uint32_t cnt;

  mempool_assert_test();

  MEM_KALLOC_TEST(128,addr,cnt,128,16*1024);
  MEM_KALLOC_TEST(256,addr,cnt,256,16*1024);
  MEM_KALLOC_TEST(1024,addr,cnt,1k,1*1024);
  MEM_KALLOC_TEST(4*1024,addr,cnt,4k,16*1024);
  MEM_KALLOC_TEST(1*1024*1024,addr,cnt,1m,2);
  MEM_KALLOC_TEST(4*1024*1024,addr,cnt,4m,4);
  MEM_KALLOC_TEST(16*1024*1024,addr,cnt,16m,2);


  void *ptr = (void*)HEAD_128_BLOCK;
  for(int i = 0;i <  16*1024;i++){
    kfree(ptr);
    ptr += 128;
  }
  ptr = (void*)(HEAD_128_BLOCK+1);
  kfree(ptr);

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

}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};

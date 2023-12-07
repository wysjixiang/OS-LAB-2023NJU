#include <common.h>
#include "pmm.h"

static void *kalloc(size_t size);
static void kfree(void *ptr);

// mempool init
static int mempool_init(void);



static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

  if(!mempool_init())
    printf("Mem init error\n");

  
  printf("heap_start = %p, heap_end = %p\n", heap.start,heap.end);
  printf("heap_128_start = %p\n",mempool_head_128.next);
  printf("heap_16m_start = %p\n",(void*)mempool_head_16m.next);

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
  void *addr = (void*)HEAD_128_BLOCK;
  for(int i = 1; i < 16*1024; i++){
    ptr->addr = addr;
    ptr->next = ++ptr;
    addr += 128;
  }
  ptr->next = NULL;

  ptr = head_256_mem;
  addr = (void*)HEAD_256_BLOCK;
  for(int i = 1; i < 16*1024; i++){
    ptr->addr = addr;
    ptr->next = ++ptr;
    addr += 256;
  }
  ptr->next = NULL;

  ptr = head_1k_mem;
  addr = (void*)HEAD_1k_BLOCK;
  for(int i = 0; i < 1*1024; i++){
    ptr->addr = addr;
    ptr->next = ++ptr;
    addr += 1024;
  }
  ptr->next = NULL;

  ptr = head_4k_mem;
  addr = (void*)HEAD_4k_BLOCK;
  for(int i = 1; i < 16*1024; i++){
    ptr->addr = addr;
    ptr->next = ++ptr;
    addr += 4*1024;
  }
  ptr->next = NULL;

  ptr = head_1m_mem;
  addr = (void*)HEAD_1m_BLOCK;
  for(int i = 1; i < 2; i++){
    ptr->addr = addr;
    ptr->next = ++ptr;
    addr += 1*1024*1024;
  }
  ptr->next = NULL;

  ptr = head_4m_mem;
  addr = (void*)HEAD_4m_BLOCK;
  for(int i = 1; i < 4; i++){
    ptr->addr = addr;
    ptr->next = ++ptr;
    addr += 4*1024*1024;
  }
  ptr->next = NULL;

  ptr = head_16m_mem;
  addr = (void*)HEAD_16m_BLOCK;
  for(int i = 1; i < 2; i++){
    ptr->addr = addr;
    ptr->next = ++ptr;
    addr += 4*1024;
  }
  ptr->next = NULL;

  mempool_head_128.next = head_128_mem; 
  mempool_head_256.next = head_256_mem; 
  mempool_head_1k.next = head_1k_mem ;
  mempool_head_4k.next = head_4k_mem ;
  mempool_head_1m.next = head_1m_mem ;
  mempool_head_4m.next = head_4m_mem ;
  mempool_head_16m.next = head_16m_mem; 

}



static void *kalloc(size_t size) {
  return NULL;
}

static void kfree(void *ptr) {
}


MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};

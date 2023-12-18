#ifndef __OS_H__
#define __OS_H__

#include <common.h>

#define STACK_SIZE 8192

typedef enum {
  TASK_NULL = 0,
  TASK_READY,
  TASK_RUNNING,
  TASK_DEAD,
  TASK_SLEEPING,
} task_status;

struct task {
  // TODO
  int id;
  task_status status;
  const char *name;
  struct task *next;
  Context *context;
  int magic_number;
  void *stack;
};

struct spinlock {
  // TODO
  int lock;
  int cpu;
  const char *name;
};

struct semaphore {
  // TODO
  struct spinlock lock;
  int count;
  const char *name;
  task_t *wl; // waiting list
};

struct handler_table {
  int seq;
  int event;
  handler_t handler;
  struct handler_table *next;
};

typedef struct handler_table handler_table_t;

struct task_header {
    task_t *ready_task_head;
    task_t *ready_task_tail;
    task_t *sleeping_task_head;
    task_t *sleeping_task_tail;
    task_t *dead_task_head;
    task_t *dead_task_tail;
};

typedef struct task_header task_header_t;

#endif
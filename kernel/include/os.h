#include <common.h>

struct task {
  // TODO
};

struct spinlock {
  // TODO
  int value;
};

struct semaphore {
  // TODO
};

struct handler_table {
  int seq;
  Event event;
  Context *handler(Event ev, Context *context);
  struct handler_table *next;
};

typedef struct handler_table handler_table_t;
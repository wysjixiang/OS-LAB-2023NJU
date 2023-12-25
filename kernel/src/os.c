#include <common.h>
#include <unistd.h>
#include <devices.h>

static handler_table_t handler_head = {
  .seq = 0,
  .event = 0,
  .handler = NULL,
  .next = NULL,
};


static void tty_reader(void* arg) {
  device_t* tty = dev->lookup(arg);
  char cmd[128], resp[128], ps[16];
  snprintf(ps, 16, "(%s) $ ", arg);
  while (1) {
    tty->ops->write(tty, 0, ps, strlen(ps));
    int nread = tty->ops->read(tty, 0, cmd, sizeof(cmd) - 1);
    cmd[nread] = '\0';
    sprintf(resp, "tty reader task: got %d character(s).\n", strlen(cmd));
    tty->ops->write(tty, 0, resp, strlen(resp));
  }
}


static void os_init() {
  pmm->init();
  kmt->init();
  dev->init();
  // kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty1",-1);
  // kmt->create(pmm->alloc(sizeof(task_t)), "tty_reader", tty_reader, "tty2",-1);
}


sem_t nest;
spinlock_t put_lock;

static void os_run() {

  int cpu = cpu_current();
  printf("Hello World! from CPU#%d\n",cpu);

  iset(true);

  yield();
  assert(0);

}

static Context* os_trap(Event ev, Context *context) {
  Context *next_context = NULL;
  for(handler_table_t *table = handler_head.next; table != NULL; table = table->next) {
    if(table->event == EVENT_NULL || table->event == ev.event){
      Context *r = table->handler(ev,context);
      panic_on(r && next_context, "returing multiple contexts");
      if(r) next_context = r;
    }
  }

  panic_on(!next_context, "returning NULL context!");
//  panic_on(sane_context(next_context), "returning to invalid context!");
  return next_context;
}

// for irq registers
static void os_on_irq(int seq, int ev, handler_t handler ) {
  handler_table_t *last = &handler_head;
  handler_table_t *next = handler_head.next;

  while(next != NULL && next->seq <= seq){
    last = next;
    next = next->next;
  }
  handler_table_t *new_one = (handler_table_t*)pmm->alloc(sizeof(handler_table_t));
  last->next = new_one;
  new_one->event = ev;
  new_one->seq = seq;
  new_one->handler = handler;
  new_one->next = next;

}


MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = os_on_irq,
};

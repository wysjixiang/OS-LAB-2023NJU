#include <common.h>


handler_table_t handler_head = {
  .seq = 0;
  .event = 0,
  .handler = NULL,
  .next = NULL,
};


static void os_init() {
  pmm->init();
}

static void os_run() {
  iset(true);
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    putch(*s == '*' ? '0' + cpu_current() : *s);
  }
  while (1) ;
}

static Context* os_trap(Event ev, Context *context) {
  Context *next_context = NULL;
  for(handler_table_t *table = handler_head.next; table != NULL; table = table->next) {
    if(table->event == EVENT_NULL || table->event == ev){
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
static void os_on_irq(int seq, Event ev, Context *context, Context *handler(Event ev, Context *context)) {
  handler_table_t *last = &handler_head;
  handler_table_t *next = handler_head.next;

  while(next != NULL && next->seq <= seq){
    last = next;
    next = next->next;
  }
  handler_table_t *new_one = (handler_table_t*)pmm->malloc(sizeof(handler_table_t));
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

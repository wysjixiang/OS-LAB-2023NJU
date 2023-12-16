#include <os.h>

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){

}
static void kmt_teardown(task_t *task){

}
static void kmt_spin_init(spinlock_t *lk, const char *name){

}
static void kmt_spin_lock(spinlock_t *lk){

}
static void kmt_spin_unlock(spinlock_t *lk){

}
static void kmt_sem_init(sem_t *sem, const char *name, int value){

}
static void kmt_sem_wait(sem_t *sem){

}
static void kmt_sem_signal(sem_t *sem){
    
}


Context *kmt_context_save(Event ev, Context *context){
    // will do regardless of event number


}

Context* kmt_schedule(Event ev, Context* context){
    // check if ev is not for schedule, so no need to schedule
    if(ev != EVENT_)
}

static void kmt_init(){
    os->on_irq(INT_MIN, EVENT_NULL,kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL,kmt_schedule);
}




MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .spin_init = ,
    .sem_init = ,
};

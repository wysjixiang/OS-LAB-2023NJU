#include <os.h>
#include <limits.h>

#define MAX_CPU 8

task_header_t *task_head_table;


spinlock_t spinlock_entity;
sem_t sem_kmt_entity;

spinlock_t *spinlock_kmt = &spinlock_entity;
sem_t *sem_kmt = &sem_kmt_entity;

task_t* current_task[MAX_CPU];
static int irq_nest[MAX_CPU];
static int irq_istatus[MAX_CPU];

// function declarations
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
static void kmt_teardown(task_t *task);
static void kmt_spin_init(spinlock_t *lk, const char *name);
static void kmt_spin_lock(spinlock_t *lk);
static void kmt_spin_unlock(spinlock_t *lk);
static void kmt_sem_init(sem_t *sem, const char *name, int value);
static void kmt_sem_wait(sem_t *sem);
static void kmt_sem_signal(sem_t *sem);
Context *kmt_context_save(Event ev, Context *context);
Context* kmt_schedule(Event ev, Context* context);
static void kmt_init();

//
static int task_id = 1;

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){

    DEBUG_PRINTF(" kmt create!");


    task->status = TASK_READY; 
    task->name = name;
    task->next = NULL; // tail insert
    task->stack = (void *)pmm->alloc(STACK_SIZE);
    
    DEBUG_PRINTF(" no room?");


    // context
    Area kstack = (Area){ task->stack, task->stack + STACK_SIZE };
    task->context = kcontext(kstack, entry, arg);

    task->magic_number = 0x12345678;
    task_header_t *p = task_head_table;
    kmt_spin_lock(spinlock_kmt); // get lock
    task->id = task_id++;

    if(p->ready_task_tail == NULL){
        p->ready_task_head = task;
        p->ready_task_tail = task;
    } else{
        p->ready_task_tail->next = task;
    }
    kmt_spin_unlock(spinlock_kmt); // release
    
    return 0;
}

static void kmt_teardown(task_t *task){
    panic("Teardown not implemented");
}


// lock

static void pushcli(){
    int istatus = ienabled();
    iset(false);
    int cpu = cpu_current();
    if(irq_nest[cpu] == 0){
        irq_istatus[cpu] = istatus;
    }
    irq_nest[cpu]++;
}

static void popcli(){
    int cpu = cpu_current();
    assert(ienabled() == 0);
    assert(irq_nest[cpu]-- > 0);
    if(irq_nest[cpu] == 0 && irq_istatus[cpu]){
        iset(true);
    }
}


static void kmt_spin_init(spinlock_t *lk, const char *name){
    lk->lock = 0;
    lk->cpu = -1;
    lk->name = name;
}

static void kmt_spin_lock(spinlock_t *lk){
    while (1) {
        intptr_t value = atomic_xchg(&lk->lock, 1);
        if (value == 0) {
            break;
        }
    }
    pushcli();
    lk->cpu = cpu_current();
}

static void kmt_spin_unlock(spinlock_t *lk){
    atomic_xchg(&lk->lock, 0);
    popcli();
}

static void kmt_sem_init(sem_t *sem, const char *name, int value){
    assert(value == 0 || value == 1);
    kmt_spin_init(&sem->lock,name);
    sem->count = value;
    sem->name = name;
}

static void kmt_sem_wait(sem_t *sem){
    kmt_spin_lock(&sem->lock);
    while(sem->count == 0) {
        // need to sleep
        // TODO: how to get this threads id??
        // task->status = TASK_SLEEPING; then add to sleeping list;

        // int cpu = cpu_current();
        // assert(current_task[cpu] != NULL);
        // current_task[cpu]->status = TASK_SLEEPING;
        // kmt_spin_unlock(&sem->lock);
        // yield();
        // kmt_spin_lock(&sem->lock);


        // test version
        kmt_spin_unlock(&sem->lock);
        yield();
        kmt_spin_lock(&sem->lock);

    }
    sem->count--;
    kmt_spin_unlock(&sem->lock);
    
    // test 
    DEBUG_PRINTF("no need to yield right?");

}

static void kmt_sem_signal(sem_t *sem){

    // check if current task is available!!
    if(current_task[cpu_current()] == NULL){

        kmt_spin_lock(&sem->lock);
        assert(sem->count >= 0);
        sem->count++;
        kmt_spin_unlock(&sem->lock);
        return;
    }


    task_header_t *p = task_head_table;
    // first use big locker for task_head data
    kmt_spin_lock(spinlock_kmt);

    if(p->sleeping_task_head != NULL){
        // need to wake up
        task_t *task = p->sleeping_task_head;
        task_t *next = task->next;
        if(next == NULL){
            p->sleeping_task_head = NULL;
            p->sleeping_task_tail = NULL;
        } else{
            p->sleeping_task_head = next;
        }

        if(p->ready_task_tail == NULL){
            p->ready_task_head = task;
            task->next = NULL;
            p->ready_task_tail = task;
        } else{
            p->ready_task_tail->next = task;
            task->next = NULL;
            p->ready_task_tail = task;
        }

        kmt_spin_unlock(spinlock_kmt);
        yield();   
    }
    // when no waiting list, check if current[cpu]->status is sleeping

    int istatus = current_task[cpu_current()]->status;
    if(istatus == TASK_SLEEPING){
        current_task[cpu_current()]->status = TASK_READY;
        kmt_spin_unlock(spinlock_kmt);
        yield();
    } else{
        
        kmt_spin_lock(&sem->lock);
        sem->count++;
        kmt_spin_unlock(&sem->lock);
        kmt_spin_unlock(spinlock_kmt);
    }
}

Context *kmt_context_save(Event ev, Context *context){

    DEBUG_PRINTF("context_save");


    // will do regardless of event number
    kmt_spin_lock(spinlock_kmt);
    int cpu = cpu_current();
    if(current_task[cpu] != NULL){
        current_task[cpu]->context = context;
    }
    kmt_spin_unlock(spinlock_kmt);
    return NULL;

}

static void push_task(task_t *task, task_status status){

    task_t *head, *tail;
    assert(status == TASK_READY || status == TASK_SLEEPING);

    if(status == TASK_READY){
        head = task_head_table->ready_task_head;
        tail = task_head_table->ready_task_tail;
        task->status = TASK_READY;
        task->id = -1;
        if(tail != NULL){
            tail->next = task;
            task_head_table->ready_task_tail = task;
        } else{
            task_head_table->ready_task_head = task;
            task_head_table->ready_task_tail = task;
        }
    }  

    if(status == TASK_SLEEPING){
        head = task_head_table->sleeping_task_head;
        tail = task_head_table->sleeping_task_tail;
        task->status = TASK_SLEEPING;
        task->id = -1;
        if(tail != NULL){
            tail->next = task;
            tail = task;
        } else{
            head = task;
            tail = task;
        }
    }
}

static task_t* pop_task(task_status status){
    task_t *head, *tail;
    task_t *next, *ret;
    assert(status == TASK_READY || status == TASK_SLEEPING);

    if(status == TASK_READY){
        head = task_head_table->ready_task_head;
        tail = task_head_table->ready_task_tail;
        if(head != NULL){
            ret = head;
            next = head->next;
            ret->next = NULL;
            task_head_table->ready_task_head = next;
            if(next == NULL){
                task_head_table->ready_task_tail = NULL;
            }
            return ret;
        } else{
            return NULL;
        }
    }  

    if(status == TASK_SLEEPING){
        head = task_head_table->sleeping_task_head;
        tail = task_head_table->sleeping_task_tail;
        if(head != NULL){
            ret = head;
            next = head->next;
            ret->next = NULL;
            task_head_table->sleeping_task_head = next;
            if(next == NULL){
                task_head_table->sleeping_task_tail = NULL;
            }
            return ret;
        } else{
            return NULL;
        }
    }

    return NULL;
}


Context* kmt_schedule(Event ev, Context* context){
    // check if ev is not for schedule, so no need to schedule
    int cpu = cpu_current();
    if(ev.event == EVENT_YIELD || ev.event == EVENT_IRQ_TIMER){
        // schedule
        kmt_spin_lock(spinlock_kmt);
        assert(current_task[cpu]->status != TASK_DEAD);

        // first push task back to list
        push_task(current_task[cpu], current_task[cpu]->status);
        // then pop out a new task from ready list
        task_t *next_task = pop_task(TASK_READY);
        next_task->status = TASK_RUNNING;
        next_task->id = cpu;
        assert(next_task->magic_number == 0x12345678);
        current_task[cpu] = next_task;
        kmt_spin_unlock(spinlock_kmt);
    }

    DEBUG_PRINTF("kmt_schedule");

    return current_task[cpu]->context;
}

static void kmt_init(){

    for(int i=0;i<MAX_CPU;i++){
        current_task[i] = NULL;
    }

    for(int i=0;i<MAX_CPU;i++){
        
    }

    // register task head table
    task_head_table = (task_header_t*)pmm->alloc(sizeof(task_header_t));
    task_head_table->ready_task_head = NULL;
    task_head_table->sleeping_task_head = NULL;
    task_head_table->dead_task_head = NULL;
    task_head_table->ready_task_tail = NULL;
    task_head_table->sleeping_task_tail = NULL;
    task_head_table->dead_task_tail = NULL;
    // spinlock init
    kmt_spin_init(spinlock_kmt, "spinlock_for_kmt");
    // sem_init
    kmt_sem_init(sem_kmt, "semaphore_for_kmt",0);

    // register irq task for context switch and task schedule
    os->on_irq(INT_MIN, EVENT_NULL,kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL,kmt_schedule);
}




MODULE_DEF(kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = kmt_spin_init,
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};

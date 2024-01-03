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
static task_t* buf_task[MAX_CPU];

// function declarations
static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg, int bind_cpu);
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

static task_t* pop_task(task_status status);
static void push_task(task_t *task, task_status status);
//

static void dead_loop(){
    while(1){
        ;
    }
}



static int task_id = 1;

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg, int bind_cpu){
    task->bind_cpu = bind_cpu;
    task->status = TASK_READY; 
    task->name = name;
    task->next = NULL; // tail insert
    task->stack = pmm->alloc(STACK_SIZE);

    // context
    Area kstack = (Area){ task->stack, task->stack + STACK_SIZE };
    task->context = kcontext(kstack, entry, arg);

    task->magic_number = 0x12345678;
    kmt_spin_lock(spinlock_kmt); // get lock
    task->id = task_id++;
    push_task(task,task->status);
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
    pushcli();
    #define MAX_LOCK_FETCH_TIME 30000
    int cnt = 0;
    while (1) {
        intptr_t value = atomic_xchg(&lk->lock, 1);
        if (value == 0) {
            break;
        }
        if(cnt++ > MAX_LOCK_FETCH_TIME){
            // test
            DEBUG_PRINTF("trying fetch lock:%s",lk->name);
            assert(0);
            cnt = 0;
        }
    }
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
    sem->wl = NULL;
}

static void kmt_sem_wait(sem_t *sem){
    kmt_spin_lock(&sem->lock);
    if(sem->count == 0) {
        // need to sleep

        // test version
        assert(current_task[cpu_current()] != NULL);
        // if is not a thread, illegal operation
        task_t *p = sem->wl;
        int cpu = cpu_current();
        task_t *task = current_task[cpu];
        while(p && p->next) p = p->next;
        if(p == NULL){
            sem->wl = task;
        } else{
            p->next = task;
        }
        task->next = NULL;
        task->status = TASK_SLEEPING;

        kmt_spin_unlock(&sem->lock);
        yield();
        return ;
    }
    sem->count--;
    kmt_spin_unlock(&sem->lock);

}

static void kmt_sem_signal(sem_t *sem){
    kmt_spin_lock(&sem->lock);
    task_t *p = sem->wl;

    // check if current task is available!!
    if(current_task[cpu_current()] == NULL || p == NULL){

        assert(sem->count >= 0);
        sem->count++;
        kmt_spin_unlock(&sem->lock);
        return;
    }

    if(p != NULL){
        // need to wake up

        // first check if current task->status == sleeping
        int cpu = cpu_current();
        kmt_spin_lock(spinlock_kmt);

        if(current_task[cpu]->status == TASK_SLEEPING){
            task_t *ref_task = sem->wl;
            task_t *last = NULL;
            while(ref_task != NULL && ref_task != current_task[cpu]){
                last = ref_task;
                ref_task = ref_task->next;
            }
            if(ref_task == current_task[cpu]){
                if(last == NULL){
                    sem->wl = ref_task->next;
                } else{
                    last->next = ref_task->next;
                }

                ref_task->status = TASK_READY;
                ref_task->next = NULL;
                kmt_spin_unlock(spinlock_kmt);
                kmt_spin_unlock(&sem->lock);
                return ;
            }
        }
        
        sem->wl = p->next;
        p->status = TASK_READY;
        push_task(p,TASK_READY);
        kmt_spin_unlock(spinlock_kmt);
        kmt_spin_unlock(&sem->lock);
    
        return ;
    }
    
}

Context *kmt_context_save(Event ev, Context *context){
    kmt_spin_lock(spinlock_kmt);

    DEBUG_PRINTF("event = %d",ev.event);

    int cpu = cpu_current();
    DEBUG_PRINTF("context_save");
    if(current_task[cpu] != NULL){
        DEBUG_PRINTF("save from CPU#%d, task->name:%s", cpu, current_task[cpu]->name);
    }

    // will do regardless of event number
    if(current_task[cpu] != NULL){
        current_task[cpu]->context = context;
    }
    kmt_spin_unlock(spinlock_kmt);
    return NULL;

}

static void push_task(task_t *task, task_status status){

    if(status == TASK_SLEEPING)
        return ;

    task_t *head, *tail;
    assert(status == TASK_RUNNING || status == TASK_SLEEPING || status == TASK_READY);
    task->next = NULL;

    if(status == TASK_RUNNING || status == TASK_READY){
        head = task_head_table->ready_task_head;
        tail = task_head_table->ready_task_tail;
        task->status = TASK_READY;
        if(tail != NULL){
            tail->next = task;
            task_head_table->ready_task_tail = task;
        } else{
            task_head_table->ready_task_head = task;
            task_head_table->ready_task_tail = task;
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


    if(ev.event == EVENT_YIELD || ev.event == EVENT_IRQ_TIMER || ev.event == EVENT_IRQ_IODEV){
        // schedule
        kmt_spin_lock(spinlock_kmt);

        // first check if current task is NULL
        if(current_task[cpu] != NULL){
            assert(current_task[cpu]->status != TASK_DEAD);
            
            if(current_task[cpu]->bind_cpu != -2){
                // first push task back to list
                push_task(current_task[cpu], current_task[cpu]->status);
            }
        }

        // then pop out a new task from ready list
        task_t *next_task = pop_task(TASK_READY);

        // test
        if(next_task == NULL){
            next_task = buf_task[cpu];
        } 

        // cpu-bind task 
        //TODO:
        // while(next_task->bind_cpu != -1 && next_task->bind_cpu != cpu){
        //     // push back to ready list
        //     push_task(next_task, TASK_READY);
        //     next_task = pop_task(TASK_READY);
        // }

        next_task->status = TASK_RUNNING;
        assert(next_task->magic_number == 0x12345678);
        current_task[cpu] = next_task;
        kmt_spin_unlock(spinlock_kmt);
    }

    DEBUG_PRINTF("kmt_schedule. task->name:%s, task->context:%p",current_task[cpu]->name,current_task[cpu]->context);

    kmt_spin_lock(spinlock_kmt);
    task_t *p = task_head_table->ready_task_head;
    while(p){
        DEBUG_PRINTF("ready_list:%s",p->name);
        p = p->next;
    }
    kmt_spin_unlock(spinlock_kmt);

    return current_task[cpu]->context;
}

static void thread_test_20(void *arg){
  while(1){
    int cpu = cpu_current();
    printf("cpu#%d calling!\n",cpu);
    // usleep(500000);

    // *****************
    // why usleep() function will let system timer disable?
  }
}

static void thread_test1(void *arg){
  while(1){
    int cpu = cpu_current();
    printf("cpu#%d threading!\n",cpu);
  }
}

static void kmt_init(){

    for(int i=0;i<MAX_CPU;i++){
        current_task[i] = NULL;
    }

    for(int i=0; i < MAX_CPU;i++){
        buf_task[i] = (task_t*)pmm->alloc(sizeof(task_t));
        buf_task[i]->bind_cpu = -2;
        buf_task[i]->status = TASK_READY; 
        buf_task[i]->name = "dead_loop per cpu";
        buf_task[i]->next = NULL; // tail insert
        buf_task[i]->stack = pmm->alloc(STACK_SIZE);

        // context
        Area kstack = (Area){ buf_task[i]->stack, buf_task[i]->stack + STACK_SIZE };
        buf_task[i]->context = kcontext(kstack, dead_loop, NULL);
        buf_task[i]->magic_number = 0x12345678;
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

    // task_t *p = task_head_table->ready_task_head;
    // int cnt = 0;
    // p = pop_task(TASK_READY);
    // printf("ready_list:\n");
    // while(p){
    //     printf("No.%d:%s\n",cnt++,p->name);
    //     p = pop_task(TASK_READY);
    // }
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

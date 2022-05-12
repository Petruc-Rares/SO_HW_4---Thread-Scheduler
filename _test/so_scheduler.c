#include "so_scheduler.h"

typedef struct scheduler scheduler;
typedef struct thread thread;

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4

#define NO_WAIT_EVENT 0

struct scheduler {
    unsigned int time_quantum;
    unsigned int io;
    // 0 - not initialised, 1 - otherwise
    unsigned char initialised;
    thread **threads;
    unsigned int no_threads;
    thread *running_thread;
};

struct thread {
    // not sure it will be used
    so_handler *func;
    //priority of the thread
    int priority;
    // NEW, READY, RUNNING, WAITING, TERMINATED
    unsigned int state;
    // how much time a thread has left
    // for the current quantum
    unsigned int time_quantum_left;
    //sem_t is_terminated;
    //sem_t is_running;
    HANDLE thread_id;
    unsigned int no_times_on_processor;
    unsigned int wait_event;
};

scheduler my_scheduler;

DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io)
{
    if ((time_quantum == 0) ||
        (io > SO_MAX_NUM_EVENTS) ||
        (my_scheduler.initialised != 0))
        return -1;

    my_scheduler.time_quantum = time_quantum;
    my_scheduler.io = io;
    my_scheduler.initialised = 1;
    my_scheduler.running_thread = NULL;

    return 0;
}

DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority)
{
	return 0;
}

DECL_PREFIX int so_wait(unsigned int io)
{
	return 0;
}

DECL_PREFIX int so_signal(unsigned int io)
{
	return 0;
}

DECL_PREFIX void so_exec(void)
{
}

DECL_PREFIX void so_end(void)
{
	int i;
	for (i = 0;  i < my_scheduler.no_threads; i++) {
        //sem_destroy(&my_scheduler.threads[i]->is_running);
        //sem_destroy(&my_scheduler.threads[i]->is_terminated);
        //pthread_join(my_scheduler.threads[i]->thread_id, NULL);
        //free(my_scheduler.threads[i]);
    }
    // etc...
    //free(my_scheduler.threads);

    // make current scheduler inactive (initialized)
    my_scheduler.initialised = 0;
}

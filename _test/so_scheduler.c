#include "so_scheduler.h"
#include <stdlib.h>

typedef struct scheduler scheduler;
typedef struct thread thread;

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4

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
    unsigned int priority;
    // NEW, READY, RUNNING, WAITING, TERMINATED
    unsigned int state;
    // how much time a thread has left
    // for the current quantum
    unsigned int time_quantum_left;
    
    // used for waiting -> running
    pthread_mutex_t waiting_lock;
    pthread_cond_t  waiting_cond;

};

scheduler my_scheduler;

int so_init(unsigned int time_quantum, unsigned int io) {
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

int insert_thread(thread *crt_thread) {
    // grow the number of threads the scheduler knows about
    my_scheduler.no_threads++;
    // make more space for the ones available to hold one more
    my_scheduler.threads = (thread **) realloc(my_scheduler.threads, my_scheduler.no_threads * sizeof(crt_thread));
    if (my_scheduler.threads == NULL)
        return -1;
    // add the new thread to the list of threads in my_scheduler
    my_scheduler.threads[my_scheduler.no_threads - 1] = crt_thread;

    return 0;
}

void* start_thread(void *params)
{
	thread *started_thread = (thread *) params;

    // wait to be planned
    // TODO - ma gandesc sa pun ceva de genul daca e cineva in running sau nu
    // gen my_scheduler->running_thread != NULL
    while (my_scheduler->running_thread) {
        pthread_cond_wait(start_thread->waiting_cond, start_thread->waiting_lock);
    }

    // call handler (prio)
    (started_thread->func)(start_thread->prio);

	return NULL;
}

tid_t so_fork(so_handler *func, unsigned int priority) {
    // no handler to call or invalid priority
    if ((func == NULL) || (priority > SO_MAX_PRIO))
        return INVALID_TID;

    pthread_t tid;
    // used for return values for different allocations
    int rc;
    
    // initializare thread struct
    thread *crt_thread = (thread *) malloc(sizeof(thread));
    if (crt_thread == NULL)
        return INVALID_TID;

    crt_thread->func = func;
    crt_thread->priority = priority;
    rc = pthread_cond_init(crt_thread->waiting_cond, NULL);
    if (rc)
        return INVALID_TID;
    rc = pthread_mutex_init(&crt_thread->waiting_lock, NULL);
    if (rc)
        return INVALID_TID;

    // not sure here TODO check
    crt_thread->state = NEW;

    // creare new thread that will execute start_thread function
    rc = pthread_create(&tid, NULL, (void *) start_thread, &crt_thread);
    
    // pthread_create failed
    if (rc)
        return INVALID_TID;


    // check if not first thread to be scheduled
    if (my_scheduler->no_threads) {
        so_exec();
    }

    rc = insert_thread(crt_thread);
    if (rc < 0)
        return INVALID_TID;


    // return id of the new created thread
    return tid;
}

int so_wait(unsigned int io) {
    // check io exists
    // not sure if here thread running
    // has to decrease time left
}

int so_signal(unsigned int io) {
    // not sure if here thread running
    // has to decrease time left
}

void so_exec(void) {
    // for sure here decrease time of the running thread
    my_scheduler.running_thread->time_quantum_left--;

    // if quantum expired for running plan next best priority
    // thread to run
    if (my_scheduler.running_thread->time_quantum_left == 0) {

    }
/* do work
    check scheduler
    if (preempted)
        block();
    return;*/

    // cumva block ar putea fi un mutex
    // care e deblocat de abia cand se merge
    // prin lista de waiting sa se poata debloca

    // I: totusi nu stiu daca se trece mai intai
    // prin waiting sau ready, care e prioritar

    // R: sau sunt doar chestii diferite, ca
    // din waiting trb sa treci in ready, o sa vad
}

void so_end(void) {
    // TODO - free used resources

    // sa faci destroy la conditii
    // pthread_cond_destroy(pthread_cond_t *cond);
    // pthread_mutex_destroy(pthread_mutex_t *mutex);

    // for (i = 0;  i < my_scheduelr.no_threads; i++)
    // pthread_join
    // etc...

    // make current scheduler inactive (initialized)
    my_scheduler.initialised = 0;
}   
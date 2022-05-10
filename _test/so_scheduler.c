#include "priqueue.h"

// for malloc
#include <stdlib.h>

// for printf
#include <stdio.h>

// for sleep (will be deleted probably TODO)
#include <unistd.h>

// for semaphore
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

typedef struct scheduler scheduler;

#define NEW 0
#define READY 1
#define RUNNING 2
#define WAITING 3
#define TERMINATED 4

#define NO_WAIT_EVENT 0

struct scheduler {
    unsigned int time_quantum;
    unsigned int io;
    thread *running_thread;
    priqueue *terminated_threads;
    priqueue *ready_threads;
};


scheduler *my_scheduler = NULL;

int so_init(unsigned int time_quantum, unsigned int io) {
    // no time quantum
    // number of io events are over the maximum admitted
    // scheduler reinitialized
    if ((time_quantum == 0) ||
        (io > SO_MAX_NUM_EVENTS) ||
        (my_scheduler != NULL))
        return -1;

    my_scheduler = (scheduler *) malloc(1 * sizeof(scheduler));
    if (!my_scheduler)
        return -1;

    my_scheduler->time_quantum = time_quantum;
    my_scheduler->io = io;
    my_scheduler->running_thread = NULL;   
    my_scheduler->ready_threads = create_queue();
    my_scheduler->terminated_threads = create_queue();

    return 0;
}

void make_schedule() {
    //printf("ruleaza %u cu cuanta %u\n", my_scheduler->running_thread->thread_id, my_scheduler->running_thread->time_quantum_left);
    // or just terminated thread
    if (my_scheduler->running_thread->state == TERMINATED) {
        my_scheduler->running_thread = peek(my_scheduler->ready_threads);

        // last thread to execute something in the program
        if (!my_scheduler->running_thread) {
            my_scheduler->running_thread = NULL;
            return;
        }

        //printf("s-a ales %u\n",  my_scheduler->running_thread->thread_id);

        sem_post(&(my_scheduler->running_thread->can_run));
        return;
    }

    // check whether higher priority thread appeared
    //printf("Se alege candidat\n");
    thread *prev_running_thread = my_scheduler->running_thread;
    thread *candidate = peek(my_scheduler->ready_threads);

    if (candidate) {
        //printf("s-a gasit un candidat cu id %u\n", candidate->thread_id);
    } else {
        //printf("Niciun candidat\n");
    }
//term
    // there isa  candidate which has more priority than current running thread
    if ((candidate) && (candidate->priority > my_scheduler->running_thread->priority)) {
        enqueue(my_scheduler->ready_threads, prev_running_thread);

        sem_post(&(candidate->can_run));
        sem_wait(&(prev_running_thread->can_run));
        dequeue(my_scheduler->ready_threads);

        my_scheduler->running_thread = prev_running_thread;
        my_scheduler->running_thread->time_quantum_left = my_scheduler->time_quantum;
    } else {
        // no candidate or no better candidate was found

        if (my_scheduler->running_thread->time_quantum_left == 0) {
            // todo check candidate exists
            if ((candidate) && (candidate->priority == my_scheduler->running_thread->priority)) {
                enqueue(my_scheduler->ready_threads, prev_running_thread);

                sem_post(&(candidate->can_run));
                sem_wait(&(prev_running_thread->can_run));
                dequeue(my_scheduler->ready_threads);

                my_scheduler->running_thread = prev_running_thread;
            }

            my_scheduler->running_thread->time_quantum_left = my_scheduler->time_quantum;
        }
    }

    // check waiting and all of that shiet
}

int insert_thread(thread *crt_thread) {

}

thread *init_thread(so_handler *func, unsigned int priority) {
    thread *crt_thread = (thread *) malloc(sizeof(thread));
    if (crt_thread == NULL)
        return NULL;

    crt_thread->func = func;
    crt_thread->priority = priority;
    crt_thread->time_quantum_left = my_scheduler->time_quantum;
    crt_thread->state = NEW;
    sem_init(&crt_thread->thread_started, 0, 0);
    sem_init(&crt_thread->can_run, 0, 0);  

    return crt_thread; 
}

void* start_thread(void *params)
{
    thread *crt_thread = (thread *) params;

    // @crt_thread is ready to execute

    enqueue(my_scheduler->ready_threads, crt_thread);
    sem_post(&(crt_thread->thread_started));

    // wait for @crt_thread to be the only
    // thread running
    sem_wait(&(crt_thread->can_run));
    dequeue(my_scheduler->ready_threads);
    my_scheduler->running_thread = crt_thread;

    // call handler(prio)
    (crt_thread->func)(crt_thread->priority);

    enqueue(my_scheduler->terminated_threads, crt_thread);
    my_scheduler->running_thread->state = TERMINATED;
    //printf("s-a terminat %u\n", my_scheduler->running_thread->thread_id);
    make_schedule();
    //if (!my_scheduler->running_thread) {
    //sem_post(&(crt_thread->thread_started));

    return NULL;
}

tid_t so_fork(so_handler *func, unsigned int priority) {
   // no handler to call or invalid priority
    if ((func == NULL) || (priority > SO_MAX_PRIO))
        return INVALID_TID;

    // used for return values of different allocations/function calls
    int rc;

    thread *crt_thread = init_thread(func, priority);
    if (!crt_thread) {
        return INVALID_TID;
    }

    rc = pthread_create(&crt_thread->thread_id, NULL, &start_thread, crt_thread);
    if (rc)
        return INVALID_TID;

    //printf("din %u se naste %u\n", pthread_self(), crt_thread->thread_id);

    // not first so_fork
    if (my_scheduler->running_thread) {
        sem_wait(&(crt_thread->thread_started));
        so_exec();
    } else {
        sem_post(&(crt_thread->can_run));
        sem_wait(&(crt_thread->thread_started));
    }

    return crt_thread->thread_id;
}

int so_wait(unsigned int io) {

}

int so_signal(unsigned int io) {
    
}



void so_exec(void) {
    my_scheduler->running_thread->time_quantum_left--;

    make_schedule();
}

void so_end(void) {
    if (my_scheduler) {
        // mai intai un join dupa toate threadurile
        // TODO - remove sleep, find better solution for sync
        //sleep(1);
        sleep(1);
        priqueue *terminated = my_scheduler->terminated_threads;

        priqueue_node *terminated_thread = terminated->head;

        while (terminated_thread) {
            pthread_join(terminated_thread->info->thread_id, NULL);
            terminated_thread = terminated_thread->next;
        }


        //print_queue(my_scheduler->ready_threads);
        clear_queue(my_scheduler->ready_threads);
        //printf("\n\n\n");
        //print_queue(my_scheduler->terminated_threads);
        clear_queue(my_scheduler->terminated_threads);
        free(my_scheduler);
        my_scheduler = NULL;
    }
}   
#include "so_scheduler.h"

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
    sem_t is_terminated;
    sem_t is_running;
    pthread_t thread_id;
    unsigned int no_times_on_processor;
    unsigned int wait_event;
};

scheduler my_scheduler;

void make_schedule(thread *prev_thread) {
    if (prev_thread->state == WAITING) {
        thread *best_candidate = NULL;
        int flag = 0;
        thread *aux = NULL;

        for (int i = 0; i < my_scheduler.no_threads; i++) {

            //printf("compare with %u with state: %u\n", my_scheduler.threads[i]->thread_id,
           // my_scheduler.threads[i]->state ;

            if (my_scheduler.threads[i] == prev_thread) continue;
            if ((my_scheduler.threads[i]->state == TERMINATED) ||
               (my_scheduler.threads[i]->state == WAITING)) continue;

            if (best_candidate == NULL) {
                best_candidate = my_scheduler.threads[i];
                continue;
            }

            unsigned int compare_priority =
                 my_scheduler.threads[i]->priority;
            unsigned int compare_no_times_on_processor =
                 my_scheduler.threads[i]->no_times_on_processor;

            //printf("compare: %u %u\n", compare_priority, compare_no_times_on_processor);
            //printf("best: %u %u\n", best_candidate->priority, best_candidate->no_times_on_processor);

            if (compare_priority > best_candidate->priority) {
                best_candidate = my_scheduler.threads[i];
                flag = 2;
            } else if (compare_priority == best_candidate->priority) {
                // test 12
                if (compare_no_times_on_processor < best_candidate->no_times_on_processor) {
                    best_candidate = my_scheduler.threads[i];
                    flag = 2;
                }
                // test 13
                else if ((flag == 0) && (compare_no_times_on_processor == prev_thread->no_times_on_processor)) {
                    aux = my_scheduler.threads[i];
                    flag = 1;
                }
            }
        }
        // test 13
        // TODO - 1 MORE CONDITION IS NEEDED I THINK ABOUT best_candidate
        if (flag == 1) {
            best_candidate = aux;
        }


        sem_post(&(best_candidate->is_running));
        sem_wait(&(prev_thread->is_running));
        prev_thread->state = READY;
        // mark update state
        sem_post(&(prev_thread->is_terminated));
        sem_wait(&(prev_thread->is_running));
        prev_thread->state = RUNNING;
        prev_thread->time_quantum_left = my_scheduler.time_quantum;
        prev_thread->no_times_on_processor++;
        my_scheduler.running_thread = prev_thread; 
    } else if ((prev_thread->time_quantum_left == 0) &&
                (prev_thread->state != TERMINATED)) {
        thread *best_candidate = prev_thread;
        int flag = 0;
        thread *aux = NULL;
        //printf("best candidate tid la inceput: %u\n", best_candidate->thread_id);

        for (int i = 0; i < my_scheduler.no_threads; i++) {

            //printf("compare with %u with state: %u\n", my_scheduler.threads[i]->thread_id,
           // my_scheduler.threads[i]->state ;

            if (my_scheduler.threads[i] == prev_thread) continue;
            if ((my_scheduler.threads[i]->state == TERMINATED) ||
               (my_scheduler.threads[i]->state == WAITING)) continue;


            unsigned int compare_priority =
                 my_scheduler.threads[i]->priority;
            unsigned int compare_no_times_on_processor =
                 my_scheduler.threads[i]->no_times_on_processor;

            //printf("compare: %u %u\n", compare_priority, compare_no_times_on_processor);
            //printf("best: %u %u\n", best_candidate->priority, best_candidate->no_times_on_processor);

            if (compare_priority > best_candidate->priority) {
                best_candidate = my_scheduler.threads[i];
                flag = 2;
            } else if (compare_priority == best_candidate->priority) {
                // test 12
                if (compare_no_times_on_processor < best_candidate->no_times_on_processor) {
                    best_candidate = my_scheduler.threads[i];
                    flag = 2;
                }
                // test 13
                else if ((flag == 0) && (compare_no_times_on_processor == prev_thread->no_times_on_processor)) {
                    aux = my_scheduler.threads[i];
                    flag = 1;
                }
            }
        }
        // test 13
        // TODO - 1 MORE CONDITION IS NEEDED I THINK ABOUT best_candidate
        if (flag == 1) {
            best_candidate = aux;
        }

        //printf("best candidate tid la final: %u\n", best_candidate->thread_id); 
       // printf("prev_thread no_times: %d\n", prev_thread->no_times_on_processor);
        //printf("best_candidate no_times: %d\n", best_candidate->no_times_on_processor);
        // TODO put at sleep the previous running thread
        // and wake up current candidate

        //printf("\na expirat cuanta\n");
        //printf("\nse blocheaza: %u\n", prev_thread->thread_id);
        //printf("\nse da drumul la: %u\n", best_candidate->thread_id);
        if (best_candidate == prev_thread) {
            prev_thread->state = RUNNING;
            prev_thread->time_quantum_left = my_scheduler.time_quantum;
            prev_thread->no_times_on_processor++;
            my_scheduler.running_thread = prev_thread;
        } else {
            // otherwise wake up thread @best_candidate

            // this make wake up a thread in start_thread
            // or a thread who once was in so_exec
            //printf("\n\n");
            sem_post(&(best_candidate->is_running));
            prev_thread->state = READY;
            // mark update state
            sem_post(&(prev_thread->is_terminated));
            sem_wait(&(prev_thread->is_running));
            prev_thread->state = RUNNING;
            prev_thread->time_quantum_left = my_scheduler.time_quantum;
            prev_thread->no_times_on_processor++;
            my_scheduler.running_thread = prev_thread;
        }
    } else {
        // quantum not expired
        // check if a bigger priority thread exists

        thread *best_candidate = NULL;
        int best_priority;
        unsigned int best_no_times_on_processor;
        int flag = 0;
        thread *aux = NULL;

        if (prev_thread->state == TERMINATED) {
            best_candidate = prev_thread;
            best_priority = -1;
            // TODO REMOVE MAGIC CONSTANTS
            best_no_times_on_processor = 4000000000;
        } else {
            best_candidate = prev_thread;
            best_priority = prev_thread->priority;
            best_no_times_on_processor = prev_thread->no_times_on_processor;
        }

        for (int i = 0; i < my_scheduler.no_threads; i++) {
            if ((my_scheduler.threads[i]->state == WAITING)
              ||  (my_scheduler.threads[i]->state == TERMINATED)
              || (my_scheduler.threads[i] == prev_thread)) continue;
              

            int compare_priority =
                 my_scheduler.threads[i]->priority;
            int compare_no_times_on_processor =
                 my_scheduler.threads[i]->no_times_on_processor;


            //printf("de comparat: %u\n", compare_no_times_on_processor)
            //printf("compare priority: %d\n", compare_priority);
            // TODO - add no times on processor here i guess
            // if a new better priority thread appeared
            if (compare_priority > best_priority) {
                best_candidate = my_scheduler.threads[i];
                best_priority = my_scheduler.threads[i]->priority;
                best_no_times_on_processor = my_scheduler.threads[i]->no_times_on_processor;
                flag = 2;
            }
                // interested just in priority if not terminated
            if (prev_thread->state != TERMINATED) continue;
            if (compare_priority == best_priority) {
                if (compare_no_times_on_processor < best_candidate->no_times_on_processor) {
                    best_candidate = my_scheduler.threads[i];
                    best_priority = my_scheduler.threads[i]->priority;
                    best_no_times_on_processor = my_scheduler.threads[i]->no_times_on_processor;
                    flag = 2;
                } else if ((flag == 0) && (compare_no_times_on_processor == prev_thread->no_times_on_processor)) {
                    aux = my_scheduler.threads[i];
                    flag = 1;
                }
            }
        }

        // OBSERVATIE: se afiseaza o singura data
        // SE INTRA ACI cand da failed stress test round robin

        // todo: check test_exec ce valori da el cand pica

        if (flag == 1) {
            best_candidate = aux;
        }

        if (best_candidate == prev_thread) {
          //  printf("\nNu s-a gasit un candidat cu prio mai mare\n");
            return;
        }
    
        //printf("\nS-a gasit un candidat cu prio mai mare\n");
        // printf("\nse blocheaza: %u\n", prev_thread->thread_id);
        //printf("\nse da drumul la: %u\n", best_candidate->thread_id);
 
        // found a thread with a bigger priority
        sem_post(&(best_candidate->is_running));
        if (prev_thread->state != TERMINATED) {
            prev_thread->state = READY;  
           // printf("se blocheaza: %u\n", pthread_self());      
            sem_wait(&(prev_thread->is_running));
            prev_thread->state = RUNNING;
            prev_thread->time_quantum_left = my_scheduler.time_quantum;
            prev_thread->no_times_on_processor++;
            my_scheduler.running_thread = prev_thread;
        }
    }
}

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
	thread *crt_thread = (thread *) params;

    // thread already running

    if (my_scheduler.running_thread) {
        crt_thread->state = READY;
        sem_post(&(crt_thread->is_terminated));
    }

    // wait to be planned
    // sem_wait() ; de unde primeste sem_post?
    // prob din so_exec dupa ce se alege cel mai bun
    sem_wait(&(crt_thread->is_running));

    crt_thread->state = RUNNING;
    crt_thread->no_times_on_processor = 1;
    my_scheduler.running_thread = crt_thread;

    if (my_scheduler.running_thread == NULL)
        sem_post(&(crt_thread->is_terminated));
    (crt_thread->func)(crt_thread->priority);

    // when reach here thread finished work
    crt_thread->state = TERMINATED;
    make_schedule(crt_thread);

	return NULL;
}

tid_t so_fork(so_handler *func, unsigned int priority) {
   // printf("intram\n");

    // no handler to call or invalid priority
    if ((func == NULL) || (priority > SO_MAX_PRIO))
        return INVALID_TID;

    //pthread_t tid;
    // used for return values for different allocations
    int rc;
    
    // initializare thread struct
    thread *crt_thread = (thread *) malloc(sizeof(thread));
    if (crt_thread == NULL)
        return INVALID_TID;

    crt_thread->func = func;
    crt_thread->priority = priority;
    crt_thread->state = NEW;
    crt_thread->time_quantum_left = my_scheduler.time_quantum;
    crt_thread->no_times_on_processor = 0;
    crt_thread->wait_event = NO_WAIT_EVENT;
    sem_init(&crt_thread->is_terminated, 0, 0);
    sem_init(&crt_thread->is_running, 0, 0);

    // creare new thread that will execute start_thread function
    rc = pthread_create(&crt_thread->thread_id, NULL, &start_thread, crt_thread);
    
   // printf("a fost creat thread-ul cu id-ul %u\n", crt_thread->thread_id);

    // pthread_create failed
    if (rc)
        return INVALID_TID;

    rc = insert_thread(crt_thread);
    if (rc < 0)
        return INVALID_TID;


    //printf("no_threads: %d\n", my_scheduler.no_threads);

    // check if not first thread to be scheduled
    if (my_scheduler.no_threads != 1) {
        // NOT SURE OF THIS
        // more generally, when so_exec is being
        // run - if just by checker or also
        // by me
        //printf("de aici\n\n");
        sem_wait(&(crt_thread->is_terminated));
        so_exec();
        //printf("si am iesit\n\n");
    } else {
        sem_post(&(crt_thread->is_running));
    }

    //printf("no threads after: %d\n", my_scheduler.no_threads);

    // return id of the new created thread
    return crt_thread->thread_id;
}

int so_wait(unsigned int io) {
    if ((io >= my_scheduler.io) || (io < 0))
        return -1;

    my_scheduler.running_thread->wait_event = io;
    my_scheduler.running_thread->state = WAITING;
    // give control to another thread
    so_exec();

    return 0;
}

int so_signal(unsigned int io) {
    // not sure if here thread running
    // has to decrease time left
    if ((io >= my_scheduler.io) || (io < 0))
        return -1;
    
    int no_waken_threads = 0;

    for (int it = 0; it < my_scheduler.no_threads; it++) {
        thread *crt_thread = my_scheduler.threads[it];

        // wake up all threads waiting for the current
        if ((crt_thread->state == WAITING) &&
            (crt_thread->wait_event == io)) {
                sem_post(&(crt_thread->is_running));
                no_waken_threads++;
                crt_thread->wait_event = NO_WAIT_EVENT;
                // wait for thread to update its' state
                sem_wait(&(crt_thread->is_terminated));
            }
    }

    // now make only one truly run
    so_exec();

    return no_waken_threads;
}



void so_exec(void) {
    thread *prev_thread = my_scheduler.running_thread;

    //printf("now running: %u, din %u\n", prev_thread->thread_id, pthread_self());
    //printf("no_threads: %d\n", my_scheduler.no_threads);

    if (prev_thread == NULL) {
        // not sure will ever enter this (TODO check)
        return;
    }
    // for sure here decrease time of the running thread
    prev_thread->time_quantum_left--;

    //printf("quantum_left: %d\n", my_scheduler.running_thread->time_quantum_left);
    //printf("state: %d\n", my_scheduler.running_thread->state);

    // if quantum expired for running plan next best priority
    // thread to run
    make_schedule(prev_thread);
    //sleep(1);
}

void so_end(void) {
    // TODO - free used resources

    for (int i = 0;  i < my_scheduler.no_threads; i++) {
        sem_destroy(&my_scheduler.threads[i]->is_running);
        sem_destroy(&my_scheduler.threads[i]->is_terminated);
        pthread_join(my_scheduler.threads[i]->thread_id, NULL);
        //free(my_scheduler.threads[i]);
    }
    // etc...
    //free(my_scheduler.threads);

    // make current scheduler inactive (initialized)
    my_scheduler.initialised = 0;
}   
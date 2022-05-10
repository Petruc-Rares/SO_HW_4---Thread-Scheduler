// guide after:
// http://freesourcecode.net/cprojects/112198/A-generic-queue-in-c-in-c#.YnkY1IxBxPa
// https://github.com/kostakis/Generic-Queue

#include "so_scheduler.h"
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>

#ifndef PRIQUEUE_H_
#define PRIQUEUE_H_

typedef struct priqueue priqueue;
typedef struct priqueue_node priqueue_node;
typedef struct thread thread;

struct thread {
    // not sure it will be used
    so_handler *func;
    //priority of the thread
    int priority;
    // TODO - not sure state is needed anymore
    // NEW, READY, RUNNING, WAITING, TERMINATED
    unsigned int state;
    // how much time a thread has left
    // for the current quantum
    unsigned int time_quantum_left;
    sem_t thread_started;
    sem_t can_run;
    pthread_t thread_id;
    unsigned int no_times_on_processor;
    unsigned int wait_event;
};

struct priqueue_node {
    thread *info;
    struct priqueue_node *next;
};

struct priqueue {
    priqueue_node *head;
    priqueue_node *tail;
};

priqueue_node *create_node(void *elem);

priqueue *create_queue();

void enqueue(priqueue *q, thread *elem);

thread *dequeue(priqueue *q);

thread *peek(priqueue *q);

// 1 if queue empty
// 0 otherwise
int is_empty_queue(priqueue *q);

void clear_queue(priqueue *q);

#endif /* PRIQUEUE_H_ */
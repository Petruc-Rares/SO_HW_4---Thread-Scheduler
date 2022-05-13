#include "priqueue.h"
#include <windows.h>
#include <stdio.h>

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
	thread *running_thread;
	priqueue *terminated_threads;
	priqueue *ready_threads;
	priqueue **waiting_threads;
	HANDLE program_over;
};

scheduler *my_scheduler;

void make_schedule(void)
{
	thread *prev_running_thread = NULL;
	thread *candidate = NULL;

	 // or just terminated thread
	if ((my_scheduler->running_thread->state == TERMINATED) ||
	   (my_scheduler->running_thread->state == WAITING)) {
		my_scheduler->running_thread =
			peek(my_scheduler->ready_threads);

		// last thread to execute something in the program
		if (!my_scheduler->running_thread) {
			ReleaseSemaphore(my_scheduler->program_over, 1, NULL);
			my_scheduler->running_thread = NULL;
			return;
		}

		ReleaseSemaphore(my_scheduler->running_thread->can_run,
			1, NULL);
		return;
	}

	// check whether higher priority thread appeared
	//printf("Se alege candidat\n");
	prev_running_thread = my_scheduler->running_thread;
	candidate = peek(my_scheduler->ready_threads);

	// there is a candidate which has more priority
	// than current running thread
	if ((candidate) &&
		(candidate->priority >
		my_scheduler->running_thread->priority)) {
		enqueue(my_scheduler->ready_threads, prev_running_thread);

		ReleaseSemaphore(candidate->can_run, 1, NULL);
		WaitForSingleObject(prev_running_thread->can_run, INFINITE);
		dequeue(my_scheduler->ready_threads);

		my_scheduler->running_thread = prev_running_thread;
		my_scheduler->running_thread->time_quantum_left =
			my_scheduler->time_quantum;
	} else {
		// no candidate or no better candidate was found

		if (my_scheduler->running_thread->time_quantum_left == 0) {
			// todo check candidate exists
			if ((candidate) &&
				(candidate->priority ==
				my_scheduler->running_thread->priority)) {
				enqueue(my_scheduler->ready_threads,
					prev_running_thread);

				ReleaseSemaphore(candidate->can_run, 1, NULL);
				WaitForSingleObject(
					prev_running_thread->can_run, INFINITE);
				dequeue(my_scheduler->ready_threads);

				my_scheduler->running_thread =
					prev_running_thread;
			}

			my_scheduler->running_thread->time_quantum_left =
				my_scheduler->time_quantum;
		}
	}

	// check waiting and all of that shiet
}


thread *init_thread(so_handler *func, unsigned int priority)
{
	thread *crt_thread = (thread *) malloc(sizeof(thread));

	if (crt_thread == NULL)
		return NULL;

	crt_thread->func = func;
	crt_thread->priority = priority;
	crt_thread->time_quantum_left = my_scheduler->time_quantum;
	crt_thread->state = NEW;

	crt_thread->thread_started = CreateSemaphore(NULL, 0, 1, NULL);
	crt_thread->can_run = CreateSemaphore(NULL, 0, 1, NULL);
	crt_thread->ended = CreateSemaphore(NULL, 0, 1, NULL);
	crt_thread->status_updated = CreateSemaphore(NULL, 0, 1, NULL);

	return crt_thread;
}

DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io)
{
	unsigned int i;

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
	if (io) {
		my_scheduler->waiting_threads =
			(priqueue **) malloc(io * sizeof(priqueue));
		if (!my_scheduler->waiting_threads)
			exit(1);

		for (i = 0; i < io; i++)
			my_scheduler->waiting_threads[i] = create_queue();
	} else {
		my_scheduler->waiting_threads = NULL;
	}
	my_scheduler->program_over = CreateSemaphore(NULL, 0, 1, NULL);

	return 0;
}

DWORD WINAPI start_thread(LPVOID params);

DWORD WINAPI start_thread(LPVOID params)
{
	thread *crt_thread = (thread *) params;

	// @crt_thread is ready to execute
	enqueue(my_scheduler->ready_threads, crt_thread);
	ReleaseSemaphore(crt_thread->thread_started, 1, NULL);

	// wait for @crt_thread to be the only
	// thread running

	WaitForSingleObject(crt_thread->can_run, INFINITE);
	dequeue(my_scheduler->ready_threads);
	my_scheduler->running_thread = crt_thread;

	// call handler(prio)
	(crt_thread->func)(crt_thread->priority);

	enqueue(my_scheduler->terminated_threads, crt_thread);
	my_scheduler->running_thread->state = TERMINATED;
	//printf("s-a terminat %u\n", my_scheduler->running_thread->thread_id);
	make_schedule();

	ReleaseSemaphore(crt_thread->ended, 1, NULL);

	return NULL;
}

DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority)
{
	// used for return values of different allocations/function calls
	HANDLE rc;
	thread *crt_thread = NULL;
	DWORD IDThread;

	if ((func == NULL) || (priority > SO_MAX_PRIO))
		return INVALID_TID;

	crt_thread = init_thread(func, priority);
	if (!crt_thread)
		return INVALID_TID;

	rc = CreateThread(NULL,
					  0,
					  start_thread,
					  (LPVOID) crt_thread,
					  0,
					  &IDThread);

	if (rc == NULL)
		return INVALID_TID;

	crt_thread->thread_id = rc;


	// not first so_fork
	if (my_scheduler->running_thread) {
		WaitForSingleObject(crt_thread->thread_started, INFINITE);
		so_exec();
	} else {
		ReleaseSemaphore(crt_thread->can_run, 1, NULL);
		WaitForSingleObject(crt_thread->ended, INFINITE);
	}

	return IDThread;
}

DECL_PREFIX int so_wait(unsigned int io)
{
	thread *waiting_thread = NULL;

	if ((io >= my_scheduler->io) || (io < 0)) {
		so_exec();
		return -1;
	}

	waiting_thread = my_scheduler->running_thread;

	waiting_thread->state = WAITING;

	enqueue(my_scheduler->waiting_threads[io], waiting_thread);
	so_exec();

	WaitForSingleObject(waiting_thread->can_run, INFINITE);

	enqueue(my_scheduler->ready_threads, waiting_thread);
	waiting_thread->state = READY;

	ReleaseSemaphore(waiting_thread->status_updated, 1, NULL);

	WaitForSingleObject(waiting_thread->can_run, INFINITE);

	dequeue(my_scheduler->ready_threads);
	my_scheduler->running_thread = waiting_thread;
	my_scheduler->running_thread->time_quantum_left =
		my_scheduler->time_quantum;

	return 0;
}

DECL_PREFIX int so_signal(unsigned int io)
{
	int no_waken_threads = 0;
	priqueue *waiting_threads = NULL;
	thread *waiting_thread = NULL;

	if ((io >= my_scheduler->io) || (io < 0)) {
		so_exec();
		return -1;
	}

	waiting_threads = my_scheduler->waiting_threads[io];

	// get count of threads that are woken up
	waiting_thread = NULL;

	while (waiting_thread = dequeue(waiting_threads)) {
		ReleaseSemaphore(waiting_thread->can_run, 1, NULL);
		no_waken_threads++;
		// wait to place in queue for ready threads
		WaitForSingleObject(waiting_thread->status_updated, INFINITE);
	}

	so_exec();

	return no_waken_threads;
}

DECL_PREFIX void so_exec(void)
{
	my_scheduler->running_thread->time_quantum_left--;

	make_schedule();
}

DECL_PREFIX void so_end(void)
{
	if (my_scheduler) {
		unsigned int i;
		priqueue *terminated = NULL;
		priqueue_node *terminated_thread = NULL;

		if (my_scheduler->running_thread != NULL)
			WaitForSingleObject(my_scheduler->program_over,
								INFINITE);

		terminated = my_scheduler->terminated_threads;

		terminated_thread = terminated->head;

		while (terminated_thread) {
			WaitForSingleObject(terminated_thread->info->thread_id,
				INFINITE);
			terminated_thread = terminated_thread->next;
		}


		clear_queue(my_scheduler->ready_threads);
		clear_queue(my_scheduler->terminated_threads);

		for (i = 0; i < my_scheduler->io; i++)
			clear_queue(my_scheduler->waiting_threads[i]);

		free(my_scheduler->waiting_threads);

		free(my_scheduler);
		my_scheduler = NULL;
	}
}

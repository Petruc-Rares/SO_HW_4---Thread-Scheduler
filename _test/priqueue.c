#include "priqueue.h"
#include <stdio.h>
#include <stdlib.h>

priqueue *create_queue() {
	priqueue *prio_queue = (priqueue *) malloc(sizeof(priqueue));
	if (!prio_queue) {
		printf("Failed to alloc priority queue\n");
		exit(1);
	}
	prio_queue->head = NULL;
	prio_queue->tail = NULL;

	return prio_queue;
}

priqueue_node *create_node(void *elem)
{
	priqueue_node *new_node = (priqueue_node *)
		malloc(sizeof(priqueue_node));

	if (!new_node) {
		printf("Failed to alloc new node\n");
		exit(1);
	}
	new_node->info = elem;
	new_node->next = NULL;

	return new_node;
}

void enqueue(priqueue *q, thread *elem)
{
	priqueue_node *new_node = create_node(elem);
	   // find the right position in queue
	priqueue_node *prev = NULL;
	priqueue_node *crt = NULL;

	if (is_empty_queue(q)) {
		q->head = new_node;
		q->tail = q->head;
		return;
	}

	crt = q->head;

	// go to the corresponding position
	while ((crt) && (crt->info->priority >= new_node->info->priority)) {
		prev = crt;
		crt = crt->next;
	}

	if (prev) {
		priqueue_node *aux_next = prev->next;

		prev->next = new_node;
		new_node->next = aux_next;
	} else {
		new_node->next = q->head;
		q->head = new_node;
	}

	// q->tail
	if (new_node->next == NULL)
		q->tail = new_node;
}


thread *dequeue(priqueue *q)
{
	priqueue_node *elem = NULL;
	thread *thread_return = NULL;

	if ((q == NULL) || (q->head == NULL) || (q->tail == NULL))
		return NULL;

	elem = q->head;
	q->head = elem->next;

	thread_return = elem->info;
	free(elem);

	// FIRST ELEM
	if (q->head == NULL)
		q->tail = NULL;

	return thread_return;
}

int is_empty_queue(priqueue *q)
{
	return ((q == NULL) || ((q->head == q->tail) &&
		(q->head == NULL)));
}

void print_queue(priqueue *q)
{
	priqueue_node *crt_node = q->head;


	while (crt_node) {
		printf("crt_node->prio: %u\n", crt_node->info->priority);
		crt_node = crt_node->next;
	}
}

thread *peek(priqueue *q)
{
	if (is_empty_queue(q))
		return NULL;

	return q->head->info;
}

void free_info(thread *info)
{
	//sem_destroy(&(info->thread_started));
	//sem_destroy(&(info->can_run));
	free(info);
}

void clear_queue(priqueue *q)
{
	priqueue_node *crt_node = NULL;

	if (q == NULL)
		return;

	crt_node = q->head;

	while (crt_node) {
		priqueue_node *aux = crt_node;
		// TODO free thread and destroy semaphores
		free_info(crt_node->info);
		crt_node = crt_node->next;
		free(aux);
	}

	free(q);
}

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	struct pcb_t *addProc = malloc(sizeof(struct pcb_t));
	*addProc = *proc;
	addProc->prio = addProc->priority;
	int index;
	for (int i = 0; i <q->size;i++){
		if(addProc->priority < q->proc[i]->priority) 
		{
			index = i;
			break;
		}
	}
	for (int i = q->size; i> index; i--) q->proc[i] = q->proc[i-1];
	q->proc[index] = addProc;
	q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */

	if(q->size == 0) return NULL;

	struct pcb_t *result = malloc(sizeof(struct pcb_t));
	*result = *(q->proc[q->size-1]);
	free(q->proc[q->size-1]);
	q->size--;
	return result;
}


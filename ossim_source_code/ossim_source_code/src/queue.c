#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	//The queue is sort from the smallest
	//The highest out first

	if(q->size == MAX_QUEUE_SIZE) return;								//Return if size is max
	int index;															//Index to store proc
	for (int i = 0; i <q->size;i++)	if(proc->prio <= q->proc[i]->prio) 	//Search for a place to insert 		
	{																	
		index = i;														
		break;
	}
	for (int i = q->size; i > index; i--) q->proc[i] = q->proc[i-1];	//Push another element up
	q->proc[index] = proc;
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
	q->proc[q->size-1] = NULL;
	q->size--;
	return result;
}


/* 
	Phan Van Hai - BKHN 
*/
// A C program to demonstrate linked list based implementation of queue

#ifndef _USER_QUEUE_H
#define _USER_QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h> 

typedef __uint16_t	uint16_t;
// A linked list (LL) node to store a queue entry
typedef struct QNode {
	char *str;
	struct QNode* next;
} QNode;

// The queue, front stores the front node of LL and rear stores the
// last node of LL
typedef struct Queue {
	pthread_mutex_t *mutex;
	uint16_t cap;
	uint16_t size;
	QNode *front, *rear;
} Queue;

void lock_queue(Queue *q);
void unlock_queue(Queue *q);
Queue *createQueue(uint16_t cap);
void enQueue(Queue* q, const char *str);
char *deQueue(Queue* q);
uint16_t sizeQueue(Queue *q);
// doc phan tu o dau Queue - khong su dung mutex
char *readQNode(Queue *q);
// xoa va free phan tu o dau Queue - khong su dung mutex
void clearQNode(Queue *q);
void freeQueue(Queue *q);

#endif
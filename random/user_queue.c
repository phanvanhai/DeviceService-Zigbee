#include "user_queue.h"

// A utility function to create a new linked list node.
static QNode* newNode(const char *str)
{
	QNode* temp = (QNode*)malloc(sizeof(QNode));
	temp->str = strdup(str);
	temp->next = NULL;
	
	return temp;
}

static void freeQNode (QNode *qn)
{	
	if( qn != NULL)
	{
		if( qn->str != NULL )
			free(qn->str);
	}
	free(qn);
}

// A utility function to create an empty queue
Queue* createQueue(uint16_t cap)
{	
	Queue* q = (Queue*)malloc(sizeof(Queue));
    q->mutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init (q->mutex, NULL);    
	q->cap	= cap;
	q->size = 0;
	q->front = q->rear = NULL;
	return q;
}

// The function to add a key k to q
void enQueue(Queue* q, const char *str)
{
	pthread_mutex_lock (q->mutex);
	// Create a new LL node
	QNode* temp = newNode(str);
	if( q->size == q->cap)
	{
		clearQNode(q);
	}	
	q->size++;
	// If queue is empty, then new node is front and rear both
	if (q->rear == NULL) 
	{
		q->front = q->rear = temp;
		pthread_mutex_unlock (q->mutex);
		return;
	}

	// Add the new node at the end of queue and change rear
	q->rear->next = temp;
	q->rear = temp;
	pthread_mutex_unlock (q->mutex);
}
void lock_queue(Queue *q)
{
	pthread_mutex_lock (q->mutex);
}
void unlock_queue(Queue *q)
{
	pthread_mutex_unlock (q->mutex);
}


char *readQNode(Queue *q)
{	
	if (q->front == NULL)
		return NULL;	
	return strdup(q->front->str);
}

 void clearQNode(Queue *q)
 {     
	if (q->front == NULL)
		return;
	QNode* temp = q->front;		
	// move front one node ahead	
	q->front = q->front->next;
	q->size--;
	// If front becomes NULL, then change rear also as NULL
	if (q->front == NULL)
	{
		q->rear = NULL;		
	}	
    
	freeQNode(temp);    
 }

uint16_t sizeQueue(Queue *q)
{
    pthread_mutex_lock (q->mutex);
    uint16_t size = q->size;
    pthread_mutex_unlock (q->mutex);
    return size;
}
// Function to remove a key from given queue q
char *deQueue(Queue* q)
{
	pthread_mutex_lock (q->mutex);
	// If queue is empty, return NULL.
	if (q->front == NULL)
	{
		pthread_mutex_unlock (q->mutex);
		return NULL;
	}
			
	// Store previous front and move front one node ahead
	QNode *temp = q->front;	
	char *result = strdup(temp->str);

	q->front = q->front->next;
	q->size--;
	// If front becomes NULL, then change rear also as NULL
	if (q->front == NULL)
	{
		q->rear = NULL;		
	}	
	freeQNode(temp);
	pthread_mutex_unlock (q->mutex);
	return result;
}

void freeQueue(Queue *q)
{
	if( q == NULL )
		return;
	while (q->front != NULL)
	{
		QNode* temp = q->front;
		q->front = q->front->next;
		freeQNode(temp);
	}
    free(q->mutex);
	free(q);	
}

#include <stdio.h>
#include<string.h>
#include <stdlib.h>
#include<pthread.h> 
#include <sys/queue.h> // queue taken from FreeBSD 10
// element = node
// field == nodes (field la con tro node)
// The data type for the node
struct node * e = NULL;
// khoi tao bien 
//pthread_mutex_t mutex_MetaData;
pthread_mutex_t mutex_Push;
pthread_mutex_t mutex_TX;
pthread_mutex_t mutex_RX;
pthread_mutex_t mutex_Gethandler;
pthread_mutex_t mutex_Discovery;
int thread_id;
pthread_t ThreadPush;
pthread_t ThreadTX;
pthread_t ThreadRX;

// int i = 0;
typedef struct node
{
    char *c;
    // This macro does the magic to point to other nodes
    TAILQ_ENTRY(node) nodes;

} node;

typedef TAILQ_HEAD(queue, node) queue;
// khoi tao hai queue
queue QueuePush,QueueTX,QueueRX,QueueGetHandler, QueueDiscovery;

void QueueInit()
{
    // khoi tao mutex
    pthread_mutex_init (&mutex_TX, NULL);
    pthread_mutex_init (&mutex_RX, NULL);
    pthread_mutex_init (&mutex_Push, NULL);
    pthread_mutex_init (&mutex_Gethandler, NULL);
    pthread_mutex_init (&mutex_Discovery, NULL);
   
    // khoi tao Queue
    TAILQ_INIT(&QueuePush);
    //TAILQ_INIT(&MetaData);
    TAILQ_INIT(&QueueTX);
    TAILQ_INIT(&QueueRX);
    TAILQ_INIT(&QueueGetHandler);
    TAILQ_INIT(&QueueDiscovery);

}

int  AddNode(queue *add,char * str,pthread_mutex_t * mutex)
{
    pthread_mutex_lock (mutex);
    e = malloc(sizeof(struct node));       
    if (e == NULL)
    {
        fprintf(stderr, "malloc failed");
        pthread_mutex_unlock (mutex);
        return 0;
    }
    e->c = strdup(str);
    TAILQ_INSERT_TAIL(add, e, nodes);
    // printf("Dang add %s---\n",str);
    e = NULL;
    pthread_mutex_unlock (mutex);
    return 1;
}
int TakeNode(queue *take,char * str,pthread_mutex_t * mutex)
{
    pthread_mutex_lock (mutex);
    if(!TAILQ_EMPTY(take))
    {
        e=TAILQ_FIRST(take); 
        strcpy(str,e->c);
        TAILQ_REMOVE(take,e,nodes);
        free(e->c);
        free(e);
        // printf("Dang take %s--\n",str);
        e=NULL;
        pthread_mutex_unlock (mutex);
        return 1;

    }
    else
    {
        pthread_mutex_unlock (mutex);;
        return 0;

    }
}
void DeleteAllNode(queue *head,pthread_mutex_t *mutex)
{
    pthread_mutex_lock (mutex);
    while (!TAILQ_EMPTY(head))
    {
        e = TAILQ_FIRST(head);
        TAILQ_REMOVE(head, e, nodes);
        free(e->c);
        free(e);
        e = NULL;
    }
    pthread_mutex_unlock (mutex);
}

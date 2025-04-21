#include <pthread.h>

typedef int DataType;

typedef struct node {
  DataType value;
  struct node* next;
} LNode, *LinkList;

typedef struct {
  LinkList head;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} list_lock_t;

void listInit(list_lock_t* list);
void producer(list_lock_t* list, DataType value);
void consumer(list_lock_t* list);
int getListSize(list_lock_t* list);
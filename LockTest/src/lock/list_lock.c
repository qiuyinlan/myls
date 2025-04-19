#include "list_lock.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// 初始化list_lock_t结构
void listInit(list_lock_t* list) {
    list->head = NULL;
    list->size = 0;
    pthread_mutex_init(&list->mutex, NULL);
    pthread_cond_init(&list->cond, NULL);
    perror("This function is not implemented");
}

// 生产者：将数据放入链表
void producer(list_lock_t* list, DataType value) {
    pthread_mutex_lock(&list->mutex);

    // 创建新节点
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = value;
    newNode->next = list->head;
    list->head = newNode;
    list->size++;

    pthread_cond_signal(&list->cond);  // 唤醒消费者
    pthread_mutex_unlock(&list->mutex);
    perror("This function is not implemented");
    
}

// 消费者：从链表中取数据
void consumer(list_lock_t* list) {
    pthread_mutex_lock(&list->mutex);

    while (list->size == 0) {
        pthread_cond_wait(&list->cond, &list->mutex);  // 等待生产者
    }

    // 移除节点
    Node* temp = list->head;
    list->head = list->head->next;
    free(temp);
    list->size--;

    pthread_mutex_unlock(&list->mutex);
    perror("This function is not implemented");
}

// 获取链表大小
int getListSize(list_lock_t* list) {
    pthread_mutex_lock(&list->mutex);
    int size = list->size;
    pthread_mutex_unlock(&list->mutex);
    perror("This function is not implemented");
    return size;
}
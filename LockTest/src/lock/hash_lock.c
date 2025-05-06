#include "hash_lock.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// 初始化hash_lock_t结构
void hashInit(hash_lock_t* bucket) {
    for (int i = 0; i < HASHNUM; i++) {
        bucket->table[i].head = NULL;
        pthread_mutex_init(&bucket->table[i].mutex, NULL);
    }
}

// 通过key获取对应的value
int getValue(hash_lock_t* bucket, int key) {
    int hashIndex = HASH(key);
    pthread_mutex_lock(&bucket->table[hashIndex].mutex);

    Hlist current = bucket->table[hashIndex].head;
    while (current != NULL) {
        if (current->key == key) {
            int value = current->value;
            pthread_mutex_unlock(&bucket->table[hashIndex].mutex);
            return value;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&bucket->table[hashIndex].mutex);
    return -1;  // Key not found
}

// 向哈希桶中添加一个节点
void insert(hash_lock_t* bucket, int key, int value) {
    int hashIndex = HASH(key);
    pthread_mutex_lock(&bucket->table[hashIndex].mutex);

    Hlist current = bucket->table[hashIndex].head;
    while (current != NULL) {
        if (current->key == key) {
            current->value = value;  // Update existing key
            pthread_mutex_unlock(&bucket->table[hashIndex].mutex);
            return;
        }
        current = current->next;
    }

    // Insert new node
    Hnode* newNode = (Hnode*)malloc(sizeof(Hnode));
    newNode->key = key;
    newNode->value = value;
    newNode->next = bucket->table[hashIndex].head;
    bucket->table[hashIndex].head = newNode;

    pthread_mutex_unlock(&bucket->table[hashIndex].mutex);
}

// 重新设置一个节点的key
int setKey(hash_lock_t* bucket, int key, int new_key) {
    int oldIndex = HASH(key);
    int newIndex = HASH(new_key);

    pthread_mutex_lock(&bucket->table[oldIndex].mutex);
    Hlist prev = NULL;
    Hlist current = bucket->table[oldIndex].head;

    while (current != NULL) {
        if (current->key == key) {
            if (prev != NULL) {
                prev->next = current->next;
            } else {
                bucket->table[oldIndex].head = current->next;
            }
            pthread_mutex_unlock(&bucket->table[oldIndex].mutex);

            // Move node to new index
            pthread_mutex_lock(&bucket->table[newIndex].mutex);
            current->key = new_key;
            current->next = bucket->table[newIndex].head;
            bucket->table[newIndex].head = current;
            pthread_mutex_unlock(&bucket->table[newIndex].mutex);

            return 0;  // Success
        }
        prev = current;
        current = current->next;
        
    }

    pthread_mutex_unlock(&bucket->table[oldIndex].mutex);
    return -1;  // Key not found
}
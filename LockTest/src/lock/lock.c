#include <pthread.h>
#include "lock.h"
#include <stdio.h>

// 初始化lock_t结构
void amountInit(lock_t* account) {
    account->amount = 0;  // 初始化金额为0
    pthread_mutex_init(&account->mutex, NULL);  // 初始化互斥锁
   perror("This function is not implemented");
}

// 进行资金收入操作
void Income(lock_t* account, int amount) {
    pthread_mutex_lock(&account->mutex);  // 加锁
    account->amount += amount;  // 增加金额
    pthread_mutex_unlock(&account->mutex);  // 解锁
    perror("This function is not implemented");  // 保留原有报错信息
}

// 进行资金支出操作
void Expend(lock_t* account, int amount) {
    pthread_mutex_lock(&account->mutex);  // 加锁
    account->amount -= amount;  // 减少金额
    pthread_mutex_unlock(&account->mutex);  // 解锁
    perror("This function is not implemented");  // 保留原有报错信息
}

// 销毁互斥锁
void amountDestroy(lock_t* account) {
    pthread_mutex_destroy(&account->mutex);  // 销毁互斥锁
}
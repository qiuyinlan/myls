#include <iostream>
#include <pthread.h>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
using namespace std;
class Threadpool
{
    public:
    static mutex printMutex; 
    //pthread_t thread[p_num];
    Threadpool(size_t p_num):stop(false),pendingTasks(0)
    {
        for(int i = 0;i < p_num;i++)
        {
            pthread_t thread;
     pthread_create(&thread, nullptr, &Threadpool::workerFunction, this);
     workers.push_back(thread);
        }
    }
    ~Threadpool()
    {
        
        {
        unique_lock<std::mutex> lock(queueMutex);
        stop = true;
        condition.notify_all();
        }
        for(pthread_t &worker : workers)
        {
            pthread_join(worker,nullptr);
        }
    }

    template <class F>
    void enqueue(F&& f)
    {
        unique_lock<std::mutex> lock(queueMutex);
        if(stop)
        {
            throw std::runtime_error("ThreadPool has been stopped");
        }
        ++pendingTasks;
        taskQueue.emplace(forward<F>(f));
    
        condition.notify_one();
    }

    void wait()
{
    unique_lock lock(queueMutex);
    condition.wait(lock, [this] {
        return taskQueue.empty() && pendingTasks == 0;
    });
}

    private:
    static void* workerFunction(void* arg)
    {
        Threadpool* pool = static_cast<Threadpool*>(arg);
        while(true)
        {
            function<void()> task;
            {
             unique_lock<mutex> lock(pool->queueMutex);
             pool->condition.wait(lock, [pool] {
                return pool->stop || !pool->taskQueue.empty();
                });
                
                if (pool->stop && pool->taskQueue.empty())
                return nullptr;
                
                task = move(pool->taskQueue.front());
                pool->taskQueue.pop();

            }
            task();
            --pool->pendingTasks;
            if (pool->pendingTasks == 0 && pool->taskQueue.empty())
            {
                pool->condition.notify_all();
            }
        }
        return nullptr;

    }
    private:
vector<pthread_t> workers; // 工作线程
queue<function<void()>> taskQueue; // 任务队列
mutex queueMutex; // 保护任务队列的互斥锁
condition_variable condition; // 条件变量，用于线程同步
atomic<bool> stop; // 标志，表示是否停止线程池
// static mutex printMutex; 
atomic<int> pendingTasks; // 记录未完成的任务数量

};
mutex Threadpool::printMutex;
void jiecheng(int start,int &result)
{
    result = 1;
    for(int i = 1;i <= start;i++)
    {
        result = i * result;
    }
}
void printTask(int i) {
    lock_guard<mutex> lock(Threadpool::printMutex);
    
    //cout << "Task " << id << " is being executed by thread " << pthread_self() << endl;
    int result;
    jiecheng(i,result);
    cout << i << "的阶乘为" << result << endl;
    }
    int main()
    {
        int p_num = 11;
        Threadpool pool(p_num);
        
    
        for (int i = 1; i < p_num; i++)
        
        {
            pool.enqueue([i] {
                printTask(i);
            });
        }
    
        pool.wait();
        cout << "所有任务完成，程序退出。" << endl;
        return 0;
    }
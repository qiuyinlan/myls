#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

// 任务队列
std::queue<std::function<void()>> taskQueue;
std::mutex queueMutex;
std::condition_variable condition;
bool stop = false;

// 线程池
void workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [] { return !taskQueue.empty() || stop; });

            if (stop && taskQueue.empty())
                return;

            task = std::move(taskQueue.front());
            taskQueue.pop();
        }
        task();
    }
}

void addTask(const std::function<void()>& task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        taskQueue.push(task);
    }
    condition.notify_one();
}

void shutdownThreadPool(std::vector<std::thread>& threads) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &thread : threads) {
        if (thread.joinable())
            thread.join();
    }
}

int main() {
    const int numThreads = 10;
    std::vector<std::thread> threads;

    // 创建线程池
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(workerThread);
    }

    // 添加任务
    for (int i = 0; i < 20; ++i) {
        addTask([i] {
            std::cout << "Task " << i << " is being processed by thread " << std::this_thread::get_id() << std::endl;
        });
    }

    // 关闭线程池
    shutdownThreadPool(threads);

    return 0;
}

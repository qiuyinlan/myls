#include <chrono>
#include <random>
#include <thread>

class thread_guard {

 public:
  explicit thread_guard(std::thread& _t) : t(_t) {}

  ~thread_guard() {
    if (t.joinable()) t.join();
  }

  thread_guard(const thread_guard&) = delete;
  thread_guard& operator=(const thread_guard&) = delete;

  void static sleep() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(1, 100);
    int SleepTime = distribution(gen);
    std::this_thread::sleep_for(std::chrono::milliseconds(SleepTime));
  } 

 private:
  std::thread& t;
};

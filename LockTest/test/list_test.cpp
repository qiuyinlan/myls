#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "test.h"

TEST_F(ListLockTest, InitTest) {
  ASSERT_TRUE(list_->head == nullptr) << "List is not initialized...";

  ASSERT_EQ(pthread_mutex_trylock(&list_->mutex), 0)
      << "Lock is not initialized";
  pthread_mutex_unlock(&list_->mutex);

  ASSERT_EQ(pthread_cond_signal(&list_->cond), 0) << "cond is not initialized";
}

TEST_F(ListLockTest, BasicTest) {
  std::vector<std::thread> pro_threads;
  std::vector<std::thread> con_threads;

  for (int tid = 0; tid < 4; tid++) {
    std::thread t(producer, list_.get(), tid);
    pro_threads.push_back(std::move(t));
  }

  for (auto& t : pro_threads) {
    t.join();
  }

  ASSERT_EQ(getListSize(list_.get()), 4);

  for (int tid = 0; tid < 4; tid++) {
    std::thread t(consumer, list_.get());
    con_threads.push_back(std::move(t));
  }

  for (auto& t : con_threads) {
    t.join();
  }
  ASSERT_EQ(getListSize(list_.get()), 0);
  ASSERT_TRUE(list_->head == nullptr) << "Resources not released";
}

TEST_F(ListLockTest, MixedConcurrentTest) {
  std::vector<std::thread> threads;
  const int keys_per_thread = 10;
  auto& list = list_;

  for (int tid = 0; tid < 4; tid++) {
    std::thread t([&list, tid] {
      for (uint32_t i = 0; i < keys_per_thread; i++) {
        producer(list.get(), tid * i);
      }
      for (uint32_t i = 0; i < keys_per_thread; i++) {
        consumer(list.get());
      }
      for (uint32_t i = 0; i < keys_per_thread; i++) {
        producer(list.get(), tid * i);
      }
    });
    threads.push_back(std::move(t));
  }

  for (auto& t : threads) {
    t.join();
  }

  ASSERT_EQ(getListSize(list.get()), 4 * keys_per_thread);

  std::thread conthread([&list] {
    for (int i = 0; i < 4* keys_per_thread; i++) {
      consumer(list.get());
    }
  });

  conthread.join();

  ASSERT_EQ(getListSize(list.get()), 0);
  ASSERT_TRUE(list->head == nullptr);
}

TEST_F(ListLockTest, LongTimeTest) {
  std::shared_ptr<std::atomic_bool> stop =
      std::make_shared<std::atomic_bool>(false);
  auto& list = list_;

  std::thread pro_thread([&list, stop] {
    while (!stop->load()) {
      producer(list.get(), 1);
      RandomSleep();
    }
  });

  std::thread con_thread([&list, stop] {
    while (!stop->load() || getListSize(list.get()) != 0) {
      consumer(list.get());
      RandomSleep();
    }
  });

   std::this_thread::sleep_for(std::chrono::seconds(5));

  stop->store(true);

  pro_thread.join();
  con_thread.join();

  ASSERT_EQ(getListSize(list.get()), 0);
  ASSERT_TRUE(list_->head == nullptr);
}
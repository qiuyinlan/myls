![GitHub License](https://img.shields.io/github/license/tiny-sky/LockTest?style=flat&logo=linux&color=fedcba&link=https%3A%2F%2Fcreativecommons.org%2Flicenses%2Fby-sa%2F4.0%2F)

# OVERVIEW

LockTest 是一个涉及 Linux 系统编程的多线程并发同步任务。

编程语言：C语言。

知识点：多线程并发模型、锁、条件变量、数据结构等。

开发环境：如果使用 VSCode，确保已经下载 CMake、C/C++ Extension Pack 和 clangd 插件。

或者你可以手动下载

Ubuntu 用户

```sh
sudo apt-get install cmake clangd
```

Arch Linux 用户

```sh
yay -S cmake clangd
```

# Build

clone 该仓库到本地

要从命令行构建测试环境，请执行以下命令：

```sh
mkdir build
cd build
cmake ..
```

# Task Directions

## lock_test

涉及范围：

- 简单锁的使用

文档说明：[lock_test](./doc/lock_test/lock_test.md)

## list_lock

涉及范围：

- 互斥锁的使用
- 条件变量的使用
- 生产者与消费者模型

文档说明: [list_lock](./doc/list_test/list_test.md)

## hash_lock

涉及范围：

- 多把互斥锁的使用
- 哈希桶与哈希函数

文档说明: [hash_lock](./doc/hash_test/hash_test.md)

# Testing

我们将 GTest 用于单元测试用例，你可以从命令行单独编译和运行每一个测试：

```sh
cd build
make lock_test -j$(nproc)
make list_test hash_test -j$(nproc)
./test/lock_test
./test/list_test
./test/hash_test
```

默认的编译模式为: Debug Mode，这样你将会看到更详细的错误信息

这将会启用 [AddressSanitizer](https://github.com/google/sanitizers)

如果你想启用其他的 Sanitizers

```
cmake -DLOCK_SANITIZER=thread ..
make -j$(nproc)
```

# Error Report

在 Debug Mode 下，如果出现内存错误，你将会看到内存错误报告

你可以使用 Valgrind 来定位出错位置

```sh
valgrind --tool=memcheck --leak-check=full ./test/lock_test 
```

你也可以使用 Valgrind 来检测多线程死锁问题

```sh
valgrind --tool=helgrind ./test/lock_test 
```

# Submission

开始时间：

截至时间：

提交要求：

# Declaration

- 每一位同学都应该独立完成这项任务
- 你可以和其他人讨论这项任务的细节
- 禁止复制抄袭他人代码，或让 ChatGPT 生成代码。
- 在这项任务中，你可以在 Google 上搜索，或者询问 ChatGPT 还未掌握的知识。

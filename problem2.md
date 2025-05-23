#include<stdio.h>
什么情况下会明显看到延迟
延迟在以下情况下会变得明显：

1. 长时间运行的交互式程序

fflush

#include <stdio.h>
#include <unistd.h>

int main() {
printf("请等待系统准备就绪");  // 没有\n
sleep(3);  // 暂停3秒

char input[100];
scanf("%s", input);  // 等待用户输入

return 0;
}
在这个例子中，用户可能会面对空白屏幕3秒，不知道程序在等待什么。

# argv，argc

argc命令行参数个数，argv字符指针数组，指针指向每一个命令行参数
char*  指向字符型类型的指针
char** 指向字符指针的指针

argv：传递给Shell程序本身的参数
args：用户在Shell中输入的命令参数
args通常用于程序内部的函数参数，表示一个通用的参数数组

# io

系统io直接调用系统操作的系统调用
标准io,和用户交互有关

# 为什么redirect_io函数使用指针的指针作为参数

# 缓冲区指针的作用与原理

缓冲区指针(_buf)是FILE结构中非常重要的一个组件，它指向一块内存区域，用于临时存储从文件读取的数据或等待写入文件的数据。
当您调用fgetc()等读取函数时，系统不会每次都从硬盘读取一个字符
而是一次性从硬盘读取一大块数据（如8KB）到缓冲区
然后从缓冲区中一个字符一个字符地返回给程序
当缓冲区中的数据用完后，再次从硬盘读取一块数据到缓冲区

# fp

FILE指针确实是一个真正的指针，它指向一个FILE结构体变量
FILE结构体的封装特性
结构体名称的隐藏
FILE结构体的具体实现在C标准库内部
我们只知道它的类型名是"FILE"，但不知道具体实例的变量名
这些变量通常是在库函数内部创建的，没有暴露给用户的变量名
访问限制
我们不能直接访问FILE结构体的内部成员
例如，我们不能写fp->_fd或fp->_buf
必须通过标准库函数如fread(), fwrite(), fseek()等间接操作
抽象接口
C标准只定义了FILE类型和操作它的函数
不同的C库实现可能有不同的FILE结构体内部结构
程序员只需要知道如何使用FILE指针，而不需要了解其内部细节
类比说明
这就像使用遥控器控制电视：
遥控器 = FILE指针
电视内部电路 = FILE结构体
我们知道按下遥控器上的按钮会产生什么效果

为什么这样设计很好
这种封装设计有几个优点：
简化使用
程序员不需要了解复杂的文件系统细节
只需要掌握几个简单的函数接口
提高可移植性
不同操作系统的文件操作细节可能完全不同
封装这些差异使得同一段代码可以在不同系统上运行

每个打开的文件有自己的FILE结构体
之前的疑惑的关键在没有去了解文件类型结构体的成员，他成员里有缓冲区指针，文件描述符，文件当前位置，文件长度等

从文件名到文件描述符
当您调用fopen("test.txt", "r")时，发生了以下步骤：

1. C标准库调用操作系统的open()系统调用（在Linux/Unix中）
   操作系统根据文件路径在文件系统中查找文件
   找到文件后，操作系统创建一个文件描述符（一个整数，如3）
   操作系统在内核中维护一个表，将这个文件描述符与文件的物理位置关联起来
2. 文件描述符返回给C标准库，
3. C标准库创建FILE结构，将文件描述符存储在_fd字段中
   返回指向这个FILE结构的指针

# 文件打开的本质原因

资源分配和初始化
操作系统需要分配内存和资源来管理文件访问
需要创建数据结构来跟踪文件位置和状态
权限检查
操作系统需要验证您是否有权限读取或写入该文件
检查文件是否存在、是否被锁定等
文件定位
操作系统需要将文件名转换为实际的物理位置
需要加载文件系统元数据来找到文件
并发控制
多个程序可能同时访问同一个文件
打开操作建立了访问控制机制

#文件
= 保险箱中的物品
打开文件 = 验证身份并获取保险箱钥匙
读写文件 = 存取保险箱中的物品
关闭文件 = 锁上保险箱并归还钥匙
你不能直接拿走保险箱里的东西，必须先通过身份验证，获取访问权限。

# 文件描述符是关键的中间环节：

它是操作系统用来跟踪打开文件的数字ID
ILE指针指向内存中的FILE结构
FILE结构包含文件描述符，这是操作系统分配的文件标识
文件描述符是操作系统内部表的索引，该表将描述符映射到实际文件
操作系统负责将文件描述符转换为硬盘上的物理位置
5. 当您通过FILE指针操作文件时，实际上是通过多层抽象间接访问硬盘上的文件

# 文件与普通变量的区别

文件与普通变量的本质区别

1. 存储位置不同
   普通变量：存储在内存中，CPU可以直接访问
   文件：存储在外部设备(硬盘)上，CPU不能直接访问
2. 访问速度差异巨大
   内存访问：纳秒级(10^-9秒)
   硬盘访问：毫秒级(10^-3秒)，比内存慢约100万倍
3. 访问方式不同
   变量：通过内存地址直接访问
   文件：需要通过操作系统和驱动程序间接访问
4. 生命周期不同
   普通变量：程序结束后消失
   文件：持久存储，程序结束后仍然存在

普通变量就像你口袋里的笔记本，随时可以拿出来看或写
文件则像是存放在图书馆的书籍：
需要登记才能借阅(权限控制)
一次借多本更有效率(缓冲区)
多人可能想借同一本书(并发控制)
借书和还书有规定流程(打开和关闭)

文件操作与普通变量操作的不同设计源于它们的本质差异：
文件是外部设备上的持久数据，访问慢且复杂
变量是内存中的临时数据，访问快且简单
文件可能被多个用户/程序共享，需要控制访问
文件访问需要性能优化，因此使用缓冲区

# pipe

pipe_pos变量初始化为-1，表示"未找到"或"无效索引"
《C程序设计语言》书中第4章和第5章中有多处应用

if (strcmp(args[i], "|") == 0) {
pipe_pos = i;，为什么等于i

## 为什么要记录管道位置

变量i在这里是循环索引，表示当前正在检查的参数位置。当找到管道符号时，i正好是该符号在参数数组中的索引。

## pipe函数

pipe() 函数用于创建一个管道，pipefd 是它的参数，用于存储管道的两个文件描述符
里面传的是数组

#进程控制（exec函数族）
通过 execv，你可以在当前进程中执行其他程序，并通过 args 传递命令行参数给新程序。

==开始学==

# 全局变量

pwd.h：提供与用户信息相关的功能，常用于获取用户的用户名、主目录等。
工作目录：是进程的一个属性，表示当前操作的目录。可以通过 getcwd() 获取，并通过 chdir() 更改。
执行文件的当前目录：是程序启动时所在的目录，可能与工作目录相同，但可以在程序运行过程中更改工作目录

shell里面 cd啥啥是基础命令要自己写的

## 为什么使用全局变量

这些变量需要在多个函数中访问和修改。例如，cmd_cd 函数需要更新这两个变量，而 print_prompt 函数需要读取 current_dir 来显示当前目录。
状态保持：它们用于保持Shell的状态信息，确保在整个Shell的生命周期中，目录信息是一致的。
简化代码：使用全局变量可以避免在每个需要访问这些信息的函数中传递参数，简化了代码结构。

# 信号

SIGCHLD：
含义：当一个子进程终止、暂停或继续时，父进程会收到 SIGCHLD 信号。
忽略原因：通过忽略 SIGCHLD，父进程不需要显式地处理子进程的终止状态。这可以防止子进程变成僵尸进程，因为系统会自动清理它们。
2. SIGQUIT：
含义：SIGQUIT 信号通常由用户通过键盘（Ctrl+\）发送，用于终止进程并生成核心转储。
忽略原因：忽略 SIGQUIT 可以防止用户通过键盘中断Shell的执行，保持Shell的稳定性。
SIGTERM：
含义：SIGTERM 是一个终止信号，通常用于请求程序正常退出。它是一个可以被捕获、忽略或处理的信号。
忽略原因：通过忽略 SIGTERM，Shell可以防止被外部进程或用户意外终止。这有助于保持Shell的持续运行，除非用户明确使用 exit 命令退出。

控制：通过忽略或处理信号，Shell可以更好地控制其行为和生命周期

## 如何处理僵尸进程

1. 忽略 SIGCHLD：
   自动清理：在 Linux 系统中，忽略 SIGCHLD 信号会让内核自动清理子进程的资源，避免僵尸进程的产生。这是因为内核会在子进程终止时自动释放其资源。
   不忽略 SIGCHLD：
   手动处理：如果不忽略 SIGCHLD，你需要在父进程中捕获这个信号，并使用 wait() 或 waitpid() 来手动清理子进程的资源。

### sigint

signal interupt
SIGINT 通常由用户通过键盘组合键 Ctrl+C 触发。这个信号会发送给当前在前台运行的进程组

### getcwd

char *getcwd(char *buf, size_t size);
参数，，它将路径存储在 buf 中，并确保路径以空字符 \0 结尾。
char *buf：指向一个字符数组的指针，用于存储当前工作目录的路径。
size_t size：buf 的大小，以字节为单位。
返回值
成功时，返回 buf，即当前工作目录的路径。
失败时，返回 NULL，并设置 errno 以指示错误

错误处理：
如果 buf 的大小不足以存储完整路径，getcwd 将返回 NULL，并设置 errno 为 ERANGE。
如果发生其他错误（如无权限访问当前目录），getcwd 也会返回 NULL，并设置相应的 errno。
动态分配：
在 GNU 扩展中，如果 buf 为 NULL，getcwd 会使用 malloc 动态分配足够大的缓冲区来存储路径。调用者需要负责释放这个缓冲区。
4. 常见用途：
在命令行程序中显示当前目录。
在文件操作中获取当前目录以便于相对路径的处理。
通过 getcwd，程序可以动态获取当前工作目录的信息，这在需要处理文件路径或显示当前目录时非常有用。

## strtok

strtok(cmd, "|") 函数将字符串 cmd 按管道符 | 分割，返回第一个子字符串

strcspn 是 "string complement span" 的缩写，它是 C 标准库中的一个字符串处理函数。
函数功能
strcspn 函数用于计算字符串中连续不包含指定字符集合中任何字符的最大长度

> 当我们调用 strcspn(cmd, "\n") 时：
> 函数扫描 cmd 字符串
> 寻找第一个 \n 字符
> 返回从字符串开头到第一个 \n 之前的字符数

cmd[strcspn(cmd, "\n")] = 0; 的作用是：
找到第一个换行符的位置（或者如果没有换行符，则是字符串的末尾）
在该位置放置一个空字符 \0
数组元素改变

# 管道处理

在使用 strtok 函数分割字符串时，它会将分隔符（在这个例子中是管道符 |）替换为 \0（NULL 字符）。这样做有几个重要原因：

1. 创建独立的子字符串
   将管道符替换为 NULL 字符可以将原始字符串分割成多个独立的、以 NULL 结尾的子字符串。在 C 语言中，每个字符串都必须以 NULL 字符结尾才能被正确处理

- 管道的本质
  管道本质上是一个内存中的缓冲区，有读端和写端
  这两端分别用文件描述符表示，一个用于读取数据，一个用于写入数据

数据写入过程
当进程调用 write(pipefd[1], data, size) 时：
系统调用转换：
用户程序通过 write() 系统调用进入内核态
内核识别文件描述符 pipefd[1] 属于管道
数据复制：
内核将用户空间的数据 data 复制到内核空间的管道缓冲区
复制的数据量为 size 字节（或直到缓冲区满）
指针更新：
内核更新管道缓冲区的写入指针，指向下一个可写位置
如果缓冲区已满，进程会被阻塞，直到有空间可写
唤醒读取者：
如果有进程正在等待读取数据，内核会唤醒它们
数据读取过程
当进程调用 read(pipefd[0], buffer, size) 时：系统调用转换：
用户程序通过 read() 系统调用进入内核态
内核识别文件描述符 pipefd[0] 属于管道
数据复制：
内核将管道缓冲区中的数据复制到用户空间的 buffer
复制的数据量为请求的 size 字节或实际可用数据量（取较小值）
指针更新：
内核更新管道缓冲区的读取指针，指向下一个待读位置
如果缓冲区为空，进程会被阻塞，直到有数据可读
唤醒写入者：
如果有进程因缓冲区满而被阻塞，内核会唤醒它们

管道是一个先进先出（FIFO）的数据结构：
数据按照写入的顺序存储在管道缓冲区中
读取操作会按照相同的顺序取出数据

```C
int pipefd[2];
    char buffer[20];
  
    pipe(pipefd);  // 创建管道
  
    // 写入数据
    const char *message = "Hello, Pipe!";
    write(pipefd[1], message, strlen(message));
  
    // 读取数据
    int bytes_read = read(pipefd[0], buffer, sizeof(buffer)-1);
    buffer[bytes_read] = '\0';  // 添加字符串结束符
  
    printf("读取的数据: %s\n", buffer);
  
    close(pipefd[0]);
    close(pipefd[1]);
```

# execv

使用 execvp（自动搜索 PATH）
错误处理：只有在失败时才会返回 -1，并设置 errno

```C
char *args[] = {"ls", "-l", NULL};
execvp(args[0], args);  // 系统会在 PATH 中查找 "ls"
```

系统首先查找 args[0] 指定的程序
找到程序后，将整个 args 数组作为参数传递给该程序
在新程序中，args[0] 通常作为程序名，args[1] 开始是实际的命令行参数

使用 execve（需要完整路径和环境变量

```C
char *args[] = {"ls", "-l", NULL};
char *env[] = {"HOME=/home/user", "PATH=/bin:/usr/bin", NULL};
execve("/bin/ls", args, env);  // 必须指定完整路径和环境变量
```

execvp：使用当前进程的环境变量
execve：允许你指定新的环境变量

# return

返回值：return 1;
在 shell 程序中，返回值通常表示执行状态
返回 1 通常表示"继续执行 shell"，而不是退出
这不是表示错误，而是一种控制流程的方式

# io

标准io  stdio.h
fopen  fp file pointer'
系统io
open  fd  file descriptor

# write

#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t count);

# shell 命令

env: 显示所有环境变量
export: 设置环境变量

# return 值的考量

0,已经执行，不需要进一步处理
1,需要进一步处理

# 工作目录记载

static char prev_dir[MAX_DIR_LEN] = "";
初始化！！！

# main

突出程序入口：
将 main 放在最后，可以让它成为整个程序的"结论"部分
读者阅读完所有函数后，最后看到的是程序如何启动和组织这些函数

0644的含义是：
第一位0：表示这是一个八进制数
第二位6：所有者权限 = 4(读) + 2(写) + 0(不可执行) = 6
第三位4：所属组权限 = 4(读) + 0(不可写) + 0(不可执行) = 4
第四位4：其他用户权限 = 4(读) + 0(不可写) + 0(不可执行) = 4

open()函数的第三个参数：
只有当open()函数使用了O_CREAT标志（即创建新文件）时，才需要指定这个权限参数
如果只是打开已存在的文件，这个参数可以省略

args
argument
argv"是"argument vector"（参数向量）的缩

rest：剩余部分

strtok 函数
strtok 是C标准库中的一个函数，用于将字符串分割成多个部分（标记）。它的原型是：
参数解释
第一个参数 rest：
指向要分割的字符串
第一次调用时，提供要分割的完整字符串
后续调用时，提供 NULL，表示继续处理之前的字符串
第二个参数 " \t"：
这是分隔符字符串，包含空格和制表符（\t）
任何这些字符都会被视为标记之间的分隔符

> 函数工作原理
> strtok 会在字符串中查找分隔符
> 找到分隔符后，会用 '\0' 替换它，将原字符串分割成多个子字符串
> 返回指向当前标记开始位置的指针
> 内部保存一个静态指针，指向下一个要处理的位置
> 当没有更多标记时，返回 NULL

当我说它"返回 -l"时，我是在描述这个指针指向的内容，而不是返回值的类型。
第一次调用 token = strtok(str, " \t")：
strtok 从字符串开始查找，找到第一个非分隔符字符 'l'
继续查找直到遇到分隔符 ' '
将这个分隔符替换为 '\0'
内存现在变成：['l','s','\0','-','l',' ','/','h','o','m','e','\0']
返回指向 'l' 的指针，即字符串 "ls" 的起始位置,==又因为之后的空格会变成null==,所以从指针到那个地方，等于就是ls,就对应到了ls

> 当调用 strtok(NULL, delimiters) 时，第一个参数为 NULL 的含义是：
> 不是新字符串：告诉函数"我想继续处理上一次的字符串"
> 使用保存的位置：函数应该从上次停止的位置继续
> 这是一个信号：区分第一次调用和后续调用

> 直接在原始字符串中查找特殊字符可能导致歧义
> 例如：echo "a > b"中的>不是重定向符号
> 通过先分词，可以正确区分这些情况

分两步处理（先分词，再识别特殊字符）是shell解析的标准方法，因为它：
符合shell语法的设计理念
简化了处理逻辑
正确处理引号和转义
避免了歧义
更容易扩展和维护

at < input.txt 是一个使用输入重定向的shell命令。让我解释它的含义和工作原理：
命令含义
这个命令的作用是：读取文件 input.txt 的内容并显示在终端上。
组成部分

1. cat：
   这是一个Unix/Linux命令，名称来源于"concatenate"（连接）
   基本功能是读取文件内容并输出到标准输出（通常是终端）
   <：
   这是输入重定向符号
   它告诉shell将命令的标准输入从键盘改为指定的文件

at < input.txt > output.txt 是一个同时使用输入和输出重定向的shell命令。
命令含义
这个命令的作用是：将 input.txt 的内容复制到 output.txt。
组成部分
cat：读取标准输入并写入标准输出的命令
< input.txt：输入重定向，将 input.txt 作为 cat 的标准输入

> output.txt：输出重定向，将 cat 的标准输出写入 output.txt

\>：从这里输入的意思
\<：从这里输出的意思

> 个 return 语句没有指定返回值
> 它只是结束函数执行
> 函数的返回类型应该是 void（无返回值）
> 如果函数需要返回成功/失败状态，通常会使用：
> return 0; 表示成功
> return 1; 或其他非零值表示失败
> 但这里的 return; 不返回任何值，只是结束函数执行。

void函数在以下情况下使用：
执行操作而非计算值：
当函数的主要目的是执行某些操作（如打印信息、修改状态）
而不是计算并返回一个值时
通过其他方式指示状态：
通过全局变量或参数指针传递状态
通过抛出异常（在支持异常的语言中）
通过设置errno（在C语言中）
不需要指示成功/失败：
当操作总是成功，或失败时会终止程序
或者成功/失败对调用者不重要

在shell程序中，命令处理函数通常是void，因为：
命令的结果直接显示给用户
错误会打印到屏幕
shell继续运行，等待下一个命令
这种设计使得shell能够在一个命令失败后继续运行，而不会因为一个命令的失败而终止整个shell。

> 学习啥时候用三目运算符
> char* target_dir = (args_count > 1) ? args[1] : getenv("HOME");

char* target_dir;
if (args_count > 1) {
target_dir = args[1];
} else {
target_dir = getenv("HOME");
}

为什么使用 strncpy 而不是 strcpy
strncpy 比 strcpy 更安全，因为它限制了复制的字符数，防止缓冲区溢出。如果源字符串比指定的最大长度长，strncpy 只会复制指定数量的字符。

if (in_file && redirect(in_file, O_RDONLY, STDIN_FILENO) < 0) {
exit(1);
如果指定了输入重定向文件（in_file 非NULL）
尝试将标准输入重定向到该文件
如果重定向失败，则终止进程并返回错误状态

# 不理解

基本标志设置：
O_WRONLY：以只写模式打开文件（Write-ONLY）
O_CREAT：如果文件不存在，则创建它
这两个标志使用位运算符|（按位或）组合

> if (input_fd != STDIN_FILENO)
> 检查input_fd是否与标准输入的文件描述符不同
> STDIN_FILENO是标准输入的文件描述符，通常为0
> 只有当input_fd不是标准输入时才需要重定向

dup2是一个系统调用，用于复制文件描述符
它将input_fd复制到STDIN_FILENO
这意味着任何读取标准输入的操作现在都会从input_fd指向的文件读取
如果STDIN_FILENO已经打开，它会先被关闭

# execvp

例如，当用户输入 ls -l /home：

args[0] 是 "ls"
args 数组是 {"ls", "-l", "/home", NULL}
调用 execvp ==执行命令==
execvp 会在 PATH 中查找 "ls" 程序并执行它，传递这些参数
如果 execvp 失败（例如，命令不存在），通常会打印错误消息并退出子进程。

# 父进程

```c
if (!run_back) {
            waitpid(child_pid, NULL, 0);
        }
    }
```

* 表明这段代码是在fork()之后的父进程中执行的
* 在shell中，父进程是shell本身，子进程用于执行命令

run\_back 是一个布尔变量，表示命令是否应该在后台运行

* waitpid(child\_pid, NULL, 0)：
* waitpid 是一个系统调用，用于等待指定的子进程结束
* child\_pid 是子进程的进程ID，在之前的 fork() 调用中获得
* NULL 表示不关心子进程的退出状态
* 0 表示等待选项，这里是阻塞等待（直到子进程结束才返回）

# 提示符

```c
printf("\033[1;36m%s@xShell:\033[1;33m%s\033[0m$ ", 
               user_info->pw_name, current_path);
        fflush(stdout);
```

* 在C语言中，标准输出（stdout）通常是行缓冲的
* 这意味着输出会被存储在缓冲区中，直到遇到换行符（\\n）或缓冲区满
* 如果不使用 fflush，提示符可能不会立即显示，而是留在缓冲区中
* 用户会看到一个空白终端，不知道shell已经准备好接受输入

fgets(input\_buffer, BUFFER\_SIZE, stdin)

从标准输入（键盘）读取一行文本

strcspn 是C标准库中的一个字符串处理函数，用于查找字符串中第一个匹配指定字符集合中任意字符的位置。

size_t strcspn(const char *str1, const char *str2);

strcspn 名称来源于 "string complement span"

strcspn(input\_buffer, "\\n") 返回字符串中第一个换行符的位置（如果没有换行符，则返回字符串长度）。将该位置的字符设为 0（即 \\0，字符串结束符）

cd -需要自己编写代码实现，因为：

* 它是shell的扩展功能，不是文件系统或操作系统的内置特性
* 它需要shell维护状态（记住上一个工作目录）
* 它需要特殊的参数处理逻辑

而cd ..不需要特殊实现，因为..是文件系统本身支持的特殊目录，chdir()函数可以直接处理。

* WIFEXITED(status)：
* 这是一个宏，用于检查子进程是否正常退出（而不是被信号终止）
* 如果子进程通过调用exit()或从main()函数返回而终止，则返回真（非零值）
* 如果子进程因为收到信号而终止（如被kill命令终止），则返回假（0）

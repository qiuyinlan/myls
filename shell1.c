#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>
#include <stdbool.h>

#define BUFFER_SIZE 2048
#define AR_MAX 128
#define PIPELINE_MAX 16
#define PATH_MAX 8192
#define DEFAULT_PERM 0644

// 信号
void no_signal() {
    // 忽略(Ctrl+C)
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("signal (SIGINT)");
        exit(1);
    }
    
}

// 重定向
int redirect(const char* filename, int mode, int target_fd) {
    int fd = open(filename, mode, DEFAULT_PERM);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    if (dup2(fd, target_fd) < 0) {
        perror("dup2");
        close(fd);
        return -1;
    }
    
    close(fd);
    
    return 0;
}

// 处理单条命令

void process_cmd(char* cmdstr, int input_fd, int output_fd, int run_back) {
    static char last_d[PATH_MAX] = ""; 
    char* args[AR_MAX];
    int args_count = 0;
    char *in_file = NULL, *out_file = NULL;
    int outputmode = 0;
    
    // 命令参数
    char* token;
    char* rest = cmdstr;
    
    while ((token = strtok(rest, " \t")) != NULL) {
        rest = NULL; 

        if (strcmp(token, "&") == 0) {
            run_back = 1;
        } 
        else if (strcmp(token, "<") == 0) {
            in_file = strtok(NULL, " \t");
            rest = NULL;
        } 
        else if (strcmp(token, ">") == 0) {
            out_file = strtok(NULL, " \t");
            rest = NULL;
            outputmode = 0;
        } 
        else if (strcmp(token, ">>") == 0) {
            out_file = strtok(NULL, " \t");
            rest = NULL;
            outputmode = 1;
        } 
        else {
            args[args_count++] = token;
        }
    }
    
    args[args_count] = NULL;
    
    // 跳过空
    if (args_count == 0) {
        return;
    }
    
    // 处理cd
    if (strcmp(args[0], "cd") == 0) {
        char current_path[PATH_MAX];
        if (getcwd(current_path, sizeof(current_path)) == NULL) {
            perror("getcwd");
            return;
        }
        
        char* target_dir;
    if (args_count > 1) {
            target_dir = args[1];
        }
    else {
            target_dir = getenv("HOME");
        }
        
        //cd -
        if (target_dir && strcmp(target_dir, "-") == 0) {
            if (last_d[0] == '\0') {
                fprintf(stderr, "cd: No previous directory\n");
                return;
            }
            target_dir = last_d;
        }
        
        if (chdir(target_dir) != 0) {
            perror("chdir");
            return;
        }
        
        // 保存上一个目录
        strncpy(last_d, current_path, PATH_MAX - 1);
        last_d[PATH_MAX - 1] = '\0';
        return;
    }
    
    // 子进程执行外部命令
    pid_t child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        return;
    }
    
    if (child_pid == 0) {
        // 输入重定向
        if (in_file && redirect(in_file, O_RDONLY, STDIN_FILENO) < 0) {
            exit(1);
        }
        
        // 输出重定向
        if (out_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= (outputmode ? O_APPEND : O_TRUNC);
            
            if (redirect(out_file, flags, STDOUT_FILENO) < 0) {
                exit(1);
            }
        }
        
        // 管道
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        // 执行
        execvp(args[0], args);
        fprintf(stderr, "execvp: Command not found: %s\n", args[0]);
        exit(1);
    } 
    else {
        // 父进程
        if (!run_back) {
            int status;
            waitpid(child_pid, &status, 0);
            
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) != 0) {
                    fprintf(stderr, "Command exited with status %d\n", WEXITSTATUS(status));
                }
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "Command terminated by signal %d\n", WTERMSIG(status));
            }
        } else {
            printf("[1] %d\n", child_pid);
        }
    }
}

// 启动shell
void x_shell() {
    char current_path[PATH_MAX];
    char input_buffer[BUFFER_SIZE];
    struct passwd* user_info;
    
    // 获取用户信息
    uid_t user_id = getuid();
    if ((user_info = getpwuid(user_id)) == NULL) {
        perror("getpwuid");
        exit(1);
    }
    
    //主循环
    while (true) {
        // 获取当前工作目录
        if (getcwd(current_path, PATH_MAX) == NULL) {
            perror("getcwd");
            exit(1);
        }
        
        // 显示提示符
        printf("\033[1;36m%s@xShell:\033[1;33m%s\033[0m$ ", 
               user_info->pw_name, current_path);
        fflush(stdout);
        
        // 读取用户输入
        if (!fgets(input_buffer, BUFFER_SIZE, stdin)) {
            break;
        }
        
        input_buffer[strcspn(input_buffer, "\n")] = 0;
        
    
        if (strlen(input_buffer) == 0) {
            continue;
        }
        
        // 管道命令
        char* command_segments[PIPELINE_MAX];
        int segment_count = 0;
        char input_copy[BUFFER_SIZE];
        
        
        strcpy(input_copy, input_buffer);
        
        // 分割管道命令
        char* segment = strtok(input_copy, "|");
        while (segment && segment_count < PIPELINE_MAX) {
            command_segments[segment_count++] = segment;
            segment = strtok(NULL, "|");
        }
        
        // 执行管道命令
        int input_descriptor = STDIN_FILENO;
        int pipe_fds[2];
        
        for (int i = 0; i < segment_count; i++) {
            // 为非最后一个命令创建管道
            if (i < segment_count - 1) {
                if (pipe(pipe_fds) < 0) {
                    perror("pipe");
                    break;
                }
            }
            
            // 执行当前命令段
            process_cmd(
                command_segments[i], 
                input_descriptor, 
                (i < segment_count - 1) ? pipe_fds[1] : STDOUT_FILENO,
                0
            );
            
            // 关闭已使用的文件描述符
            if (i > 0) {
                close(input_descriptor);
            }
            
            if (i < segment_count - 1) {
                close(pipe_fds[1]);
                input_descriptor = pipe_fds[0];
            }
        }
    }
}

int main() {

    no_signal();
    x_shell();
    
    return 0;
}
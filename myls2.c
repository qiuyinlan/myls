#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

// 文件类型枚举
typedef enum {
    FT_REGULAR,
    FT_DIRECTORY,
    FT_SYMLINK,
    FT_SPECIAL
} FileType;

// 显示选项结构
typedef struct {
    unsigned int flags;
    #define SHOW_HIDDEN   (1 << 0)
    #define SHOW_DETAIL   (1 << 1)
    #define SHOW_RECUR    (1 << 2)
    #define SORT_TIME     (1 << 3)
    #define SORT_REV      (1 << 4)
    #define SHOW_INODE    (1 << 5)
    #define SHOW_BLOCKS   (1 << 6)
} DisplayOpts;

// 文件节点结构
typedef struct FileNode {
    char *name;
    char *path;
    FileType type;
    struct stat info;
    struct FileNode *next;
} FileNode;

// 文件列表结构
typedef struct {
    FileNode *head;
    size_t count;
    blkcnt_t total_blocks;
} FileList;

// 颜色定义
#define COLOR_RESET   "\033[0m"
#define COLOR_DIR     "\033[1;34m"
#define COLOR_LINK    "\033[1;36m"
#define COLOR_EXEC    "\033[1;32m"

// 内存分配错误处理
static void memory_error(void) {
    fprintf(stderr, "内存分配失败\n");
    exit(EXIT_FAILURE);
}

// 创建文件节点
static FileNode *create_node(const char *name, const char *path, const struct stat *st) {
    FileNode *node = calloc(1, sizeof(FileNode));
    if (!node) memory_error();
    
    node->name = strdup(name);
    node->path = strdup(path);
    if (!node->name || !node->path) memory_error();
    
    node->info = *st;
    
    if (S_ISDIR(st->st_mode)) node->type = FT_DIRECTORY;
    else if (S_ISLNK(st->st_mode)) node->type = FT_SYMLINK;
    else if (S_ISREG(st->st_mode)) node->type = FT_REGULAR;
    else node->type = FT_SPECIAL;
    
    return node;
}

// 插入排序实现
static void insert_sorted(FileList *list, FileNode *node, const DisplayOpts *opts) {
    int time_sort = opts->flags & SORT_TIME;
    int reverse = opts->flags & SORT_REV;
    
    int should_insert_before(FileNode *a, FileNode *b) {
        if (time_sort) {
            if (a->info.st_mtime != b->info.st_mtime)
                return reverse ? 
                    a->info.st_mtime < b->info.st_mtime :
                    a->info.st_mtime > b->info.st_mtime;
        }
        return reverse ?
            strcmp(a->name, b->name) > 0 :
            strcmp(a->name, b->name) < 0;
    }
    
    if (!list->head || should_insert_before(node, list->head)) {
        node->next = list->head;
        list->head = node;
        return;
    }
    
    FileNode *current = list->head;
    while (current->next && !should_insert_before(node, current->next)) {
        current = current->next;
    }
    node->next = current->next;
    current->next = node;
}

// 格式化权限字符串
static void format_permissions(mode_t mode, char *perms) {
    static const char *rwx = "rwxrwxrwx";
    for (int i = 0; i < 9; i++)
        perms[i] = (mode & (1 << (8-i))) ? rwx[i] : '-';
    perms[9] = '\0';
}

// 获取文件颜色
static const char *get_color(const FileNode *node) {
    if (node->type == FT_DIRECTORY) return COLOR_DIR;
    if (node->type == FT_SYMLINK) return COLOR_LINK;
    if (node->info.st_mode & S_IXUSR) return COLOR_EXEC;
    return COLOR_RESET;
}

// 打印文件信息
static void print_file_info(const FileNode *node, const DisplayOpts *opts) {
    if (opts->flags & SHOW_INODE)
        printf("%lu ", node->info.st_ino);
        
    if (opts->flags & SHOW_BLOCKS)
        printf("%lu ", node->info.st_blocks / 2);
        
    if (opts->flags & SHOW_DETAIL) {
        char perms[10];
        format_permissions(node->info.st_mode, perms);
        
        struct passwd *pw = getpwuid(node->info.st_uid);
        struct group *gr = getgrgid(node->info.st_gid);
        
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", 
                localtime(&node->info.st_mtime));
        
        printf("%c%s %3lu %s %s %8lu %s ",
               node->type == FT_DIRECTORY ? 'd' : '-',
               perms, node->info.st_nlink,
               pw ? pw->pw_name : "unknown",
               gr ? gr->gr_name : "unknown",
               node->info.st_size, time_str);
    }
    
    printf("%s%s%s", get_color(node), node->name, COLOR_RESET);
    if (opts->flags & SHOW_DETAIL) printf("\n");
    else printf("  ");
}

// 释放文件列表
static void free_list(FileList *list) {
    FileNode *current = list->head;
    while (current) {
        FileNode *next = current->next;
        free(current->name);
        free(current->path);
        free(current);
        current = next;
    }
}

// 处理目录
static void process_directory(const char *path, const DisplayOpts *opts) {
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "无法打开目录 %s: %s\n", path, strerror(errno));
        return;
    }
    
    FileList list = {0};
    struct dirent *entry;
    
    while ((entry = readdir(dir))) {
        if (!opts->flags & SHOW_HIDDEN && entry->d_name[0] == '.')
            continue;
            
        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (lstat(full_path, &st) == -1) continue;
        
        FileNode *node = create_node(entry->d_name, full_path, &st);
        insert_sorted(&list, node, opts);
        list.count++;
        list.total_blocks += st.st_blocks;
    }
    
    closedir(dir);
    
    if (opts->flags & SHOW_BLOCKS)
        printf("总计 %lu\n", list.total_blocks / 2);
        
    FileNode *current = list.head;
    while (current) {
        print_file_info(current, opts);
        current = current->next;
    }
    
    if (!(opts->flags & SHOW_DETAIL)) printf("\n");
    
    if (opts->flags & SHOW_RECUR) {
        current = list.head;
        while (current) {
            if (current->type == FT_DIRECTORY &&
                strcmp(current->name, ".") != 0 &&
                strcmp(current->name, "..") != 0) {
                printf("\n%s:\n", current->path);
                process_directory(current->path, opts);
            }
            current = current->next;
        }
    }
    
    free_list(&list);
}

int main(int argc, char *argv[]) {
    DisplayOpts opts = {0};
    int opt;
    
    while ((opt = getopt(argc, argv, "alRtris")) != -1) {
        switch (opt) {
            case 'a': opts.flags |= SHOW_HIDDEN; break;
            case 'l': opts.flags |= SHOW_DETAIL; break;
            case 'R': opts.flags |= SHOW_RECUR; break;
            case 't': opts.flags |= SORT_TIME; break;
            case 'r': opts.flags |= SORT_REV; break;
            case 'i': opts.flags |= SHOW_INODE; break;
            case 's': opts.flags |= SHOW_BLOCKS; break;
            default:
                fprintf(stderr, "用法: %s [-alRtris] [路径...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    if (optind >= argc) {
        process_directory(".", &opts);
    } else {
        for (; optind < argc; optind++) {
            if (argc > optind + 1)
                printf("%s:\n", argv[optind]);
            process_directory(argv[optind], &opts);
            if (argc > optind + 1)
                printf("\n");
        }
    }
    
    return 0;
}
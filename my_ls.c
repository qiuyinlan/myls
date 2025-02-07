#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>    
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>  

#define MAX_PATH_LENGTH 256
#define INITIAL_CAPACITY 16
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RESET "\033[0m"

typedef struct DisplayOptions {
    bool show_hidden;
    bool long_format;
    bool recursive;
    bool sort_by_time;
    bool reverse_sort;
    bool show_inode;
    bool show_blocks;
    int link_width;
    int size_width;
    int block_width;
} DisplayOptions;

typedef struct FileMetadata {    
    char path[MAX_PATH_LENGTH];  
    ino_t inode_number;         
    off_t file_size;           
    time_t modified_time;      
    mode_t file_mode;         
    uid_t owner_id;          
    gid_t group_id;         
    nlink_t link_count;    
    blkcnt_t block_count; 
} FileMetadata;

typedef struct {
    FileMetadata *files;
    int count;
    int capacity;
} FileList;

// 处理错误信息并退出程序
// message: 错误信息字符串
void handle_error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

// 安全的内存分配函数，如果分配失败会自动处理错误

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) handle_error("Memory allocation failed");
    return ptr;
}

// 创建一个新的文件列表

FileList *create_file_list(int initial_capacity) {
    FileList *list = safe_malloc(sizeof(FileList));
    list->files = safe_malloc(sizeof(FileMetadata) * initial_capacity);
    list->count = 0;
    list->capacity = initial_capacity;
    return list;
}

// 释放文件列表占用的内存
// list: 要释放的文件列表
void free_file_list(FileList *list) {
    free(list->files);
    free(list);
}

// 向文件列表中添加一个新的文件

void add_to_file_list(FileList *list, FileMetadata *file) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->files = realloc(list->files, sizeof(FileMetadata) * list->capacity);
        if (!list->files) handle_error("Memory reallocation failed");
    }
    list->files[list->count++] = *file;
}

// 反转文件列表中的元素顺序

void reverse_array(FileMetadata *array, int count) {
    for (int i = 0; i < count / 2; i++) {
        FileMetadata temp = array[i];
        array[i] = array[count - 1 - i];
        array[count - 1 - i] = temp;
    }
}

// 比较两个文件的路径名（用于排序）

int compare_by_path(const void *a, const void *b) {    
    return strcmp(((FileMetadata *)a)->path, ((FileMetadata *)b)->path);
}

// 根据修改时间比较两个文件（用于排序）

int compare_by_time(const void *a, const void *b) {    
    const FileMetadata *file_a = (const FileMetadata *)a;
    const FileMetadata *file_b = (const FileMetadata *)b;
    if (file_a->modified_time != file_b->modified_time) {
        return (file_a->modified_time < file_b->modified_time) - (file_a->modified_time > file_b->modified_time);
    }
    return strcmp(file_a->path, file_b->path);
}

// 格式化文件权限信息
// permissions: 输出的权限字符串
void format_permissions(mode_t mode, char *permissions) {
    const char *rwx = "rwxrwxrwx";
    for(int i = 0; i < 9; i++) {
        permissions[i] = (mode & (1 << (8-i))) ? rwx[i] : '-';
    }
    permissions[9] = '\0';
}

// 获取文件的显示颜色
// mode: 文件模式
// 返回值: 颜色控制字符串
const char *get_file_color(mode_t mode) {
    if (S_ISDIR(mode)) {
        return COLOR_BLUE;  // 目录显示蓝色
    }
    return "";  // 普通文件不使用颜色
}

// 显示单个文件的信息

void display_file_entry(const FileMetadata *file, const DisplayOptions *options) {
    if (options->long_format) {
        if (options->show_inode) printf("%8lu ", file->inode_number);
        if (options->show_blocks) printf("%4lu ", file->block_count / 2);

        char permissions[11];
        format_permissions(file->file_mode, permissions);
        printf("%c%s", S_ISDIR(file->file_mode) ? 'd' : '-', permissions);

        struct passwd *pwd = getpwuid(file->owner_id);
        struct group *grp = getgrgid(file->group_id);

        printf(" %*lu %s %s %*ld ", 
               options->link_width, file->link_count,
               pwd ? pwd->pw_name : "unknown",
               grp ? grp->gr_name : "unknown",
               options->size_width, file->file_size);

        char time_str[32];
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", 
                localtime(&file->modified_time));
        printf("%s %s%s%s\n", time_str, 
               get_file_color(file->file_mode), file->path, 
               S_ISDIR(file->file_mode) ? COLOR_RESET : "");  // 只有在显示蓝色时才重置
    } else {
        if (options->show_inode) printf("%8lu ", file->inode_number);
        if (options->show_blocks) printf("%4lu ", file->block_count / 2);
        printf("%s%s%s  ", 
               get_file_color(file->file_mode), file->path,
               S_ISDIR(file->file_mode) ? COLOR_RESET : "");  // 只有在显示蓝色时才重置
    }
}

// 更新显示选项中的宽度信息

void update_display_options(FileList *list, DisplayOptions *options) {
    // 初始化变量
    long unsigned int max_size = 0;
    long unsigned int current_size = 0;
    nlink_t max_link = 0;
    nlink_t current_link = 0;
    int i = 0;
    
    // 遍历所有文件找最大值
    while (i < list->count) {
        // 获取当前文件的链接数
        current_link = list->files[i].link_count;
        
        // 比较链接数
        if (current_link > max_link) {
            max_link = current_link;
        }
        
        // 获取当前文件的大小
        current_size = list->files[i].file_size;
        
        // 比较文件大小
        if (current_size > max_size) {
            max_size = current_size;
        }
        
        // 移动到下一个文件
        i = i + 1;
    }

    // 计算显示宽度
    char temp_buffer[32];
    
    // 计算链接数的显示宽度
    sprintf(temp_buffer, "%lu", (unsigned long)max_link);
    options->link_width = strlen(temp_buffer);
    
    // 计算文件大小的显示宽度
    sprintf(temp_buffer, "%lu", max_size);
    options->size_width = strlen(temp_buffer);
    
    // 确保最小显示宽度
    if (options->link_width < 2) {
        options->link_width = 2;
    }
    
    if (options->size_width < 2) {
        options->size_width = 2;
    }
}

// 列出指定目录中的文件

void list_directory(const char *dir_path, DisplayOptions *options) {
    if (!dir_path) return;

    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Cannot open directory '%s': %s\n", dir_path, strerror(errno));
        return;
    }

    FileList *list = create_file_list(INITIAL_CAPACITY);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (!options->show_hidden && entry->d_name[0] == '.') continue;

        char full_path[MAX_PATH_LENGTH];
        size_t dir_len = strlen(dir_path);
        size_t name_len = strlen(entry->d_name);
        
        // 检查路径长度是否超出限制
        if (dir_len + name_len + 2 > MAX_PATH_LENGTH) {
            fprintf(stderr, "Path too long: %s/%s\n", dir_path, entry->d_name);
            continue;
        }
        
        // 安全地构建完整路径
        strcpy(full_path, dir_path);
        if (full_path[dir_len - 1] != '/') {
            strcat(full_path, "/");
        }
        strcat(full_path, entry->d_name);

        struct stat file_stat;
        if (lstat(full_path, &file_stat) == -1) continue;

        FileMetadata metadata = {0};
        strncpy(metadata.path, entry->d_name, MAX_PATH_LENGTH - 1);
        metadata.path[MAX_PATH_LENGTH - 1] = '\0';  // 确保字符串结束
        metadata.inode_number = file_stat.st_ino;
        metadata.file_size = file_stat.st_size;
        metadata.modified_time = file_stat.st_mtime;
        metadata.file_mode = file_stat.st_mode;
        metadata.owner_id = file_stat.st_uid;
        metadata.group_id = file_stat.st_gid;
        metadata.link_count = file_stat.st_nlink;
        metadata.block_count = file_stat.st_blocks;

        add_to_file_list(list, &metadata);
    }

    qsort(list->files, list->count, sizeof(FileMetadata),
          options->sort_by_time ? compare_by_time : compare_by_path);

    if (options->reverse_sort) {
        reverse_array(list->files, list->count);
    }

    update_display_options(list, options);

    if (options->recursive) {
        printf("\n%s:\n", dir_path);
    }

    for (int i = 0; i < list->count; i++) {
        display_file_entry(&list->files[i], options);
    }
    if (!options->long_format) printf("\n");

    if (options->recursive) {
        for (int i = 0; i < list->count; i++) {
            if (S_ISDIR(list->files[i].file_mode) &&
                strcmp(list->files[i].path, ".") != 0 &&
                strcmp(list->files[i].path, "..") != 0) {
                char new_path[MAX_PATH_LENGTH];
                if (snprintf(new_path, sizeof(new_path), "%s/%s", 
                             dir_path, list->files[i].path) >= sizeof(new_path)) {
                    fprintf(stderr, "Path too long: %s/%s\n", dir_path, list->files[i].path);
                    continue;
                }
                list_directory(new_path, options);
            }
        }
    }

    free_file_list(list);
    closedir(dir);
}

// 主函数
int main(int argc, char *argv[]) {
    int opt;
    DisplayOptions options = {0};

    while ((opt = getopt(argc, argv, "alRtris")) != -1) {
        switch (opt) {
            case 'a': 
                options.show_hidden = true; 
                break;
                
            case 'l': 
                options.long_format = true; 
                break;
                
            case 'R': 
                options.recursive = true; 
                break;
                
            case 't': 
                options.sort_by_time = true; 
                break;
                
            case 'r': 
                options.reverse_sort = true; 
                break;
                
            case 'i': 
                options.show_inode = true; 
                break;
                
            case 's': 
                options.show_blocks = true; 
                break;
                
            case '?':
                fprintf(stderr, "Unknown option: %c\n", optopt);
                fprintf(stderr, "Usage: %s [-alRtris] [directory...]\n", argv[0]);
                exit(EXIT_FAILURE);
                
            default: 
                fprintf(stderr, "Usage: %s [-alRtris] [directory...]\n", argv[0]);
                fprintf(stderr, "Options:\n");
                fprintf(stderr, "  -a: show hidden files\n");
                fprintf(stderr, "  -l: use long listing format\n");
                fprintf(stderr, "  -R: list subdirectories recursively\n");
                fprintf(stderr, "  -t: sort by modification time\n");
                fprintf(stderr, "  -r: reverse order while sorting\n");
                fprintf(stderr, "  -i: print the index number of each file\n");
                fprintf(stderr, "  -s: print the allocated size of each file\n");
                exit(EXIT_FAILURE);
        }
    }

    const char *path = optind < argc ? argv[optind] : ".";
    list_directory(path, &options);

    return 0;
}
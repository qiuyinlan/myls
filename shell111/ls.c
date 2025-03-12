#include <dirent.h>
#include <grp.h>
#include <limits.h>
#include <linux/limits.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define COLOR_RESET "\033[0m"
#define COLOR_DIR "\033[1;34m"
#define COLOR_EXEC "\033[1;32m"
#define COLOR_SYMLINK "\033[1;36m"

typedef struct {
    bool show_all;
    bool long_list;
    bool recursive;
    bool sort_time;
    bool reverse;
    bool show_inode;
    bool show_blocks;
} Options;

typedef struct FileNode {
    char name[NAME_MAX + 1];
    struct stat statbuf;
    struct FileNode* next;
} FileNode;

void process_directory(const char* path, const Options* opts);
FileNode* add_file(FileNode* head,
                   const char* name,
                   const struct stat* statbuf);
void free_file_list(FileNode* head);
void sort_files(FileNode** head, const Options* opts);
void print_file(const FileNode* file, const Options* opts);
void display_usage(const char* prog_name);

int main(int argc, char* argv[]) {
    Options opts = {0};
    char* targdir = ".";

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; ++j) {
                switch (argv[i][j]) {
                    case 'a':
                        opts.show_all = true;
                        break;
                    case 'l':
                        opts.long_list = true;
                        break;
                    case 'R':
                        opts.recursive = true;
                        break;
                    case 't':
                        opts.sort_time = true;
                        break;
                    case 'r':
                        opts.reverse = true;
                        break;
                    case 'i':
                        opts.show_inode = true;
                        break;
                    case 'b':
                        opts.show_blocks = true;
                        break;
                    default:
                        fprintf(stderr, "未知参数: -%c\n", argv[i][j]);
                        display_usage(argv[0]);
                        return 1;
                }
            }
        } else
            targdir = argv[i];
    }
    process_directory(targdir, &opts);
    return 0;
}

void process_directory(const char* path, const Options* opts) {
    DIR* dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return;
    }
    printf("%s:\n", path);
    struct dirent* entry;
    FileNode* flist = NULL;
    while ((entry = readdir(dir))) {
        if (!opts->show_all && entry->d_name[0] == '.')
            continue;

        char fullpath[PATH_MAX];
        snprintf(fullpath, PATH_MAX, "%s/%s", path, entry->d_name);
        struct stat statbuf;
        if (lstat(fullpath, &statbuf) == -1) {
            perror("lstat");
            continue;
        }
        flist = add_file(flist, entry->d_name, &statbuf);
    }
    closedir(dir);

    sort_files(&flist, opts);

    for (FileNode* node = flist; node; node = node->next) {
        print_file(node, opts);
    }

    free_file_list(flist);

    if (opts->recursive) {
        dir = opendir(path);
        while ((entry = readdir(dir))) {
            if (entry->d_name[0] == '.')
                continue;

            char fullpath[PATH_MAX];
            snprintf(fullpath, PATH_MAX, "%s/%s", path, entry->d_name);

            struct stat statbuf;
            if (lstat(fullpath, &statbuf) == -1) {
                perror("lstat");
                continue;
            }
            if (S_ISDIR(statbuf.st_mode)) {
                process_directory(fullpath, opts);
            }
        }
        closedir(dir);
    }
}

FileNode* add_file(FileNode* head,
                   const char* name,
                   const struct stat* statbuf) {
    FileNode* new_node = malloc(sizeof(FileNode));
    if (!new_node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strncpy(new_node->name, name, NAME_MAX);
    new_node->statbuf = *statbuf;
    new_node->next = head;
    new_node->name[NAME_MAX] = '\0';
    return new_node;
}

void sort_files(FileNode** head, const Options* opts) {
    FileNode* prev = NULL;

    while (*head) {
        FileNode* cur = *head;
        *head = (*head)->next;

        if (!prev ||
            (opts->sort_time ? cur->statbuf.st_mtime > prev->statbuf.st_mtime
                             : strcmp(cur->name, prev->name) < 0)) {
            cur->next = prev;
            prev = cur;
        } else {
            FileNode* tmp = prev;
            while (tmp->next &&
                   (opts->sort_time
                        ? cur->statbuf.st_mtime <= tmp->next->statbuf.st_mtime
                        : strcmp(cur->name, tmp->next->name) >= 0)) {
                tmp = tmp->next;
            }
            cur->next = tmp->next;
            tmp->next = cur;
        }
    }
    *head = prev;

    if (opts->reverse) {
        FileNode *p = NULL, *q = *head, *r = NULL;
        while (q) {
            r = q->next;
            q->next = p;
            p = q;
            q = r;
        }
        *head = p;
    }
}

void print_file(const FileNode* file, const Options* opts) {
    if (opts->show_inode) {
        printf("%10lu", file->statbuf.st_ino);
    }
    if (opts->show_blocks) {
        printf("%8ld", file->statbuf.st_blocks);
    }
    if (opts->long_list) {
        char perms[11] = "----------";
        if (S_ISDIR(file->statbuf.st_mode))
            perms[0] = 'd';
        if (S_ISLNK(file->statbuf.st_mode))
            perms[0] = 'l';
        if (file->statbuf.st_mode & S_IRUSR)
            perms[1] = 'r';
        if (file->statbuf.st_mode & S_IWUSR)
            perms[2] = 'w';
        if (file->statbuf.st_mode & S_IXUSR)
            perms[3] = 'x';
        if (file->statbuf.st_mode & S_IRGRP)
            perms[4] = 'r';
        if (file->statbuf.st_mode & S_IWGRP)
            perms[5] = 'w';
        if (file->statbuf.st_mode & S_IXGRP)
            perms[6] = 'x';
        if (file->statbuf.st_mode & S_IROTH)
            perms[7] = 'r';
        if (file->statbuf.st_mode & S_IWOTH)
            perms[8] = 'w';
        if (file->statbuf.st_mode & S_IXOTH)
            perms[9] = 'x';

        struct passwd* pw = getpwuid(file->statbuf.st_uid);
        struct group* gr = getgrgid(file->statbuf.st_gid);

        char timestr[26];
        strftime(timestr, sizeof(timestr), "%b %d %H:%M",
                 localtime(&file->statbuf.st_mtime));

        printf("%s %4ld %-8s %-8s %8ld %s ", perms, file->statbuf.st_nlink,
               pw ? pw->pw_name : "未知用户", gr ? gr->gr_name : "未知组",
               file->statbuf.st_size, timestr);
    }

    if (S_ISDIR(file->statbuf.st_mode)) {
        printf(COLOR_DIR "%s" COLOR_RESET, file->name);
    } else if (file->statbuf.st_mode & S_IXUSR) {
        printf(COLOR_EXEC "%s" COLOR_RESET, file->name);
    } else if (S_ISLNK(file->statbuf.st_mode)) {
        printf(COLOR_SYMLINK "%s" COLOR_RESET, file->name);
    } else {
        printf("%s", file->name);
    }
    puts("\n");
}

void free_file_list(FileNode* head) {
    while (head) {
        FileNode* tmp = head;
        head = head->next;
        free(tmp);
    }
}

void display_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s [-alRribh] [directory]\n", prog_name);
}
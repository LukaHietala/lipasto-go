#ifndef GIT_H
#define GIT_H

#define HASH_SIZE 41
#define MESSAGE_SIZE 256
#define AUTHOR_SIZE 128
#define PATH_SIZE 512

typedef struct {
    char hash[HASH_SIZE];
    char message[MESSAGE_SIZE];
    char author[AUTHOR_SIZE];
    long timestamp;
} Commit;

typedef struct {
    char path[PATH_SIZE];
    char name[MESSAGE_SIZE];
} BareRepo;

int get_commits(const char *repo_path, Commit *commits, int max);
int list_bare_repos(const char *dir_path, BareRepo *repos, int max);

#endif


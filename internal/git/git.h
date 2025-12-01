#ifndef GIT_H
#define GIT_H

#define HASH_SIZE 41
#define MESSAGE_SIZE 256
#define AUTHOR_SIZE 128

typedef struct {
    char hash[HASH_SIZE];
    char message[MESSAGE_SIZE];
    char author[AUTHOR_SIZE];
    long timestamp;
} Commit;

int get_commits(const char *repo_path, Commit *commits, int max);

#endif


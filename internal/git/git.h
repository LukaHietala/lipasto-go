#ifndef GIT_H
#define GIT_H

/** completely arbituary */
#define HASH_SIZE 41
#define MESSAGE_SIZE 256
#define AUTHOR_SIZE 128
#define PATH_SIZE 512
#define REFERENCE_NAME_SIZE 256
#define REFERENCE_SHORTHAND_SIZE 128


typedef struct {
    char path[PATH_SIZE];
    char name[MESSAGE_SIZE];
} BareRepo;

typedef struct {
    char hash[HASH_SIZE];
    char message[MESSAGE_SIZE];
    char author[AUTHOR_SIZE];
    long timestamp;
} Commit;

typedef struct {
    char hash[HASH_SIZE];
    char name[REFERENCE_NAME_SIZE];
    char shorthand[REFERENCE_SHORTHAND_SIZE];
} Reference;

int list_bare_repos(const char *dir_path, BareRepo *repos, int max);
int get_commits(const char *repo_path, Commit *commits, int max);
int get_references(const char *repo_path, Reference *references);
int get_reference_count(const char *repo_path);
#endif


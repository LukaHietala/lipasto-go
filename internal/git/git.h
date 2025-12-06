#ifndef GIT_H
#define GIT_H

/** completely arbituary */
#define HASH_SIZE 41
#define MESSAGE_SIZE 256
#define PATH_SIZE 512
#define REFERENCE_NAME_SIZE 256
#define REFERENCE_SHORTHAND_SIZE 128
#define OWNER_SIZE 256

typedef struct {
	char path[PATH_SIZE];
	char name[MESSAGE_SIZE];
} BareRepo;

/* committer or author */
typedef struct {
	char name[256];
	char email[256];
} Signature;

typedef struct {
	char hash[HASH_SIZE];
	char parent_hash[HASH_SIZE];
	char tree_id[HASH_SIZE];
	char message[MESSAGE_SIZE];
	Signature author;
	Signature committer;
	long timestamp;
} Commit;

typedef struct {
	char hash[HASH_SIZE];
	char name[REFERENCE_NAME_SIZE];
	char shorthand[REFERENCE_SHORTHAND_SIZE];
} Reference;

int list_bare_repos(const char *dir_path, BareRepo *repos, int max, char *err,
		    size_t errlen);
int get_commits(const char *repo_path, const char *ref, Commit *commits,
		int max, int skip, char *err, size_t errlen);
int get_references(const char *repo_path, Reference *references, char *err,
		   size_t errlen);
int get_reference_count(const char *repo_path, char *err, size_t errlen);
int get_commit(const char *repo_path, const char *oidstr, Commit *commit,
	       char *err, size_t errlen);
int get_repo_owner(const char *repo_path, char *owner, size_t ownerlen,
		   char *err, size_t errlen);
#endif

#include <git2.h>
#include <stdio.h>
#include <dirent.h>
#include "git.h"

/* Gets all the commits from bare repository*/
int get_commits(const char *repo_path, Commit *commits, int max) 
{
    git_libgit2_init();

    git_repository *repo = NULL;
    if (git_repository_open_bare(&repo, repo_path) != 0) {
	git_libgit2_shutdown();
	return 0;
    }

    git_revwalk *walker = NULL;
    git_revwalk_new(&walker, repo);
    git_revwalk_sorting(walker, GIT_SORT_TIME);
    git_revwalk_push_head(walker);

    git_oid oid;
    int count = 0;

    while (count < max && git_revwalk_next(&oid, walker) == 0) {
	git_commit *commit = NULL;
	if (git_commit_lookup(&commit, repo, &oid) != 0) continue;

	/* SHA-1 hashes are 40 chars, but it adds a nul byte, so we need 41 */
        git_oid_tostr(commits[count].hash, sizeof(commits[count].hash), &oid);
        
        const char *msg = git_commit_message(commit);
        snprintf(commits[count].message, sizeof(commits[count].message), "%s", msg ? msg : "");
        
        const git_signature *author = git_commit_author(commit);
        snprintf(commits[count].author, sizeof(commits[count].author), "%s", author ? author->name : "unknown");
        commits[count].timestamp = author ? author->when.time : 0;

	git_commit_free(commit);
	count++;
    }

    git_revwalk_free(walker);
    git_repository_free(repo);
    git_libgit2_shutdown();
    return count;
}

/* Lists all bare git repositories in a directory */
int list_bare_repos(const char *dir_path, BareRepo *repos, int max)
{
    git_libgit2_init();
    
    DIR *dir = opendir(dir_path);
    if (!dir) {
        git_libgit2_shutdown();
        return 0;
    }

    struct dirent *entry;
    int count = 0;

    while (count < max && (entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char full_path[PATH_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        git_repository *repo = NULL;
        if (git_repository_open_bare(&repo, full_path) == 0) {
            snprintf(repos[count].path, sizeof(repos[count].path), "%s", full_path);
            snprintf(repos[count].name, sizeof(repos[count].name), "%s", entry->d_name);
            git_repository_free(repo);
            count++;
        }
    }

    closedir(dir);
    git_libgit2_shutdown();
    return count;
}

#include <git2.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "git.h"

/* EVERYTHING NEEDS TO BE CLEANED WITH goto cleanup logic*/

/* lists all bare repos from dir */
int list_bare_repos(const char *dir_path, BareRepo *repos, int max)
{
    git_libgit2_init();

    DIR *dir = NULL;
    struct dirent *entry = NULL;
    int count = 0;
    int result = -1;

    dir = opendir(dir_path);
    if (!dir)
        goto cleanup;

	/* 
	* find all bare repos from dir
	* TODO: get rid of max, dynamic alloc later
	*/
    while (count < max && (entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        char full_path[PATH_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        git_repository *repo = NULL;
        if (git_repository_open_bare(&repo, full_path) == 0) {
            snprintf(repos[count].path, sizeof(repos[count].path), "%s", full_path);
            snprintf(repos[count].name, sizeof(repos[count].name), "%s", entry->d_name);
            git_repository_free(repo);
            repo = NULL;
            count++;
        }
    }

    result = count;

cleanup:
    if (dir)
        closedir(dir);
    git_libgit2_shutdown();
    return result;
}

/* gets all commits from repo */
int get_commits(const char *repo_path, Commit *commits, int max)
{
    git_libgit2_init();

    int result = -1;
    git_repository *repo = NULL;
    git_revwalk *walker = NULL;
    git_oid oid;
    int count = 0;

    if (git_repository_open_bare(&repo, repo_path) != 0)
        goto cleanup;

    if (git_revwalk_new(&walker, repo) != 0)
        goto cleanup;

    git_revwalk_sorting(walker, GIT_SORT_TIME);
    if (git_revwalk_push_head(walker) != 0)
        goto cleanup;

    while (count < max && git_revwalk_next(&oid, walker) == 0) {
        git_commit *commit = NULL;

	/* lookup commit by oid */
        if (git_commit_lookup(&commit, repo, &oid) != 0)
            continue;

        git_oid_tostr(commits[count].hash, sizeof(commits[count].hash), &oid);

	/* get parent commit oid */
        if (git_commit_parentcount(commit) > 0) {
            const git_oid *parent_oid = git_commit_parent_id(commit, 0);
            if (parent_oid) {
                git_oid_tostr(commits[count].parent_hash,
                              sizeof(commits[count].parent_hash),
                              parent_oid);
            }
        }

        /* get tree oid */
        const git_oid *tree_oid = git_commit_tree_id(commit);
        if (tree_oid) {
            git_oid_tostr(commits[count].tree_id,
                          sizeof(commits[count].tree_id),
                          tree_oid);
        }

	/* get commit message */
        const char *msg = git_commit_message(commit);
        snprintf(commits[count].message, sizeof(commits[count].message), "%s", msg ? msg : "");

	/* get author */
        const git_signature *author = git_commit_author(commit);
        if (author && author->name && author->email) {
            snprintf(commits[count].author.name, sizeof(commits[count].author.name), "%s", author->name);
            snprintf(commits[count].author.email, sizeof(commits[count].author.email), "%s", author->email);
            commits[count].timestamp = author->when.time;
        }

	/* get committer */
        const git_signature *committer = git_commit_committer(commit);
        if (committer && committer->name && committer->email) {
            snprintf(commits[count].committer.name, sizeof(commits[count].committer.name), "%s", committer->name);
            snprintf(commits[count].committer.email, sizeof(commits[count].committer.email), "%s", committer->email);
        }

        git_commit_free(commit);
        commit = NULL;
        count++;
    }

    result = count;

cleanup:
    if (walker)
        git_revwalk_free(walker);
    if (repo)
        git_repository_free(repo);
    git_libgit2_shutdown();
    return result;
}

/* gets all references from repo */
int get_references(const char *repo_path, Reference *references)
{
    git_libgit2_init();

    git_repository *repo = NULL;
    git_reference_iterator *iter = NULL;
    git_reference *ref = NULL;
    git_reference *resolved = NULL;
    int result = -1;
    int error = 0;
    int count = 0;

    if (git_repository_open_bare(&repo, repo_path) != 0)
        goto cleanup;

    if ((error = git_reference_iterator_new(&iter, repo)) != 0)
        goto cleanup;

    /* iterate over all references */
    while (!(error = git_reference_next(&ref, iter))) {
	/* get reference details */
        const char *name = git_reference_name(ref);
        const char *shorthand = git_reference_shorthand(ref);
        const git_oid *oid_ptr = NULL;

	/* 
	* resolve reference to its target, for sybolic stuff 
	* https://lore.kernel.org/git/AANLkTinsJkzYggMtNrLRv-qNxRncrXSe6A46Z=d8xkw7@mail.gmail.com/
	*/
        if (git_reference_resolve(&resolved, ref) == 0)
            oid_ptr = git_reference_target(resolved);

        if (oid_ptr) {
            git_oid_tostr(references[count].hash,
                          sizeof(references[count].hash),
                          oid_ptr);
        } else {
            references[count].hash[0] = '\0';
        }

        snprintf(references[count].name, sizeof(references[count].name), "%s", name ? name : "");
        snprintf(references[count].shorthand, sizeof(references[count].shorthand), "%s", shorthand ? shorthand : "");

        git_reference_free(ref);
        ref = NULL;
	/* resolve reference needs to be freed too*/
        if (resolved) {
            git_reference_free(resolved);
            resolved = NULL;
        }

        count++;
    }

    if (error != GIT_ITEROVER)
        goto cleanup;

    result = count;

cleanup:
    if (ref)
        git_reference_free(ref);
    if (resolved)
        git_reference_free(resolved);
    if (iter)
        git_reference_iterator_free(iter);
    if (repo)
        git_repository_free(repo);
    git_libgit2_shutdown();
    return result;
}

/* for dynamically allocating slice on go side */
int get_reference_count(const char *repo_path)
{
    git_libgit2_init();

    git_repository *repo = NULL;
    git_reference_iterator *iter = NULL;
    git_reference *ref = NULL;
    int result = -1;
    int error = 0;
    int count = 0;

    if (git_repository_open_bare(&repo, repo_path) != 0)
        goto cleanup;

    if ((error = git_reference_iterator_new(&iter, repo)) != 0)
        goto cleanup;

    while (!(error = git_reference_next(&ref, iter))) {
        count++;
        git_reference_free(ref);
        ref = NULL;
    }

    if (error != GIT_ITEROVER)
        goto cleanup;

    result = count;

cleanup:
    if (ref)
        git_reference_free(ref);
    if (iter)
        git_reference_iterator_free(iter);
    if (repo)
        git_repository_free(repo);
    git_libgit2_shutdown();
    return result;
}

/* gets single commit by oidstr */
int get_commit(const char *repo_path, const char *oidstr, Commit *commit)
{
    if (commit == NULL)
        return -1;

    git_libgit2_init();

    int result = -1;
    git_repository *repo = NULL;
    git_commit *commit_obj = NULL;
    git_oid oid;

    if (git_repository_open_bare(&repo, repo_path) != 0)
        goto cleanup;

    if (git_oid_fromstr(&oid, oidstr) != 0)
        goto cleanup;

    if (git_commit_lookup(&commit_obj, repo, &oid) != 0)
        goto cleanup;

    git_oid_tostr(commit->hash, sizeof(commit->hash), &oid);

    /* get parent commit oid */
    if (git_commit_parentcount(commit_obj) > 0) {
        const git_oid *parent_oid = git_commit_parent_id(commit_obj, 0);
        if (parent_oid) {
            git_oid_tostr(commit->parent_hash,
                          sizeof(commit->parent_hash),
                          parent_oid);
        }
    }

    /* get tree oid */
    const git_oid *tree_oid = git_commit_tree_id(commit_obj);
    if (tree_oid) {
        git_oid_tostr(commit->tree_id, sizeof(commit->tree_id), tree_oid);
    }

   /* get commit message */
    const char *msg = git_commit_message(commit_obj);
    snprintf(commit->message, sizeof(commit->message), "%s", msg ? msg : "");

    /* get author */
    const git_signature *author = git_commit_author(commit_obj);
    if (author) {
        snprintf(commit->author.name, sizeof(commit->author.name), "%s",
                 author->name ? author->name : "");
	snprintf(commit->author.email, sizeof(commit->author.email), "%s",
                 author->email ? author->email : "");

        commit->timestamp = author->when.time;
    }
	
    /* get committer */
    const git_signature *committer = git_commit_committer(commit_obj);
    if (committer) {
        snprintf(commit->committer.name, sizeof(commit->committer.name), "%s",
                 committer->name ? committer->name : "");
	snprintf(commit->committer.email, sizeof(commit->committer.email), "%s",
                 committer->email ? committer->email : "");

    }

    result = 0;

cleanup:
    if (commit_obj)
        git_commit_free(commit_obj);
    if (repo)
        git_repository_free(repo);
    git_libgit2_shutdown();
    return result;
}

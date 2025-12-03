#include <git2.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "git.h"

/* Lists all bare git repositories in a directory */
int list_bare_repos(const char *dir_path, BareRepo *repos, int max)
{
	git_libgit2_init();
	
	DIR *dir = opendir(dir_path);
	if (!dir) {
		git_libgit2_shutdown();
	return -1;
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

/* Gets all the commits from bare repository*/
int get_commits(const char *repo_path, Commit *commits, int max) 
{
	git_libgit2_init();

	git_repository *repo = NULL;
	if (git_repository_open_bare(&repo, repo_path) != 0) {
		git_libgit2_shutdown();
		return -1;
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

int get_references(const char* repo_path, Reference *references)
{
	git_libgit2_init();
	
	git_repository *repo = NULL;
	if (git_repository_open_bare(&repo, repo_path) != 0) {
	git_libgit2_shutdown();
	return -1;
	}

	git_reference_iterator *iter = NULL;
	int error = git_reference_iterator_new(&iter, repo);
	
	if (error != 0) {
		git_repository_free(repo);
		git_libgit2_shutdown();
		return -1;
	}

	git_reference *ref = NULL;
	int count = 0;

	while (!(error = git_reference_next(&ref, iter))) {
	const char *name = git_reference_name(ref); 
	const char *shorthand = git_reference_shorthand(ref);

	git_reference *resolved = NULL;
	const git_oid *oid_ptr = NULL;

	if (git_reference_resolve(&resolved, ref) == 0) {
		oid_ptr = git_reference_target(resolved);
	}

	if (oid_ptr) {
		git_oid_tostr(references[count].hash,
						sizeof(references[count].hash),
						oid_ptr);
	} else {
		/* no valid oid :(*/
		references[count].hash[0] = '\0';
	}        

	snprintf(references[count].name, sizeof(references[count].name), "%s", name ? name : "");
	snprintf(references[count].shorthand, sizeof(references[count].shorthand), "%s", shorthand ? shorthand : "");

	git_reference_free(ref); 
	git_reference_free(resolved);
	resolved = NULL;
	ref = NULL;

	count++;
	}

	if (error != GIT_ITEROVER) {
	git_reference_iterator_free(iter);
		git_repository_free(repo);
		git_libgit2_shutdown();
	return -1;
	}

	git_reference_iterator_free(iter);
	git_repository_free(repo);
	git_libgit2_shutdown();

	return count;
}

int get_reference_count(const char *repo_path)
{
	git_libgit2_init();

	git_repository *repo = NULL;
	if (git_repository_open_bare(&repo, repo_path) != 0) {
		git_libgit2_shutdown();
		return -1;
	}

	git_reference_iterator *iter = NULL;
	int error = git_reference_iterator_new(&iter, repo);
	if (error != 0) {
		git_repository_free(repo);
		git_libgit2_shutdown();
		return -1;
	}

	git_reference *ref = NULL;
	int count = 0;

	while (!(error = git_reference_next(&ref, iter))) {
		count++;
		git_reference_free(ref);
		ref = NULL;
	}

	git_reference_iterator_free(iter);
	git_repository_free(repo);
	git_libgit2_shutdown();

	if (error != GIT_ITEROVER)
		return -1;

	return count;
}

/* Get commit with oid (string)*/
int get_commit(const char* repo_path, const char* oidstr, Commit *commit)
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

	/* get first parent if there are parents */
	if (git_commit_parentcount(commit_obj) > 0) {
		const git_oid *parent_oid = git_commit_parent_id(commit_obj, 0);
		if (parent_oid) {
			git_oid_tostr(commit->parent_hash,
						  sizeof(commit->parent_hash),
						  parent_oid);
		}
	}

	/* get tree id */
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
		snprintf(commit->author, sizeof(commit->author), "%s",
				 author->name ? author->name : "");
		commit->timestamp = author->when.time;
	}

	/* get committer */
	const git_signature *committer = git_commit_committer(commit_obj);
	if (committer) {
		snprintf(commit->committer, sizeof(commit->committer), "%s",
				 committer->name ? committer->name : "");
	}

	result = 0;

cleanup:
	if (commit_obj) {
		git_commit_free(commit_obj);
	}

	if (repo) {
		git_repository_free(repo);
	}

	git_libgit2_shutdown();
	return result;
}

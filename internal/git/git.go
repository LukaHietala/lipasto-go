package git

/*
#cgo LDFLAGS: -lgit2
#include "git.h"
#include <stdlib.h>
*/
import "C"

import (
	"time"
	"unsafe"
)

type BareRepo struct {
	Path string
	Name string
}

type Signature struct {
	Name string
	Email string
}

type Commit struct {
	Hash       string
	ParentHash string
	TreeID     string
	Message    string
	Author     Signature
	Committer  Signature
	Timestamp  time.Time
}

type Reference struct {
	Hash      string
	Name      string
	Shorthand string
}

func ListBareRepos(dirPath string, max int) []BareRepo {
	cRepos := make([]C.BareRepo, max)
	count := C.list_bare_repos(C.CString(dirPath), &cRepos[0], C.int(max))

	repos := make([]BareRepo, count)
	for i := 0; i < int(count); i++ {
		repos[i] = BareRepo{
			Path: C.GoString(&cRepos[i].path[0]),
			Name: C.GoString(&cRepos[i].name[0]),
		}
	}
	return repos
}

func GetCommits(repoPath string, ref string, max int) []Commit {
	cCommits := make([]C.Commit, max)
	count := C.get_commits(C.CString(repoPath), C.CString(ref), &cCommits[0], C.int(max))

	if count <= 0 {
		return nil
	}

	commits := make([]Commit, count)
	for i := 0; i < int(count); i++ {
		commits[i] = commitFromC(&cCommits[i])
	}
	return commits
}

func GetReferences(repoPath string) []Reference {
	cRepo := C.CString(repoPath)
	defer C.free(unsafe.Pointer(cRepo))

	count := int(C.get_reference_count(cRepo))
	if count <= 0 {
		return nil
	}

	cRefs := make([]C.Reference, count)

	n := int(C.get_references(cRepo, (*C.Reference)(unsafe.Pointer(&cRefs[0]))))
	if n <= 0 {
		return nil
	}

	refs := make([]Reference, n)
	for i := range refs {
		refs[i] = Reference{
			Hash:      C.GoString(&cRefs[i].hash[0]),
			Name:      C.GoString(&cRefs[i].name[0]),
			Shorthand: C.GoString(&cRefs[i].shorthand[0]),
		}
	}
	return refs
}

func GetCommit(repoPath string, oidstr string) Commit {
	cRepo := C.CString(repoPath)
	defer C.free(unsafe.Pointer(cRepo))

	cOidstr := C.CString(oidstr)
	defer C.free(unsafe.Pointer(cOidstr))

	var cCommit C.Commit
	if C.get_commit(cRepo, cOidstr, &cCommit) != 0 {
		return Commit{}
	}

	return commitFromC(&cCommit)
}

func commitFromC(cCommit *C.Commit) Commit {
	if cCommit == nil {
		return Commit{}
	}

	return Commit{
		Hash:       C.GoString(&cCommit.hash[0]),
		ParentHash: C.GoString(&cCommit.parent_hash[0]),
		TreeID:     C.GoString(&cCommit.tree_id[0]),
		Message:    C.GoString(&cCommit.message[0]),
		Author:     Signature{ C.GoString(&cCommit.author.name[0]), C.GoString(&cCommit.author.email[0]) },
		Committer:  Signature{ C.GoString(&cCommit.committer.name[0]), C.GoString(&cCommit.committer.email[0]) },
		Timestamp:  time.Unix(int64(cCommit.timestamp), 0),
	}
}

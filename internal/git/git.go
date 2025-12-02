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

type Commit struct {
	Hash      string
	Message   string
	Author    string
	Timestamp time.Time
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

func GetCommits(repoPath string, max int) []Commit {
	cCommits := make([]C.Commit, max)
	count := C.get_commits(C.CString(repoPath), &cCommits[0], C.int(max))

	if count <= 0 {
		return nil
	}

	commits := make([]Commit, count)
	for i := 0; i < int(count); i++ {
		commits[i] = Commit{
			Hash:      C.GoString(&cCommits[i].hash[0]),
			Message:   C.GoString(&cCommits[i].message[0]),
			Author:    C.GoString(&cCommits[i].author[0]),
			Timestamp: time.Unix(int64(cCommits[i].timestamp), 0),
		}
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
	for i := 0; i < n; i++ {
		refs[i] = Reference{
			Hash:      C.GoString(&cRefs[i].hash[0]),
			Name:      C.GoString(&cRefs[i].name[0]),
			Shorthand: C.GoString(&cRefs[i].shorthand[0]),
		}
	}
	return refs
}

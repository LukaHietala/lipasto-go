package git

/*
#cgo LDFLAGS: -lgit2
#include "git.h"
*/
import "C"
import "time"

type Commit struct {
	Hash      string
	Message   string
	Author    string
	Timestamp time.Time
}

type BareRepo struct {
	Path string
	Name string
}

func GetCommits(repoPath string, max int) []Commit {
	cCommits := make([]C.Commit, max)
	count := C.get_commits(C.CString(repoPath), &cCommits[0], C.int(max))

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

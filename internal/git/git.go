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

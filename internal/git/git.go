package git

/*
#cgo LDFLAGS: -lgit2
#include <git2.h>
#include "git.h"
#include <stdlib.h>
*/
import "C"

import (
	"fmt"
	"sync"
	"time"
	"unsafe"
)

type BareRepo struct {
	Path string
	Name string
}

type Signature struct {
	Name  string
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

var (
	initOnce     sync.Once
	shutdownOnce sync.Once
)

// init libgit2 only once, improves performance about 5 %
func Init() {
	initOnce.Do(func() {
		C.git_libgit2_init()
	})
}

// shutdown libgit2 only once
func Shutdown() {
	shutdownOnce.Do(func() {
		C.git_libgit2_shutdown()
	})
}

/*
* relying fully on libgit2 errors, very clear and simple
* Libgit2 errors for reference: https://github.com/libgit2/libgit2/blob/main/include/git2/errors.h
 */
const errorBufferSize = 512

func ListBareRepos(dirPath string, max int) ([]BareRepo, error) {
	if max <= 0 {
		return []BareRepo{}, nil
	}

	cDir := C.CString(dirPath)
	defer C.free(unsafe.Pointer(cDir))

	cRepos := make([]C.BareRepo, max)
	errBuf := make([]C.char, errorBufferSize)

	status := C.list_bare_repos(cDir,
		(*C.BareRepo)(unsafe.Pointer(&cRepos[0])),
		C.int(max),
		errBufPointer(errBuf),
		C.size_t(len(errBuf)))
	if status < 0 {
		return nil, errorFromStatus(status, errBuf)
	}

	count := int(status)
	repos := make([]BareRepo, count)
	for i := range count {
		repos[i] = BareRepo{
			Path: C.GoString(&cRepos[i].path[0]),
			Name: C.GoString(&cRepos[i].name[0]),
		}
	}
	return repos, nil
}

func GetCommits(repoPath string, ref string, max int, skip int) ([]Commit, error) {
	if max <= 0 {
		return []Commit{}, nil
	}

	cRepo := C.CString(repoPath)
	defer C.free(unsafe.Pointer(cRepo))
	cRef := C.CString(ref)
	defer C.free(unsafe.Pointer(cRef))

	cCommits := make([]C.Commit, max)
	errBuf := make([]C.char, errorBufferSize)

	status := C.get_commits(cRepo,
		cRef,
		&cCommits[0],
		C.int(max),
		C.int(skip),
		errBufPointer(errBuf),
		C.size_t(len(errBuf)))
	if status < 0 {
		return nil, errorFromStatus(status, errBuf)
	}

	count := int(status)
	commits := make([]Commit, count)
	for i := range count {
		commits[i] = commitFromC(&cCommits[i])
	}
	return commits, nil
}

func GetReferences(repoPath string) ([]Reference, error) {
	cRepo := C.CString(repoPath)
	defer C.free(unsafe.Pointer(cRepo))

	errBuf := make([]C.char, errorBufferSize)
	countStatus := C.get_reference_count(cRepo, errBufPointer(errBuf), C.size_t(len(errBuf)))
	if countStatus < 0 {
		return nil, errorFromStatus(countStatus, errBuf)
	}

	count := int(countStatus)
	if count == 0 {
		return []Reference{}, nil
	}

	cRefs := make([]C.Reference, count)
	refErrBuf := make([]C.char, errorBufferSize)
	status := C.get_references(cRepo,
		(*C.Reference)(unsafe.Pointer(&cRefs[0])),
		errBufPointer(refErrBuf),
		C.size_t(len(refErrBuf)))
	if status < 0 {
		return nil, errorFromStatus(status, refErrBuf)
	}

	n := int(status)
	refs := make([]Reference, n)
	for i := range refs {
		refs[i] = Reference{
			Hash:      C.GoString(&cRefs[i].hash[0]),
			Name:      C.GoString(&cRefs[i].name[0]),
			Shorthand: C.GoString(&cRefs[i].shorthand[0]),
		}
	}
	return refs, nil
}

func GetCommit(repoPath string, oidstr string) (Commit, error) {
	cRepo := C.CString(repoPath)
	defer C.free(unsafe.Pointer(cRepo))

	cOidstr := C.CString(oidstr)
	defer C.free(unsafe.Pointer(cOidstr))

	var cCommit C.Commit
	errBuf := make([]C.char, errorBufferSize)
	status := C.get_commit(cRepo, cOidstr, &cCommit, errBufPointer(errBuf), C.size_t(len(errBuf)))
	if status < 0 {
		return Commit{}, errorFromStatus(status, errBuf)
	}

	return commitFromC(&cCommit), nil
}

func GetRepoOwner(repoPath string) (string, error) {
	cRepo := C.CString(repoPath)
	defer C.free(unsafe.Pointer(cRepo))

	ownerBuf := make([]C.char, 256)
	errBuf := make([]C.char, errorBufferSize)

	status := C.get_repo_owner(cRepo,
		(*C.char)(unsafe.Pointer(&ownerBuf[0])),
		C.size_t(len(ownerBuf)),
		errBufPointer(errBuf),
		C.size_t(len(errBuf)))
	if status < 0 {
		return "", errorFromStatus(status, errBuf)
	}

	return C.GoString((*C.char)(unsafe.Pointer(&ownerBuf[0]))), nil
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
		Author:     Signature{C.GoString(&cCommit.author.name[0]), C.GoString(&cCommit.author.email[0])},
		Committer:  Signature{C.GoString(&cCommit.committer.name[0]), C.GoString(&cCommit.committer.email[0])},
		Timestamp:  time.Unix(int64(cCommit.timestamp), 0),
	}
}

func errBufPointer(buf []C.char) *C.char {
	if len(buf) == 0 {
		return nil
	}
	return (*C.char)(unsafe.Pointer(&buf[0]))
}

func errorFromStatus(status C.int, buf []C.char) error {
	message := ""
	if len(buf) > 0 {
		message = C.GoString((*C.char)(unsafe.Pointer(&buf[0])))
	}
	if message == "" {
		/* this should never happen! https://libgit2.org/docs/reference/main/errors/git_error_last.html */
		message = "unknown git error"
	}
	return fmt.Errorf("git error %d: %s", int(status), message)
}

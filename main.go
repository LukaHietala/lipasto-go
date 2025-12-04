package main

import (
	"net/http"
	"strconv"

	"lipasto/internal/git"

	"github.com/gin-gonic/gin"
)

const reposDir = "/home/lhietala/testing"

func main() {
	r := gin.Default()
	r.LoadHTMLGlob("templates/*")
	r.Static("/static", "./static")

	r.GET("/", func(c *gin.Context) {
		repos, err := git.ListBareRepos(reposDir, 100)
		if err != nil {
			respondWithGitError(c, err)
			return
		}
		if len(repos) == 0 {
			c.String(http.StatusNotFound, "no repositories found")
			return
		}
		c.HTML(http.StatusOK, "repos.html", gin.H{"Repos": repos})
	})

	r.GET("/:repo", func(c *gin.Context) {
		repoName := c.Param("repo")
		ref := c.DefaultQuery("ref", "HEAD")
		page, err := strconv.Atoi(c.DefaultQuery("page", "0"))

		if err != nil {
			c.String(http.StatusBadRequest, "invalid page number")
			return
		}

		repoPath := reposDir + "/" + repoName

		perPage := 50
		skip := page * perPage
		commits, gitErr := git.GetCommits(repoPath, ref, perPage, skip)
		if gitErr != nil {
			respondWithGitError(c, gitErr)
			return
		}
		if len(commits) == 0 {
			c.String(http.StatusNotFound, "repository has no commits in range (%d-%d)... how did we get here?", skip, skip+perPage-1)
			return
		}

		c.HTML(http.StatusOK, "commits.html", gin.H{"Commits": commits, "RepoName": repoName})
	})

	r.GET("/:repo/commit/:hash", func(c *gin.Context) {
		repoName := c.Param("repo")
		commitHash := c.Param("hash")
		repoPath := reposDir + "/" + repoName

		commit, err := git.GetCommit(repoPath, commitHash)
		if err != nil {
			respondWithGitError(c, err)
			return
		}

		c.HTML(http.StatusOK, "commit.html", gin.H{
			"Commit":   commit,
			"RepoName": repoName,
		})
	})

	r.GET("/:repo/refs", func(c *gin.Context) {
		repoName := c.Param("repo")
		repoPath := reposDir + "/" + repoName
		references, err := git.GetReferences(repoPath)
		if err != nil {
			respondWithGitError(c, err)
			return
		}
		if len(references) == 0 {
			c.String(http.StatusNotFound, "repository has no references")
			return
		}
		c.HTML(http.StatusOK, "refs.html", gin.H{"References": references, "RepoName": repoName})
	})

	r.Run()
}

func respondWithGitError(c *gin.Context, err error) {
	message := err.Error()
	if message == "" {
		message = "internal git error"
	}
	c.String(http.StatusInternalServerError, message)
}

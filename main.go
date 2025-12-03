package main

import (
	"lipasto/internal/git"

	"github.com/gin-gonic/gin"
)

const reposDir = "/home/lhietala/testing"

func main() {
	r := gin.Default()
	r.LoadHTMLGlob("templates/*")
	r.Static("/static", "./static")

	r.GET("/", func(c *gin.Context) {
		repos := git.ListBareRepos(reposDir, 100)
		if repos == nil {
			c.String(404, "no repositories found")
			return
		}
		c.HTML(200, "repos.html", gin.H{"Repos": repos})
	})

	r.GET("/:repo", func(c *gin.Context) {
		repoName := c.Param("repo")
		repoPath := reposDir + "/" + repoName
		commits := git.GetCommits(repoPath, 50)
		if commits == nil {
			c.String(404, "repository not found or has no commits")
			return
		}

		c.HTML(200, "commits.html", gin.H{"Commits": commits, "RepoName": repoName})
	})

	r.GET(":repo/commit/:hash", func(c *gin.Context) {
		repoName := c.Param("repo")
		commitHash := c.Param("hash")
		repoPath := reposDir + "/" + repoName

		commit := git.GetCommit(repoPath, commitHash)
		if commit.Hash == "" {
			c.String(404, "commit not found")
			return
		}

		c.HTML(200, "commit.html", gin.H{
			"Commit":   commit,
			"RepoName": repoName,
		})
	})

	r.GET("/:repo/refs", func(c *gin.Context) {
		repoName := c.Param("repo")
		repoPath := reposDir + "/" + repoName
		references := git.GetReferences(repoPath)
		if references == nil {
			c.String(404, "repository not found or has no references")
			return
		}
		c.HTML(200, "refs.html", gin.H{"References": references, "RepoName": repoName})
	})

	r.Run()
}

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
		c.HTML(200, "repos.html", gin.H{"Repos": repos})
	})

	r.GET("/:repo", func(c *gin.Context) {
		repoName := c.Param("repo")
		repoPath := reposDir + "/" + repoName
		commits := git.GetCommits(repoPath, 50)
		c.HTML(200, "commits.html", gin.H{"Commits": commits, "RepoName": repoName})
	})

	r.GET("/:repo/refs", func(c *gin.Context) {
		repoName := c.Param("repo")
		repoPath := reposDir + "/" + repoName
		references := git.GetReferences(repoPath)
		c.HTML(200, "refs.html", gin.H{"References": references, "RepoName": repoName})
	})

	r.Run()
}

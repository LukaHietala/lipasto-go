package main

import (
	"lipasto/internal/git"

	"github.com/gin-gonic/gin"
)

const reposDir = "/tmp"

func main() {
	r := gin.Default()
	r.LoadHTMLGlob("templates/*")

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

	r.Run()
}

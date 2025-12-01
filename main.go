package main

import (
	"lipasto/internal/git"

	"github.com/gin-gonic/gin"
)

func main() {
	r := gin.Default()
	r.LoadHTMLGlob("templates/*")

	r.GET("/commits", func(c *gin.Context) {
		commits := git.GetCommits("/tmp/bare-repo", 50)
		c.HTML(200, "commits.html", gin.H{"Commits": commits})
	})

	r.Run()
}

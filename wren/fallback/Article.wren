import "Article" for ArticleScript

class Script is ArticleScript {

  construct new(article) {
    super(article)
    article.fiber = Fiber.new { execute() }
  }

  execute() {
    wait_until(1)
  }

  update() {
  }

  destroy() {
  }
}

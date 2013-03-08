git = require '../lib/git'
path = require 'path'

describe "git", ->
  describe ".open(path)", ->
    describe "when the path is a repository", ->
      it "returns a repository", ->
        expect(git.open(__dirname)).not.toBeNull()

    describe "when the path isn't a repository", ->
      it "returns null", ->
        expect(git.open('/tmp/path/does/not/exist')).toBeNull()

  describe ".getPath()", ->
    it "returns the path to the .git directory", ->
      repositoryPath = git.open(__dirname).getPath()
      expect(repositoryPath).toBe path.join(path.dirname(__dirname), '.git/')

  describe ".getWorkingDirectory()", ->
    it "returns the path to the working directory", ->
      workingDirectory = git.open(__dirname).getWorkingDirectory()
      expect(workingDirectory).toBe path.dirname(__dirname)

  describe ".getHead()", ->
    describe "when a branch is checked out", ->
      it "returns the branch's full path", ->
        repo = git.open(path.join(__dirname, 'fixtures/master.git'))
        expect(repo.getHead()).toBe 'refs/heads/master'

    describe "when the HEAD is detached", ->
      it "return the SHA-1 that is checked out", ->
        repo = git.open(path.join(__dirname, 'fixtures/detached.git'))
        expect(repo.getHead()).toBe '50719ab369dcbbc2fb3b7a0167c52accbd0eb40e'

  describe ".getShortHead()", ->
    describe "when a branch is checked out", ->
      it "returns the branch's name", ->
        repo = git.open(path.join(__dirname, 'fixtures/master.git'))
        expect(repo.getShortHead()).toBe 'master'

    describe "when the HEAD is detached", ->
      it "return the abbreviated SHA-1 that is checked out", ->
        repo = git.open(path.join(__dirname, 'fixtures/detached.git'))
        expect(repo.getShortHead()).toBe '50719ab'

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

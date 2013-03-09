git = require '../lib/git'
path = require 'path'
fs = require 'fs'
wrench = require 'wrench'
temp = require 'temp'

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

  describe ".isIgnored(path)", ->
    describe "when the path is undefined", ->
      it "return false", ->
        repo = git.open(path.join(__dirname, 'fixtures/ignored.git'))
        expect(repo.isIgnored()).toBe false

    describe "when the path is ignored", ->
      it "returns true", ->
        repo = git.open(path.join(__dirname, 'fixtures/ignored.git'))
        expect(repo.isIgnored('a.txt')).toBe true

    describe "when the path is not ignored", ->
      it "return false", ->
        repo = git.open(path.join(__dirname, 'fixtures/ignored.git'))
        expect(repo.isIgnored('b.txt')).toBe false

  describe ".isSubmodule(path)", ->
    describe "when the path is undefined", ->
      it "return false", ->
        repo = git.open(path.join(__dirname, 'fixtures/submodule.git'))
        expect(repo.isSubmodule()).toBe false

    describe "when the path is a submodule", ->
      it "returns true", ->
        repo = git.open(path.join(__dirname, 'fixtures/submodule.git'))
        expect(repo.isSubmodule('a')).toBe true

    describe "when the path is not a submodule", ->
      it "return false", ->
        repo = git.open(path.join(__dirname, 'fixtures/submodule.git'))
        expect(repo.isSubmodule('b')).toBe false

  describe ".getConfigValue(key)", ->
    it "returns the value for the key", ->
      repo = git.open(path.join(__dirname, 'fixtures/master.git'))
      expect(repo.getConfigValue("core.repositoryformatversion")).toBe '0'
      expect(repo.getConfigValue("core.ignorecase")).toBe 'true'
      expect(repo.getConfigValue("not.section")).toBe null

  describe '.isPathModified(path)', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    describe 'when a path is deleted', ->
      it 'returns true', ->
        expect(repo.isPathModified('a.txt')).toBe true

    describe 'when a path is modified', ->
      it 'returns true', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'a.txt'), 'changing a.txt', 'utf8')
        expect(repo.isPathModified('a.txt')).toBe true

    describe 'when a path is new', ->
      it 'returns false', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'new.txt'), 'new', 'utf8')
        expect(repo.isPathModified('new.txt')).toBe false

  describe '.isPathNew(path)', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    describe 'when a path is deleted', ->
      it 'returns false', ->
        expect(repo.isPathNew('a.txt')).toBe false

    describe 'when a path is modified', ->
      it 'returns false', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'a.txt'), 'changing a.txt', 'utf8')
        expect(repo.isPathNew('a.txt')).toBe false

    describe 'when a path is new', ->
      it 'returns true', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'new.txt'), 'new', 'utf8')
        expect(repo.isPathNew('new.txt')).toBe true

  describe '.getUpstreamBranch()', ->
    describe 'when no upstream branch exists', ->
      it 'returns null', ->
        repo = git.open(path.join(__dirname, 'fixtures/master.git'))
        expect(repo.getUpstreamBranch()).toBe null

    describe 'when an upstream branch exists', ->
      it 'returns the full path to the branch', ->
        repo = git.open(path.join(__dirname, 'fixtures/upstream.git'))
        expect(repo.getUpstreamBranch()).toBe 'refs/remotes/origin/master'

  describe '.checkoutHead(path)', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    describe 'when the path exists', ->
      it 'replaces the file contents with the HEAD revision and returns true', ->
        filePath = path.join(repo.getWorkingDirectory(), 'a.txt')
        fs.writeFileSync(filePath, 'changing a.txt', 'utf8')
        expect(repo.checkoutHead('a.txt')).toBe true
        expect(fs.readFileSync(filePath, 'utf8')).toBe 'first line\n'

    describe 'when the path is undefined', ->
      it 'returns false', ->
        expect(repo.checkoutHead()).toBe false

  describe '.getReferenceTarget(branch)', ->
    it 'returns the SHA-1 for a reference', ->
      repo = git.open(path.join(__dirname, 'fixtures/master.git'))
      expect(repo.getReferenceTarget('HEAD2')).toBe null
      expect(repo.getReferenceTarget('HEAD')).toBe 'b2c96bdffe1a8f239c2d450863e4a6caa6dcb655'
      expect(repo.getReferenceTarget('refs/heads/master')).toBe 'b2c96bdffe1a8f239c2d450863e4a6caa6dcb655'

  describe '.getDiffStats(path)', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    describe 'when the path is deleted', ->
      it 'returns of number of lines deleted', ->
        expect(repo.getDiffStats('a.txt')).toEqual {added: 0, deleted: 1}
    describe 'when the path is modified', ->
      it 'returns the number of lines added and deleted', ->
        filePath = path.join(repo.getWorkingDirectory(), 'a.txt')
        fs.writeFileSync(filePath, 'changing\na.txt', 'utf8')
        expect(repo.getDiffStats('a.txt')).toEqual {added: 2, deleted: 1}

    describe 'when the path is new', ->
      it 'returns that no lines were added or deleted', ->
        filePath = path.join(repo.getWorkingDirectory(), 'b.txt')
        fs.writeFileSync(filePath, 'changing\nb.txt\nwith lines', 'utf8')
        expect(repo.getDiffStats('b.txt')).toEqual {added: 0, deleted: 0}

  describe '.getStatuses()', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)
      newFilePath = path.join(repo.getWorkingDirectory(), 'b.txt')
      fs.writeFileSync(newFilePath, '', 'utf8')


    it 'returns the status of all modified paths', ->
      statuses = repo.getStatuses()
      expect(statuses['a.txt']).toBe 1 << 9
      expect(statuses['b.txt']).toBe 1 << 7


  describe '.getAheadBehindCount()', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/ahead-behind.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    it 'returns the number of commits ahead of and behind the upstream branch', ->
      counts = repo.getAheadBehindCount()
      expect(counts).toEqual {ahead: 3, behind: 2}

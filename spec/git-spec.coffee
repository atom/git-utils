git = require '../lib/git'
path = require 'path'
fs = require 'fs'
{exec} = require 'child_process'
wrench = require 'wrench'
temp = require 'temp'
_ = require 'underscore'

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
      currentGitPath = path.join(path.dirname(__dirname), '.git/')
      currentGitPath = currentGitPath.replace(/\\/g, '/') if process.platform is 'win32'
      expect(repositoryPath).toBe currentGitPath

  describe ".getWorkingDirectory()", ->
    it "returns the path to the working directory", ->
      workingDirectory = git.open(__dirname).getWorkingDirectory()
      cwd = path.dirname(__dirname)
      cwd = cwd.replace(/\\/g, '/') if process.platform is 'win32'
      expect(workingDirectory).toBe cwd

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

  describe ".setConfigValue(key, value)", ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    it "sets the key to the value in the config", ->
      expect(repo.setConfigValue()).toBe false
      expect(repo.setConfigValue('1')).toBe false
      expect(repo.setConfigValue('a.b', 'test')).toBe true
      expect(repo.getConfigValue('a.b')).toBe 'test'
      expect(repo.setConfigValue('a.b.c', 'foo')).toBe true
      expect(repo.getConfigValue('a.b.c')).toBe 'foo'

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

  describe '.isPathDeleted(path)', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    describe 'when a path is deleted', ->
      it 'returns true', ->
        expect(repo.isPathDeleted('a.txt')).toBe true

    describe 'when a path is modified', ->
      it 'returns false', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'a.txt'), 'changing a.txt', 'utf8')
        expect(repo.isPathDeleted('a.txt')).toBe false

    describe 'when a path is new', ->
      it 'returns false', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'new.txt'), 'new', 'utf8')
        expect(repo.isPathDeleted('new.txt')).toBe false

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

  describe '.checkoutReference(reference, [create])', ->
    repo = null
    repoDirectory = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/references.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)
      expect(repo.getHead()).toBe 'refs/heads/master'

    describe 'when a local reference exists', ->
      it 'checks a branch out if passed a short reference', ->
        expect(repo.checkoutReference('getHeadOriginal')).toBe true
        expect(repo.getHead()).toBe 'refs/heads/getHeadOriginal'

      it 'checks a branch out if passed a long reference', ->
        expect(repo.checkoutReference('refs/heads/getHeadOriginal')).toBe true
        expect(repo.getHead()).toBe 'refs/heads/getHeadOriginal'

      # in this test, we need to fake a commit and try to switch to a new branch
      it 'does not check a branch out if the dirty tree interferes', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'README.md'), 'great words', 'utf8')
        gitCommandHandler = jasmine.createSpy('gitCommandHandler')
        exec "cd #{repoDirectory} && git add . && git commit -m 'update README'", gitCommandHandler

        waitsFor ->
          gitCommandHandler.callCount is 1

        runs ->
          expect(repo.checkoutReference('refs/heads/getHeadOriginal')).toBe true
          fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'README.md'), 'more words', 'utf8')
          expect(repo.checkoutReference('refs/heads/master')).toBe false
          expect(repo.getHead()).toBe 'refs/heads/getHeadOriginal'

      it 'does check a branch out if the dirty tree does not interfere', ->
        fs.writeFileSync(path.join(repo.getWorkingDirectory(), 'new_file.md'), 'a new file', 'utf8')
        expect(repo.checkoutReference('refs/heads/getHeadOriginal')).toBe true

    describe 'when a local reference doesn\'t exist', ->
      it 'does nothing if branch creation was not specified', ->
        expect(repo.checkoutReference('refs/heads/whoop-whoop')).toBe false

      it 'creates the new branch (if asked to)', ->
        expect(repo.checkoutReference('refs/heads/whoop-whoop', true)).toBe true
        expect(repo.getHead()).toBe 'refs/heads/whoop-whoop'

      it 'does nothing if the new branch is malformed (even if asked to)', ->
        expect(repo.checkoutReference('refs/heads/inv@{id', true)).toBe false
        expect(repo.getHead()).toBe 'refs/heads/master'

      describe 'when a short reference is passed', ->
        it 'does nothing if branch creation was not specified', ->
          expect(repo.checkoutReference('bananas')).toBe false

        it 'creates the new branch (if asked to)', ->
          expect(repo.checkoutReference('bananas', true)).toBe true
          expect(repo.getHead()).toBe 'refs/heads/bananas'

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

  describe '.getReferences()', ->
    it 'returns a list of all the references', ->
      referencesObj =
                      heads: [ 'refs/heads/diff-lines', 'refs/heads/getHeadOriginal', 'refs/heads/master' ]
                      remotes: [ 'refs/remotes/origin/HEAD', 'refs/remotes/origin/getHeadOriginal', 'refs/remotes/origin/master', 'refs/remotes/upstream/HEAD', 'refs/remotes/upstream/master' ]
                      tags: [ 'refs/tags/v1.0', 'refs/tags/v2.0' ]

      repo = git.open(path.join(__dirname, 'fixtures/references.git'))
      expect(repo.getReferences()).toEqual referencesObj

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

    describe 'when the repository has no HEAD', ->
      it 'returns that no lines were added and deleted', ->
        repoDirectory = temp.mkdirSync('node-git-repo-')
        wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
        repo = git.open(repoDirectory)
        fs.unlinkSync(path.join(repoDirectory, '.git/HEAD'))
        expect(repo.getDiffStats('b.txt')).toEqual {added: 0, deleted: 0}

  describe '.getHeadBlob(path)', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    describe 'when the path is modified', ->
      it 'returns the HEAD blob contents', ->
        filePath = path.join(repo.getWorkingDirectory(), 'a.txt')
        fs.writeFileSync(filePath, 'changing\na.txt', 'utf8')
        expect(repo.getHeadBlob('a.txt')).toBe 'first line\n'

    describe 'when the path is not modified', ->
      it 'returns the HEAD blob contents', ->
        expect(repo.getHeadBlob('a.txt')).toBe 'first line\n'

    describe 'when the path does not exist', ->
      it 'returns null', ->
        expect(repo.getHeadBlob('i-do-not-exist.txt')).toBeNull()

  describe '.getStatus([path])', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

      newFilePath = path.join(repo.getWorkingDirectory(), 'b.txt')
      fs.writeFileSync(newFilePath, '', 'utf8')

      fs.mkdirSync(path.join(repo.getWorkingDirectory(), '.git/info'))
      ignoreFile = path.join(repo.getWorkingDirectory(), '.git/info/exclude')
      fs.writeFileSync(ignoreFile, 'c.txt', 'utf8')
      ignoredFilePath = path.join(repo.getWorkingDirectory(), 'c.txt')
      fs.writeFileSync(ignoredFilePath, '', 'utf8')

    describe 'when no path is specified', ->
      it 'returns the status of all modified paths', ->
        statuses = repo.getStatus()
        expect(_.keys(statuses).length).toBe 2
        expect(statuses['a.txt']).toBe 1 << 9
        expect(statuses['b.txt']).toBe 1 << 7

    describe 'when a path is specified', ->
      it 'returns the status of the given path', ->
        expect(repo.getStatus('a.txt')).toBe 1 << 9

  describe '.getAheadBehindCount()', ->
    repo = null

    beforeEach ->
      repoDirectory = temp.mkdirSync('node-git-repo-')
      wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/ahead-behind.git'), path.join(repoDirectory, '.git'))
      repo = git.open(repoDirectory)

    it 'returns the number of commits ahead of and behind the upstream branch', ->
      counts = repo.getAheadBehindCount()
      expect(counts).toEqual {ahead: 3, behind: 2}

      counts = repo.getAheadBehindCount('refs/heads/master')
      expect(counts).toEqual {ahead: 3, behind: 2}

      counts = repo.getAheadBehindCount('master')
      expect(counts).toEqual {ahead: 3, behind: 2}

      counts = repo.getAheadBehindCount('refs/heads/masterblaster')
      expect(counts).toEqual {ahead: 0, behind: 0}

      counts = repo.getAheadBehindCount('')
      expect(counts).toEqual {ahead: 0, behind: 0}

  describe '.getLineDiffs(path, text)', ->
    it 'returns all hunks that differ', ->
      repo = git.open(path.join(__dirname, 'fixtures/master.git'))

      diffs = repo.getLineDiffs('a.txt', 'first line is different')
      expect(diffs.length).toBe 1
      expect(diffs[0].oldStart).toBe 1
      expect(diffs[0].oldLines).toBe 1
      expect(diffs[0].newStart).toBe 1
      expect(diffs[0].newLines).toBe 1

      diffs = repo.getLineDiffs('a.txt', 'first line\nsecond line')
      expect(diffs.length).toBe 1
      expect(diffs[0].oldStart).toBe 1
      expect(diffs[0].oldLines).toBe 0
      expect(diffs[0].newStart).toBe 2
      expect(diffs[0].newLines).toBe 1

      diffs = repo.getLineDiffs('a.txt', '')
      expect(diffs.length).toBe 1
      expect(diffs[0].oldStart).toBe 1
      expect(diffs[0].oldLines).toBe 1
      expect(diffs[0].newStart).toBe 0
      expect(diffs[0].newLines).toBe 0

    it "returns null for paths that don't exist", ->
      repo = git.open(path.join(__dirname, 'fixtures/master.git'))

      diffs = repo.getLineDiffs('i-dont-exists.txt', 'content')
      expect(diffs).toBeNull()

  describe '.relativize(path)', ->
    it 'relativizes the given path to the working directory of the repository', ->
      repo = git.open(__dirname)
      workingDirectory = repo.getWorkingDirectory()

      expect(repo.relativize(path.join(workingDirectory, 'a.txt'))).toBe 'a.txt'
      expect(repo.relativize(path.join(workingDirectory, 'a/b/c.txt'))).toBe 'a/b/c.txt'
      expect(repo.relativize('a.txt')).toBe 'a.txt'
      expect(repo.relativize('/not/in/working/dir')).toBe '/not/in/working/dir'
      expect(repo.relativize(null)).toBe null
      expect(repo.relativize()).toBeUndefined()
      expect(repo.relativize('')).toBe ''

    describe 'when the opened path is a symlink', ->
      it 'relativizes against both the linked path and the real path', ->
        # Creating symbol link on Windows requires administrator permission so
        # we just skip this test.
        return if process.platform is 'win32'

        repoDirectory = fs.realpathSync(temp.mkdirSync('node-git-repo-'))
        linkDirectory = path.join(fs.realpathSync(temp.mkdirSync('node-git-repo-')), 'link')
        wrench.copyDirSyncRecursive(path.join(__dirname, 'fixtures/master.git'), path.join(repoDirectory, '.git'))
        fs.symlinkSync(repoDirectory, linkDirectory)

        repo = git.open(linkDirectory)
        expect(repo.relativize(path.join(repoDirectory, 'test1'))).toBe 'test1'
        expect(repo.relativize(path.join(linkDirectory, 'test2'))).toBe 'test2'
        expect(repo.relativize(path.join(linkDirectory, 'test2/test3'))).toBe 'test2/test3'
        expect(repo.relativize('test2/test3')).toBe 'test2/test3'

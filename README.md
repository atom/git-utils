# Git Node module [![Build Status](https://travis-ci.org/atom/node-git.png)](https://travis-ci.org/atom/node-git)

Helpers for working with Git repositories built natively on top of
[libgit2](http://libgit2.github.com).

## Installing

```sh
npm install git-utils
```

## Building
  * Clone the repository with the `--recurse` option to get the libgit2
    submodule
  * Run `npm install`
  * Run `grunt` to compile the native and CoffeeScript code

## Documentation

### git.open(path)

Open the repository at the given path.  This will return `null` if the
repository at the given path does not exist or cannot be opened.

```coffeescript
git = require 'git-utils'

repository = git.open('/Users/me/repos/node')
```

### Repository.checkoutHead(path)

### Repository.getAheadBehindCount()

### Repository.getCommitCount(fromCommit, toCommit)

### Repository.getConfigValue(key)

### Repository.getDiffStats(path)

### Repository.getHeadBlob(path)

Similar to `git show HEAD:<path>`. Retrieves the blob as defined in HEAD.

### Repository.getHead()

### Repository.getLineDiffs(path, text)

### Repository.getMergeBase(commit1, commit2)

### Repository.getPath()

### Repository.getReferenceTarget(ref)

### Repository.getShortHead()

### Repository.getStatus([path])

### Repository.getUpstreamBranch([branch])

### Repository.getWorkingDirectory()

### Repository.isIgnored(path)

### Repository.isPathModified(path)

### Repository.isPathNew(path)

### Repository.isStatusModified(status)

### Repository.isStatusNew(status)

### Repository.isSubmodule(path)

### Repository.refreshIndex()

### Repository.release()

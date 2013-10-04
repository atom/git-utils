# Git Node module [![Build Status](https://travis-ci.org/atom/node-git.png?branch=master)](https://travis-ci.org/atom/node-git)

Helpers for working with Git repositories built natively on top of
[libgit2](http://libgit2.github.com).

This module requires [CMake](http://www.cmake.org/) for building libgit2 and it
is a prerequisite for both installing this module and building it locally.

## Installing

```sh
npm install git-utils
```

## Building
  * Clone the repository with the `--recurse` option to get the libgit2
    submodule
  * Install [CMake](http://www.cmake.org/)
  * Run `npm install`
  * Run `grunt` to compile the native and CoffeeScript code
  * Run `grunt test` to run the specs

## Docs

### git.open(path)

Open the repository at the given path.  This will return `null` if the
repository at the given path does not exist or cannot be opened.

```coffeescript
git = require 'git-utils'

repository = git.open('/Users/me/repos/node')
```

### Repository.checkoutHead(path)

Restore the contents of a path in the working directory and index to the
version at HEAD. Similar to running `git reset HEAD -- <path>` and then a
`git checkout HEAD -- <path>`.

`path` - The string repository-relative path to checkout.

Returns `true` if the checkout was successful, `false` otherwise.

### Repository.checkoutReference(reference, [create])

Checks out a branch in your repository.

`reference` - The string reference to checkout
`create` - A Boolean value which, if `true` creates the new `reference` if it doesn't exist.

Returns `true` if the checkout was successful, `false` otherwise.

### Repository.getAheadBehindCount(branch)

Get the number of commits the branch is ahead/behind the remote branch it
is tracking.  Similar to the commit numbers reported by `git status` when a
remote tracking branch exists.

`branch` - The branch name to lookup ahead/behind counts for. (default: `HEAD`)

Returns an object with `ahead` and `behind` keys pointing to integer values
that will always be >= 0.

### Repository.getCommitCount(fromCommit, toCommit)

Get the number of commits between `fromCommit` and `toCommit`.

`fromCommit` - The string commit SHA-1 to start the rev walk at.

`toCommit` - The string commit SHA-1 to end the rev walk at.

Returns the number of commits between the two, always >= 0.

### Repository.getConfigValue(key)

Get the config value of the given key.

`key` - The string key to retrieve the value for.

Returns the configuration value, may be `null`.

### Repository.setConfigValue(key, value)

Get the config value of the given key.

`key` - The string key to set in the config.

`value` - The string value to set in the config for the given key.

Returns `true` if setting the config value was successful, `false` otherwise.

### Repository.getDiffStats(path)

Get the number of lines added and removed comparing the working directory
contents of the given path to the HEAD version of the given path.

`path` - The string repository-relative path to diff.

Returns an object with `added` and `deleted` keys pointing to integer values
that always be >= 0.

### Repository.getHeadBlob(path)

Get the blob contents of the given path at HEAD. Similar to
`git show HEAD:<path>`.

`path` - The string repository-relative path.

Returns the string contents of the HEAD version of the path.

### Repository.getHead()

Get the reference or SHA-1 that HEAD points to such as `refs/heads/master`
or a full SHA-1 if the repository is in a detached HEAD state.

Returns the string reference name or SHA-1.

### Repository.getLineDiffs(path, text)

Get the line diffs comparing the HEAD version of the given path and the given
text.

`path` - The string repository-relative path.

`text` - The string text to diff the HEAD contents of the path against.

Returns an array of objects that have `oldStart`, `oldLines`, `newStart`, and
`newLines` keys pointing to integer values, may be `null` if the diff fails.

### Repository.getMergeBase(commit1, commit2)

Get the merge base of two commits.

`commit1` - The string SHA-1 of the first commit.

`commit2` - The string SHA-1 of the second commit.

Returns the string SHA-1 of the merge base of `commit1` and `commit2` or `null`
if there isn't one.

### Repository.getPath()

Get the path of the repository.

Returns the string absolute path of the opened repository.

### Repository.getReferences()

Gets all the local and remote references.

Returns an object with three keys: `heads`, `remotes`, and `tags`.
Each key can be an array of strings containing the reference names.

### Repository.getReferenceTarget(ref)

Get the target of the given reference.

`ref` - The string reference.

Returns the string target of the given reference.

### Repository.getShortHead()

Get a possibly shortened version of value returns by `getHead()`. This will
remove leading segments of `refs/heads`, `refs/tags`, or `refs/remotes` and will
also shorten the SHA-1 of a detached HEAD to 7 characters.

Returns a string shortened reference name or SHA-1.

### Repository.getStatus([path])

Get the status of a single path or all paths in the repository.  This will not
include ignored paths.

`path` - An optional repository-relative path to limit the status reporting to.

Returns an integer status number if a path is specified and returns an object
with path keys and integer status values if no path is specified.

### Repository.getUpstreamBranch([branch])

Get the upstream branch of the given branch.

`branch` - The branch to find the upstream branch of (default: `HEAD`)

Returns the string upstream branch reference name.

### Repository.getWorkingDirectory()

Get the working directory of the repository.

Returns the string absolute path to the repository's working directory.

### Repository.isIgnored(path)

Get the ignored status of a given path.

`path` - The string repository-relative path.

Returns `true` if the path is ignored, `false` otherwise.

### Repository.isPathModified(path)

Get the modified status of a given path.

`path` - The string repository-relative path.

Returns `true` if the path is modified, `false` otherwise.

### Repository.isPathNew(path)

Get the new status of a given path.

`path` - The string repository-relative path.

Returns `true` if the path is new, `false` otherwise.

### Repository.isPathDeleted(path)

Get the deleted status of a given path.

`path` - The string repository-relative path.

Returns `true` if the path is deleted, `false` otherwise.

### Repository.isStatusModified(status)

Check if a status value represents a modified path.

`status` - The integer status value.

Returns `true` if the status is a modified one, `false` otherwise.

### Repository.isStatusNew(status)

Check if a status value represents a new path.

`status` - The integer status value.

Returns `true` if the status is a new one, `false` otherwise.

### Repository.isStatusDeleted(status)

Check if a status value represents a deleted path.

`status` - The integer status value.

Returns `true` if the status is a deleted one, `false` otherwise.

### Repository.isSubmodule(path)

Check if the path is a submodule in the index.

`path` - The string repository-relative path.

Returns `true` if the path is a submodule, `false` otherwise.

### Repository.refreshIndex()

Reread the index to update any values that have changed since the last time the
index was read.

### Repository.relativize(path)

Relativize the given path to the repository's working directory.

`path` - The string path to relativize.

Returns a repository-relative path if the given path is prefixed with the
repository's working directory path.

### Repository.release()

Release the repository and close all file handles it has open.  No other methods
can be called on the `Repository` object once it has been released.

fs = require 'fs'
path = require 'path'
{Repository} = require('bindings')('git.node')

statusIndexNew = 1 << 0
statusIndexModified = 1 << 1
statusIndexDeleted = 1 << 2
statusIndexRenamed = 1 << 3
statusIndexTypeChange = 1 << 4
statusWorkingDirNew = 1 << 7
statusWorkingDirModified = 1 << 8
statusWorkingDirDelete = 1 << 9
statusWorkingDirTypeChange = 1 << 10
statusIgnore = 1 << 14

modifiedStatusFlags = statusWorkingDirModified | statusIndexModified |
                      statusWorkingDirDelete | statusIndexDeleted |
                      statusWorkingDirTypeChange | statusIndexTypeChange

newStatusFlags = statusWorkingDirNew | statusIndexNew

Repository::getWorkingDirectory = ->
  @getPath()?.replace(/\/\.git\/?$/, '')

Repository::getShortHead = ->
  head = @getHead()
  return head unless head?
  return head.substring(11) if head.indexOf('refs/heads/') is 0
  return head.substring(10) if head.indexOf('refs/tags/') is 0
  return head.substring(13) if head.indexOf('refs/remotes/') is 0
  return head.substring(0, 7) if head.match(/[a-fA-F0-9]{40}/)
  return head

Repository::isStatusModified = (status=0) ->
  (status & modifiedStatusFlags) > 0

Repository::isPathModified = (path) ->
  @isStatusModified(@getStatus(path))

Repository::isStatusNew = (status=0) ->
  (status & newStatusFlags) > 0

Repository::isPathNew = (path) ->
  @isStatusNew(@getStatus(path))

Repository::getUpstreamBranch = (branch) ->
  branch ?= @getHead()
  return null unless branch?.length > 11
  return null unless branch.indexOf('refs/heads/') is 0

  shortBranch = branch.substring(11)

  branchMerge = @getConfigValue("branch.#{shortBranch}.merge")
  return null unless branchMerge?.length > 11
  return null unless branchMerge.indexOf('refs/heads/') is 0

  branchRemote = @getConfigValue("branch.#{shortBranch}.remote")
  return null unless branchRemote?.length > 0

  "refs/remotes/#{branchRemote}/#{branchMerge.substring(11)}"

Repository::getAheadBehindCount = (branch='HEAD')->
  if branch isnt 'HEAD' and branch.indexOf('refs/heads/') isnt 0
    branch = "refs/heads/#{branch}"

  counts =
    ahead: 0
    behind: 0
  headCommit = @getReferenceTarget(branch)
  return counts unless headCommit?.length > 0

  upstream = @getUpstreamBranch()
  return counts unless upstream?.length > 0
  upstreamCommit = @getReferenceTarget(upstream)
  return counts unless upstreamCommit?.length > 0

  mergeBase = @getMergeBase(headCommit, upstreamCommit)
  return counts unless mergeBase?.length > 0

  counts.ahead = @getCommitCount(headCommit, mergeBase)
  counts.behind = @getCommitCount(upstreamCommit, mergeBase)
  counts

Repository::relativize = (path) ->
  return path unless path?
  return path unless path[0] is '/'

  workingDirectory = @getWorkingDirectory()
  if workingDirectory and path.indexOf("#{workingDirectory}/") is 0
    path.substring(workingDirectory.length + 1)
  else if @openedWorkingDirectory and path.indexOf("#{@openedWorkingDirectory}/") is 0
    path.substring(@openedWorkingDirectory.length + 1)
  else
    path

realpath = (unrealPath) ->
  try
    fs.realpathSync(unrealPath)
  catch e
    unrealPath

exports.open = (repositoryPath) ->
  symlink = realpath(repositoryPath) isnt repositoryPath

  repository = new Repository(repositoryPath)
  if repository.exists()
    if symlink
      workingDirectory = repository.getWorkingDirectory()
      while repositoryPath isnt path.sep
        if realpath(repositoryPath) is workingDirectory
          repository.openedWorkingDirectory = repositoryPath
          break
        repositoryPath = path.resolve(repositoryPath, '..')
    repository
  else
    null

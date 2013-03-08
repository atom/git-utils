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

Repository.prototype.getWorkingDirectory = ->
  @getPath()?.replace(/\/\.git\/?$/, '')

Repository.prototype.getShortHead = ->
  head = @getHead()
  return head.substring(11) if head.indexOf('refs/heads/') is 0
  return head.substring(10) if head.indexOf('refs/tags/') is 0
  return head.substring(13) if head.indexOf('refs/remotes/') is 0
  return head.substring(0, 7) if head.match(/[a-fA-F0-9]{40}/)
  return head

Repository.prototype.isPathModified = (path) ->
  (@getStatus(path) & modifiedStatusFlags) > 0

Repository.prototype.isPathNew = (path) ->
  (@getStatus(path) & newStatusFlags) > 0

Repository.prototype.getUpstreamBranch = (branch) ->
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

exports.open = (path) ->
  repository = new Repository(path)
  if repository.exists()
    repository
  else
    null

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

modifiedStatusFlags = statusWorkingDirModified |
                      statusWorkingDirDelete |
                      statusWorkingDirTypeChange |
                      statusIndexModified |
                      statusIndexDeleted |
                      statusIndexTypeChange


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

exports.open = (path) ->
  repository = new Repository(path)
  if repository.exists()
    repository
  else
    null

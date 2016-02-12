path = require 'path'
fs = require 'fs-plus'
{Repository} = require '../build/Release/git.node'

statusIndexNew = 1 << 0
statusIndexModified = 1 << 1
statusIndexDeleted = 1 << 2
statusIndexRenamed = 1 << 3
statusIndexTypeChange = 1 << 4
statusWorkingDirNew = 1 << 7
statusWorkingDirModified = 1 << 8
statusWorkingDirDelete = 1 << 9
statusWorkingDirTypeChange = 1 << 10
statusIgnored = 1 << 14
statusConflicted = 1 << 15

modifiedStatusFlags = statusWorkingDirModified | statusIndexModified |
                      statusWorkingDirDelete | statusIndexDeleted |
                      statusWorkingDirTypeChange | statusIndexTypeChange |
                      statusConflicted

newStatusFlags = statusWorkingDirNew | statusIndexNew

deletedStatusFlags = statusWorkingDirDelete | statusIndexDeleted

indexStatusFlags = statusIndexNew | statusIndexModified |
                   statusIndexDeleted | statusIndexRenamed |
                   statusIndexTypeChange

Repository::release = ->
  submoduleRepo?.release() for submodulePath, submoduleRepo of @submodules
  @_release()

Repository::getWorkingDirectory = ->
  @workingDirectory ?= @_getWorkingDirectory()?.replace(/\/$/, '')

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

Repository::isStatusDeleted = (status=0) ->
  (status & deletedStatusFlags) > 0

Repository::isPathDeleted = (path) ->
  @isStatusDeleted(@getStatus(path))

Repository::isStatusConflicted = (status=0) ->
  (status & statusConflicted) > 0

Repository::isPathConflicted = (path) ->
  @isStatusConflicted(@getStatus(path))

Repository::isPathStaged = (path) ->
  @isStatusStaged(@getStatus(path))

Repository::isStatusIgnored = (status=0) ->
  (status & statusIgnored) > 0

Repository::isStatusStaged = (status=0) ->
  (status & indexStatusFlags) > 0

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

Repository::checkoutReference = (branch, create)->
  if branch.indexOf('refs/heads/') isnt 0
    branch = "refs/heads/#{branch}"

  @checkoutRef(branch, create)

Repository::relativize = (path) ->
  return path unless path

  if process.platform is 'win32'
    path = path.replace(/\\/g, '/')
  else
    return path unless path[0] is '/'

  if @caseInsensitiveFs
    lowerCasePath = path.toLowerCase()

    workingDirectory = @getWorkingDirectory()
    if workingDirectory
      workingDirectory = workingDirectory.toLowerCase()
      if lowerCasePath.indexOf("#{workingDirectory}/") is 0
        return path.substring(workingDirectory.length + 1)
      else if lowerCasePath is workingDirectory
        return ''

    if @openedWorkingDirectory
      workingDirectory = @openedWorkingDirectory.toLowerCase()
      if lowerCasePath.indexOf("#{workingDirectory}/") is 0
        return path.substring(workingDirectory.length + 1)
      else if lowerCasePath is workingDirectory
        return ''
  else
    workingDirectory = @getWorkingDirectory()
    if workingDirectory
      if path.indexOf("#{workingDirectory}/") is 0
        return path.substring(workingDirectory.length + 1)
      else if path is workingDirectory
        return ''

    if @openedWorkingDirectory
      if path.indexOf("#{@openedWorkingDirectory}/") is 0
        return path.substring(@openedWorkingDirectory.length + 1)
      else if path is @openedWorkingDirectory
        return ''

  path

Repository::submoduleForPath = (path) ->
  path = @relativize(path)
  return null unless path

  for submodulePath, submoduleRepo of @submodules
    if path is submodulePath
      return submoduleRepo
    else if path.indexOf("#{submodulePath}/") is 0
      # Handle submodules inside of submodules
      path = path.substring(submodulePath.length + 1)
      return submoduleRepo.submoduleForPath(path) ? submoduleRepo

  null

Repository::isWorkingDirectory = (path) ->
  return false unless path

  if process.platform is 'win32'
    path = path.replace(/\\/g, '/')
  else
    return false unless path[0] is '/'

  if @caseInsensitiveFs
    lowerCasePath = path.toLowerCase()
    return true if lowerCasePath is @getWorkingDirectory()?.toLowerCase()
    return true if lowerCasePath is @openedWorkingDirectory?.toLowerCase()
  else
    return true if path is @getWorkingDirectory()
    return true if path is @openedWorkingDirectory

  false

realpath = (unrealPath) ->
  try
    fs.realpathSync(unrealPath)
  catch e
    unrealPath

isRootPath = (repositoryPath) ->
  if process.platform is 'win32'
    /^[a-zA-Z]+:[\\\/]$/.test(repositoryPath)
  else
    repositoryPath is path.sep

openRepository = (repositoryPath) ->
  symlink = realpath(repositoryPath) isnt repositoryPath

  repositoryPath = repositoryPath.replace(/\\/g, '/') if process.platform is 'win32'
  repository = new Repository(repositoryPath)
  if repository.exists()
    repository.caseInsensitiveFs = fs.isCaseInsensitive()
    if symlink
      workingDirectory = repository.getWorkingDirectory()
      while not isRootPath(repositoryPath)
        if realpath(repositoryPath) is workingDirectory
          repository.openedWorkingDirectory = repositoryPath
          break
        repositoryPath = path.resolve(repositoryPath, '..')
    repository
  else
    null

openSubmodules = (repository) ->
  repository.submodules = {}
  for relativePath in repository.getSubmodulePaths() when relativePath
    submodulePath = path.join(repository.getWorkingDirectory(), relativePath)
    if submoduleRepo = openRepository(submodulePath)
      if submoduleRepo.getPath() is repository.getPath()
        submoduleRepo.release()
      else
        openSubmodules(submoduleRepo)
        repository.submodules[relativePath] = submoduleRepo

exports.open = (repositoryPath) ->
  repository = openRepository(repositoryPath)
  openSubmodules(repository) if repository?
  repository

{Repository} = require('bindings')('git.node')

Repository.prototype.getWorkingDirectory = ->
  @getPath()?.replace(/\/\.git\/?$/, '')

exports.open = (path) ->
  repository = new Repository(path)
  if repository.exists()
    repository
  else
    null

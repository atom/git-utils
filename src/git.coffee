{Repository} = require('bindings')('git.node')

exports.open = (path) ->
  repository = new Repository(path)
  if repository.exists()
    repository
  else
    null

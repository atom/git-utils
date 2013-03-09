# Git Node module

Helpers for working with Git repositories built natively on
[libgit2](http://libgit2.github.com).

## Building
  * Clone the repository
  * Run `npm install`
  * Run `grunt` to compile the native and CoffeeScript code

## Installing

```sh
npm install git-helpers
```

## Documentation

### git.open(path)

```coffeescript
git = require 'git-helpers'

repository = git.open('/Users/me/repos/node')
```

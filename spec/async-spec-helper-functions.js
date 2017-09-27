exports.beforeEach = function beforeEach (fn) {
  global.beforeEach(function () {
    const result = fn()
    if (result instanceof Promise) {
      waitsForPromise(result, this)
    }
  })
}

exports.afterEach = function afterEach (fn) {
  global.afterEach(function () {
    const result = fn()
    if (result instanceof Promise) {
      waitsForPromise(result, this)
    }
  })
}

;['it', 'fit', 'ffit', 'fffit'].forEach((name) => {
  exports[name] = function (description, fn) {
    if (fn === undefined) {
      global[name](description)
      return
    }

    global[name](description, function () {
      const result = fn()
      if (result instanceof Promise) {
        waitsForPromise(result, this)
      }
    })
  }
})

function waitsForPromise (promise, spec) {
  let done = false
  let error = null

  global.waitsFor('test promise to resolve', () => {
    return done
  })

  promise.then(
    () => {
      done = true
    },
    (e) => {
      spec.fail(e)
      done = true
    }
  )
}

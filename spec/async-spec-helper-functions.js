exports.beforeEach = function beforeEach (fn) {
  global.beforeEach(function () {
    const result = fn()
    if (result instanceof Promise) {
      waitsForPromise(result)
    }
  })
}

exports.afterEach = function afterEach (fn) {
  global.afterEach(function () {
    const result = fn()
    if (result instanceof Promise) {
      waitsForPromise(result)
    }
  })
}

;['it', 'fit', 'ffit', 'fffit'].forEach((name) => {
  exports[name] = function (description, fn) {
    if (fn === undefined) {
      global[name](description)
      return
    }

    global[name](description, () => {
      const result = fn()
      if (result instanceof Promise) {
        waitsForPromise(result)
      }
    })
  }
})

function waitsForPromise (promise) {
  let done = false
  let error = null

  global.waitsFor('test promise to resolve', () => {
    expect(error).toBeNull()
    return done
  })

  promise.then(
    () => {
      done = true
    },
    (e) => {
      error = e
      done = true
    },
  )
}

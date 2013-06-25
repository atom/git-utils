module.exports = (grunt) ->
  grunt.initConfig
    pkg: grunt.file.readJSON('package.json')

    coffee:
      glob_to_multiple:
        expand: true
        cwd: 'src'
        src: ['*.coffee']
        dest: 'lib'
        ext: '.js'

    coffeelint:
      options:
        no_empty_param_list:
          level: 'error'
        max_line_length:
          level: 'ignore'

      src: ['src/**/*.coffee']
      test: ['spec/**/*.coffee']

    cpplint:
      files: ['src/**/*.cc']
      reporter: 'spec'
      verbosity: 1
      filters:
        build:
          include: false
        legal:
          copyright: false
        readability:
          braces: false
        runtime:
          sizeof: false
        whitespace:
          line_length: false

    shell:
      rebuild:
        command: 'npm build .'
        options:
          stdout: true
          stderr: true
          failOnError: true

      test:
        command: 'jasmine-focused --captureExceptions --coffee spec/'
        options:
          stdout: true
          stderr: true
          failOnError: true

  grunt.loadNpmTasks('grunt-contrib-coffee')
  grunt.loadNpmTasks('grunt-shell')
  grunt.loadNpmTasks('grunt-coffeelint')
  grunt.loadNpmTasks('node-cpplint')
  grunt.registerTask('lint', ['coffeelint', 'cpplint'])
  grunt.registerTask('default', ['lint', 'coffee', 'shell:rebuild'])
  grunt.registerTask('test', ['default', 'shell:test'])
  grunt.registerTask 'clean', ->
    path = require 'path'
    rm = require('rimraf').sync
    rm('build')
    rm(path.join('deps', 'libgit2', 'build'))

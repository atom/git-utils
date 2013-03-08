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

    shell:
      rebuild:
        command: 'node-gyp rebuild'
        options:
          stdout: true
          stderr: true

      test:
        command: 'jasmine-node --coffee spec/'
        options:
          stdout: true
          stderr: true

  grunt.loadNpmTasks('grunt-contrib-coffee')
  grunt.loadNpmTasks('grunt-shell')
  grunt.registerTask('default', ['coffee', 'shell:rebuild'])
  grunt.registerTask('test', ['coffee', 'shell'])

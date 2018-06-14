var gulp = require('gulp');
var gzip = require('gulp-gzip');
var gfi  = require('gulp-file-insert');
var rename = require('gulp-rename');
var livereload = require('gulp-livereload');

// Takes the original index.html.dev file and creates 2 files
// index.html    for localhost:8000
// index.html.gz for production
gulp.task('compress', function(){
  gulp.src('../esp/data/index.html.dev')
    .pipe(gfi({
      "/* inject css.css */": "../esp/data/css.css",
      "// inject vue.min.js": "../esp/data/vue.min.js",
      "// inject main.js": "../esp/data/main.js"
    }))
    .pipe(rename('./index.html'))
    .pipe(gulp.dest('../esp/data/'))
    .pipe(gzip())
    .pipe(rename('./index.gz'))
    .pipe(gulp.dest('../esp/data/'))
    .pipe(livereload());
});

// Watch changes to index.html.dev, css.css, main.js'
gulp.task('watch', function(){
  livereload.listen();
  gulp.watch(['../esp/data/index.html.dev', '../esp/data/css.css', '../esp/data/main.js'], ['compress']);
})

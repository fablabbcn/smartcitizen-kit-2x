var gulp = require('gulp');
var gzip = require('gulp-gzip');
var gfi  = require('gulp-file-insert');
var rename = require('gulp-rename');
var livereload = require('gulp-livereload');

// Takes the original build_data/build_index.html file and creates 2 files
// build_data/index.html    for localhost:8000
// data/index.html.gz for production
gulp.task('compress', function(){
  gulp.src('../esp/build_data/index.html')
    .pipe(gfi({
      "/* inject css.css */": "../esp/build_data/css.css",
      "// inject vue.min.js": "../esp/build_data/vue.min.js",
      "// inject main.js": "../esp/build_data/main.js"
    }))
    .pipe(rename('./index.html'))
    .pipe(gulp.dest('../esp/build_data/'))
    .pipe(gzip())
    .pipe(rename('./index.html.gz'))
    .pipe(gulp.dest('../esp/data/'))
    .pipe(livereload());
});

// Watch changes to index.html.dev, css.css, main.js'
gulp.task('watch', function(){
  livereload.listen();
  gulp.watch(['../esp/build_data/index.html', '../esp/build_data/css.css', '../esp/build_data/main.js'], ['compress']);
})

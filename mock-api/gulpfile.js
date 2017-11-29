var gulp = require('gulp');
var gzip = require('gulp-gzip');
var gfi  = require('gulp-file-insert');

gulp.task('compress', function(){
  gulp.src('../esp/data/index.html')
    .pipe(gfi({
      "/* inject css.css */": "../esp/data/css.css",
      "// inject vue.min.js": "../esp/data/vue.min.js",
      "// inject main.js": "../esp/data/main.js"
    }))
    .pipe(gzip())
    .pipe(gulp.dest('../esp/data/'));
});

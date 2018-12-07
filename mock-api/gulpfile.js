var gulp = require('gulp');
var gzip = require('gulp-gzip');
var gfi  = require('gulp-file-insert');
var rename = require('gulp-rename');
var livereload = require('gulp-livereload');
var favicon = require('gulp-base64-favicon');
var fs = require('fs');

// Takes the original build_data/build_index.html file and creates 2 files
// build_data/index.html    for localhost:8000
//  and build_data/index.html.gz for generating the embeded header file
gulp.task('compress', function(){
  gulp.src('../esp/build_data/build_index.html')
    .pipe(favicon("../esp/build_data"))
    .pipe(gfi({
      '/* inject css.css */': '../esp/build_data/css.css',
      '// inject vue.min.js': '../esp/build_data/vue.min.js',
      '// inject main.js': '../esp/build_data/main.js'
    }))
    .pipe(rename('./index.html'))
    .pipe(gulp.dest('../esp/build_data/'))
    .pipe(gzip())
    .pipe(rename('./index.html.gz'))
    .pipe(gulp.dest('../esp/build_data/'))
    .pipe(livereload());
});

// Watch changes to index.html.dev, css.css, main.js'
gulp.task('watch', function(){
  livereload.listen();
  gulp.watch(['../esp/build_data/build_index.html', '../esp/build_data/css.css', '../esp/build_data/main.js'], ['buildfs_embeded']);
})

// Generates header file for embeding the html page
gulp.task('buildfs_embeded', ['compress'], function() {
 
    var source = '../esp/build_data/index.html.gz';
    var destination = '../esp/src/index.html.gz.h';
 
    var wstream = fs.createWriteStream(destination);
    wstream.on('error', function (err) {
        console.log(err);
    });
 
    var data = fs.readFileSync(source);
 
    wstream.write('#define index_html_gz_len ' + data.length + '\n');
    wstream.write('const uint8_t index_html_gz[] PROGMEM = {')
 
    for (i=0; i<data.length; i++) {
        if (i % 1000 == 0) wstream.write("\n");
        wstream.write('0x' + ('00' + data[i].toString(16)).slice(-2));
        if (i<data.length-1) wstream.write(',');
    }
 
    wstream.write('\n};')
    wstream.end();
});

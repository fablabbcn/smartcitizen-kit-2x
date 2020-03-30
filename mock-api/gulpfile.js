const { series } = require('gulp');
const { parallel } = require('gulp');
const { watch } = require('gulp');
var log = require('fancy-log');
var gulp = require('gulp');
var gzip = require('gulp-gzip');
var gfi  = require('gulp-file-insert');
var rename = require('gulp-rename');
var livereload = require('gulp-livereload');
// favicon lib has not been updated in 3 years, needs to support Gulp 4
var favicon = require('gulp-base64-favicon');
var fs = require('fs');
var del = require('del');

// Takes the original build_data/build_index.html file and creates 2 files
// build_data/index.html    for localhost:8000
//  and build_data/index.html.gz for generating the embeded header file
function compress(cb){
  log('compressing..')
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

  cb();
};

function clean() {
  return del([
    '../esp/build_data/index.html.gz',
    '../esp/src/index.html.gz.h'
  ], {force: true});
};

// Watch changes to index.html.dev, css.css, main.js
function watchFiles(){
  livereload.listen();
  gulp.watch(
    [
      '../esp/build_data/*_index.html',
      '../esp/build_data/css.css',
      '../esp/build_data/main.js'
    ],
    series(compress, buildfs)
  )
}

// Generates header file for embeding the html page
function buildfs(cb) {
  log('building fs...');
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

  cb();
}

exports.clean = clean;
exports.compress = compress;
exports.buildfs = buildfs;
exports.watch = watchFiles;

exports.default = series(watchFiles);

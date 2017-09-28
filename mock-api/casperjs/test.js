// TODO: Change url to a server we can boot up locally

var url = 'https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/'

if (false){
  // If we are running webserver locally with 'python SimpleHTTPServer' f.x.
  url = 'http://localhost:8000'
}

var x = require('casper').selectXPath;

casper.on('http.status.404', function(resource){
  this.echo('404 -- page not found: ' + resource.url);
});


casper.test.begin("Find a few elements on our github webpage", 7, function(test) {
  casper.start(url, function() {
  }).then(function() {
    test.assertEvalEquals(function(){
      return document.querySelectorAll('#wifisetup > form > .field-group')[1].outerText;
    }, 'Password\n');
    test.assertTitleMatch(/^SCK/i, 'The page title starts with SCK');
    test.assertExists('#main', '#main exist');
    test.assertExists('form', 'form exist');
    test.assertExists('#main > .btn', '#main buttons exist');
    test.assertSelectorHasText('#main > .btn', 'OnlineOffline');
    test.assertVisible('#wifisetup', '#wifisetup is visible');
    this.echo(' __ Done __')
  }).run(function() {
    test.done();
  });
});

casper.test.begin("Fill in the form and submit", 0, function(test) {
  casper.start('https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/');

  casper.then(function(){
    console.log(' ---- waiting 1 sec for page load (github) ---- ');
    casper.wait(1400, function(){

      //console.log("Page Title " + document.location.href)
      //console.log(document.querySelector('form'))
      //casper.waitUntilVisible('.school_nav.content-panel');
      //this.captureSelector('captures/schools-nav-open.png', '.school_nav.content-panel');

      this.waitForSelector('form', function(){
        this.fill('form', {
          'token': '123456',
          'password': 'SuperPssword'
        }, true);

        //casper.capture('xx.png');
        casper.click('#connect');
      });

    });
  }).run(function() {
    test.done();
  });

});



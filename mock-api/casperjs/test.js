// TODO: Change url to a server we can boot up locally

var url = 'http://localhost:8000'

var x = require('casper').selectXPath;

casper.on('http.status.404', function(resource){
  this.echo('404 -- page not found: ' + resource.url);
});

casper.on('remote.message', function(msg){
  this.echo('-- remote message: ' + msg)
})

//casper.test.begin("Find a few elements on our github webpage", 4, function(test) {
//  console.log('Testing on: ' + urlgh);
//  casper.start(urlgh);
//  console.log('Waiting 1.4 sec for page to load');
//  casper.then(function() {
//    casper.wait(1400, function(){
//      test.assertEvalEquals(function(){
//        return document.querySelectorAll('#wifisetup > form > .field-group')[1].outerText;
//      }, 'Password\n');
//      test.assertTitleMatch(/^SCK/i, 'The page title starts with SCK');
//      test.assertExists('#main', '#main exist');
//      test.assertExists('form', 'form exist');
//      this.echo(' __ Done __')
//
//    });
//  }).run(function() {
//    test.done();
//  });
//});

casper.test.begin("Find elements on our local page", 5, function(test) {
  console.log('Testing on: ' + url);

  casper.start(url);

  casper.then(function(){
    //casper.echo("-- document.location.href: " + document.location.href);
    this.echo('CurrentUrl: ' + this.getCurrentUrl());
    //console.log(document.querySelector('form'))
    //casper.waitUntilVisible('.school_nav.content-panel');
    //this.captureSelector('captures/schools-nav-open.png', '.school_nav.content-panel');
    test.assertSelectorHasText('#connect', 'Connect');
    test.assertTitleMatch(/^SCK/i, 'The page title starts with SCK');
    test.assertTitle('SCK Setup', 'The page title is exactly SCK Setup');
    test.assertExists('#refreshbtn', '#refreshbtn exists - (Refresh Wifi button)');
    test.assertSelectorHasText('#advancedbtn', 'Advanced');

    this.waitForSelector('form', function(){
      this.fill('form', {
        'token': '123456',
        'password': 'SuperPssword'
      }, true);

      //this.echo(this.getFormValues('form').token);
      //this.echo(this.getFormValues('form').password);

      casper.click('#connect');
      //casper.wait(3000)
      casper.capture('xx.png');
    });

    //this.debugPage();
    //this.debugHTML();

  }).run(function() {
    test.done();
  });

});



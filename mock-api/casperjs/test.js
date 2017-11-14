// TODO: Change url to a server we can boot up locally

var url = 'http://localhost:8000'

var x = require('casper').selectXPath;

casper.on('http.status.404', function(resource){
  this.echo('404 -- page not found: ' + resource.url);
});

casper.on('remote.message', function(msg){
  this.echo('--> remote msg: ' + msg)
});

casper.test.begin("Find elements on our local page", 7, function(test) {
  console.log('Testing on: ' + url);

  casper.start(url);

  casper.then(function(){
    casper.capture('0.png');
    //casper.echo("-- document.location.href: " + document.location.href);
    this.echo('CurrentUrl: ' + this.getCurrentUrl());
    //casper.waitUntilVisible('.school_nav.content-panel');
    //this.captureSelector('captures/schools-nav-open.png', '.school_nav.content-panel');
    test.assertSelectorHasText('#connect', 'Connect');
    test.assertTitleMatch(/^SCK/i, 'The page title starts with SCK');
    test.assertTitle('SCK Setup', 'The page title is exactly SCK Setup');
    //test.assertExists('#refreshbtn', '#refreshbtn exists - (Refresh Wifi button)');
    test.assertExists('#ssid', 'Wifi dropdown exists');
    test.assertExists('#label-advanced', 'Checkbox for advanced exists');
    casper.click('#start');
  }).then(function(){
    casper.capture('1.png');

    this.waitForSelector('.field-token', function(){
      this.fill('.field-token', {
        'token': '123451',
      }, true);
      //this.echo(this.getElementAttribute('input[name="token"]').token);
      //this.echo(this.getFormValues('form').password);
    });

  }).then(function(){
    casper.capture('2.png');
    casper.click('.next');

  }).then(function(){
    casper.capture('3.png');
    casper.sendKeys('input[name="password"]', 'xxx')

    // Select wifi dropdown
    this.evaluate(function(){
      document.querySelector('#ssid').selectedIndex = 1;
    });

  }).then(function(){
    casper.capture('4.png');
    // Connect
    casper.click('#connect');

    // Debug helpers
    //this.debugPage();
    //this.debugHTML();
    //console.log(document.querySelector('form'))
    //require('utils').dump( this.getElementInfo('#ssid') );
  }).then(function(){
    // Check for RED GREEN text
    test.assertSelectorHasText('li', 'RED');
    test.assertSelectorHasText('li', 'GREEN');

    casper.capture('5.png');
  }).run(function() {
    test.done();
  });

});

var url = 'http://localhost:3000/status'
casper.test.begin("Find elements on our local page", 0, function(test) {

  casper.start(url);

  casper.then(function(){
    casper.echo('Testing API');
    //test.assertTitle('ab');  // No titles on an API?
    //this.debugPage();
  }).run(function() {
    test.done();
  });
});

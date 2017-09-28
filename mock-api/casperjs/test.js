var x = require('casper').selectXPath;

casper.test.begin("Find a few elements on our github webpage", 4, function(test) {
  casper.start('https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/', function() {
  }).then(function() {
    test.assertEvalEquals(function(){
      return document.querySelectorAll('#wifisetup > form > .field-group')[1].outerText;
    }, 'Password\n');
    test.assertTitleMatch(/^SCK/i, 'The page title starts with SCK');
    test.assertExists('#main', '#main exist');
    test.assertExists('form', 'form exist');
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
          'password': 'SuperPassword'
        }, true);

        //casper.capture('xx.png');
        casper.click('#connect');
      });



    });
  }).run(function() {
    test.done();
  });


});



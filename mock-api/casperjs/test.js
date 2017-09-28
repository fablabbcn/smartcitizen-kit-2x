var x = require('casper').selectXPath;

casper.test.begin("Find something on our github webpage", 1, function(test) {
    casper.start('https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/', function() {
    }).then(function() {
        test.assertEvalEquals(function(){
            return document.querySelectorAll('#wifisetup > form > .field-group')[1].outerText;
        }, 'Password\n');
    }).run(function() {
        test.done();
    });
});

casper.test.begin("Fill in the form", 1, function(test) {
  casper.start('https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/');

  casper.then(function(){

    console.log('waiting 2 sec');
    casper.wait(2000, function(){
      var x = this.echo(this.getTitle());

      test.assertTitleMatch(/^SCK/i, 'The title starts with SCK');
      test.assertExists('body', 'We have a body');
      //test.assertExists('form', 'We have a form');
      //test.assertExists('label', 'We have a label');

      //console.log("Page Title " + document.location.href)
      //console.log(document.querySelector('form'))
      //casper.waitUntilVisible('.school_nav.content-panel');

      //this.captureSelector('captures/schools-nav-open.png', '.school_nav.content-panel');


      //this.waitForSelector('form', function(){
      //  this.fill('form', {
      //    'token': '123456',
      //    'password': 'abcvd'
      //  }, true);
      //});

      casper.capture('xx.png');

      //this.thenClick(x('//*[@id="app"]/form/div[4]/button[1]'), function(){
      //  console.log("Connecting ...");
      //});

    });
  }).run(function() {
    test.done();
  });


});



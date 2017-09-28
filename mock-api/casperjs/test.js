casper.test.begin("Find something on a webpage", 1, function(test) {
    casper.start('https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/', function() {
        //this.waitForSelector('form[action="/search"]');
    }).then(function() {
        test.assertEvalEquals(function(){
            return document.querySelector('h2').outerText;
        }, 'Wifi Setup');
    }).run(function() {
        test.done();
    });

    casper.start('https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/', function() {
    }).then(function() {
        test.assertEvalEquals(function(){
            return document.querySelector('body > #app > form > .field-group > label').outerText;
        }, 'SSID');
    }).run(function() {
        test.done();
    });
});



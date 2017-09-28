# SmartCitizenKit
Developer version of Smart Citizen Kit 1.5 Firmware

You can see the current (master branch) state of the mobile UI setup here:

https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/

### Development

To work on the frontend UI (HTML, CSS, JavaScript - Vue.js)

`cd esp/data`

`python -m SimpleHTTPServer`

And visit localhost:8000 in browser

### The Mock Api (optional development tool)

Instead of going offline to talk to the kit via WIFI, there is a /mock-api folder where you can start your mock API.

`cd mock-api`

`npm install`

`npm run start`

And in */esp/data/main.js* change the apiUrl to 'localhost:3000' instead of '192.168.1.1'


### TESTING - (Work in progress)

You can run End to End test (for the Web UI) against the current master branch with this command:

Inside `/mock-api`

`npm test`

If you want the tests to be run automatically everytime you edit `mock-api/casperjs/test` use:

`npm run autotest`

Add edit tests under `mock-api/casperjs/test`

TODO:

* We need to run the test against the branch you are working on, not master.
* So we need to start the service with 'python SimpleHTTPServer' or something on Travis?
* Also start the mock-api?

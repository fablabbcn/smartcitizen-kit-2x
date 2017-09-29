# SmartCitizenKit
Developer version of Smart Citizen Kit 1.5 Firmware

You can see the current (master branch) state of the mobile UI setup here:

https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/

### Development

The app consists of 3 things

* The core firmware (C++)
* Frontend, a mobile web UI for setting up the kit. `localhost:8000`
* mock-api - for mocking the API of the kit `localhost:3000`


#### Frontend + api setup

All the HTML, CSS, JavaScript (Vue.js) are under *esp/data*


Inside the /mock-api folder do:

1. `cd mock-api`

2. `npm install`

3. `npm run web`  - Starts frontend on [localhost:8000](localhost:8000)

4. `npm run api` - Starts api on [localhost:3000](localhost:3000)

If your mock-api is not responding, see */esp/data/main.js*, **theUrl** should be (your API url:port)

#### TESTING - (Work in progress)

You can run End to End test (for the Web UI) against the current master branch with this command:

`npm test`

If you want the tests to be run automatically everytime you edit `mock-api/casperjs/test` use:

`npm run autotest`

Add edit tests under `mock-api/casperjs/test`

TODO:

* We need to run tests against the branch you are working on, not master like it is currently doing.
* So we need to start the service with 'npm run api' and 'npm run web' or something on Travis?
* Also start the mock-api?
* Move the frontend to /mock-api, and create a process which compiles it + concatinates and puts in esp/data?

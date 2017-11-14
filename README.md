# SmartCitizenKit
Developer version of Smart Citizen Kit 1.5 Firmware

Find the new version here:

https://github.com/fablabbcn/smartcitizen-kit-20

You can see this repos current (master branch) state of the mobile UI setup here:

https://fablabbcn.github.io/smartcitizen-kit-15/esp/data/

## Development

The app consists of 3 things

* The core firmware (C++)
* Frontend, a mobile web UI for setting up the kit. `localhost:8000`
* mock-api - for mocking the API of the kit `localhost:3000`

All branches and pull requests on Github are auto tested with Travis

### The core firmware (C++)

The technology used:

* Platformio


#### Uploading the firmware to the kit

1. In folder ./sam/ do:

  `pio run -t upload`

2. in folder ./esp/ do:

  `pio run -t upload`

3. To upload the Frontend website code to the filesystem do:

  `pio run -t uploadfs`


### Frontend + api setup

The technology used here is:
* HTML, CSS, JavaScript
* (Vue.js)

And the files are under *esp/data*


#### Starting frontend development

Inside the ./mock-api folder do:

1. `npm install`

1. `npm run web` - Starts frontend on [localhost:8000](http://localhost:8000)

1. `npm run api` - Starts api on [localhost:3000](http://localhost:3000)

If your mock-api is not responding, see */esp/data/main.js*, **theUrl** should be (your API url:port)

#### Testing frontend

You can run End to End test (for the Web UI) against the current master branch with this command:

`npm test`

If you want the tests to be run automatically everytime you edit `mock-api/casperjs/test` use:

`npm run autotest`

Edit tests under `mock-api/casperjs/test`

####  TODO / ideas:

* Should we move the frontend to /mock-api, and create a process which compiles it + concatinates and puts the 'dist' in esp/data?

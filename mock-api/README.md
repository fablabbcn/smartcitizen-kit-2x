
# Frontend + api setup
You can see the (master branch) mobile UI setup [here](https://fablabbcn.github.io/smartcitizen-kit-21/esp/build_data/index.html)

The technology used here is:
* HTML, CSS, JavaScript
* Vue.js

And the files are under *esp/build_data*

[Here](process.md) you can read about why we chose Vue.js and what the thought process was like.

## Starting frontend development

Inside the ./mock-api folder do:

1. `npm install`

1. `npm run web` - Starts frontend on [localhost:8000](http://localhost:8000)

1. `npm run api` - Starts api on [localhost:3000](http://localhost:3000)

1. `gulp watch` - Watches changes and creates 2 files automatically; `final.html` and `index.gz`

Now you can start editing **esp/build_data/build_index.html**

If your mock-api is not responding, see */esp/build_data/main.js*, **theUrl** should be (your API url:port)

## Testing frontend

You can run End to End test (for the Web UI) against the current master branch with this command:

`npm test`

If you want the tests to be run automatically everytime you edit `mock-api/casperjs/test` use:

`npm run autotest`

Edit tests under `mock-api/casperjs/test`

**UPDATE NEEDED!!**

###  TODO / ideas:

- [ ] Should we move the frontend to /mock-api, and create a process which compiles it + concatinates and puts the 'dist' in esp/data?
- [ ] Instead of using a node.js mock-api, can we use the embedded C++ API of the kit somehow?

Creating the Smart Citizen onboarding application
===

Here we will discuss the process of making the Smart Citizen kit onboarding application.

What makes the development of this app interesting, is that we are limited to these **critical factors**:
1. The app will be **offline!**
2. The chip can only hold **1MB** of files!
3. The Kit is using a minimal webserver, so serving multiple files and clients can really slow it down.


This means that we have to:
* Save as much space as possible (**compress everything**)
* Limit our http requests (join all the files to a **single** one).

And what we **cannot** do:
* use any CDN (content delivery networks)!
* use big shiny libraries!


What we first wanted was a nice mobile like experience for the users. Most of our users will be using a mobile phone so we tried a couple of **Materialize** libraries, but of course, that increased the loading time for the kit, so we decided to use plain CSS. 
In the end, the only external library we allowed in was VueJS. We concluded that the benefits Vue gave us outweighed the filesize but the `vue.min.js` is 77k unzipped.


### User walkthrough example
When a user will use the app for the first time, he will do the following:
1. Turn on the kit (with a power cord)
2. Search for Wifi
2. Select the kit Wifi named `SmartCitizen[xxxx]`
3. The kit will offer a portal connection, a popup should appear on the device
4. The user clicks the popup and joins the Wifi

We are now **offline**, which means we cannot:
* Load any external libraries from a CDN like Bootstrap or React etc.
* The filesize of the chip is 1MB.


### Other

Since this is a web application, you can even try it here:
https://fablabbcn.github.io/smartcitizen-kit-20/esp/data/index.html

### Issues

We had issues when we were serving multiple files from the kit, sometimes the loading simply stopped. We suspected that multiple HTTP requests on the devise were to blame, so we tried joining all the files (js, CSS, html, svg etc) into a single one. That turned out to be much more robust. We also tried compressing the final file which saved a lot of space, and after this change we have not experienced a single failure in the loading process!

After gzip compression the file size of `index.html`  went from **105kb** to **38kb**:

![pic](https://i.imgur.com/vGq71Vm.png)


### Final conclusion

The Technology we ended up using on the frontend:

* Vue.js (to be able to rerender and control the app navigation)
* HTML 
* CSS (no framework)

While developing we also used:
* npm 
* gulp

Which helped us with the following:
* Start a "mock-api" to imitate the communication with the Smart Citizen Kit.
* Watch for changes and automatically join all the CSS & .js files into the .html file and compress them with gzip.
* Automatically run frontend tests (CasperJS)


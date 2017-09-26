# SmartCitizenKit
Developer version of Smart Citizen Kit 1.5 Firmware


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

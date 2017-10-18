var app = new Vue({
  el: '#app',
  data: {
    theApi: window.location.href,
    development: false,
    advanced: false,
    browsertime: Math.floor(Date.now() / 1000),
    debuginfo: [],
    devicetime: 0,
    errors: [],
    intervals: false,
    kitinfo: false,
    notification: '',
    publishinterval: 2,
    readinginterval: 60,
    selectedWifi: '',
    sensors: false,
    sensor1: false,
    sensor2: false,
    sensor3: false,
    sensor4: false,
    setuppath: 'online',
    sdlog: false,
    usertoken: '',
    wifiname: '',
    wifipass: '',
    wifisync: true,
    wifis: {
      "nets": [{
        "ssid": "Fake-WIFI-1",
        "ch": 1,
        "rssi": -64
      }, {
        "ssid": "Fake-Wifi-2",
        "ch": 1,
        "rssi": -89
      }]
    }
  },
  mounted: function () {
    // When the app is mounted
    this.selectApiUrl();
    var that = this;
    console.log('Vue.js mounted, fetching data at startup');

    setTimeout(function() {
      return that.jsGet('aplist');
    }, 500);

    // This checks if connection to the kit has been lost, every 5 sec
    this.every5sec();
  },
  methods: {
    every5sec: function () {
      var that = this;
      //console.log('I should check /status');

      setTimeout(function(){
        return that.every5sec();
      }, 5000);
    },
    selectApiUrl: function () {
      // If we are running this from the kit,
      // the API should be on the same IP and port
      // Most likely a 192.168.*.1/status

      this.errors.push('Select API url')
      // If we are using port 8000, we are in development, and the API should be on port 3000
      if (window.location.port === '8000') {
        this.theApi = 'http://' + window.location.hostname + ':3000/';
        this.development = true;
      } else {
        this.theApi = window.location.href;
      }

      console.log('Using API : ' + this.theApi);
    },
    selectPath: function (path) {
      this.setuppath = path;
    },
    jsGet: function (path, purpose) {
      // Backup function to fetch with pure javascript
      // (If we cannot use extra libraries due to space issues etc)
      var that = this;

      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open("GET", this.theApi + path, false); // false for synchronous request
      xmlHttp.send(null);

      if (path = 'online') {
        that.wifis = JSON.parse(xmlHttp.responseText);
        that.notification = 'JS Fetching wifi list';
      }
      if (path = 'status'){
        that.notification = 'JS Fetching status';
        that.browsertime = Math.floor(Date.now() / 1000);
        //that.devicetime = response.data.time;
      }

    },
    // Not actually a POST request, but a GET with params
    jsPost: function(path, purpose){
      this.browsertime = Math.floor(Date.now() / 1000);
      var that = this;
      // Available parameters:
      // /set?ssid=value1&password=value2&token=value3&epoch=value

      var xmlHttp = new XMLHttpRequest();

      if (purpose == 'online'){
        xmlHttp.open("GET", that.theApi + path +
            '?ssid=' + that.selectedWifi +
            '&password=' + that.wifipass +
            '&token=' + that.usertoken +
            '&epoch=' + that.browsertime);
        xmlHttp.send(null);
        console.log(xmlHttp.responseText);

        console.log(response);
        //  that.errors.push(e);
      }

      if (purpose == 'synctime'){
        xmlHttp.open("GET", this.theApi + path + '?epoch=' + this.browsertime);
        xmlHttp.send(null);
        console.log('synctime GET response:');
        // TODO: check if we get a 200 from device?
        console.log(xmlHttp.responseText);
      }

    },
  },
  computed: {
    sortedWifi: function () {
      this.wifis.nets.sort(function (a, b) {
        return a.rssi - b.rssi;
      });
      return this.wifis.nets.reverse();
    }
  }
});


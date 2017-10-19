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
    var el = document.getElementById('loading')
      el.parentNode.removeChild(el);
    // When the app is mounted
    this.selectApiUrl();
    var that = this;
    console.log('Vue.js mounted, fetching data at startup');

    setTimeout(function() {
      return that.jsGet('aplist');
      this.notify('Using later', 3000);
    }, 1500);

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

      // If we are using port 8000, we are in development, and the API should be on port 3000
      if (window.location.port === '8000') {
        this.theApi = 'http://' + window.location.hostname + ':3000/';
        this.development = true;
      } else {
        this.theApi = window.location.href;
      }

      console.log('Using API : ' + this.theApi);
      this.notify('Using API', 3000);
    },
    httpGet: function(theUrl) {
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open( "GET", theUrl, false ); // false for synchronous request
      xmlHttp.send( null );
      return xmlHttp.responseText;
    },
    jsGet: function(path) {
      var that = this;
      var res = this.httpGet(this.theApi + path);

      if (path == 'aplist') {
        that.wifis = JSON.parse(res);
        that.notification = 'Getting wifi list..';
        this.notify('Get aplist', 1000);
      }
      if (path == 'status'){
        that.notification = 'Getting status..';
        that.debuginfo = JSON.parse(res);
      }

    },
    // Not actually a POST request, but a GET with params
    jsPost: function(path, purpose){
      this.browsertime = Math.floor(Date.now() / 1000);
      var res;
      var that = this;
      // Available parameters:
      // /set?ssid=value1&password=value2&token=value3&epoch=value

      if (purpose == 'connect'){
        that.notification = 'Connecting online..';
        res = that.httpGet(that.theApi + path +
            '?ssid=' + that.selectedWifi +
            '&password=' + that.wifipass +
            '&token=' + that.usertoken +
            '&epoch=' + that.browsertime);
      }

      if (purpose == 'synctime'){
        that.notification = 'Starting to log on SD CARD..';
        res = this.httpGet(this.theApi +  path + '?epoch=' + this.browsertime);
      }
      console.log(res);

    },

    // TODO: add className option
    notify: function(msg, duration, className){

      var newtoast = document.createElement("div");
      newtoast.className = "toast";
      newtoast.innerHTML = msg;
      document.getElementById("toast-wrapper").appendChild(newtoast);

      setTimeout(function(){
        newtoast.outerHTML = "";
        delete newtoast;
      }, duration);

      console.log('Notify', msg)
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


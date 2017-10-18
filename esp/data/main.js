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
      return that.axiosFetch('aplist');
    }, 500);

    // This checks if connection to the kit has been lost, every 5 sec
    this.every5sec();
  },
  methods: {
    every5sec: function () {
      var that = this;

      axios.get(this.theApi + 'status').then(function(response) {
      }).catch(function(e) {
        //that.errors.push(e);
      });

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
    },
    selectPath: function (path) {
      this.setuppath = path;
    },
    showAdvanced: function () {
      this.advanced = !this.advanced;
    },
    jsFetch: function (path) {
      // Backup function to fetch with pure javascript
      // (If we cannot use extra libraries due to space issues etc)
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open("GET", this.theApi + path, false); // false for synchronous request
      xmlHttp.send(null);
      myjson = JSON.parse(xmlHttp.responseText);
      this.wifis = myjson;
    },
    axiosFetch: function (path) {
      var that = this;
      that.notification = 'Fetching...';
      this.errors.push('fetching')
      axios.get(this.theApi + path)
        .then(function(response)  {
          //console.log(response.data);
          that.errors.push('fetch inside')

          // If this is a call for the Wifi list, populate this.wifis
          if (path == 'aplist'){
            that.errors.push('wifi list')
            that.wifis = response.data;
            that.notification = 'Fetching wifi list';
          }
          if (path == 'status'){
            that.browsertime = Math.floor(Date.now() / 1000);
            that.devicetime = response.data.time;
            that.debuginfo = response.data;
            that.notification = 'Fetching status';
          }

        })
      .catch(function(e) {
        // Printing errors to the app output so our users can tell us.
        //that.errors.push(e);
        that.notification = 'Error';
        that.errors.push('fetch error')
      });
    },
    axiosGet: function(path, purpose){
      this.browsertime = Math.floor(Date.now() / 1000);
      var that = this;
      // Available parameters:
      // /set?ssid=value1&password=value2&token=value3&epoch=value
      if (purpose == 'online'){
        axios.get(that.theApi + path +
            '?ssid=' + that.selectedWifi +
            '&password=' + that.wifipass +
            '&token=' + that.usertoken +
            '&epoch=' + that.browsertime
        ).then(function(response) {
          console.log(response);
        }).catch(function(e) {
          that.errors.push(e);
        });
      }
      if (purpose == 'synctime'){
        axios.get(this.theApi + path + '?epoch=' + this.browsertime)
          .then(function(response) {
            console.log('synctime GET response:');
            console.log(response);
            // TODO: check if we get a 200 from device?
          }).catch(function(e) {
            that.errors.push(e);
          });;
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


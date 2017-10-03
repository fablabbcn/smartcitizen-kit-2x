var app = new Vue({
  el: '#app',
  data: {
    theUrl: window.location.href,
    selectedwifi: '',
    advanced: true,
    notification: '(Status of the app)',
    apiresponse: '',
    browsertime: Math.floor(Date.now() / 1000),
    debuginfo: [],
    devicetime: 0,
    errors: [],
    setuppath: 'online',
    usertoken: '',
    wifiname: '',
    wifipass: '',
    wifis: {
      "nets": [
      {
        "ssid": "Fake-WIFI-1",
        "ch": 1,
        "rssi": -64
      },
      {
        "ssid": "Fake-Wifi-2",
        "ch": 1,
        "rssi": -89
      }]
    },
  },
  mounted: function() {
    // When the app is mounted
    this.selectApiUrl();
    console.log(' Vue.js mounted, fetching data at startup');
    setTimeout (() => this.axiosFetch('aplist'), 100);
    setTimeout (() => this.axiosFetch('status'), 300);
  },
  methods: {
    selectApiUrl: function(){
      // If we are running this from the kit,
      // the API should be on the same IP and port
      // Most likely a 192.168.*.1/status
      console.log('Checking which url to use for the API');
      this.notification = 'Checking which url to use for the API';

      if (window.location.href.includes('192') ) {
        this.theUrl = window.location.href;
      }else{
        // mock API
        this.theUrl = 'http://localhost:3000/';
      }
      console.log('Using: ' + this.theUrl);
    },
    selectPath: function(path){
      this.setuppath = path;
    },
    showAdvanced: function(){
      this.advanced = !this.advanced;
    },
    jsFetch: function (path) {
      // Backup function to fetch with pure javascript
      // (If we cannot use extra libraries due to space issues etc)
      this.notification = 'Fetching data...';
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open( "GET", this.theUrl + path, false ); // false for synchronous request
      xmlHttp.send( null );
      myjson = JSON.parse(xmlHttp.responseText);
      this.wifis = myjson;
    },
    axiosFetch: function(path) {
      this.notification = 'Fetching data... /' + path;
      axios.get(this.theUrl + path)
        .then(response => {
          this.notification = 'Data fetched.';
          //console.log(response.data);

          // If this is a call for the Wifi list, populate this.wifis
          if (path == 'aplist'){
            this.wifis = response.data;
            this.notification = 'Wifi list updated.';
          }
          if (path == 'status'){
            this.notification = 'Status fetched.';
            this.browsertime = Math.floor(Date.now() / 1000);
            this.devicetime = response.data.time;
            this.debuginfo = response.data;
          }

        })
      .catch(e => {
        // Printing errors to the app output so our users can tell us.
        this.errors.push(e);
      });
    },
    axiosGet: function(path, purpose){
      this.browsertime = Math.floor(Date.now() / 1000);
      // Available parameters:
      // /set?ssid=value1&password=value2&token=value3&epoch=value
      if (purpose == 'online'){
        this.notification = 'Trying to connect online...';
        axios.get(this.theUrl + path +
            '?ssid=' + this.selectedwifi +
            '&password=' + this.wifipass +
            '&token=' + this.usertoken +
            '&epoch=' + this.browsertime
        ).then(response => {
          this.apiresponse = response.data;
          console.log('wifi GET response:');
          console.log(response);
        }).catch(e => {
          this.errors.push(e);
        });
      }
      if (purpose == 'synctime'){
        this.notification = 'Syncing time (request)..';
        axios.get(this.theUrl + path + '?epoch=' + this.browsertime)
          .then(response => {
            console.log('synctime GET response:');
            console.log(response);
            // TODO: check if we get a 200 from device?
            this.notification = 'Device synced and has started working!'
          }).catch(e => {
            this.errors.push(e);
          });;
      }
    },
    axiosPost: function(path) {
      console.log('POST request');
      this.browsertime = Math.floor(Date.now() / 1000);
      this.notification = 'Sending POST data... Please wait!';
      axios.post(this.theUrl + path, {
        ssid: this.selectedwifi,
        password: this.wifipass,
        token: this.usertoken,
        epoch: this.browsertime
      })
      .then(response => {
        console.log(response);
        this.notification = response.data;
      })
      .catch(e => {
        this.errors.push(e);
      })
    }

  },
  computed:{
    sortedWifi: function(){
      this.wifis.nets.sort((a,b) =>{
        return a.rssi - b.rssi;
      });
      return this.wifis.nets.reverse();
    }
  }
});

var app = new Vue({
  el: '#app',
  data: {
    // mock API
    //theUrl: 'http://localhost:3000/',
    theUrl: 'http://192.168.1.1/',
    selectedwifi: '',
    advanced: 'false',
    appstatus: '(Status of the app)',
    browsertime: Math.floor(Date.now() / 1000),
    devicetime: '',
    currentToken: '',
    currentSSID: '',
    currentPassword: '',
    wifiname: '',
    wifipass: '',
    usertoken: '',
    setuppath: 'wifi',
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
    statusapi: [],
    errors: []
  },
  mounted: function() {
    // When the app is mounted
    console.log(' Vue.js mounted, fetching data at startup');
    setTimeout (() => this.axiosFetch('aplist'), 300);
    setTimeout (() => this.axiosFetch('conf'), 900);
  },
  methods: {
    selectPath: function(path){
      this.setuppath = path;
    },
    showAdvanced: function(){
      this.advanced = !this.advanced;
    },
    jsFetch: function (path) {
      // Backup function to fetch with pure javascript
      // (If we cannot use extra libraries due to space issues etc)
      this.appstatus = 'Fetching data...';
      var xmlHttp = new XMLHttpRequest();
      xmlHttp.open( "GET", this.theUrl + path, false ); // false for synchronous request
      xmlHttp.send( null );
      myjson = JSON.parse(xmlHttp.responseText);
      this.wifis = myjson;
    },
    axiosFetch: function(path) {
      this.appstatus = 'Fetching data... /' + path;
      axios.get(this.theUrl + path)
        .then(response => {
          this.appstatus = 'Data fetched.';
          console.log(response.data);

          // If this is a call for the Wifi list, populate this.wifis
          if (path == 'aplist'){
            this.wifis = response.data;
            this.appstatus = 'Wifi list updated.';
          }
          if (path == 'conf'){
            this.currentSSID = response.data.nets[0].ssid;
            this.currentToken = response.data.token;
            this.devicetime = response.data.time;
            this.appstatus = 'Config info updated.';
          }
          if (path == 'status'){
            this.appstatus = 'Status fetched. (Console)';
            this.statusapi = response.data.status;
          }

        })
      .catch(response => {
        // Printing errors to the app output so our users can tell us.
        this.errors.push(response);
      });
    },
    axiosGet: function(path){
      this.appstatus = 'Trying to connect online...';
      // /set?ssid=value1&password=value2&token=value3&epoch=value
      axios.get(this.theUrl + path +
          '?ssid=' + this.selectedwifi +
          '&password=' + this.wifipass +
          '&token=' + this.usertoken +
          '&epoch=' + this.browsertime
      ).then(response => {
        this.appstatus = response.data.statusText;
        console.log('GET response:');
        console.log(response);
      }).catch(e => {
        this.errors.push(e);
      });
    },
    axiosPost: function(path) {
      console.log('POST request');
      this.appstatus = 'Sending POST data... Please wait!';
      axios.post(this.theUrl + path, {
        ssid: this.selectedwifi,
        password: this.wifipass,
        token: this.usertoken,
        epoch: this.browsertime
      })
      .then(response => {
        console.log(response);
        this.appstatus = response.data;
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

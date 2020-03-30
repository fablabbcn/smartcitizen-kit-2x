// TODO poner esto dentro de un onDocumentReady()
var app = new Vue({
  el: '#app',
  data: {
    allowForceUpdate: false,
    browsertime: Math.floor(Date.now() / 1000),
    currentPage: 0,
    development: false,
    devicetime: 0,
    file: '',
    intervals: false,
    kitinfo: false,
    kitstatus: [],
    lastPage: 0,
    logging: [],
    page: [
       {'visible': true,  'footer': 'HOME',         'backTo': 0, 'back': false },
       {'visible': false, 'footer': 'REGISTER Key', 'backTo': 0, 'back': true  },
       {'visible': false, 'footer': 'REGISTER WiFi','backTo': 1, 'back': true  },
       {'visible': false, 'footer': 'Connecting',   'backTo': 0, 'back': false },
       {'visible': false, 'footer': 'Connecting',   'backTo': 0, 'back': true },
       {'visible': false, 'footer': 'SD card',      'backTo': 0, 'back': true },
       {'visible': false, 'footer': 'Kit info',     'backTo': 0, 'back': true },
       {'visible': false, 'footer': 'Empty',        'backTo': 0, 'back': true },
    ],
    publishinterval: 2,
    readinginterval: 60,
    sdlog: false,
    selectedWifi: '',
    sensor1: false,
    sensor2: false,
    sensor3: false,
    sensor4: false,
    sensors: false,
    showDebug: false,
    showExperimental: false,
    showInterval: false,
    showSdCard: false,
    theApi: window.location.protocol + '//' + window.location.host + '/',
    usertoken: '',
    usertokenError: '',
    version: 'SCK 2.0 / SAM V0.0.2 / ESP V0.0.2',
    vueVersion: Vue.version,
    weHaveTriedConnecting: false,
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
    this.logging.push('App started.');

    this.checkIfDebugMode;

    // 1. Remove loading screen
    var el = document.getElementById('loading');
    el.parentNode.removeChild(el);

    var el1 = document.getElementById('app');
    el1.style.display = 'block';

    // 2. Select which API to use, dev vs prod
    this.selectApiUrl();

    // 3. Fetch available Wifis + status
    this.jsGet('aplist');
    this.jsGet('status');
    // This can only be run once. We don't want the usertoken to be updated every 9
    // seconds if the user is typing it in
    this.jsGet('token');

    // This checks if connection to the kit has been lost, every X sec
    this.periodic(6000);
  },
  methods: {
    allowFirmwareUpdate: function(){
      this.allowForceUpdate = 'true';
    },
    copyTextToClipboard: function(containerid){
      // We need to copy the text temporary into a textBox to be able to copy it to clipboard.
      var textToCopy = document.getElementById('kitinfo').innerText;
      var textBox = document.createElement('textarea');
      textBox.value = textToCopy;
      document.body.appendChild(textBox);
      textBox.select();
      document.execCommand('copy');
      textBox.remove();
      document.getElementById('copied-notification').innerHTML = 'Text copied!'
    },
    periodic: function (ms) {
      var that = this;

      // TODO: should we check status every 5 sec?
      setTimeout(function(){
        that.jsGet('status');
        return that.periodic(ms);
      }, ms);
    },

    selectApiUrl: function () {
      // If we are running this from the kit,
      // the API should be on the same IP and port
      // Most likely a 192.168.*.1/status

      // If we are using port 8000, we are in development, and the API should be on port 3000
      if (window.location.port === '8000') {
        this.theApi = 'http://' + window.location.hostname + ':3000/';
        this.development = true;
      }
    },

    xmlWrapper: function(theUrl, callback) {
      //console.log('theurl: ' + theUrl);
      var xmlHttp = new XMLHttpRequest();
      var that = this;

      xmlHttp.onreadystatechange =  function() {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
          callback(xmlHttp.responseText);
        }
      }
      xmlHttp.onerror = function(e){
        // Don't show this error, if we have tried connecting. Only on real API failures
        if (!that.weHaveTriedConnecting) {
          that.notify('Your kit is not responding', 5000, 'bg-red');
        }
      }
      xmlHttp.open( "GET", theUrl, true ); // false for synchronous request, true = async
      xmlHttp.send( null );
    },

    jsGet: function(path) {
      var that = this;

      this.xmlWrapper(this.theApi + path, function(res){
        if (path === 'aplist') {
          that.wifis = JSON.parse(res);
        }
        if (path === 'status'){
          //that.notify('Getting status', 1000);
          that.kitstatus = JSON.parse(res);
        }
        if (path === 'token'){
          tmpToken = JSON.parse(res)['token'];
          if (tmpToken !== "null") {
            that.usertoken = tmpToken;
          }
        }

      });

    },
    // Not actually a POST request, but a GET with params
    jsPost: function(path, purpose){
      this.browsertime = Math.floor(Date.now() / 1000);
      var that = this;
      // Available parameters:
      // /set?ssid=value1&password=value2&token=value3&epoch=value

      if (purpose == 'connect'){
        this.weHaveTriedConnecting = true;
        this.notify('Kit is trying to connect online...', 5000);
        that.xmlWrapper(that.theApi + path +
            '?ssid=' + encodeURIComponent(that.selectedWifi) +
            '&password=' + encodeURIComponent(that.wifipass) +
            '&token=' + that.usertoken +
            '&epoch=' + that.browsertime +
            '&mode=network',
            function(res){
              console.log('Kit response: ' + res);
            });
        // After we try to connect, we go to the next page.
        this.gotoPage();
      }

      if (purpose == 'synctime'){
        this.notify('Starting to log on SD CARD..', 2000);
        this.xmlWrapper(this.theApi +  path + '?epoch=' + this.browsertime + '&mode=sdcard', function(res){
          // TODO: What is the correct response.key from the kit?
          that.notify(JSON.parse(res).todo, 5000);
        });
      }

    },

    checkIfTokenIsValid(){
      if (this.usertoken.length === 6) {
        this.gotoPage();
      }else{
        this.usertokenError = 'You need exactly 6 characters'
      }
    },
    gotoPage: function(num){

      // Keep lastPage in memory
      this.lastPage = this.currentPage;

      // Hide current page
      this.page[this.currentPage].visible = false;

      // If no page specified, we go to the next page
      if ( typeof num === 'undefined') {
        // Don't go too far, when clicking 'Next'
        if ( this.currentPage === (this.page.length - 1)) {
          //console.log('Last page: ' + this.currentPage)
          return;
        }
        this.currentPage = parseInt(this.currentPage + 1);
      }else{
        this.currentPage = parseInt(num);
      }

      // Show it
      this.page[this.currentPage].visible = true;

      this.logging.push('Go to page: ' + this.currentPage);
    },

    notify: function(msg, duration, className){
      if (duration === 'undefined') {
        console.log('no duration');
        duration = 1000;
      }

      //All events should also go to the logging section
      this.logging.push(msg);

      var newtoast = document.createElement("div");
      if (className) {
        newtoast.className = "toast " + className;
      }else{
        newtoast.className = "toast";
      }
      newtoast.innerHTML = msg;
      document.getElementById("toast-wrapper").appendChild(newtoast);

      setTimeout(function(){
        newtoast.outerHTML = "";
        delete newtoast;
      }, duration);

      console.log('Notify:', msg);
    },

    checkUploadForm: function(e){
      this.file = this.$refs.file.files[0];
    },

    // POST the file to /action via AJAX
    submitFirmware: function(e){
      var that = this;
      firmStatus = document.getElementById('firmware-update-status');
      firmStatusExtra = document.getElementById('firmware-status-extra');
      firmStatus.classList = '';
      firmStatus.classList += 'text-yellow';
      firmStatus.innerHTML = 'Updating...';
      firmStatusExtra.innerHTML = ' ';

      let formData = new FormData();
      let req = new XMLHttpRequest();
      formData.append('file', this.file);

      req.onreadystatechange = function() {
        if (req.readyState === 4) {
          console.log('request:', req);
          firmStatus.innerHTML = ' ' + req.response;
          firmStatusExtra.innerHTML = ' ';

          // Color
          if (req.response.startsWith("ERROR")) {
            that.notify('Update failed', 5000, 'bg-red');
	    firmStatusExtra.innerHTML = 'Something went wrong :(<br/>Please be sure to select the right file and try again !!';
            firmStatus.classList = '';
            firmStatus.classList += 'text-red';
          } else if (req.response.startsWith("Succeed")) {
            that.weHaveTriedConnecting = true;
            that.notify('Kit Updated...', 5000);
	    firmStatusExtra.innerHTML = 'Congratulations !!<br/>Your kit will restart so you can reconnect and complete the configuration process.<br/>If you need a Device key go to <span class="text-blue"><a href="https://start.smartcitizen.me">start.smartcitizen.me</a></span> to obtain a new one.';
            firmStatus.classList = '';
            firmStatus.classList += 'text-green';
          }
        }
      }
      req.onerror = function(e){
        console.log('error:', e);
      }
      req.onprogress = function(e){
        console.log('progress:', e)
      }
      req.onload = function(e){
        console.log('onload:', e)
      }
      req.open("POST", this.theApi + 'update');
      //req.open("GET", this.theApi + 'ping');
      req.send(formData);
      console.log('formData sent via AJAX to: ' + this.theApi + 'update');
    },
  },
  computed: {
    checkIfDebugMode: function(){
       if (window.location.hash === '#debug'){
         // Enables debug buttons
         this.showDebug = true;
       }
    },
    sortedWifi: function () {
      this.wifis.nets.sort(function (a, b) {
        return a.rssi - b.rssi;
      });
      return this.wifis.nets.reverse();
    },
    usertokenCheck: function(){
      return this.usertoken.length === 6;
    },
    selectedWifiCheck: function(){
      return this.selectedWifi.length > 0;
    },
    checkIfFileSelected: function(){
      return this.file.size > 0;
    }
  }
});


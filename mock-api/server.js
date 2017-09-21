var express = require('express'),
    cors = require('cors'),
    bodyParser = require('body-parser'),
    app = express(),
    port = process.env.PORT || 3000;
var fs = require("fs");

app.use(cors())

// Tell express to use the body-parser middleware and to not parse extended bodies
//app.use(bodyParser.urlencoded({ extended: false }))
app.use(bodyParser.json())

app.get('/', function(req, res){
  res.send('This API mocks the SCK API, for development purposes. This file is at mock-api/servers.js');
})

app.get('/cors', function(req, res, next){
  res.json({msg: 'This is CORS enabled for all origins!'})
})

app.get('/set*', function(req, res){
  console.log(req.query);
  res.status(200).send({statusText:'Mock API approves user|pass'});
})

// Try to get a filename of "*.json"
app.get('/*', function(req, res){
  fs.readFile(__dirname + "/" +  req.params[0] + ".json", "utf8", function(err, data){
  if (err){
    res.status(404).send({url: req.originalUrl + ' not found'});
  }
    setTimeout( (function() {
      res.end(data);
    }), 900)
  });
})

app.post('/set', function(req,res){
  // Wait 1 sec to emulate Wifi connection latency
  console.log(req.body);
  setTimeout( (function() {
      res.end('Connected to wifi!');
  }), 1500)
})


// Middleware to catch wrong urls
app.use(function(req, res) {
  res.status(404).send({url: req.originalUrl + ' not found!'})
});

app.listen(port);

console.log('Mock API started on port: ' + port);

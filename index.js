var express = require('express')
var fs = require('fs')
var formidable = require('formidable')
var SerialPort = require("serialport");
var SerialPort2 = require("serialport");
const Readline = require('@serialport/parser-readline')
const { parse } = require('json2csv');
var sqlite3 = require('sqlite3').verbose();
var app = express()
const { exec } = require("child_process");

var deviceport; 
var device;

var db = new sqlite3.Database('seeohtwo.db');

//Config
var data = fs.readFileSync('./seeohtwoconfig.json'),Config;
try {
  Config = JSON.parse(data);
}
catch (err) {
  console.log('Error parsing config')
  console.log(err);
}

//globals
var location_id = 0 //the location we tag the data with.  See DB for list of locations
var bLogDataOn = false //don't start logging until web front end says to start

SerialPort.list().then(ports => {
    ports.forEach(function(port) {
        if(port.manufacturer==Config.manufacturerstring) {
            deviceport=port.path; 
        }
      console.log(port.path);
      console.log(port.pnpId);
      console.log(port.manufacturer);
    });
    
    if (deviceport==undefined) {
	console.log("No FTDI/ESP32 device found");
	process.exit(1);
	}

	device = new SerialPort2(deviceport, {baudRate: 9600	});

    parser = device.pipe(new Readline({ delimiter: '\n' }));
		
    parser.on('data',function (data) {
        console.log('Data:', data)
        var sensorreading;
        sensorreading = parseJSONSafely(data)
        if ((bLogDataOn)&&(typeof(sensorreading.co2) != "undefined"))
            {
                try {
                    query = "insert into seeohtwo (co2, location_id) values ("+sensorreading.co2+","+location_id+");";
                    console.log(query);
                    db.run(query);
                }
                catch (exception) {
                    console.log(exception)
                }            
            }
      })
});

function parseJSONSafely(str) {
    try {
       return JSON.parse(str);
    }
    catch (e) {
       console.log(e);
       // Return a default object, or null based on use case.
       return {}
    }
 }

 var server = app.listen(Config.port, function () {

    var host = server.address().address
    var port = server.address().port
  
    console.log('seeohtwo App Listening At http://%s:%s', host, port)
  
  });


app.get('/', function(req, res){	

    strQuery= "select location_id, case when location_id = "+location_id+" then 1 else 0 end \"currentlocation\", description from location;"


    db.all(strQuery, function(err,rows) {
        res.render('index.ejs', {
        bLogDataOn: bLogDataOn,
        locations: rows
        });
    });
});

app.get('/togglebLogDataOn', function(req, res){
    if (bLogDataOn) {
        bLogDataOn=false;
    }
    else {
        bLogDataOn=true;
    }
	res.redirect('/');
});

app.get('/shutdown', function(req, res){
    exec("shutdown now", (error, stdout, stderr) => {
        if (error) {
            console.log(`error: ${error.message}`);
            return;
        }
        if (stderr) {
            console.log(`stderr: ${stderr}`);
            return;
        }
        console.log(`stdout: ${stdout}`);
    });
    res.send("Shutting Down - Turn off Pi in 1 min.  For shutdown to work, suid bit must be set on shutdown by running as root: chmod 4755 /sbin/shutdown")
});


app.use(express.static('public'))

app.get('/recentstats', function(req, res){

    strQuery = "select round(avg(A.co2)) \"last6avg\",min(A.timestamp) \"min6time\", round(avg(B.co2)) \"last60avg\",min(B.timestamp) \"min60time\" FROM (select co2,timestamp from seeohtwo order by seeohtwo_id desc limit 6) A, (select co2,timestamp from seeohtwo order by seeohtwo_id desc limit 60) B";
    db.all(strQuery, function(err,rows) {
            servertime = new Date().toISOString()
            recentstats = {
                last6avg: rows[0].last6avg, 
                min6time: rows[0].min6time, 
                last60avg: rows[0].last60avg, 
                min60time: rows[0].min60time,
                servertime: servertime }
            res.setHeader('Content-disposition', 'attachment; filename=readinglog.csv');
            res.set('Content-Type', 'application/json');
            res.status(200).send(recentstats);
        
    });
    
    

});

//select avg(co2) FROM (select co2 from seeohtwo order by seeohtwo_id desc limit 2) A

app.get('/setlocation', function(req, res){
    if (typeof req.query.locationid !== 'undefined') {
        templocationid= parseInt(req.query.locationid);
        if (Number.isInteger(templocationid)) {
            if (templocationid<0) {iStreamSensorMode=0;}
            else {location_id=templocationid;}
            }
        //tell sensor we've changed locations, recalibrate for new humidity/temperature
        device.write('h', function(err) {
            if (err) {
              return console.log('Error on write: ', err.message)
            }
            console.log('Sent humidity calibration request')
          })
    }
	res.redirect('/');
});

app.get('/getcsv', function(req, res){

    fields = ['seeohtwo_id', 'co2', 'description', 'Timestamp'];
    opts = {fields}

    strQuery = "select seeohtwo_id, co2, description, Timestamp from seeohtwo a, location b where  a.location_id = b.location_id";
    db.all(strQuery, function(err,rows) {
            csv = parse(rows, opts)
            res.setHeader('Content-disposition', 'attachment; filename=readinglog.csv');
            res.set('Content-Type', 'text/csv');
            res.status(200).send(csv);
        
    });
    
});	

app.post('/addlocation', function (req, res){
    var form = new formidable.IncomingForm();
    form.parse(req, function (err, fields, files) {
		addlocation = fields.addlocation;
        strQuery = "INSERT INTO location (\"description\") VALUES (\""+addlocation+"\");";

        db.run(strQuery, function(err) {
            if (err) {
				console.log(err.message)
			}
            res.redirect('/');
        });
    });
});

app.get('/purgedb', function (req, res){
    strQuery = "delete from seeohtwo; vacuum;";
    db.run(strQuery, function(err) {
        if (err) {
            console.log(err.message)
        }
        res.redirect('/');
    });
});
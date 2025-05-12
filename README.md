# SeeOhTwo
A datalogger intended for measuring equivalent CO2 to compare "air freshness" in different spaces.  

For more information about this project, see [https://www.hotelexistence.ca/inexpensivesensorsproxycovidrisk/](https://www.hotelexistence.ca/inexpensivesensorsproxycovidrisk/).

The data logger is implemented in [NodeJS](https://nodejs.org/en/) and I'm using the Adafruit SHTC3 Humidity & Temperature breakout board and the Adafruit SGP30 gas sensor breakout board on the Arduino platform.

# Arduino Source

The Arduino source is in SeeOhTwo/arduino-seeohtwo/arduino-seeohtwo.ino .  The board can be used without the data logger / Node server, and provides some guidance with each indicator LED.  

| Arduino GPIO Output  Pin| CO2eq range |
|----------------------|---------|
| 5                  | 400    |
| 0             | 401-599     |
| 4       | 600-799     |
| 13              | 800-999     |
| 16              | 1000-1199     |
| 15              | 1200+     |

The sensor takes a reading every 5 seconds, and sends a JSON payload with the following format over USB.  
{ "co2":400, "voc":0, "abshumidity":3.04, "relhumidity":16.09, "temperature":21.56}

With the ESP8266, it would be quite easy to send this to a web service hosted on the cloud, however, I chose local logging, as I wanted to capture readings in places without internet connectivity, like elevators and parking garages.

# Data Logger

I'm using a Raspberry Pi as my data logger, but anything that runs Node will do.  Download the code and run ‘npm install’ to install the dependencies and launch the server application as follows:
```
node SeeOhTwo.js
```
The code looks for a USB connected device based on the manufacturer's string defined in seeohtwoconfig.json - for the SparkFun board I'm using, this is "FTDI" - you may have to update this setting for your board.  The logger allows you to start/stop logging, associate a location with the data being logged, and to download a CSV file with the collected data.  

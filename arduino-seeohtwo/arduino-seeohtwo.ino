/* Minor tweaks by Richard Audette to spit out JSON for a data logging app */

/*
  Library for the Sensirion SGP30 Indoor Air Quality Sensor
  By: Ciara Jekel
  SparkFun Electronics
  Date: June 28th, 2018
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  SGP30 Datasheet: https://cdn.sparkfun.com/assets/c/0/a/2/e/Sensirion_Gas_Sensors_SGP30_Datasheet.pdf

  Feel like supporting our work? Buy a board from SparkFun!
  https://www.sparkfun.com/products/14813

  This example gets relative humidity from a sensor, converts it to absolute humidty,
  and updates the SGP30's humidity compensation with the absolute humidity value.
*/

#include "SparkFun_SGP30_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_SGP30
#include "SparkFun_SHTC3.h" // Click here to get the library: http://librarymanager/All#SparkFun_SHTC3
#include <Wire.h>

//LED Indicators
#define greenone 5
#define greentwo 0
#define yellowone 4
#define yellowtwo 13
#define redone 16
#define redtwo 15

SGP30 mySensor; //create an instance of the SGP30 class
SHTC3 humiditySensor; //create an instance of the SHTC3 class
long t1, t2;

byte count = 0;

//I pulled this out and made it global
//because I wanted it included in the first reading
uint16_t sensHumidity;
float humidity;
float temperature;
double absHumidity;

boolean bBootBlink = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);
  //Initialize the SGP30
  if (mySensor.begin() == false) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }

  pinMode(greenone, OUTPUT);
  pinMode(greentwo, OUTPUT);
  pinMode(yellowone, OUTPUT);
  pinMode(yellowtwo, OUTPUT);
  pinMode(redone, OUTPUT);
  pinMode(redtwo, OUTPUT);

  //Initialize the humidity sensor and ping it
  humiditySensor.begin();
  // Call "update()" to command a measurement, wait for measurement to complete, and update the RH and T members of the object 
  SHTC3_Status_TypeDef result = humiditySensor.update();
  delay(190);  
  // Measure Relative Humidity from the SHTC3
  humidity = humiditySensor.toPercent();

  //Measure temperature (in C) from the SHTC3
  temperature = humiditySensor.toDegC();

  //Convert relative humidity to absolute humidity
  absHumidity = RHtoAbsolute(humidity, temperature);

  //Convert the double type humidity to a fixed point 8.8bit number
  sensHumidity = doubleToFixedPoint(absHumidity);

  //Initializes sensor for air quality readings
  //measureAirQuality should be called in one second increments after a call to initAirQuality
  mySensor.initAirQuality();

  //First fifteen readings will be
  //CO2: 400 ppm  TVOC: 0 ppb
  //as the device calibrates - lets just flush
  //And... let's play with the lights for a fun boot sequence  
  int i;
  for (i = 1; i < 16; i++)
  {
    if (bBootBlink==true) {
      digitalWrite(greenone, LOW);
      digitalWrite(greentwo, HIGH);
      digitalWrite(yellowone, LOW);
      digitalWrite(yellowtwo, HIGH);
      digitalWrite(redone, LOW);
      digitalWrite(redtwo, HIGH);   
      bBootBlink = false;  
    }
    else {
      digitalWrite(greenone, HIGH);
      digitalWrite(greentwo, LOW);
      digitalWrite(yellowone, HIGH);
      digitalWrite(yellowtwo, LOW);
      digitalWrite(redone, HIGH);
      digitalWrite(redtwo, LOW);   
      bBootBlink = true;  
    }
     mySensor.measureAirQuality();
     delay(1000);
  }

  //Set the humidity compensation on the SGP30 to the measured value
  //If no humidity sensor attached, sensHumidity should be 0 and sensor will use default
  mySensor.setHumidity(sensHumidity);
  //Serial.print("Absolute humidity compensation set to: ");
  //Serial.print(absHumidity);
  //Serial.println("g/m^3 ");
  delay(100);
  t1 = millis();
}

void loop() {
  //First fifteen readings will be
  //CO2: 400 ppm  TVOC: 0 ppb
  if ( millis() >= t1 + 5000) //only will occur if 5 second has passed
  {
    t1 = millis();  //measure CO2 and TVOC levels
    mySensor.measureAirQuality();
    setindicator(mySensor.CO2);
    Serial.print("{ \"co2\":");
    Serial.print(mySensor.CO2);
    Serial.print(", \"voc\":");
    Serial.print(mySensor.TVOC);
    Serial.print(", \"abshumidity\":");
    Serial.print(absHumidity);
    Serial.print(", \"relhumidity\":");
    Serial.print(humidity);
    Serial.print(", \"temperature\":");
    Serial.print(temperature);
    Serial.println("}");
  }
  if (Serial.available()) //check if new data is available on serial port
  {
    char ch = Serial.read(); 
    if (ch == 'h' || ch == 'H') //check if the char input matches either "h" or "H" and if it does, run the compensation routine from the setup
    {
      SHTC3_Status_TypeDef result = humiditySensor.update();
      delay(190);  
      // Measure Relative Humidity from the SHTC3
      humidity = humiditySensor.toPercent();
    
      //Measure temperature (in C) from the SHTC3
      temperature = humiditySensor.toDegC();
    
      //Convert relative humidity to absolute humidity
      absHumidity = RHtoAbsolute(humidity, temperature);
    
      //Convert the double type humidity to a fixed point 8.8bit number
      sensHumidity = doubleToFixedPoint(absHumidity);
    
      //Set the humidity compensation on the SGP30 to the measured value
      //If no humidity sensor attached, sensHumidity should be 0 and sensor will use default
      mySensor.setHumidity(sensHumidity);
      //Serial.print("Absolute Humidity Compensation set to: ");
      //Serial.print(absHumidity);
      //Serial.println("g/m^3 ");
      delay(100);      
    }
  }
}

double RHtoAbsolute (float relHumidity, float tempC) {
  double eSat = 6.11 * pow(10.0, (7.5 * tempC / (237.7 + tempC)));
  double vaporPressure = (relHumidity * eSat) / 100; //millibars
  double absHumidity = 1000 * vaporPressure * 100 / ((tempC + 273) * 461.5); //Ideal gas law with unit conversions
  return absHumidity;
}

uint16_t doubleToFixedPoint( double number) {
  int power = 1 << 8;
  double number2 = number * power;
  uint16_t value = floor(number2 + 0.5);
  return value;
}

void setindicator(uint16_t co2levels) {
  digitalWrite(greenone, LOW);
  digitalWrite(greentwo, LOW);
  digitalWrite(yellowone, LOW);
  digitalWrite(yellowtwo, LOW);
  digitalWrite(redone, LOW);
  digitalWrite(redtwo, LOW);
  
  if (co2levels<=400) {digitalWrite(greenone, HIGH);}
  if ((co2levels>400)&&(co2levels<600)) {digitalWrite(greentwo, HIGH);}
  if ((co2levels>=600)&&(co2levels<800)) {digitalWrite(yellowone, HIGH);}
  if ((co2levels>=800)&&(co2levels<1000)) {digitalWrite(yellowtwo, HIGH);}
  if ((co2levels>=1000)&&(co2levels<1200)) {digitalWrite(redone, HIGH);}
  if (co2levels>1200) {digitalWrite(redtwo, HIGH);}

}

#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>  

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_MOTION 2
#define HUMIDITY_SENSOR_DIGITAL_PIN 4
#define MOTION_INPUT_SENSOR 3
#define INTERRUPT MOTION_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

MySensor gw;
DHT dht;
float lastTemp;
float lastHum;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msg(CHILD_ID_MOTION, V_TRIPPED);

void setup()  
{ 
  gw.begin();
  pinMode(MOTION_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 
  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("Multi", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_MOTION, S_MOTION);

  metric = gw.getConfig().isMetric;
}

void loop()      
{  
  delay(dht.getMinimumSamplingPeriod());

  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
  } else if (temperature != lastTemp) {
    lastTemp = temperature;
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    gw.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
  }
  
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      gw.send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
  }

  boolean tripped = digitalRead(MOTION_INPUT_SENSOR) == HIGH; 
  gw.send(msg.set(tripped?"1":"0"));  // Send tripped value to gw 

  gw.sleep(INTERRUPT,CHANGE, SLEEP_TIME);  
}



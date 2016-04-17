// Example sketch showing how to control physical relays. 
// This example will remember relay state even after power failure.

#include <MySensor.h>
#include <SPI.h>
#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

#define RC_PIN 5
#define NUMBER_OF_OUTLETS 5 // Total number of attached relays

int rfMessageON[NUMBER_OF_OUTLETS]=
{
  26442,
  26443,
  26445,
  26446,
  26447
};

int rfMessageOFF[NUMBER_OF_OUTLETS]= 
{
  26434,
  26435,
  26437,
  26438,
  26439
};

MySensor gw;

void setup()  
{   
  // Initialize library and add callback for incoming messages
  gw.begin(incomingMessage);
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Relay", "1.0");

  mySwitch.enableTransmit(RC_PIN);

  // Fetch relay status
  for (int sensor=1; sensor<=NUMBER_OF_OUTLETS; sensor++) {
    // Register all sensors to gw (they will be created as child devices)
    gw.present(sensor, S_LIGHT);
    // Set all of them OFF upon start
    mySwitch.send(rfMessageOFF[sensor], 24);
    MyMessage msg(sensor, V_LIGHT);
    gw.send(msg.set(0));
  }
}


void loop() 
{
  // Alway process incoming messages whenever possible
  gw.process();
}

void incomingMessage(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LIGHT) {
     // Change relay state
     if (message.getBool())
     {
       mySwitch.send(rfMessageON[message.sensor], 24);
     }
     else
     {
       mySwitch.send(rfMessageOFF[message.sensor], 24);
     }
     // Store state in eeprom
     gw.saveState(message.sensor, message.getBool());
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}


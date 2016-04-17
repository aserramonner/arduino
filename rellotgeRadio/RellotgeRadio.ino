/**
 * Radio FM Alarm Clock
 *
 * tecnoground.cat
 *
 */
 
#include <EEPROM.h>
#include <HT1632.h>
#include <Wire.h>
#include <RTC_DS3231.h>
#include <radio.h>
#include <si4705.h>

typedef RTC_DS3231 Clock;
typedef SI4705 Radio;

// Analog pin used for volume potentiometer
#define ANALOG_VOLUME A0
// Analog pin used for light sensor
#define ANALOG_LIGHT  A1
// AMP output
#define MUTE_PIN 5

#include "Display.h"
#include "StateMachine.h"
#include "Radio.h"
#include "Alarms.h"
#include "Widgets.h"
#include "ui.h"

// Realtime clock
// I2C pins are A4: SDA and A5: SCL
#define SDATA                A4
#define SCLOCK               A5
Clock rtc;

// LED display
#define DATA 2
#define WR   3
#define CS   4
Display display = Display(DATA, WR, CS);

// Initialize the global state machine
StateMachine stateMachine = StateMachine(ST_LAST, EV_LAST, &(globalTransitions_[0][0]), ST_CLOCK);

// Screens
#define SCREEN_CLOCK       0
#define SCREEN_MENU        1
#define SCREEN_DATE_SETUP  2
#define SCREEN_TIME_SETUP  3
#define SCREEN_ALARM_SETUP 4
#define SCREEN_RADIO_SETUP 5
#define NUM_SCREENS        6

Screen *screens[NUM_SCREENS];
FMRadio *fmRadio;

Buttons buttons;

const char str_starting[] PROGMEM      = { "Starting clock" };
const char str_alarm[] PROGMEM         = { "Alarm event received in main loop: " };
const char str_event[] PROGMEM         = { "Event received in main loop: " };
const char str_eventused[] PROGMEM     = { "Event used in screen" };
const char str_no_transition[] PROGMEM = { "No transition for event " };

void debug(const char *pgmstr)
{
//    char buffer[50];
//    strcpy_P(buffer, (char*)pgm_read_word(pgmstr)); // Necessary casts and dereferencing, just copy.
//    Serial.println(buffer);
}

void setup() {
  Serial.begin(9600);
  debug(str_starting);
  // Start the radio
  fmRadio = new FMRadio(&stateMachine, &radio);
  
  display.initDisplay();
  // Create the UI screens
  screens[SCREEN_CLOCK]       = new ClockScreen(&display, &stateMachine, &rtc, fmRadio);
  screens[SCREEN_MENU]        = new MenuScreen(&display, &stateMachine, &rtc);
  screens[SCREEN_DATE_SETUP]  = new DateSetup(&display, &rtc);
  screens[SCREEN_TIME_SETUP]  = new TimeSetup(&display, &rtc);
  screens[SCREEN_ALARM_SETUP] = new AlarmSetup(&display);
  screens[SCREEN_RADIO_SETUP] = new RadioSetup(&display);
  // Initialize the buttons connected to pins 7,8,9
  buttons.init(9,8,7);  
  // Read the NVM
  for(byte i=0; i<MAX_ALARMS; i++)   alarms[i].readAlarmFromNvm(i);
  for(byte i=0; i<MAX_STATIONS; i++) stations[i].readStationFromNvm(i);
  
  pinMode(MUTE_PIN, OUTPUT);
  digitalWrite(MUTE_PIN, 0);
}

uint8_t currentScreen = SCREEN_CLOCK;
uint8_t initialView   = ClockScreen::CLOCK;
uint8_t lastVolume    = 0;

int8_t readButtons()
{
   int8_t evt = -1;
   
   uint8_t b = buttons.getKeys();
   if (b == Buttons::BT_UP)   evt = UP;
   if (b == Buttons::BT_DOWN) evt = DOWN;
   if (b == Buttons::BT_OK)   evt = OK;
   
   if (b == (Buttons::BT_DOWN|Buttons::BT_OK)) evt = SLEEP_OFF;
   if (b == (Buttons::BT_UP|Buttons::BT_DOWN)) evt = TOGGLE_DISPLAY;
   
   return evt;
}

uint8_t readVolume()
{
   uint16_t analogVolume = analogRead(ANALOG_VOLUME);
   return 15*analogVolume/1024;
}

void loop() 
{    
  int8_t attributes = stateMachine.getAttributes();
  
  if (attributes&RADIO_ON)
  {
    int8_t vol = readVolume();
    if (vol!=lastVolume) 
    {
      radio.setVolume(vol);
      lastVolume = vol;
    }
  }
  else
  {
    // Evaluate the alarms
    int8_t event = evaluateAlarms(&rtc);
    
    if (event != -1)
    {
      // Trigger the alarm
      fmRadio->switchOn();
    }
  }
  
  // Poll the buttons
  int8_t event = readButtons();
  
  if (event != -1)
  {
    debug(str_event);
    Serial.println(event);
    // Send the event to the current screen first.
    // If the screen takes it, the output event will be -1
    event = screens[currentScreen]->sendEvent(event);
      
    if (event != -1)
    {
       // If the screen has not used it, send it to the global state machine
       event = stateMachine.sendEvent(event);
    }
    else
    {
      debug(str_eventused);
    }
  }
  else
  {
    // No event, so just tell the screen and the state machine that we are idle
//    screens[currentScreen]->sendEvent(IDLE);
    stateMachine.idle();
  }
    
  if (event == -1)
  {
    uint8_t prevScreen = currentScreen;
    initialView = 0;
    
    // No event to process or someone has used the event, so maybe we need to refresh
    // Process the global state
    switch(stateMachine.getState())
    {
    case ST_CLOCK:
      currentScreen = SCREEN_CLOCK;
      initialView   = ClockScreen::CLOCK;
      break;
      
    case ST_INFO:
      currentScreen = SCREEN_CLOCK;
      initialView   = ClockScreen::INFO;
      break;
      
    case ST_RADIO_ON:
      stateMachine.setAttribute(RADIO_ON); 
      
    case ST_RADIO:
      currentScreen = SCREEN_CLOCK;
      initialView   = ClockScreen::RADIO;
      break;
      
    case ST_MENU:
    case ST_SET_TIME:
    case ST_SET_DATE:
    case ST_SET_ALARMS:
    case ST_SET_RADIO:
      currentScreen = SCREEN_MENU;
      break;
  
    case ST_TIME_SETUP:
      currentScreen = SCREEN_TIME_SETUP;
      break;

    case ST_DATE_SETUP:
      currentScreen = SCREEN_DATE_SETUP;
      break;
    
    case ST_ALARM_SETUP:
      currentScreen = SCREEN_ALARM_SETUP;
      break;

    case ST_RADIO_SETUP:
      currentScreen = SCREEN_RADIO_SETUP;
      break;
      
    default:;
    }

    // Set the initial view for the screen to come
    screens[currentScreen]->setInitialView(initialView); // If initialView is 0, start from the beginning
    
    // Finally, hide one screen and show the next one
    if (prevScreen!=currentScreen) screens[prevScreen]->hide();
    screens[currentScreen]->show(stateMachine.getState());
  }
  else
  {
     debug(str_no_transition);
     Serial.println(event);
  }
  
  delay(300);
}

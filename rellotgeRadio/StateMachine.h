/*
 * Simple state machine to control the clock.
 */
 
#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

enum { NONE=0,
       RADIO_ON=1,
       ALARM_ON=2,
       SLEEP_ON=3,
     } Attributes;

enum { ST_NULL=0, 
       ST_CLOCK, 
       ST_MENU, 
       ST_SET_TIME, 
       ST_TIME_SETUP, 
       ST_SET_DATE, 
       ST_DATE_SETUP,
       ST_SET_ALARMS,
       ST_ALARM_SETUP,
       ST_RADIO,
       ST_SET_RADIO,
       ST_RADIO_SETUP,
       ST_EXIT,
       ST_RADIO_ON,
       ST_INFO,
       ST_LAST } State;

enum { UP=0, DOWN=1, OK=2, EV_LAST, SLEEP_OFF, TOGGLE_DISPLAY } Events;

class StateMachine
{
public:
    
    StateMachine(uint8_t numStates, uint8_t numEvents, const uint8_t *transitions, uint8_t firstState)
    {
      state_ = firstState;
      numStates_ = numStates;
      numEvents_ = numEvents;
      transitions_ = transitions;
      attributes_ = NONE;
    }
    
    int8_t idle()
    {
      return -1;
    }
    
    int8_t getState()
    {
        return state_;
    }
    
    void setAttribute(uint8_t attribute)
    {
       attributes_ |= attribute;
    }

    void clearAttribute(uint8_t attribute)
    {
       attributes_ &= 0xFF-attribute;
    }

    uint8_t getAttributes()
    {
       return attributes_;
    }
    
    const char *getStateLabel()
    {
       return labels_[state_];
    }
            
    int8_t sendEvent(int8_t event)
    {
       if (event<0) return event;
       int8_t nextState = (int8_t)pgm_read_byte(&(transitions_[state_*numEvents_+event]));
       if (nextState != ST_NULL)
       {
         state_ = nextState;
         return -1;
       }
       return event;
    }
    
private:
   int8_t  state_;
   uint8_t attributes_;
   uint8_t numStates_;
   uint8_t numEvents_;
   const uint8_t *transitions_;
   static const char *labels_[ST_LAST];
};

const PROGMEM uint8_t globalTransitions_[ST_LAST][EV_LAST] = {
   { ST_NULL, ST_NULL, ST_NULL },
   { ST_RADIO, ST_INFO, ST_NULL }, // ST_CLOCK
   { ST_INFO, ST_RADIO, ST_SET_TIME }, // ST_MENU
   { ST_SET_RADIO, ST_SET_DATE, ST_TIME_SETUP }, // ST_SET_TIME
   { ST_NULL, ST_NULL, ST_CLOCK }, // ST_TIME_SETUP
   { ST_SET_TIME, ST_SET_ALARMS, ST_DATE_SETUP }, // ST_SET_DATE
   { ST_NULL, ST_NULL, ST_CLOCK }, // ST_DATE_SETUP
   { ST_SET_DATE, ST_SET_RADIO, ST_ALARM_SETUP }, // ST_SET_ALARMS
   { ST_NULL, ST_NULL, ST_CLOCK }, // ST_ALARM_SETUP
   { ST_MENU, ST_CLOCK, ST_RADIO_ON }, // ST_RADIO
   { ST_SET_ALARMS, ST_EXIT, ST_RADIO_SETUP }, // ST_SET_RADIO
   { ST_NULL, ST_NULL, ST_RADIO }, // ST_RADIO_SETUP
   { ST_SET_RADIO, ST_SET_TIME, ST_CLOCK }, // ST_EXIT
   { ST_RADIO, ST_RADIO, ST_RADIO }, // ST_RADIO_ON
   { ST_CLOCK, ST_MENU, ST_NULL } // ST_INFO
};

const char * StateMachine::labels_[ST_LAST] = {
  "NULL",
  "Clock",
  "Menu",
  "Hora",
  "",
  "Data",
  "",
  "Alarmes",
  "AlrmSet",
  "Radio",
  "Radio",
  "RdioSet",
  "Surt",
  "Alarm"
};

#endif

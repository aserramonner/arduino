/*
 * User interface.
 */

#include "Display.h"
#include "StateMachine.h"

const char str_no_widgets[] PROGMEM       = { "No widgets added to the setup." };
const char str_too_many[] PROGMEM         = { "Too many widgets" };
const char str_send_event_setup[] PROGMEM = { "Send event setup: " };
const char str_current_view[] PROGMEM     = { "Current view is: " };

/**
 * Common interface for screens
 */
class Screen
{
public:
   Screen(Display *display) 
   {
      display_ = display;
   };
   
   /*
    * Called to request the screen to fill the bitmap 
    */
   virtual void show(int8_t state) = 0;
   virtual void hide()
   {
      display_->getBitmap()->clear();
   }

   
   /*
    * Request the screen to process an event.
    * Returns -1 if the screen has used the event and 
    * no one else needs to care about it.
    */
   virtual int8_t sendEvent(int8_t event) = 0;
   
   /*
    * Set the first view
    */
   virtual void setInitialView(uint8_t view) {};

   /*
    *
    */
   virtual void addWidget(Widget *w) {};

protected:
   Display *display_;
};

class Buttons
{
public:
   
 enum { BT_NONE=0, BT_UP=1, BT_DOWN=2, BT_OK=4 } ButtonCode;

   Buttons()
   {
   }
   
   void init(uint8_t pinUp, uint8_t pinDown, uint8_t pinOk)
   {
     pinUp_   = pinUp;
     pinDown_ = pinDown;
     pinOk_   = pinOk;
     
     pinMode(pinUp_, INPUT);
     pinMode(pinDown_, INPUT);
     pinMode(pinOk_, INPUT);
   }
   
   uint8_t getKeys()
   {
     uint8_t r = 0;
#if 0
     uint8_t lastR = 0;     
     do
     { 
       // Wait until release to decide which code was it
       if (r>lastR) lastR = r;
       r = 0;
       if (digitalRead(pinUp_))   r |= BT_UP;
       if (digitalRead(pinDown_)) r |= BT_DOWN;
       if (digitalRead(pinOk_))   r |= BT_OK;
     } while(r);
     
     return lastR;
#else
       if (digitalRead(pinUp_))   r |= BT_UP;
       if (digitalRead(pinDown_)) r |= BT_DOWN;
       if (digitalRead(pinOk_))   r |= BT_OK;
       
       return r;
#endif
   }
   
private:
   uint8_t pinUp_;
   uint8_t pinDown_;
   uint8_t pinOk_;
};

class MenuScreen : public Screen
{
public:
    MenuScreen(Display *display, StateMachine *sm, Clock *rtc) : Screen(display)
    {
      sm_ = sm;
      clock_ = rtc;
      timeWidget_.setBitmap(0,0, display_->getBitmap());
      text_ = new Text(24,8,font);
      text_->setBitmap(0,9,display_->getBitmap());
    }
        
    void show(int8_t state)
    {
        DateTime time;
        const char *label = sm_->getStateLabel();
        time = clock_->now();
        timeWidget_.setTime(time.hour(),time.minute(),time.second());
        timeWidget_.draw();       
        text_->setText(label);
        display_->refresh();
    }

    int8_t sendEvent(int8_t event)
    {
       return event;
    }

private:
   Clock *clock_;
   Text *text_;
   StateMachine *sm_;
};

class SetupScreen : public Screen
{
public:    

#define MAX_WIDGETS 4

    SetupScreen(Display *display) : Screen(display)
    {
      numWidgets_    = 0;
      currentWidget_ = 0;
    }

    /**
     * Add a widget for the setup
     * The order will be preserved
     */
    void addWidget(Widget *widget)
    {
      widgets_[numWidgets_] = widget;
      numWidgets_++;
      if (numWidgets_>MAX_WIDGETS) debug(str_too_many);
    }

    void show(int8_t state)
    {
      if (numWidgets_ == 0) debug(str_no_widgets);
        
      // All painting happens here
      // Draw all the widgets
      Serial.println(numWidgets_);
      for(uint8_t i=0; i<numWidgets_; i++) widgets_[i]->draw();
      display_->refresh();
            
      // Set blinking for the next refresh if required
      widgets_[currentWidget_]->setBlink(true);
    }
    
    /**
     * Tells the setup to start in the specified widget.
     */
    void setInitialView(uint8_t currentWidget)
    {
       numWidgets_ = 0; // Reset widgets
       currentWidget_ = currentWidget; // Start by setting DD
    }
        
    int8_t sendEvent(int8_t event)
    {
       debug(str_send_event_setup);
       Serial.println(event);
      
       // If we receive events, do not blink  
       for(uint8_t i=0; i<numWidgets_; i++) widgets_[i]->setBlink(false);
       
       int8_t newEvent = event;
       switch(event)
       {
          case DOWN:
          case UP:
             widgets_[currentWidget_]->sendEvent(event);
             newEvent = -1; // we have used the event
             break;
             
          case OK:
             currentWidget_++;
             // If we are not in the last widget, we have used the event
             if (currentWidget_<numWidgets_) newEvent = -1;
             break;
       }
       
       return newEvent;
    }

protected:
    Widget *widgets_[MAX_WIDGETS];
    uint8_t numWidgets_;
    uint8_t currentWidget_;
};

class ClockScreen : public Screen
{
public:
    enum { CLOCK, INFO, RADIO, ALARM, DATE, TEMPERATURE } View;

    ClockScreen(Display *display, StateMachine *sm, Clock *clock, FMRadio *radio) : Screen(display)
    {
      sm_         = sm;
      clock_      = clock;
      fmRadio_    = radio;
      nowShowing_ = CLOCK;
      currentAlarm_   = -1;
      
      // Place all the widgets
      timeWidget_.setBitmap(0,0, display_->getBitmap());               
      ad_.setBitmap(0, 9, display_->getBitmap());
      day_.setBitmap(6,9, display_->getBitmap());
      bottomText_.setBitmap(1, 9, display_->getBitmap());
    }
    
    void setInitialView(uint8_t view)
    {
       if (view == currentView_) return;
       currentView_ = view;
       
       debug(str_current_view);
       Serial.println(view);
       
       // If the view has changed, start from a fixed state
       if (view == INFO)  nowShowing_ = DATE;
       if (view == CLOCK) nowShowing_ = CLOCK;
       if (view == RADIO) nowShowing_ = RADIO;
    }
                
    int8_t sendEvent(int8_t event)
    {
       // We accept OK to act in display
       if (event == OK)
       {
          if (nowShowing_ == CLOCK) 
          {  
             nowShowing_   = ALARM;
             currentAlarm_ = 0;
             return -1;
          }
          else if (nowShowing_ == ALARM)
          {
            currentAlarm_++;
            if (currentAlarm_ == MAX_ALARMS)
            {
              currentAlarm_ = -1;
              nowShowing_ = CLOCK;
            }
            return -1;
          }
          else if (nowShowing_ == DATE)
          {
             nowShowing_ = TEMPERATURE;
             return -1;
          }
          else if (nowShowing_ == TEMPERATURE)
          {
             nowShowing_ = DATE;
             return -1;
          }
          else if (nowShowing_ == RADIO)
          {
            // Start the radio or change the station
            uint8_t attrs = sm_->getAttributes();
            if (attrs&RADIO_ON)
            {
              fmRadio_->nextStation();
            }
            else
            {
              fmRadio_->switchOn();
            }
            
            return -1;
          }
       }
       else if (event == SLEEP_OFF)
       {
          if  (sm_->getAttributes() & RADIO_ON)
          {
            // Stop the radio no matter what
            fmRadio_->switchOff();
          }
          else
          {
            // Sleep mode
            fmRadio_->switchOn();
            //timer.set(59);
          }
          
          return -1;
       }
       else if (event == TOGGLE_DISPLAY)
       {
          display_->toggleState();
          return -1;
       }
       
       return event;
    }
    
    void hide()
    {
       //bottomText_.setBlink(false);
    }
    
    void show(int8_t state)
    {
      DateTime time;
      uint8_t hour, minute, second;
      uint8_t dow;
      uint8_t dayOfWeek;
      char infoString[50];
      
      // The clock widget is always running
      // Except in the alarm view where it is overriden by the alarm time
      time = clock_->now();
      timeWidget_.setTime(time.hour(), time.minute(), time.second());
            
      // Get the day (1=monday) also used in many views      
      dayOfWeek = time.dayOfWeek();
      
      switch(nowShowing_)
      {
      case ALARM:
          alarms[currentAlarm_].getAt(hour, minute, dow);
          second = -1; // no tick
          timeWidget_.setTime(hour, minute, second);
          
      case CLOCK:
          ad_.setCurrent(currentAlarm_);
          ad_.setAlarms(alarms);
          break;
      
      case DATE:
          sprintf(infoString, "%d/%d/%d", time.day(), time.month(), time.year());
          bottomText_.setText(infoString);
          break;
          
      case TEMPERATURE:
          {
            int t = clock_->getTempAsWord();
            sprintf(infoString, "%d'C", t>>8);
            bottomText_.setText(infoString);
          }
          break;

      case RADIO:
#if 0      
          if ((currentStation_==-1) && (sm_->getAttributes()&RADIO_ON)) sprintf(infoString, "Off");
          else if (currentStation_==-1) sprintf(infoString, "Radio Off");
          else sprintf(infoString, "%d.%d", stations[currentStation_].getStation()/10, stations[currentStation_].getStation()%10);
#else
          sprintf(infoString, "%d.%d", fmRadio_->getFrequency()/10, fmRadio_->getFrequency()%10);
#endif
          bottomText_.setText(infoString);
          break;
      }

      // Everything that has to be painted must be done here      
      // On the top we show always the time widget
      timeWidget_.draw();

      // And the bottom depends on the state
      switch(nowShowing_)
      {
      case DATE:
      case TEMPERATURE:
      case RADIO:
         bottomText_.draw();
         break;
         
      case CLOCK:
      case ALARM:
        day_.set(dayOfWeek);
        day_.draw();
        ad_.draw();
        break;
      }
      
      display_->refresh();
   }

private:
   // The real time clock
   Clock *clock_;
   StateMachine *sm_;
   
   FMRadio *fmRadio_;
   
   // The active view
   byte nowShowing_;
   
   // Current alarm being displayed
   int8_t currentAlarm_;
   int8_t currentStation_;
   int8_t currentView_;
   
   uint8_t blinks_;
};

class DateSetup : public SetupScreen
{
public:
   DateSetup(Display *display, Clock *clock) : SetupScreen(display)
   {
      started_ = false;
      clock_   = clock;
   }
   
   void setInitialView(uint8_t mode)
   {
      if (started_) return;
      started_ = true;
  
      SetupScreen::setInitialView(0);
      
      // Read the current date
      DateTime date;
      date = clock_->now();       
      dayWidget_.set(date.day());
      monthWidget_.set(date.month());
      yearWidget_.set(date.year());
            
      // Place the widgets for the setup
      dayWidget_.setBitmap(0,1, display_->getBitmap());
      monthWidget_.setBitmap(13,1, display_->getBitmap());
      yearWidget_.setBitmap(0,9, display_->getBitmap());
      addWidget(&dayWidget_);
      addWidget(&monthWidget_);
      addWidget(&yearWidget_);
   }
   
   int8_t sendEvent(int8_t event)
   {
      int8_t evt = SetupScreen::sendEvent(event);
      
      if (evt!=-1)
      {
        // save the clock
        DateTime now = clock_->now();
        DateTime date(yearWidget_.get(), monthWidget_.get(), dayWidget_.get(), now.hour(), now.minute(), now.second());
        clock_->adjust(date);
        
        started_ = false;
      }
      
      return evt;
   }
   
private:
  bool started_;
  Clock *clock_;
};

class TimeSetup : public SetupScreen
{
public:
   TimeSetup(Display *display, Clock *clock) : SetupScreen(display)
   {
      started_ = false;
      clock_   = clock;
   }
   
   void setInitialView(uint8_t mode)
   {
      if (started_) return;
      started_ = true;
  
      SetupScreen::setInitialView(0);
      
      // Read the current time
      DateTime time;
      time = clock_->now();       
      hourWidget_.set(time.hour());
      minuteWidget_.set(time.minute());
            
      // Place the widgets for the setup
      hourWidget_.setBitmap(0,1, display_->getBitmap());
      minuteWidget_.setBitmap(13,1, display_->getBitmap());
      addWidget(&hourWidget_);
      addWidget(&minuteWidget_);
   }
   
   int8_t sendEvent(int8_t event)
   {
      int8_t evt = SetupScreen::sendEvent(event);
      
      if (evt!=-1)
      {
        DateTime now = clock_->now();
        // save the clock
        DateTime time(now.year(), now.month(), now.day(), hourWidget_.get(), minuteWidget_.get(), 0);
        clock_->adjust(time);
        
        started_ = false;
      }
      
      return evt;
   }
   
private:
  bool started_;
  Clock *clock_;
};

class AlarmSetup : public SetupScreen
{
public:
   AlarmSetup(Display *display) : SetupScreen(display)
   {
      started_ = false;
      currentAlarm_ = 0;
   }

   void loadAlarm(uint8_t alarm)
   {
      uint8_t hour, minute, days;
      
      // Read the first alarm
      alarms[currentAlarm_].getAt(hour, minute, days);
      hourWidget_.set(hour);
      minuteWidget_.set(minute);
      // Override the alarm days in the widget with the contents of the alarms
      ad_.setAlarms(alarms);
      ad_.setCurrent(currentAlarm_);
      day_.reset();
   }
   
   void setInitialView(uint8_t mode)
   {
      if (started_) return;
      started_ = true;
  
      SetupScreen::setInitialView(0);

      uint8_t hour, minute, days;
      
      currentAlarm_ = 0;
            
      // Read the first alarm
      loadAlarm(currentAlarm_);
      
      // Place the widgets for the setup
      hourWidget_.setBitmap(0,1, display_->getBitmap());
      minuteWidget_.setBitmap(13,1, display_->getBitmap());
      ad_.setBitmap(0, 9, display_->getBitmap());
      day_.setBitmap(6,9, display_->getBitmap());
      addWidget(&hourWidget_);
      addWidget(&minuteWidget_);
      addWidget(&day_);
   }
   
   void show(int8_t state)
   {
      SetupScreen::show(state);      
      // Add the non-editable widget
      ad_.draw();
   }
   
   int8_t sendEvent(int8_t event)
   {
      int8_t evt = SetupScreen::sendEvent(event);
      
      if (evt!=-1)
      {
        uint8_t dow = ad_.get();
        
        if (day_.get()!=0) dow ^= (1<<day_.get()-1); 
        
        // Update the current alarm
        Alarm alarm;
        alarm.setAt(hourWidget_.get(), minuteWidget_.get(), dow);
        alarms[currentAlarm_] = alarm;
                
        if (day_.get()!=0)
        {
          // Not finished yet but days have changed
          ad_.setAlarms(alarms);
          loadAlarm(currentAlarm_);
          SetupScreen::setInitialView(2); // start in the third widget
          addWidget(&hourWidget_);
          addWidget(&minuteWidget_);
          addWidget(&day_);
          evt = -1;
        }
        else
        {
          alarm.saveAlarmToNvm(currentAlarm_);
          currentAlarm_++;
  
          if (currentAlarm_==MAX_ALARMS)
          {
            started_ = false;
          }
          else
          {
            // Edit the next alarm
            loadAlarm(currentAlarm_);
            SetupScreen::setInitialView(0); // start from the beginning
            addWidget(&hourWidget_);
            addWidget(&minuteWidget_);
            addWidget(&day_);
            evt = -1;
          }
        }
      }
      
      return evt;
   }
   
private:
  bool started_;
  uint8_t currentAlarm_;
};

class RadioSetup : public SetupScreen
{
public:
   RadioSetup(Display *display) : SetupScreen(display)
   {
      started_ = false;
      currentStation_ = 0;
   }

   void loadStation(uint8_t alarm)
   {
      int16_t frequency;
      
      // Read the first station
      frequency = stations[currentStation_].getStation();
      tuneWidget_.set(frequency);
   }
   
   void setInitialView(uint8_t mode)
   {
      if (started_) return;
      started_ = true;
  
      SetupScreen::setInitialView(0);

      currentStation_ = 0;
            
      // Read the first station
      loadStation(currentStation_);
      
      // Place the widgets for the setup
      tuneWidget_.setBitmap(0,1, display_->getBitmap());
      addWidget(&tuneWidget_);
   }
   
   void show(int8_t state)
   {
      SetupScreen::show(state);      
   }
   
   int8_t sendEvent(int8_t event)
   {
      int8_t evt = SetupScreen::sendEvent(event);
      
      if (evt!=-1)
      {        
        // Update the current station
        Station station;
        station.setStation(tuneWidget_.get());
        stations[currentStation_] = station;
                
          station.saveStationToNvm(currentStation_);
          currentStation_++;
  
          if (currentStation_==MAX_STATIONS)
          {
            started_ = false;
          }
          else
          {
            // Edit the next alarm
            loadStation(currentStation_);
            SetupScreen::setInitialView(0);
            addWidget(&tuneWidget_);
            evt = -1;
          }
        }
      
      return evt;
   }
   
private:
  bool started_;
  uint8_t currentStation_;
};

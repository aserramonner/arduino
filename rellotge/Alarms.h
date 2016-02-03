/*
 * Class to manage alarms.
 *
 */

#define MAX_ALARMS 4
#define BASE_ALARMS_NVM 0

const char str_saved_alarm[] PROGMEM  = { "Clock saved alarm to EEPROM." };
const char str_alarm_raised[] PROGMEM = { "Alarm raised" };

class Alarm
{
public:
  Alarm()
  {
    daysOfWeek_  = 0;
  }
  
  void setAt(byte hour, byte minute, byte daysOfWeek)
  {
     daysOfWeek_ = daysOfWeek;
     time_ = DateTime(0,0,0,hour,minute,0);
  }
  
  bool isEnabled(byte day)
  {
    return ((daysOfWeek_>>day)&1);
  }
  
  void getAt(byte & hour, byte & minute, byte &daysOfWeek)
  {
    hour = time_.hour();
    minute = time_.minute();
    daysOfWeek = daysOfWeek_;
  }
  
  void readAlarmFromNvm(byte i)
  {
     // Read the alarms from EEPROM
     byte hh,mm,days;
     int8_t address = i*3;
     hh   = EEPROM.read(address++);
     mm   = EEPROM.read(address++);
     days = EEPROM.read(address++);
     setAt(hh,mm,days);
  }
  
  void saveAlarmToNvm(byte i)
    {
       // Write the alarms to EEPROM
       byte hh,mm,days;
       int8_t address = i*3;
       getAt(hh,mm,days);
       EEPROM.write(address++, hh);
       EEPROM.write(address++, mm);
       EEPROM.write(address++, days);

       debug(str_saved_alarm);
    }
         
   void getAlarm(byte id, byte & hh, byte & mm, byte &daysOfWeek)
   {
      getAt(hh, mm, daysOfWeek);
   }

   void setAlarm(byte id, byte & hh, byte & mm, byte &daysOfWeek)
   {
      setAt(hh, mm, daysOfWeek);
      saveAlarmToNvm(id);
   }

   bool check(const DateTime &t)
   {
      bool day = isEnabled(t.dayOfWeek());
      return ((t.hour() == time_.hour())&&(t.minute() == time_.minute())) && day;
   }
   
private:
  byte daysOfWeek_;
  DateTime time_;   
};

Alarm alarms[MAX_ALARMS];

int8_t evaluateAlarms(Clock *rtc)
{
   DateTime t = rtc->now();
   for(uint8_t a=0; a<MAX_ALARMS; a++)
   {
      if (alarms[a].check(t))
      {
         // raise alarm
         digitalWrite(12, HIGH);
         debug(str_alarm_raised);
         return a;
      }  
   }
      
   return -1;
}

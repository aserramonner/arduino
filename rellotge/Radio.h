#define MAX_STATIONS 4
#define BASE_STATIONS_NVM 12

const char str_station[] PROGMEM = { "Station " };
const char str_loaded[] PROGMEM  = { " loaded from EEPROM." };
const char str_saved[] PROGMEM   = { " saved to EEPROM." };

class Station
{
public:
   Station()
   {
      tune_ = 0;
   }
   
   void setStation(int16_t frequency)
   {
     tune_ = frequency;
   }
   
   int16_t getStation()
   {
     return tune_;
   }
   
  void readStationFromNvm(byte i)
  {
     uint8_t hi, lo;
     int8_t address = BASE_STATIONS_NVM + i*2;
     hi   = EEPROM.read(address++);
     lo   = EEPROM.read(address++);
     
     tune_ = hi*256+lo;
     
     debug(str_station);
     Serial.print(i);
     debug(str_loaded);
  }
  
  void saveStationToNvm(byte i)
    {
       uint8_t hi, lo;
       int8_t address = BASE_STATIONS_NVM + i*2;
       hi = tune_/256;
       lo = tune_%256;
       EEPROM.write(address++, hi);
       EEPROM.write(address++, lo);

       debug(str_station);
       Serial.print(i);
       debug(str_saved);
    }

private:
   int16_t tune_;
};

Station stations[MAX_STATIONS];


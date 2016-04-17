#define MAX_STATIONS 4
#define BASE_STATIONS_NVM 12

const char str_station[] PROGMEM = { "Station " };
const char str_loaded[] PROGMEM  = { " loaded from EEPROM." };
const char str_saved[] PROGMEM   = { " saved to EEPROM." };

// FM radio
// Pin where the radio reset is connected
#define RADIO_RESET 5
Radio radio;

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

class FMRadio
{
public:
   FMRadio(StateMachine *sm, Radio *radio)
   {
      sm_    = sm;
      radio_ = radio;
      
      // Radio reset
      pinMode(RADIO_RESET, OUTPUT);
      digitalWrite(RADIO_RESET, LOW);
      delay(200);
      digitalWrite(RADIO_RESET, HIGH);
      delay(200);
      radio_->init();
      delay(200);
      radio_->setMono(true);
      radio_->setMute(true);
      radio_->debugEnable();
   }
   
   void setStation(int station)
   {
      radio_->setBandFrequency(RADIO_BAND_FM, stations[station].getStation()*10);
   }
   
   void switchOn()
   {
      sm_->setAttribute(RADIO_ON); 
      radio_->setBandFrequency(RADIO_BAND_FM, stations[0].getStation()*10);
      radio_->setMute(false);
      radio_->setSoftMute(false);
      digitalWrite(MUTE_PIN, 1);
   }
   
   void switchOff()
   {
     // Stop the radio no matter what
     sm_->clearAttribute(RADIO_ON);
     radio_->setMute(true);
     digitalWrite(MUTE_PIN, 0);
   }
   
private:
   StateMachine *sm_;
   Radio *radio_;
};


/*
 * UI helper classes.
 */
 
#define ATTR_BLINK  1
#define ATTR_SCROLL 2
#define ATTR_INKED  4

#define MAX_ALARMS  4

const char str_not_inked[] PROGMEM = { "not inked" };

/**
 * This class encapsulates a UI element 
 * that can be flushed to a bitmap buffer
 */
class Widget
{
public:
   Widget(uint8_t width, uint8_t height)
   {
      width_ = width;
      height_ = height;
      attributes_ = ATTR_INKED;
   }

   uint8_t getWidth()  { return width_; }
   uint8_t getHeight() { return height_; }

   /**
    * Assign a bitmap area to the widget
    */
   void setBitmap(int16_t x, int16_t y, Bitmap *b)
   {
      // coordinates can be negative if we are scrolling
      x_ = x;
      y_ = y;
      bitmap_ = b;
      clear();
   }

   /**
    * Clear the area owned by the widget in the underlying bitmap
    */
   void clear()
   {
     for(uint8_t i=x_; i<x_+width_; i++)
       for(uint8_t j=y_; j<y_+height_; j++) 
           bitmap_->clearBit(i,j);
   }

   /**
    * Set a pixel in the underlying bitmap
    */
   void setPixel(int8_t x, int8_t y)
   {
      bitmap_->setBit(x_+x, y_+y);
   }

   virtual void set(int16_t value) {};
   virtual int16_t get() { return -1; };
  
   virtual int8_t sendEvent(int8_t event) { return event; };

   virtual void draw();
     
  void setBlink(bool blink)
  {
    if (blink) attributes_ |= ATTR_BLINK;
    else attributes_ &= (0xFF-ATTR_BLINK);
  }
  
protected:
   uint8_t width_;
   uint8_t height_;
   Bitmap *bitmap_;
   int16_t x_;
   int16_t y_;
   uint8_t attributes_;
};

class AlarmDays : public Widget
{
public:
  AlarmDays(uint8_t alarms) : Widget(alarms+1,7) // We give it an extra column for spacing
  {
     size_ = alarms;
     for(uint8_t i=0; i<size_; i++) 
     {
       alarmDays_[i] = 0;
     }
     current_ = -1; // show all alarms
  }

  void setCurrent(int8_t current)
  {
    current_ = current;
  }

  void setAlarms(Alarm *alarms)
  {
    for(uint8_t i=0; i<size_; i++) 
    {
       uint8_t h,m,dow;
       alarms[i].getAt(h,m,dow);
       alarmDays_[i] = dow;
    }
  }

  void set(uint8_t alarmDays)
  {
     if (current_!=-1) alarmDays_[current_] = alarmDays;
  }

  int16_t get()
  {
     if (current_!=-1) return alarmDays_[current_];
     else return -1;
  }

  void draw()
  {
    int8_t first = 0; 
    int8_t last  = size_;
               
    if (current_ != -1)
    {
       first = current_;
       last  = current_+1;
    }

     clear();
     for(uint8_t i=first; i<last; i++)
     {
       for(uint8_t day = 0; day < 7; day++)
       {
         bool enabled = (alarmDays_[i]>>day)&1;
         if (enabled) setPixel(i, day);
       }
     }
  }
      
private:
  uint8_t size_;
  int8_t current_;
  uint8_t alarmDays_[MAX_ALARMS];
};

class Char8x6 : public Widget
{
public:
    Char8x6(const uint8_t *font) : Widget(6,8)
    {
      font_ = font;
    }

    void draw()
    {
    }

    void draw(char c)
    {
       clear();
       for (uint8_t i =0; i<5; i++ ) 
       {
         uint8_t line = pgm_read_byte(font_+(c*5)+i);
         for (uint8_t j = 0; j<8; j++) 
         {
           if (line & 0x1) 
           {
    	     setPixel(i, j); 
           }
           line >>= 1;
         }
       }    
    }
    
private:
  const uint8_t *font_;
};

class Text : public Widget
{
public:
  Text(uint8_t width, uint8_t height, const uint8_t *font) : Widget(width, height)
  {
     text_ = 0;
     font_ = font;
     currentShift_ = 0;
     shiftDirection_ = 1;
  }

  void setText(const char *text)
  {
     Char8x6 c(font_);
     
     if (strlen(text)!=strLen_)
     {
        strLen_ = strlen(text);
        // If the length of the text has changed, reset the scroll to prevent wrong displays
        currentShift_   = 0;
        shiftDirection_ = 1;
     }
          
     clear();
     uint8_t len = strlen(text)*c.getWidth();
     if (len==0) return;  

     if (len>width_)
     {
       attributes_ |= ATTR_SCROLL;
       // shift the letters
       currentShift_ += shiftDirection_;
       if (currentShift_>(len-width_))
       {
          shiftDirection_ = -1; 
       }
       else if (currentShift_ == 0)
       {
         shiftDirection_ = 1;
       }
     }
     
     if (attributes_ & ATTR_BLINK) attributes_ ^= ATTR_INKED;
     
     if (attributes_ & ATTR_INKED)
     {
       // Print all the chars (this can be optimized)
       for(uint8_t i=0; i<strlen(text); i++)
       {
         // Assign the bitmap relative to our position
         c.setBitmap(x_ + i*c.getWidth()-currentShift_, y_, bitmap_);
         c.draw(text[i]);
       }
     }
     else
     {
       debug(str_not_inked);
     }
     
     text_ = text;
  }
  
  void draw()
  {
  }

private:
  const uint8_t *font_;
  uint8_t currentShift_;
  int8_t  shiftDirection_;
  const char *text_;
  uint8_t strLen_;
};

/**
 * This class is used to display numbers in hours, minutes, days, months and year
 */
class Digits : public Widget
{
public:
  Digits(uint8_t ndigits, uint16_t minValue, uint16_t maxValue) : Widget(6*ndigits, 8)
  {
     nDigits_ = ndigits;
     value_   = 0;
     min_     = minValue;
     max_     = maxValue;
  }

  void set(uint16_t value)
  {  
     if (value<min_) value_ = min_;
     else if (value>max_) value_ = max_;
     else value_ = value;
  }

  int8_t sendEvent(int8_t event)
  {
    if (event==UP)
    {  
      value_++;
      if (value_>max_) value_ = min_;
      return -1;
    }
    else
    {
       value_--;
       if (value_<min_) value_ = max_;
       return -1;
    }
    return event;
  }
  
  int16_t get()
  {
     return value_;
  }
  
  void draw()
  {
     Char8x6 digit(font);
     clear();

     // Switch the inked flag to blink if required
     attributes_ ^= ATTR_INKED;
     
     // If blinking is enabled and the flag is off, we do not draw
     if (((attributes_ & ATTR_BLINK)&&(attributes_ & ATTR_INKED))||((attributes_ & ATTR_BLINK)==0))
     {     
       uint16_t d = 1;
       for(uint8_t i=0; i<nDigits_-1; i++, d=d*10);
       uint16_t value = value_;
       // Draw all the digits
       for(uint8_t j=0; j<nDigits_; j++)
       {
         uint8_t v = value/d;
         digit.setBitmap(x_+j*digit.getWidth(), y_, bitmap_);     
         digit.draw(0x30+v);
         value = value%d;
         d = d/10;
       }
     }
   }
   
private:
  int16_t  value_;
  int16_t min_;
  int16_t max_;
  uint8_t nDigits_;
};

class ClockWidget : public Widget
{
public:
  ClockWidget() : Widget(24,9) // Time takes one extra row on top to show the tick
  {
     hour_   = new Digits(2,0,23);
     minute_ = new Digits(2,0,59);
  }
  
  void setBlink(bool left, bool right)
  {     
     hour_->setBlink(left);
     minute_->setBlink(right);
  }
  
  void draw()
  {
     clear();
     hour_->setBitmap(x_, y_+1, bitmap_);
     minute_->setBitmap(x_+13, y_+1, bitmap_);
     
     hour_->draw();
     minute_->draw();
           
     if (tick_!=-1)
     {
       // Move the tick dot every second
       setPixel(x_+11+(tick_%2), y_);
     }
  }
  
  void setTime(uint8_t hh, uint8_t mm, uint8_t ss)
  {  
     hour_->set(hh);
     minute_->set(mm);
     tick_  = ss;
  }
  
private:
  Digits *hour_;
  Digits *minute_;
  int8_t tick_;
};

static char * weekDayStr_[] = { "dl", "dt", "dc", "dj", "dv", "ds", "dg" };

class DayOfWeek : public Text
{
public:
  DayOfWeek() : Text(12,8,font)
  {
      // Create a vector of day string for the alarm set
      dayStr_[0] = "ok";
      for(int8_t day = 0; day<7; day++)
      {
         dayStr_[day+1] = weekDayStr_[day];
      }
  }
  
  void draw()
  {
    setText(dayStr_[dayOfWeek_]);
  }

  void reset()
  {
    dayOfWeek_ = 0;
  }
  
  void set(int16_t dow)
  {
    // sunday=0
    dow = dow==0?7:dow;
    dayOfWeek_ = dow;
  }  
  
  int8_t sendEvent(int8_t event)
  {
    if (event==UP)
    {  
      dayOfWeek_++;
      if (dayOfWeek_>7) dayOfWeek_ = 0;
      return -1;
    }
    else
    {
       dayOfWeek_--;
       if (dayOfWeek_<0) dayOfWeek_ = 7;
       return -1;
    }
    return event;
  }

  int16_t get()
  {
    return dayOfWeek_;
  }  
     
private:
  int16_t dayOfWeek_;
  char *dayStr_[8];
};

class RadioWidget : public Widget
{
public:
  RadioWidget() : Widget(24,8)
  {
     nowPlaying_ = -1;
     text_ = new Text(24,8,font);
  }
  
  void setStation(int8_t station)
  {
     nowPlaying_ = station;     
  }  
  
  void setMessage(const char *message)
  {
    text_->setBitmap(x_,y_,bitmap_);
    text_->setText(message);
  }
  
  void draw()
  {
  }
  
private:
  // RDS text
  Text *text_;
  int8_t nowPlaying_;
};
   
/**
 * Global widgets
 * Can be reused across screens
 */
 
ClockWidget    timeWidget_; // Widget displaying 4 digits
Digits         hourWidget_   = Digits(2,0,23);
Digits         minuteWidget_ = Digits(2,0,59);
Digits         dayWidget_    = Digits(2,1,31);
Digits         monthWidget_  = Digits(2,1,12);
Digits         yearWidget_   = Digits(4,2000,2099);
Digits         tuneWidget_   = Digits(4,760,1080);
AlarmDays      ad_ = AlarmDays(MAX_ALARMS); // Widget displaying the alarm days
DayOfWeek      day_;
RadioWidget    radioWidget_;
Text           bottomText_ = Text(24,8,font);

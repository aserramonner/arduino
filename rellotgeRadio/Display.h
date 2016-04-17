/*
 * Classes to manage the LED display.
 */

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "glcdfont.c"

#define MAX_COLS 24
#define MAX_ROWS 16

/**
 * This is a buffer for the 16x24 LED screen
 * used to render the widgets 
 * before updating the screen
 */
class Bitmap
{
public:
   Bitmap()
   {
      width_  = 24;
      height_ = 16;
      nBytes_ = (width_+7)/8*height_;
   }

   void clear()
   {
     for(uint8_t i = 0; i<nBytes_; i++) bitmap_[i] = 0;
   }
   
   uint8_t *getBitmap()
   {
     return bitmap_;
   }

   void setBit(uint8_t x, uint8_t y)
   {
      if ((x<0)||(y<0)||(x>=width_)||(y>=height_)) return;
      uint16_t i = x + y*width_;
      bitmap_[i/8] |= 1<<(i%8);
   }

   void clearBit(uint8_t x, uint8_t y)
   {
      if ((x<0)||(y<0)||(x>=width_)||(y>=height_)) return;
      uint16_t i = x + y*width_;
      bitmap_[i/8] &= 0xFF^(1<<(i%8));
   }
     
private:
   uint8_t width_;
   uint8_t height_;
   uint8_t nBytes_;
   uint8_t bitmap_[24/8*16];
};

class Display : public HT1632LEDMatrix
{
public:
    Display(uint8_t data, uint8_t wr, uint8_t cs) : HT1632LEDMatrix(data, wr, cs)
    {
       enabled_ = true;
       high_    = 0;
       low_     = 1024;
       brightness_ = 0;
    }
    
    void initDisplay()
    {
       enabled_ = true;
       begin(HT1632_COMMON_16NMOS);  
       setBrightness(brightness_);
       clearScreen();
    }
    
    void unlit()
    {
      enabled_ = false;
      end();
    }
    
    void toggleState()
    {
       if (enabled_) unlit();
       else initDisplay();
    }
    
    void idle()
    {
    }
    
    void update(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t w, uint8_t h,
    			uint8_t color) 
    {
      for (uint8_t i=0; i<w; i++) {
        for (uint8_t j=0; j<h; j++ ) {
          if (bitmap[(i + j*w)/8] & _BV((i + j*w)%8)) {
    	    drawPixel(x+i, y+j, color);
          }
        }
      }
    }

    void startDrawing()
    {
       clearScreen();
    }
    
    void refresh()
    {
#if 0 // automatic brightness suppressed due to the noise induced by high PWM      
       int light = analogRead(ANALOG_LIGHT);
       if (light>high_) high_ = light;
       if (light<low_)  low_ = light;
       setBrightness(15*(light-low_)/(high_-low_));
#endif       
       clearScreen();
       // Flush the entire bitmap
       update(0, 0, bitmap_.getBitmap(), 24, 16, 1);
       writeScreen();       
       // Leave the bitmap ready for the next update (optimize)
       bitmap_.clear();
    }
     
    void endDrawing()
    {
       writeScreen();
    }

    Bitmap *getBitmap()
    {
       return &bitmap_;
    }
       
private:
    int8_t high_;
    int8_t low_;
    byte brightness_;
    Bitmap bitmap_;
    bool enabled_;
};

#endif

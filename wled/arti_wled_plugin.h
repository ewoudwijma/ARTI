/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti_wled_plugin.h
   @version 0.0.6
   @date    20211114
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
 */

#pragma once

#if ARTI_PLATFORM == ARTI_ARDUINO
  #include "FX.h"
  extern float sampleAvg;
#else
  #include <math.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//make sure the numbers here correspond to the order in which these functions are defined in wled.json!!
enum WLEDExternals
{
  F_ledCount,
  F_setPixelColor,
  F_leds,
  F_setPixels,
  F_hsv,

  F_setRange,
  F_fill,
  F_colorBlend,
  F_colorWheel,

  F_segcolor,
  F_speed,
  F_sampleAvg,

  F_shift,
  F_circle2D,

  F_constrain,
  F_random,
  F_sin,
  F_cos,
  F_abs,

  F_hour,
  F_minute,
  F_second,
  F_millis,

  F_printf
};

#if ARTI_PLATFORM != ARTI_ARDUINO
  class WS2812FX {
  public:
    uint16_t matrixWidth = 16, matrixHeight = 16;

    uint16_t XY(uint16_t x, uint16_t y) {                              // ewowi20210703: new XY: segmentToReal: Maps XY in 2D segment to to rotated and mirrored logical index. Works for 1D strips and 2D panels
        return x%matrixWidth + y%matrixHeight * matrixWidth;
    }

    double arti_external_function(uint8_t function, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
    double arti_get_external_variable(uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
    void arti_set_external_variable(double value, uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
  }; //class WS2812FX

  WS2812FX strip = WS2812FX();

#endif

double arti_external_function(uint8_t function, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull)
{
  return strip.arti_external_function(function, par1, par2, par3);
}

double arti_get_external_variable(uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull)
{
  return strip.arti_get_external_variable(variable, par1, par2, par3);
}

void arti_set_external_variable(double value, uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull)
{
  strip.arti_set_external_variable(value, variable, par1, par2, par3);
}

double WS2812FX::arti_external_function(uint8_t function, double par1, double par2, double par3) {
  // MEMORY_ARTI("fun %d(%f, %f, %f)\n", function, par1, par2, par3);
  #if ARTI_PLATFORM == ARTI_ARDUINO
    switch (function) {
      case F_setPixelColor: {
        if (par2 == 0)
          setPixelColor(((uint16_t)par1)%ledCount, CRGB::Black);
        else
          setPixelColor(((uint16_t)par1)%ledCount, color_from_palette(((uint8_t)par2)%256, true, (paletteBlend == 1 || paletteBlend == 3), 0));
        return doubleNull;
      }
      case F_random:
        return random16();
      case F_shift: {
        uint32_t saveFirstPixel = getPixelColor(0);
        for (uint16_t i=0; i<ledCount-1; i++) {
          setPixelColor(i, getPixelColor((uint16_t)(i + par1)%ledCount));
        }
        setPixelColor(ledCount - 1, saveFirstPixel);
        return doubleNull;
      }
      case F_circle2D: {
        uint16_t circleLength = min(strip.matrixWidth, strip.matrixHeight);
        uint16_t deltaWidth=0, deltaHeight=0;

        if (circleLength < strip.matrixHeight) //portrait
          deltaHeight = (strip.matrixHeight - circleLength) / 2;
        if (circleLength < strip.matrixWidth) //portrait
          deltaWidth = (strip.matrixWidth - circleLength) / 2;

        double halfLength = (circleLength-1)/2.0;

        //calculate circle positions, round to 5 digits and then round again to cater for radians inprecision (e.g. 3.49->3.5->4)
        int x = round(round((sin(radians(par1)) * halfLength + halfLength) * 10)/10) + deltaWidth;
        int y = round(round((halfLength - cos(radians(par1)) * halfLength) * 10)/10) + deltaHeight;
        return strip.XY(x,y);
      }
      case F_millis:
        return millis();
      case F_setPixels:
        setPixels(leds);
        return doubleNull;
      case F_hsv:
        return crgb_to_col(CHSV(par1, par2, par3));
      case F_constrain:
        return constrain(par1, par2, par3);
      case F_fill: {
        fill((uint32_t)par1);
        return doubleNull;
      }
      case F_colorBlend:
        return color_blend((uint32_t)par1, (uint32_t)par2, (uint16_t)par3);
      case F_segcolor:
        return SEGCOLOR((uint8_t)par1);
      case F_colorWheel:
        return color_wheel((uint8_t)par1);
      case F_setRange: {
        setRange((uint16_t)par1, (uint16_t)par2, (uint32_t)par3);
        return doubleNull;
      }
      default: {}
    }
  #else
    switch (function)
    {
      case F_setPixelColor:
        PRINT_ARTI("%s(%f, %f)\n", "setPixelColor", par1, par2);
        return doubleNull;
      case F_random:
        return rand();
      case F_shift:
        PRINT_ARTI("%s(%f)\n", "shift", par1);
        return doubleNull;
      case F_circle2D:
        PRINT_ARTI("%s(%f)\n", "circle2D", par1);
        return par1 / 2;
      case F_millis:
        return 1000;
      case F_setPixels:
        PRINT_ARTI("%s(%f)\n", "setPixels", par1);
        return doubleNull;
      case F_hsv:
        PRINT_ARTI("%s(%f, %f, %f)\n", "hsv", par1, par2, par3);
        return par1 + par2 + par3;
      case F_constrain:
        return par1 + par2 + par3;
      case F_fill:
        PRINT_ARTI("%s(%f)\n", "fill", par1);
        return doubleNull;
      case F_colorBlend:
        return par1 + par2 + par3;
      case F_segcolor:
        return par1;
      case F_colorWheel:
        return par1;
      case F_setRange:
        return par1 + par2 + par3;
    }
  #endif

  //same on Arduino or Windows
  switch (function)
  {
    case F_printf:
      PRINT_ARTI("%s(%f, %f, %f)\n", "printf", par1, par2, par3);
      return doubleNull;
    case F_sin:
      return sin(par1);
    case F_cos:
      return cos(par1);
    case F_abs:
      return abs(par1);
  }

  return doubleNull;
}

double WS2812FX::arti_get_external_variable(uint8_t variable, double par1, double par2, double par3) {
  // MEMORY_ARTI("get %d(%f, %f, %f)\n", variable, par1, par2, par3);
  #if ARTI_PLATFORM == ARTI_ARDUINO
    switch (variable)
    {
      case F_ledCount:
        return SEGLEN;
      case F_leds:
        if (par1 == doubleNull) {
          ERROR_ARTI("arti_get_external_variable leds without indices not supported yet (get leds)\n");
          return doubleNull;
        }
        else if (par2 == doubleNull)
          return leds[(uint16_t)par1];
        else
          return leds[XY((uint16_t)par1, (uint16_t)par2)]; //2D value!!
      case F_sampleAvg:
        return sampleAvg;
      case F_speed:
        return SEGMENT.speed;
      case F_hour:
        return ((double)hour(localTime));
      case F_minute:
        return ((double)minute(localTime));
      case F_second:
        return ((double)second(localTime));
    }
  #else
    switch (variable)
    {
      case F_ledCount:
        return 3;
      case F_leds:
        if (par1 == doubleNull) {
          ERROR_ARTI("arti_get_external_variable leds without indices not supported yet (get leds)\n");
          return doubleNull;
        }
        else if (par2 == doubleNull)
          return par1;
        else
          return par1 * par2; //2D value!!
      case F_sampleAvg:
        return 9;
      case F_speed:
        return 11;
      case F_hour:
        return 24;
      case F_minute:
        return 60;
      case F_second:
        return 60;
    }
  #endif

  return doubleNull;
}

bool ledsSet; //tbd: make part of ARTI wled extension

void WS2812FX::arti_set_external_variable(double value, uint8_t variable, double par1, double par2, double par3) {
  #if ARTI_PLATFORM == ARTI_ARDUINO
    // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, variable_name, par1, par2, esp_get_free_heap_size());
    switch (variable)
    {
      case F_leds:
        if (par1 == doubleNull)
          ERROR_ARTI("arti_set_external_variable leds without indices not supported yet (set leds to %f)\n", value);
        else if (par2 == doubleNull)
          leds[(uint16_t)par1%ledCount] = value;
        else
          leds[XY((uint16_t)par1%matrixWidth, (uint16_t)par2%matrixHeight)] = value; //2D value!!
        break;
    }
  #else
    switch (variable)
    {
      case F_leds:
        if (par1 == doubleNull)
          ERROR_ARTI("arti_set_external_variable leds without indices not supported yet (set leds to %f)\n", value);
        else
          RUNLOG_ARTI("arti_set_external_variable, set leds(%f, %f) to %f\n", par1, par2, value);
        break;
    }
  #endif

  switch (variable)
  {
    case F_leds:
      ledsSet = true;
      break;
    case F_ledCount:
      WARNING_ARTI("Warning: arti_set_external_variable, cannot set ledCount to %f\n", value);
      break;
  }
}
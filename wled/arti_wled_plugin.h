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
  F_setPixelColor = 0,
  F_random = 1,
  F_printf = 2,
  V_ledCount = 3,
  F_shift = 4,
  F_sin = 5,
  F_millis = 6,
  F_abs = 7,
  F_setPixels = 8,
  V_leds = 9,
  F_hsv = 10,
  V_sampleAvg = 11,
  V_speed = 12,
  F_constrain = 13,
  F_fill = 14,
  F_blend = 15,
  F_segcolor = 16,
  F_wheel = 17,
  V_hour = 18,
  V_minute = 19,
  V_second = 20,
  F_setRange = 21
};

#if ARTI_PLATFORM != ARTI_ARDUINO
  class WS2812FX {
  public:
    double arti_external_functions(uint8_t function, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
    double arti_get_external_variables(uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
    void arti_set_external_variables(double value, uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
  }; //class WS2812FX

  WS2812FX strip = WS2812FX();
#endif

double arti_external_functions(uint8_t function, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull)
{
  return strip.arti_external_functions(function, par1, par2, par3);
}

double arti_get_external_variables(uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull)
{
  return strip.arti_get_external_variables(variable, par1, par2, par3);
}

void arti_set_external_variables(double value, uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull)
{
  strip.arti_set_external_variables(value, variable, par1, par2, par3);
}

void openFile() {
  
}

double WS2812FX::arti_external_functions(uint8_t function, double par1, double par2, double par3) {
  #if ARTI_PLATFORM == ARTI_ARDUINO
    // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, functionToName(function), par1, par2, esp_get_free_heap_size());
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
      case F_blend:
        return color_blend((uint32_t)par1, (uint32_t)par2, (uint16_t)par3);
      case F_segcolor:
        return SEGCOLOR((uint8_t)par1);
      case F_wheel:
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
      case F_blend:
        return par1 + par2 + par3;
      case F_segcolor:
        return par1;
      case F_wheel:
        return par1;
      case F_setRange:
        return par1 + par2 + par3;
    }
  #endif

  switch (function)
  {
    case F_printf:
      PRINT_ARTI("%s(%f, %f, %f)\n", "printf", par1, par2, par3);
      return doubleNull;
    case F_sin:
      return sin(par1);
    case F_abs:
      return abs(par1);
  }

  return doubleNull;
}

double WS2812FX::arti_get_external_variables(uint8_t variable, double par1, double par2, double par3) {
  // RUNLOG_ARTI("Get %u %f %f %f\n", variable, par1, par2);
  #if ARTI_PLATFORM == ARTI_ARDUINO
    switch (variable)
    {
      case V_ledCount:
        return SEGLEN;
      case V_leds:
        return 3; //just some value, not implemented yet
      case V_sampleAvg:
        return sampleAvg;
      case V_speed:
        return SEGMENT.speed;
      case V_hour:
        return ((double)hour(localTime));
      case V_minute:
        return ((double)minute(localTime));
      case V_second:
        return ((double)second(localTime));
    }
  #else
    switch (variable)
    {
      case V_ledCount:
        return 3;
      case V_leds:
        return 7;
      case V_sampleAvg:
        return 9;
      case V_speed:
        return 11;
      case V_hour:
        return 24;
      case V_minute:
        return 60;
      case V_second:
        return 60;
    }
  #endif

  return doubleNull;
}

void WS2812FX::arti_set_external_variables(double value, uint8_t variable, double par1, double par2, double par3) {
  #if ARTI_PLATFORM == ARTI_ARDUINO
    // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, variable_name, par1, par2, esp_get_free_heap_size());
    switch (variable)
    {
      case V_ledCount:
        ERROR_ARTI("Error: wled_set_variables, cannot set %s to %f", "ledCount", value);
      case V_leds:
        // ERROR_ARTI("Error: wled_set_variables, cannot set %s to %s (%f)", variable_name, value, par1);
        leds[(uint16_t)par1] = value;
    }
  #else
    switch (variable)
    {
      case V_ledCount:
        ERROR_ARTI("Error: wled_set_variables, cannot set %s to %f\n", "ledCount", value);
      case V_leds:
        RUNLOG_ARTI("wled_set_variables, set %s to %f\n", "leds", value);
    }
  #endif
}
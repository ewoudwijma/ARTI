#pragma once

#if ARTI_PLATFORM == ARTI_ARDUINO && ARTI_DEFINITION == ARTI_WLED
  #include "fx.h"
  extern float sampleAvg;
#else
  #include <math.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if ARTI_PLATFORM != ARTI_ARDUINO || ARTI_DEFINITION != ARTI_WLED
  class WS2812FX {
  public:
    double arti_wled_functions(const char * function_name, double par1 = doubleNullValue, double par2 = doubleNullValue, double par3 = doubleNullValue);
    double arti_wled_get_variables(const char * variable_name, double par1 = doubleNullValue, double par2 = doubleNullValue, double par3 = doubleNullValue);
    void arti_wled_set_variables(double value, const char * variable_name, double par1 = doubleNullValue, double par2 = doubleNullValue, double par3 = doubleNullValue);
  }; //class WS2812FX

  WS2812FX strip = WS2812FX();
#endif

double WS2812FX::arti_wled_functions(const char * function_name, double par1, double par2, double par3) {
  double returnValue = doubleNullValue;
  #if ARTI_PLATFORM == ARTI_ARDUINO
    // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, function_name, par1, par2, esp_get_free_heap_size());
    if (strcmp(function_name, "setPixelColor") == 0) {
      if (par2 == 0)
        setPixelColor(((uint16_t)par1)%ledCount, CRGB::Black);
      else
        setPixelColor(((uint16_t)par1)%ledCount, color_from_palette(((uint8_t)par2)%256, true, (paletteBlend == 1 || paletteBlend == 3), 0));
    }
    else if (strcmp(function_name, "random") == 0)
      return random16();
    else if (strcmp(function_name, "shift") == 0) {
      uint32_t saveFirstPixel =  getPixelColor(0);
      for (uint16_t i=0; i<ledCount-1; i++) {
        setPixelColor(i, getPixelColor((uint16_t)(i + par1)%ledCount));
      }
      setPixelColor(ledCount - 1, saveFirstPixel);
    }
    else if (strcmp(function_name, "millis") == 0)
      return millis();
    else if (strcmp(function_name, "setPixels") == 0)
      setPixels(leds);
    else if (strcmp(function_name, "hsv") == 0) {
      return crgb_to_col(CHSV(par1, par2, par3));
    }
    else if (strcmp(function_name, "constrain") == 0) {
      return constrain(par1, par2, par3);
    }
    else if (strcmp(function_name, "fill") == 0) {
      fill((uint32_t)par1);
    }
    else if (strcmp(function_name, "blend") == 0) {
      return color_blend((uint32_t)par1, (uint32_t)par2, (uint16_t)par3);
    }
    else if (strcmp(function_name, "segcolor") == 0) {
      return SEGCOLOR((uint8_t)par1);
    }
    else if (strcmp(function_name, "wheel") == 0) {
      return color_wheel((uint8_t)par1);
    }
    else if (strcmp(function_name, "setRange") == 0) {
      setRange((uint16_t)par1, (uint16_t)par2, (uint32_t)par3);
    }
  #else
    //functions
    if (strcmp(function_name, "setPixelColor") == 0)
      PRINT_ARTI("%s(%f, %f)\n", function_name, par1, par2);
    else if (strcmp(function_name, "random") == 0)
      return rand();
    else if (strcmp(function_name, "shift") == 0)
      PRINT_ARTI("%s(%f)\n", function_name, par1);
    else if (strcmp(function_name, "millis") == 0)
      return 1000;
    else if (strcmp(function_name, "setPixels") == 0)
      PRINT_ARTI("%s(%f)\n", function_name, par1);
    else if (strcmp(function_name, "hsv") == 0) {
      PRINT_ARTI("%s(%f, %f, %f)\n", function_name, par1, par2, par3);
      return par1 + par2 + par3;
    }
    else if (strcmp(function_name, "constrain") == 0) {
      return par1 + par2 + par3;
    }
    else if (strcmp(function_name, "fill") == 0) {
      PRINT_ARTI("%s(%f)\n", function_name, par1);
    }
    else if (strcmp(function_name, "blend") == 0) {
      return par1 + par2 + par3;
    }
    else if (strcmp(function_name, "segcolor") == 0) {
      return par1;
    }
    else if (strcmp(function_name, "wheel") == 0) {
      return par1;
    }
    else if (strcmp(function_name, "setRange") == 0) {
      return par1 + par2 + par3;
    }
  #endif

  if (strcmp(function_name, "array") == 0)
    return -1; // array return tbd
  else if (strcmp(function_name, "printf") == 0)
    PRINT_ARTI("%s(%f, %f, %f)\n", function_name, par1, par2, par3);
  else if (strcmp(function_name, "sin") == 0)
    return sin(par1);
  else if (strcmp(function_name, "abs") == 0)
    return abs(par1);

  return returnValue;
}

double WS2812FX::arti_wled_get_variables(const char * variable_name, double par1, double par2, double par3) {
  double returnValue = doubleNullValue;
  #if ARTI_PLATFORM == ARTI_ARDUINO
    // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, variable_name, par1, par2, esp_get_free_heap_size());
    if (strcmp(variable_name, "ledCount") == 0)
      return SEGLEN;
    else if (strcmp(variable_name, "leds") == 0)
      return 3; //just some value, not implemented yet
    else if (strcmp(variable_name, "sampleAvg") == 0)
      return sampleAvg;
    else if (strcmp(variable_name, "speed") == 0)
      return SEGMENT.speed;
    else if (strcmp(variable_name, "hour") == 0)
      return ((double)hour(localTime));
    else if (strcmp(variable_name, "minute") == 0)
      return ((double)minute(localTime));
    else if (strcmp(variable_name, "second") == 0)
      return ((double)second(localTime));
  #else
    if (strcmp(variable_name, "ledCount") == 0)
      return 3;
    else if (strcmp(variable_name, "leds") == 0)
      return 7;
    else if (strcmp(variable_name, "sampleAvg") == 0)
      return 9;
    else if (strcmp(variable_name, "speed") == 0)
      return 11;
    else if (strcmp(variable_name, "hour") == 0)
      return 24;
    else if (strcmp(variable_name, "minute") == 0)
      return 60;
    else if (strcmp(variable_name, "second") == 0)
      return 60;
  #endif

  return returnValue;
}

void WS2812FX::arti_wled_set_variables(double value, const char * variable_name, double par1, double par2, double par3) {
  #if ARTI_PLATFORM == ARTI_ARDUINO
    // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, variable_name, par1, par2, esp_get_free_heap_size());
    if (strcmp(variable_name, "ledCount") == 0)
      ERROR_ARTI("Error: wled_set_variables, cannot set %s to %f", variable_name, value);
    else if (strcmp(variable_name, "leds") == 0)
      // ERROR_ARTI("Error: wled_set_variables, cannot set %s to %s (%f)", variable_name, value, par1);
      leds[(uint16_t)par1] = value;
  #else
    if (strcmp(variable_name, "ledCount") == 0)
      ERROR_ARTI("Error: wled_set_variables, cannot set %s to %f", variable_name, value);
    else if (strcmp(variable_name, "leds") == 0)
      RUNLOG_ARTI("wled_set_variables, set %s to %f", variable_name, value);
  #endif
}

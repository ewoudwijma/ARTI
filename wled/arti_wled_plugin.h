#pragma once

#if ARTI_PLATFORM == ARTI_ARDUINO
  #include "wled.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void wled_functions(char * returnValue, const char * function_name, const char * par1 = nullptr, const char * par2 = nullptr) {
  strcpy(returnValue, "");
  #if ARTI_PLATFORM == ARTI_ARDUINO
    // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, function_name, par1, par2, esp_get_free_heap_size());
    //functions
    if (strcmp(function_name, "setPixelColor") == 0)
      strip.setPixelColor(atoi(par1)%ledCount, strip.color_wheel(atoi(par2)%256));
    if (strcmp(function_name, "random") == 0)
      itoa(random16(), returnValue, 10);

    //variables
    if (strcmp(function_name, "ledCount") == 0)
      itoa(ledCount, returnValue, 10);
  #else
    //functions
    if (strcmp(function_name, "setPixelColor") == 0)
      PRINT_ARTI("setPixelColor(%s, %s)\n", par1, par2);
    if (strcmp(function_name, "random") == 0)
      itoa(rand(), returnValue, 10);

    //variables
    if (strcmp(function_name, "ledCount") == 0)
      strcpy(returnValue, "3");
  #endif

  if (strcmp(function_name, "array") == 0)
    strcpy(returnValue, "array return tbd");
  if (strcmp(function_name, "printf") == 0)
    PRINT_ARTI("print %s %s\n", par1, par2);
}

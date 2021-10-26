#pragma once
#include "wled.h"

#ifdef WLED_H
void wled_functions(char * returnValue, const char * function_name, const char * par1 = nullptr, const char * par2 = nullptr) {
  strcpy(returnValue, "");
  if (strcmp(function_name, "setPixelColor") == 0)
    strip.setPixelColor(atoi(par1)%ledCount, strip.color_wheel(atoi(par2)%256));
  if (strcmp(function_name, "random") == 0)
    itoa(random16(), returnValue, 10);
  if (strcmp(function_name, "array") == 0)
    strcpy(returnValue, "array return tbd");

  //variables
  if (strcmp(function_name, "ledCount") == 0)
    itoa(ledCount, returnValue, 10);
}
#endif
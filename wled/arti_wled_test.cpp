/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti_test.cpp
   @version 0.1.0
   @date    2021120
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
 */

#include "arti_wled.h"

void execute(const char *definitionName, const char *programName) 
{
  ARTI *arti = new ARTI();

  printf("open %s and %s\n", definitionName, programName);

  if (arti->setup(definitionName, programName)) 
  {
    if (strstr(definitionName, "wled")) 
    {
      uint8_t nrOfTimes = 2;
  
      if (strstr(programName, "Kitt"))
        nrOfTimes = 4;

      for (uint8_t i=0; i<nrOfTimes; i++)
        arti->loop();
    }
  }
  else
    printf("setup fail\n");

  arti->close();
  printf("done\n");
}

int main() 
{
  execute("wled.json", "Examples/default.wled");
  execute("wled.json", "Examples/ColorRandom.wled");
  execute("wled.json", "Examples/Kitt.wled");
  execute("wled.json", "Examples/Shift.wled");
  execute("wled.json", "Examples/Subpixel.wled");
  execute("wled.json", "Examples/PhaseShift.wled");
  execute("wled.json", "Examples/BrightPulse.wled");
  execute("wled.json", "Examples/Clock.wled");
  execute("wled.json", "Examples/Clock2D.wled");
  execute("wled.json", "Examples/Mover.wled");
  execute("wled.json", "Examples/WaveSins.wled");
}

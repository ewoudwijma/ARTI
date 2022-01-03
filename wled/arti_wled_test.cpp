/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti_test.cpp
   @version 0.2.3
   @date    20220103
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
  execute("wled.json", "Examples/Subpixel.wled");
  execute("wled.json", "Examples/PhaseShift.wled");
  execute("wled.json", "Examples/Mover.wled");
  execute("wled.json", "Examples/WaveSins.wled");
  execute("wled.json", "Examples/Sinelon.wled");
  execute("wled.json", "Examples/drip.wled");
  execute("wled.json", "Examples/PerlinMove.wled");
  execute("wled.json", "Examples/block_reflections.wled");
  execute("wled.json", "Examples/ripple.wled");
  execute("wled.json", "Examples/Kitt.wled");
  execute("wled.json", "Examples/beatmania.wled");
}

// Performance (fps) leds 50  300
// ================= =======  ===
// 02 CE Default          41  41      
// 03 ColorRandom         41  19
// 04 Kitt                41  41
// 05 Shift               41  25 ?
// 06 PhaseShift          41  20
// 07 Subpixel            24  6
// 11 Mover               41  30
// 12 WaveSins            17  4
// 13 Sinelon             41  41
// 14 drip                41  41
// 15 ripple              41  41
// 16 beatmania           41  38
// 17 PerlinMove          35  25
// 18 twinkleup           16  4
// 19 block reflections   13  3
// }
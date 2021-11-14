/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti_test.cpp
   @version 0.0.6
   @date    20211111
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
 */

#include "arti.h"

// char charFromProgramFile() {
//   char byte;
//   programFile.get(byte);
//   if (programFile.eof())
//     return -1;
//   else {
//     return byte;
//   }
// }

int main() {
  ARTI *arti = new ARTI();

  char definitionFile[fileNameLength];
  char programFile[fileNameLength];
  
  // strcpy(definitionFile, "pas/pas.json"); strcpy(programFile, "pas/Examples/pas1.pas");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/default.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/ColorRandom.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/Kitt.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/Shift.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/Subpixel.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/PhaseShift.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/BrightPulse.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/Clock.wled");
  // strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/Clock2D.wled");
  strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/Mover.wled");

  printf("open %s and %s\n", definitionFile, programFile);

  if (arti->setup(definitionFile, programFile)) {
    if (strstr(definitionFile, "wled")) {
      uint8_t nrOfTimes = 2;
      if (strstr(programFile, "Kitt"))
        nrOfTimes = 4;
      // else if (strstr(programFile, "Subpixel"))
      //   nrOfTimes = 100;

      for (uint8_t i=0; i<nrOfTimes; i++)
        arti->loop();
    }
  }
  else
    printf("parse fail\n");

  arti->close();
  printf("done\n");
}

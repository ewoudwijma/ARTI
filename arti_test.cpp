/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti_test.cpp
   @version 0.0.1
   @date    20211014
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

  char definitionFile[charLength];
  char programFile[charLength];
  
  // strcpy(definitionFile, "pas.json"); strcpy(programFile, "Examples/pas1.pas");
  // strcpy(definitionFile, "wled.json"); strcpy(programFile, "Examples/default.wled");
  // strcpy(definitionFile, "wled.json"); strcpy(programFile, "Examples/ColorFade.wled");
  // strcpy(definitionFile, "wled.json"); strcpy(programFile, "Examples/ColorRandom.wled");
  strcpy(definitionFile, "wled/wled.json"); strcpy(programFile, "wled/Examples/Kitt.wled");

  printf("open %s and %s\n", definitionFile, programFile);

  if (arti->openFileAndParse(definitionFile, programFile)) {
    if (arti->analyze()) {
      if (arti->interpret()) {
        if (strstr(definitionFile, "wled")) {
          uint8_t nrOfTimes = 2;
          if (strstr(programFile, "Kitt"))
            nrOfTimes = 4;

          for (uint8_t i=0; i<nrOfTimes; i++)
            arti->interpret("renderFrame");
        }

        printf("done\n");
      }
      else 
        printf("interpret fail\n");
    }
    else
      printf("analyze fail\n");
  }
  else
    printf("parse fail\n");

  arti->close();
}

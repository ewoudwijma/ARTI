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
  
  // strcpy(definitionFile, "pas.json"); strcpy(programFile, "pas1.pas");
  // strcpy(definitionFile, "wled.json"); strcpy(programFile, "ColorWalk.wled");
  // strcpy(definitionFile, "wled.json"); strcpy(programFile, "ColorFade.wled");
  strcpy(definitionFile, "wled.json"); strcpy(programFile, "Examples/ColorRandom.wled");
  // strcpy(definitionFile, "wled.json"); strcpy(programFile, "Kitt.wled");

  arti->openFileAndParse(definitionFile, programFile);
  arti->analyze();
  arti->interpret();

  if (strcmp(definitionFile, "wled.json") == 0) {
    arti->interpret("renderFrame");
    arti->interpret("renderFrame");
  }

  printf("done\n");
  arti->close();


}

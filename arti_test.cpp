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

  // arti->openFileAndParse("pas.json", "pas1.pas");
  // arti->interpret();

  arti->openFileAndParse("wled.json", "wled1.wled");
  arti->interpret("before");

  printf("done\n");

  arti->close();
}

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
  ARTI arti = ARTI();

  arti.openFileAndParse("pas.json", "pas1.pas");
  // arti.openFileAndParse("wled.json", "wled1.wled");

  arti.interpret();
  
  printf("done");

  arti.close();
}

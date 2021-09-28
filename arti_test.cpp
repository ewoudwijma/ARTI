#include "arti.h"

#include <iostream>
#include <fstream>
#include <sstream>

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
  fstream programFile;
  programFile.open("test.pas", ios::in);

  fstream definitionFile;
  definitionFile.open("pas.json", ios::in);

  fstream parseTreeFile;
  parseTreeFile.open("parsetree.json", ios::out);

  if (!programFile || !definitionFile)
  {
    DEBUG_ARTI("Files not found:\n");
  }
  else {

    //read program
    char programText[1000];
    programFile.read(programText, sizeof programText);
    // DEBUG_ARTI("%s %d", programText, strlen(programText));

    ARTI arti = ARTI(programText);

    //read definition
    DeserializationError err = deserializeJson(definitionJson, definitionFile);
    if (err) {
      DEBUG_ARTI("deserializeJson() in Lexer failed with code %s\n", err.c_str());
    }

    arti.parse();

    //write parseTree
    serializeJsonPretty(parseTreeJson, parseTreeFile);

    // char resultString[standardStringLenght];
    // arti.walk(parseTreeJson.as<JsonVariant>(), resultString);
    // DEBUG_ARTI("walk result %s", resultString);

    arti.analyze();

    arti.interpret();
    // printf("Done!!!\n");
  }

  programFile.close();
  definitionFile.close();
  parseTreeFile.close();
}

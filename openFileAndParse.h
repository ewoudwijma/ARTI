#include "arti.h"


    void openFileAndParse() {
      string parseOrLoad = "Parse";
      File programFile = WLED_FS.open("/test.pas", "r");
      File definitionFile = WLED_FS.open("/pas.json", "r");
      File parseTreeFile = WLED_FS.open(parseOrLoad=="Parse"?"/parsetreeOut.json":"/parsetree.json", parseOrLoad=="Parse"?"w":"r");

      if (!programFile || !definitionFile)
      {
        DEBUG_ARTI("Files not found\n");
      }
      else {

        //read program
        char programText[1000];
        uint16_t index = 0;
        while(programFile.available()){
          programText[index++] = (char)programFile.read();
        }
        programText[index - 1] = '\0';

        ARTI arti = ARTI(programText);

        //read definition
        DeserializationError err = deserializeJson(definitionJson, definitionFile);
        if (err) {
          DEBUG_ARTI("deserializeJson() in Lexer failed with code %s\n", err.c_str());
        }

        if (parseOrLoad == "Parse") {
          arti.parse();

          //write parseTree
          serializeJsonPretty(parseTreeJson,  parseTreeFile);
        }
        else
        {
          //read parseTree
          DeserializationError err = deserializeJson(parseTreeJson, parseTreeFile);
          if (err) {
            DEBUG_ARTI("deserializeJson() in loadParseTree failed with code %s\n", err.c_str());
          }

        }

        // char resultString[standardStringLenght];
        // // strcpy(resultString, "");
        // arti.walk(parseTreeJson.as<JsonVariant>(), resultString);
        // DEBUG_ARTI("walk result %s", resultString);

        // arti.analyze();
        // arti.interpret();

      }

      programFile.close();
      definitionFile.close();
      parseTreeFile.close();
    } // openFileAndParse

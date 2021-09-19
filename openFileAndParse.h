#include "arti.h"


    void openFileAndParse() {
      File programFile = WLED_FS.open("/test.pas", "r");
      File compilerFile = WLED_FS.open("/pas.json", "r");
      File parseTreeFile = WLED_FS.open("/parsetree.json", "w");

      if (!programFile || !compilerFile)
      {
        cout << "Files not found: " << endl;
      }
      else {
        string programContents = "";
        while(programFile.available()){
          programContents += (char)programFile.read();
        }

        string compilerContents = "";
        while(compilerFile.available()){
          compilerContents += (char)compilerFile.read();
        }

        ARTI arti = ARTI(compilerContents.c_str(), programContents.c_str());
        string buffer = arti.parse();
        arti.analyze();
        arti.interpret();

      }

      programFile.close();
      compilerFile.close();
      parseTreeFile.close();
    } // openFileAndParse

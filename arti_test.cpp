#include "arti.h"

int main() {
  fstream programFile;
  programFile.open("test.pas", ios::in);

  fstream compilerFile;
  compilerFile.open("pas.json", ios::in);

  fstream parseTreeFile;
  parseTreeFile.open("parsetree.json", ios::out);

  if (!programFile || !compilerFile)
  {
    cout << "Files not found: " << endl;
  }
  else {
    stringstream programStream;
    programStream << programFile.rdbuf(); //read the file

    stringstream compilerStream;
    compilerStream << compilerFile.rdbuf(); //read the file

    ARTI arti = ARTI();
    parseTreeFile << arti.run(compilerStream.str(), programStream.str());
  }

  programFile.close();
  compilerFile.close();
  parseTreeFile.close();
}

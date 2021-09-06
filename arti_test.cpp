#include "arti.h"

DynamicJsonDocument parseTreeJson(2048000);

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
        stringstream strStream;
        strStream << programFile.rdbuf(); //read the file
        string programContents = strStream.str(); //str holds the content of the file

        deserializeJson(bnfJson, compilerFile);

        Lexer lexer = Lexer(programContents);
        JsonObject bnfObject = bnfJson.as<JsonObject>();
        JsonObject::iterator it = bnfObject.begin();
        lexer.fillTokenTypeJson(bnfObject);
        try {
          Parser parser = Parser(lexer);
          parser.parseSymbol(parseTreeJson.as<JsonVariant>(), it->key().c_str()); //first symbol
          serializeJsonPretty(parseTreeJson, parseTreeFile);
        }
        catch (const std::exception& e)
        {
          // except (LexerError, ParserError) as e:;
          cout << "Parser error " << e.what() << endl;;
          // exit();
        }

        SemanticAnalyzer semanticAnalyzer = SemanticAnalyzer();
        try {
          semanticAnalyzer.visit(parseTreeJson.as<JsonVariant>());
        }
        catch (const std::exception& e)
        {
          // except (LexerError, ParserError) as e:;
          cout << "SemanticAnalyzer error " << e.what() << endl;
          // exit();
        }

        Interpreter interpreter = Interpreter(parseTreeJson.as<JsonVariant>());
        interpreter.interpret(semanticAnalyzer.global_scope);// interpreter.print();

      }

      programFile.close();
      compilerFile.close();
      parseTreeFile.close();

    }

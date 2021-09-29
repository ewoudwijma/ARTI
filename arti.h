// #define ARTI_DEBUG 1
#define ARTI_LOG 1
// #define ARDUINOJSON_DEFAULT_NESTING_LIMIT 100 //set this in ArduinoJson!!!
#define charLength 30
#define arrayLength 30

using namespace std;

#ifdef WLED_H
  #include "src/dependencies/json/ArduinoJson-v6.h"
#else
  #include "ArduinoJson-recent.h"
#endif

#ifdef ARTI_DEBUG
  #ifdef ESP32
    #define DEBUG_ARTI(x...) Serial.print("["); Serial.printf(x);Serial.print("]")
  #else
    #define DEBUG_ARTI printf
  #endif
#else
  #ifdef ARTI_LOG
    #ifdef ESP32
      File logFile;
      #define DEBUG_ARTI(...) logFile.print("["); logFile.printf(__VA_ARGS__);logFile.print("]")
    #else
      FILE * logFile;
      #define DEBUG_ARTI(...) fprintf(logFile, __VA_ARGS__)
    #endif
  #else
    #define DEBUG_ARTI(...)
  #endif
#endif

#ifndef ESP32
  #include <iostream>
  #include <fstream>
  #include <sstream>
#endif

const char * intToCharString(int value) {
  char buffer[charLength];
  return itoa(value, buffer, 10);
}

enum class ErrorCode {
  UNEXPECTED_TOKEN,
  ID_NOT_FOUND,
  DUPLICATE_ID,
  NONE
};

DynamicJsonDocument tokensJson(2048);
DynamicJsonDocument definitionJson(8192);
DynamicJsonDocument parseTreeJson(16384);

const char * spaces = "                                                  ";
bool errorOccurred = false;

class Token {
  private:
    uint16_t lineno;
    uint16_t column;
  public:
    const char * type;
    char value[charLength]; 
    Token() {
      this->type = "NONE";
      strcpy(this->value, "");
      this->lineno = 0;
      this->column = 0;
    }
    Token(const char * type, const char * value, uint16_t lineno=0, uint16_t column=0) {
      this->type = type;
      strcpy(this->value, value);
      this->lineno = lineno;
      this->column = column;
    }
}; //Token

class Error {
  public:
    ErrorCode error_code;
    Token token;
    const char * message;
  public:
    Error(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE", "", 0, 0), const char * message = "") {
      this->error_code = error_code;
      this->token = token;
      this->message = message;
      DEBUG_ARTI("Error %s %s %s\n", this->token.type, this->token.value, this->message); //this->error_code, 
      errorOccurred = true;
    }
};

class LexerError: public Error {
  public:
    LexerError(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE", "", 0, 0), const char * message = "") {
      DEBUG_ARTI("Lexer error %s %s %s\n", this->token.type, this->token.value, this->message); //this->error_code, 
      errorOccurred = true;
    }
};
class ParserError: public Error {
  public:
    ParserError(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE", "", 0, 0), const char * message = "") {
      DEBUG_ARTI("Parser error %s %s %s\n", this->token.type, this->token.value, this->message); //this->error_code, 
      errorOccurred = true;
    }
};
class SemanticError: public Error {};

class Lexer {
  private:
    const char * text;
  public:
    uint16_t pos;
    char current_char;
    uint16_t lineno;
    uint16_t column;
    Lexer() {
      this->text = "";
      this->pos = 0;
      this->current_char = this->text[this->pos];
      this->lineno = 1;
      this->column = 1;
    }
    Lexer(const char * programText) {
      // DEBUG_ARTI("%s\n", "Lexer init");
      this->text = programText;
      this->pos = 0;
      this->current_char = this->text[this->pos];
      this->lineno = 1;
      this->column = 1;
    }

    void fillTokensJson() {
      //system tokens
      tokensJson["ID"] = "ID";
      tokensJson["INTEGER_CONST"] = "INTEGER_CONST";
      tokensJson["REAL_CONST"] = "REAL_CONST";
      // tokensJson["EOF"]  = "EOF";

      if (definitionJson["TOKENS"].is<JsonObject>()) {
        for (JsonPair objectPair : definitionJson["TOKENS"].as<JsonObject>()) {
          // DEBUG_ARTI("%s %s\n", objectPair.key().c_str(), objectPair.value().as<const char *>());
          tokensJson[objectPair.key().c_str()] = objectPair.value();;
        }
      }
    } //fillTokensJson

    void error() {
      DEBUG_ARTI("Lexer error on %c line %d col %d\n", this->current_char, this->lineno, this->column);
      const char * message = "";
      LexerError x = LexerError(ErrorCode::NONE, Token(), message);
      DEBUG_ARTI("%s\n", "LexerError");
      errorOccurred = true;
    }

    void advance() {
      if (this->current_char == '\n') {
        this->lineno += 1;
        this->column = 0;
      }
      this->pos++;

      if (this->pos > strlen(this->text) - 1)
        this->current_char = -1;
      else {
        this->current_char = this->text[this->pos];
        this->column++;
      }
    }

    // char peek(uint8_t ahead) {
    //   uint16_t peek_pos = this->pos + ahead;

    //   if (peek_pos > this->text.length() - 1)
    //     return -1;
    //   else 
    //     this->text[peek_pos];
    // }

    void skip_whitespace() {
      while (this->current_char != -1 && isspace(this->current_char))
        this->advance();
    }

    void skip_comment() {
      while (this->current_char != '}')
        this->advance();
      this->advance();
    }

    Token number() {
      Token token = Token("NONE", "", this->lineno, this->column);

      char result[charLength] = "";
      while (this->current_char != -1 && isdigit(this->current_char)) {
        result[strlen(result)] = this->current_char;
        this->advance();
      }
      if (this->current_char == '.') {
        result[strlen(result)] = this->current_char;
        this->advance();

        while (this->current_char != -1 && isdigit(this->current_char)) {
          result[strlen(result)] = this->current_char;
          this->advance();
        }

        result[strlen(result)] = '\0';
        token.type = "REAL_CONST";
        strcpy(token.value, result);
      }
      else {
        result[strlen(result)] = '\0';
        token.type = "INTEGER_CONST";
        // token.value = result;
        strcpy(token.value, result);
      }

      // DEBUG_ARTI("%s\n", "Number!!! ", token.type, token.value);
      return token;
    }

    Token id() {
        Token token = Token("NONE", "", this->lineno, this->column);

        char result[charLength] = "";
        while (this->current_char != -1 && isalnum(this->current_char)) {
            result[strlen(result)] = this->current_char;
            this->advance();
        }
        result[strlen(result)] = '\0';

        char resultUpper[charLength];
        strcpy(resultUpper, result);
        strupr(resultUpper);

        // DEBUG_ARTI("upper %s [%s] [%s]\n", tokensJson[resultUpper].as<const char *>(), result, resultUpper);
        if (tokensJson[resultUpper].isNull()) {
            // DEBUG_ARTI("%s\n", "  id empty ");
            token.type = "ID";
            strcpy(token.value, result);
        }
        else {
            token.type = tokensJson[resultUpper];
            strcpy(token.value, resultUpper);
        }

        return token;
    }

    Token get_next_token() {
      if (errorOccurred) return Token("EOF", "",0,0);

      while (this->current_char != -1 && this->pos <= strlen(this->text) - 1 && !errorOccurred) {
        // DEBUG_ARTI("get_next_token %c\n", this->current_char);
        if (isspace(this->current_char)) {
          this->skip_whitespace();
          continue;
        }

        // if (this->current_char == '{') {
        //   this->advance();
        //   this->skip_comment();
        //   continue;
        // }

        if (isalpha(this->current_char)) {
          return this->id();
        }

        if (isdigit(this->current_char)) {
          return this->number();
        }

        // findLongestMatchingToken(tokensJson, 1);
        const char * token_type = "";
        const char * token_value = "";
          // token_value = token_value.append(1,this->current_char);

        uint8_t longestTokenLength = 0;

        for (JsonPair tokenPair: tokensJson.as<JsonObject>()) {
          const char * value = tokenPair.value().as<const char *>();
          char currentValue[charLength];
          strncpy(currentValue, this->text + this->pos, charLength);
          currentValue[strlen(value)] = '\0';
          if (strcmp(value, currentValue) == 0 && strlen(value) > longestTokenLength) {
            token_value = value;
            token_type = tokenPair.key().c_str();
            longestTokenLength = strlen(value);
          }
        }

        // DEBUG_ARTI("%s\n", "get_next_token (", token_type, ") (", token_value, ")");
        if (strcmp(token_type, "") != 0) {
          // DEBUG_ARTI("%s\n", "get_next_token tvinn", token_type, token_value);
          Token token = Token(token_type, token_value, this->lineno, this->column);
          for (int i=0; i<strlen(token_value); i++)
            this->advance();
          return token;
        }
        else {
          this->error();
        }

      }

      return Token("EOF", "",0,0);

    } //get_next_token

}; //Lexer

struct LexerPosition {
  uint16_t pos;
  char current_char;
  uint16_t lineno;
  uint16_t column;
  const char * type;
  char value[charLength];
};

class Parser {
  private:
    Lexer *lexer;
    LexerPosition positions[100]; //should be array of pointers but for some reason get seg fault
    uint16_t positions_index = 0;

  public:
    Token current_token;

    Parser(Lexer *lexer) {
      this->lexer = lexer;
      this->current_token = this->get_next_token();
    }

    void parse() {
      JsonObject definitionObject = definitionJson.as<JsonObject>();
      JsonObject::iterator it = definitionObject.begin();

      DEBUG_ARTI("Parser %s %s\n", this->current_token.type, this->current_token.value);

      const char * symbol_name = it->key().c_str();
      Result result = visit(parseTreeJson.as<JsonVariant>(), symbol_name, "", definitionJson[symbol_name], 0);

      if (result == Result::RESULTFAIL)
        DEBUG_ARTI("Program parsing failed (%s)\n", symbol_name);
    }

    Token get_next_token() {
      return this->lexer->get_next_token();
    }

    void error(ErrorCode error_code, Token token) {
      ParserError error = ParserError(error_code, token, "");
    }

    void eat(const char * token_type) {
      // DEBUG_ARTI("try to eat %s %s\n", this->current_token.type, token_type);
      if (strcmp(this->current_token.type, token_type) == 0) {
        this->current_token = this->get_next_token();
        // DEBUG_ARTI("eating %s -> %s %s\n", token_type, this->current_token.type, this->current_token.value);
      }
      else {
        this->error(ErrorCode::UNEXPECTED_TOKEN, this->current_token);
      }
    }

  void push_position() {
    // DEBUG_ARTI("%s\n", "push_position ", positions_index, this->lexer.pos);
    // uint16_t index = positions_index%100;
    positions[positions_index].pos = this->lexer->pos;
    positions[positions_index].current_char = this->lexer->current_char;
    positions[positions_index].lineno = this->lexer->lineno;
    positions[positions_index].column = this->lexer->column;
    positions[positions_index].type = this->current_token.type;
    strcpy(positions[positions_index].value, this->current_token.value);
    positions_index++;
  }

  void pop_position() {
    if (positions_index > 0) {
      positions_index--;
      // DEBUG_ARTI("%s\n", "pop_position ", positions_index, this->lexer->pos, " to ", positions[positions_index].pos);
      // uint16_t index = positions_index%100;
      this->lexer->pos = positions[positions_index].pos;
      this->lexer->current_char = positions[positions_index].current_char;
      this->lexer->lineno = positions[positions_index].lineno;
      this->lexer->column = positions[positions_index].column;
      this->current_token.type = positions[positions_index].type;
      strcpy(this->current_token.value, positions[positions_index].value);
    }
  }

  enum class  Result {
    RESULTFAIL,
    RESULTSTOP,
    RESULTCONTINUE,
  };

  Result visit(JsonVariant parseTree, const char * symbol_name, const char * operatorx, JsonVariant expression, int depth = 0) {
    if (errorOccurred) return Result::RESULTFAIL;
    Result result = Result::RESULTCONTINUE;

    if (expression.is<JsonObject>()) {
      // DEBUG_ARTI("%s visit Object %s %s %s\n", spaces+50-depth, symbol_name, operatorx, expression.as<string>().c_str());

      for (JsonPair element : expression.as<JsonObject>()) {
        const char * objectOperator = element.key().c_str();
        JsonVariant objectExpression = element.value();
        if (strcmp(objectOperator, "*") == 0) {
          // DEBUG_ARTI("%s\n", "zero or more");
          visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1); //no assign to result as optional
        }
        else if (strcmp(objectOperator, "?") == 0) {
          // DEBUG_ARTI("%s\n", "zero or one (optional) ");
          visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1); //no assign to result as optional
        }
        else if (strcmp(objectOperator, "+") == 0) {
          // DEBUG_ARTI("%s\n", "one or more ");
          result = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1);
        }
        else if (strcmp(objectOperator, "or") == 0) {
          // DEBUG_ARTI("%s\n", "or ");
          result = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1);
          if (result != Result::RESULTFAIL) result = Result::RESULTCONTINUE;
        }
        else {
          DEBUG_ARTI("%s %s %s\n", "undefined ", objectOperator, objectExpression.as<const char *>());
          result = Result::RESULTFAIL;
        }
      }
    }
    else { //not object
      if (expression.is<JsonArray>()) {
        // DEBUG_ARTI("%s visit Array %s %s %s\n", spaces+50-depth, symbol_name, operatorx, expression.as<string>().c_str());
        Result resultChild;
        if (strcmp(operatorx, "") == 0) 
          operatorx = "and";

        //check if unary or binary operator
        // if (expression.size() > 1) {
        //   DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
        //   DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
        // }

        for (JsonVariant newExpression: expression.as<JsonArray>()) {
          //Save current position, in case some of the expressions in the or array go wrong (deadend), go back to the saved position and try the next
          if (strcmp(operatorx , "or") == 0)
            push_position();

          resultChild = visit(parseTree, symbol_name, "", newExpression, depth + 1);//(operatorx == "")?"and":operatorx

          if (strcmp(operatorx, "*") == 0) resultChild = Result::RESULTCONTINUE; //0 or more always succesful

          if ((strcmp(operatorx, "or") != 0) && resultChild != Result::RESULTCONTINUE) //result should be continue for and, *, +, ?
            result = Result::RESULTFAIL;
          if ((strcmp(operatorx, "or") == 0) && resultChild != Result::RESULTFAIL) //Stop or continue is enough for an or
            result = Result::RESULTSTOP;

          if (strcmp(operatorx, "or") == 0 && resultChild == Result::RESULTFAIL) //if fail, go back and try another
            pop_position();

          if (result != Result::RESULTCONTINUE) 
            break;
        }
        if ((strcmp(operatorx, "or") == 0) && result == Result::RESULTCONTINUE) //still looking but nothing to look for
          result = Result::RESULTFAIL;
      }
      else { //not array
        const char * token_type = expression;

        //if token
        if (!tokensJson[token_type].isNull()) {

          if (strcmp(this->current_token.type, token_type) == 0) {
            // DEBUG_ARTI("%s visit token %s %s\n", spaces+50-depth, this->current_token.type, token_type);//, expression.as<string>().c_str());
            // if (current_token.type == "ID" || current_token.type == "INTEGER" || current_token.type == "REAL" || current_token.type == "INTEGER_CONST" || current_token.type == "REAL_CONST" || current_token.type == "ID" || current_token.type == "ID" || current_token.type == "ID") {
            DEBUG_ARTI("%s %s %s\n", spaces+50-depth, current_token.type, current_token.value);

            if (symbol_name[strlen(symbol_name)-1] == '*') { //if list then add in array
              JsonArray arr = parseTree[symbol_name].as<JsonArray>();
              arr[arr.size()][current_token.type] = current_token.value; //add in last element of array
            }
            else
                parseTree[symbol_name][current_token.type] = current_token.value;

            if (strcmp(token_type, "PLUS2") == 0) { //debug for unary operators (wip)
              // DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
              // DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
            }
            eat(token_type);
          }
          else {
            // DEBUG_ARTI("%s visit deadend %s %s\n", spaces+50-depth, this->current_token.type, token_type);//, expression.as<string>().c_str());
            // parseTree["deadend"] = token_type + "<>" + current_token.type;
            result = Result::RESULTFAIL;
          }
        }
        else { //not object, array or token but symbol
          const char * newSymbol_name = expression;
          JsonVariant newParseTree;

          DEBUG_ARTI("%s %s\n", spaces+50-depth, newSymbol_name);
          // DEBUG_ARTI("%s %s %s\n", spaces+50-depth, parseTree[symbol_name].as<const char *>(), newSymbol_name.c_str());
          JsonArray arr;
          if (symbol_name[strlen(symbol_name)-1] == '*') { //if list then create/get array
            if (parseTree[symbol_name].isNull()) { //create array
              parseTree[symbol_name][0]["ccc"] = "array1"; //make the connection
              arr = parseTree[symbol_name].as<JsonArray>();
            }
            else { //get array
              arr = parseTree[symbol_name].as<JsonArray>();
              arr[arr.size()]["ccc"] = "array2"; //make the connection, add new array item
            }

            newParseTree = arr[arr.size()-1];
          }
          else { //no list, create object
            if (parseTree[symbol_name].isNull()) //no object yet
              parseTree[symbol_name]["ccc"] = "list"; //make the connection, new object item

            newParseTree = parseTree[symbol_name];
          }

          // DEBUG_ARTI("%s %s\n", spaces+50-depth, newSymbol_name);
          result = visit(newParseTree, newSymbol_name, "", definitionJson[newSymbol_name], depth + 1);

          newParseTree.remove("ccc"); //remove connector

          if (result == Result::RESULTFAIL) {
            newParseTree.remove(newSymbol_name); //remove result of visit

          //   DEBUG_ARTI("%s psf %s %s\n", spaces+50-depth, parseTree[symbol_name].as<const char *>(), newSymbol_name.c_str());
          if (symbol_name[strlen(symbol_name)-1] == '*') //if list then remove empty objecy
            arr.remove(arr.size()-1);
          // else
          //   parseTree.remove(newSymbol_name); //this does not change anything...
          } //f
          // DEBUG_ARTI("%s %s\n", spaces+50-depth, parseTree[symbol_name].as<const char *>());

        }
      } //if array
    } //if object
    // DEBUG_ARTI("%s\n", spaces+50-depth, "tokenValue ", tokenValue, isArray, isToken, isObject);
    
     return result;
  } //visit

}; //Parser

#define standardStringLenght 4000

void concatenate(char * string1, const char * string2, uint16_t length = 0) {
  size_t size1 = strlen(string1);
  size_t size2 = strlen(string2);
  if (size1 < standardStringLenght - size2) // it still fits
    strncat(string1, string2, (length==0 || length > size2)?size2:length);
}

class TreeWalker {
  private:
  public:
    TreeWalker() {
    }

    void walk(JsonVariant tree, char * resultString) {
      DEBUG_ARTI("\nWalker\n");
      visit(tree, resultString);
    }

    void visit(JsonVariant parseTree, char * resultString, uint8_t depth = 0) {

      if (parseTree.is<JsonObject>()) {
        concatenate(resultString, spaces, depth); concatenate(resultString,"{\n");
        for (JsonPair element : parseTree.as<JsonObject>()) {
          const char * key = element.key().c_str();
          JsonVariant value = element.value();
          concatenate(resultString, spaces, depth); concatenate(resultString, key); concatenate(resultString, "=\n");
          // DEBUG_ARTI("%s Visit object %s %s\n", spaces+50-depth, symbol_name.c_str(), token.c_str());
          visit(value, resultString, depth + 1);
        } // key values
        concatenate(resultString, spaces, depth); concatenate(resultString,"}\n");
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          concatenate(resultString, spaces, depth); concatenate(resultString,"[\n");
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI("%s Visit array %s %s\n", spaces+50-depth, symbol_name.c_str(), token.c_str());
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
            visit(newParseTree, resultString, depth + 1);
          }
          concatenate(resultString, spaces, depth); concatenate(resultString,"]\n");
        }
        else { //not array
          const char * temp = parseTree.as<const char *>();
          concatenate(resultString, spaces, depth); concatenate(resultString, temp); concatenate(resultString,  "\n");
          // strcat(resultString, parseTree.as<const char *>()); strcat(resultString, "\n");
          // DEBUG_ARTI("%s visit element %s\n", spaces+50-depth, parseTree.as<string>().c_str());
        }
      }
      // DEBUG_ARTI("%s", localResult);
    } //visit

}; //TreeWalker

class ScopedSymbolTable; //forward declaration

class Symbol {
  private:
  public:
  
  char symbol_type[charLength];
  char name[charLength];
  char type[charLength];
  uint8_t scope_level;
  ScopedSymbolTable* scope = nullptr;
  ScopedSymbolTable* detail_scope = nullptr;

  JsonVariant block;

  Symbol(const char * symbol_type, const char * name, const char * type = "") {
    strcpy(this->symbol_type, symbol_type);
    strcpy(this->name, name);
    strcpy(this->type, type);
    this->scope_level = 0;
  }
}; //Symbol


class ScopedSymbolTable {
  private:
  public:

  Symbol* _symbols[100];
  uint16_t _symbolsIndex = 0;
  char scope_name[charLength];
  int scope_level;
  ScopedSymbolTable *enclosing_scope;
  ScopedSymbolTable *child_scopes[100];
  uint16_t child_scopesIndex = 0;

  ScopedSymbolTable(const char * scope_name, int scope_level, ScopedSymbolTable *enclosing_scope = nullptr) {
    // DEBUG_ARTI("%s\n", "ScopedSymbolTable ", scope_name, scope_level);
    strcpy(this->scope_name, scope_name);
    this->scope_level = scope_level;
    this->enclosing_scope = enclosing_scope;
  }

  void init_builtins() {
        // this->insert(BuiltinTypeSymbol('INTEGER'));
        // this->insert(BuiltinTypeSymbol('REAL'));
  }

  void insert(Symbol* symbol) {
    #ifdef _SHOULD_LOG_SCOPE
      DEBUG_ARTI("Log scope Insert %s\n", symbol->name.c_str());
    #endif
    symbol->scope_level = this->scope_level;
    symbol->scope = this;
    this->_symbols[_symbolsIndex] = symbol;
    _symbolsIndex++;
  }

  Symbol* lookup(const char * name, bool current_scope_only=false, bool child_scopes_included=false) {
    // this->log("Lookup: " + name + " " + this->scope_name);
    // DEBUG_ARTI("%s\n", "lookup ", name, this->scope_name, _symbolsIndex, child_scopesIndex);
    //  'symbol' is either an instance of the Symbol class or None;
    for (int i=0; i<_symbolsIndex; i++) {
      // DEBUG_ARTI("%s\n", "  symbols ", i, _symbols[i]->symbol_type, _symbols[i]->name, _symbols[i]->type, _symbols[i]->scope_level);
      if (strcmp(_symbols[i]->name, name) == 0) //replace with strcmp!!!
        return _symbols[i];
    }

    if (child_scopes_included) {
      for (int i=0; i<this->child_scopesIndex;i++) {
        // DEBUG_ARTI("%s\n", "  detail ", i, this->child_scopes[i]->scope_name);
        Symbol* symbol = this->child_scopes[i]->lookup(name, current_scope_only, child_scopes_included);
        if (symbol != nullptr) //symbol found
          return symbol;
      }
    }

    if (current_scope_only)
      return nullptr;
    // # recursively go up the chain and lookup the name;
    if (this->enclosing_scope != nullptr)
      return this->enclosing_scope->lookup(name);
    
    return nullptr;
  } //lookup

  void show(int depth = 0) {

    // DEBUG_ARTI("%s\n", spaces+50-depth, "show ", this->scope_name, " " , this->scope_level);
    for (int i=0; i<_symbolsIndex; i++) {
      // DEBUG_ARTI("%s\n", spaces+50-depth, "-symbols ", i, _symbols[i]->symbol_type, _symbols[i]->name, _symbols[i]->type);
    }

      for (int i=0; i<this->child_scopesIndex;i++) {
        // DEBUG_ARTI("%s\n", spaces+50-depth, "-detail ", i, this->child_scopes[i]->scope_name);
        this->child_scopes[i]->show(depth + 1);
      }
  }
}; //ScopedSymbolTable

class SemanticAnalyzer {
  private:
  public:
    ScopedSymbolTable *global_scope = nullptr;

    SemanticAnalyzer() {
    }

    void analyse() {
      DEBUG_ARTI("\nAnalyzer\n");
      visit(parseTreeJson.as<JsonVariant>());
    }

    void visit(JsonVariant parseTree, const char * treeElement = "", const char * symbol_name = "", const char * token = "", ScopedSymbolTable* current_scope = nullptr, uint8_t depth = -1) {

      // DEBUG_ARTI("%s Visit %s %s\n", spaces+50-depth, symbol_name.c_str(), token.c_str());

      if (parseTree.is<JsonObject>()) {
        for (JsonPair element : parseTree.as<JsonObject>()) {
        if (strcmp(treeElement, "") == 0 || strcmp(treeElement, element.key().c_str()) == 0 ) {
          const char * key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!definitionJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = definitionJson["SEMANTICS"][symbol_name];
            if (!expression.isNull())
            {
              if (strcmp(expression["id"], "Program") == 0) {
                const char * program_name = value["variable"]["ID"];
                this->global_scope = new ScopedSymbolTable(program_name, 1, nullptr); //current_scope

                DEBUG_ARTI("%s Program %s %d %d\n", spaces+50-depth, global_scope->scope_name, global_scope->scope_level, global_scope->_symbolsIndex); 

                // current_scope->child_scopes[current_scope->child_scopesIndex++] = this->global_scope;
                // current_scope = global_scope;
                visit(value[expression["block"].as<const char *>()], "", symbol_name, token, this->global_scope, depth + 1);

                for (int i=0; i<global_scope->_symbolsIndex; i++) {
                  Symbol* symbol = global_scope->_symbols[i];
                  DEBUG_ARTI("%s %d %s %s %s %d\n", spaces+50-depth, i, symbol->symbol_type, symbol->name, symbol->type, symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (strcmp(expression["id"], "Procedure") == 0) {

                //find the procedure name (so we must know this is a procedure...)
                const char * proc_name = value[expression["name"].as<const char *>()];
                Symbol* proc_symbol = new Symbol(symbol_name, proc_name);
                current_scope->insert(proc_symbol);

                DEBUG_ARTI("%s procedure %s %s\n", spaces+50-depth, current_scope->scope_name, proc_name);
                ScopedSymbolTable* procedure_scope = new ScopedSymbolTable(proc_name, current_scope->scope_level + 1, current_scope);
                current_scope->child_scopes[current_scope->child_scopesIndex++] = procedure_scope;
                proc_symbol->detail_scope = procedure_scope;
                // DEBUG_ARTI("%s\n", "ASSIGNING ", proc_symbol->name, " " , procedure_scope->scope_name);

                // current_scope = procedure_scope;
                visit(value[expression["formals"].as<const char *>()], "", symbol_name, token, procedure_scope, depth + 1);

                visit(value[expression["block"].as<const char *>()], "", symbol_name, token, procedure_scope, depth + 1);

                // DEBUG_ARTI("%s\n", spaces+50-depth, "end proc ", symbol_name, procedure_scope->scope_name, procedure_scope->scope_level, procedure_scope->_symbolsIndex); 

                for (int i=0; i<procedure_scope->_symbolsIndex; i++) {
                  Symbol* symbol = procedure_scope->_symbols[i];
                  DEBUG_ARTI("%s %d %s %s %s %d\n", spaces+50-depth, i, symbol->symbol_type, symbol->name, symbol->type, symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (strcmp(expression["id"], "VarSymbol") == 0) {
                // DEBUG_ARTI("%s var (from array) %s %s %s\n", spaces+50-depth, current_scope->scope_name.c_str(), value.as<const char *>().c_str(), key.c_str());
                //can be expression or array of expressions
                if (value.is<JsonArray>()) {
                  for (JsonObject newValue: value.as<JsonArray>()) {
                    const char * param_name = newValue[expression["name"].as<const char *>()];
                    char param_type[charLength]; strcpy(param_type, newValue[expression["type"].as<const char *>()].as<string>().c_str());//current_scope.lookup(param.type_node.value); //need string
                    Symbol* var_symbol = new Symbol(symbol_name, param_name, param_type);
                    current_scope->insert(var_symbol);
                    DEBUG_ARTI("%s var (from array) %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                  }
                }
                else {
                  const char * param_name = value[expression["name"].as<const char *>()];
                  char param_type[charLength]; strcpy(param_type, value[expression["type"].as<const char *>()].as<string>().c_str());//current_scope.lookup(param.type_node.value); //need string
                  Symbol* var_symbol = new Symbol(symbol_name, param_name, param_type);
                  current_scope->insert(var_symbol);
                  DEBUG_ARTI("%s var %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                }
              }
              else if (strcmp(expression["id"], "Assign") == 0) {
                JsonVariant left = value["variable"]["ID"];
                // JsonVariant right = value[expression["value"].as<const char *>()];
                DEBUG_ARTI("%s Assign %s =\n", spaces+50-depth, left.as<const char *>());

                visit(value, expression["value"], symbol_name, token, current_scope, depth + 1);
                visitCalledAlready = true;
              }
            } // is expression["id"]

          } // is symbol_name

          if (!tokensJson[key].isNull()) {
            token = key;
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Token ", token);
          }
          // DEBUG_ARTI("%s\n", spaces+50-depth, "Object ", key, value);

          if (!visitCalledAlready)
            visit(value, "", symbol_name, token, current_scope, depth + 1);

        } // key values
        }
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
            visit(newParseTree, "", symbol_name, token, current_scope, depth + 1);
          }
        }
        else { //not array
          // string element = parseTree;
          // DEBUG_ARTI("%s value notnot %s\n", spaces+50-depth, element.c_str());
          // if (definitionJson["SEMANTICS"][element])
        }
      }
    } //visit

}; //SemanticAnalyzer

class ActivationRecord {
  private:
  public:
    char name[charLength];
    char type[charLength];
    int nesting_level;
    DynamicJsonDocument *members;
    JsonObject mem;

    ActivationRecord(const char * name, const char * type, int nesting_level) {
        strcpy(this->name, name);
        strcpy(this->type, type);
        this->nesting_level = nesting_level;
        this->members = new DynamicJsonDocument(1024);
        this->mem = this->members->createNestedObject();
    }

    void set(const char * key, const char * value) {
      mem[key] = strdup(value);
    }

    const char * get(const char * key) {
      return mem[key];
    }
}; //ActivationRecord

class CallStack {
  ActivationRecord* _records[100];
  uint8_t _recordsCounter = 0;
  public:
    // CallStack() {
    //     // this->_records = [];
    // }

    void push(ActivationRecord* ar) {
      // DEBUG_ARTI("%s\n", "Push ", ar->name);
        this->_records[_recordsCounter++] = ar;
    }

    ActivationRecord* pop() {
      // DEBUG_ARTI("%s\n", "Pop ", this->peek()->name);
        return this->_records[_recordsCounter--];
    }

    ActivationRecord* peek() {
        return this->_records[_recordsCounter-1];
    }
}; //CallStack

class Calculator {
  private:
    char stack[100][charLength];
    uint8_t stack_index = 0;
  public:
  Calculator() {

  }

  void push(const char * key, const char * value) {
    // DEBUG_ARTI("calc push %s %s\n", key.c_str(), value.c_str());
      strcpy(stack[stack_index++], value);
  }

  const char * peek() {
    // DEBUG_ARTI("Peek %s\n", stack[stack_index-1].c_str());
    return stack[stack_index-1];
  }

  const char * pop() {
    if (stack_index>0)
      stack_index--;
    // DEBUG_ARTI("Pop %s\n", stack[stack_index].c_str());
    return stack[stack_index];
  }

}; //Calculator

class Interpreter {
  private:
  CallStack call_stack;
  ScopedSymbolTable *global_scope;
  Calculator calculator;

  public:

  Interpreter(SemanticAnalyzer *analyzer) {
    this->global_scope = analyzer->global_scope;
  }

  void interpret() {
    if (global_scope != nullptr) { //due to undefined procedures??? wip

      DEBUG_ARTI("\ninterpret %s %d %d %d\n", global_scope->scope_name, global_scope->scope_level, global_scope->_symbolsIndex, global_scope->child_scopesIndex); 
      for (int i=0; i<global_scope->_symbolsIndex; i++) {
        Symbol* symbol = global_scope->_symbols[i];
        DEBUG_ARTI("scope %s %s %s %d\n", symbol->symbol_type, symbol->name, symbol->type, symbol->scope_level); 
      }
    }
    else
      DEBUG_ARTI("Interpret global scope is nullptr\n");

    visit(parseTreeJson.as<JsonVariant>());
  }

            // ["PLUS2", "factor"],
          // [ "MINUS2", "factor"],

    void visit(JsonVariant parseTree, const char * treeElement = "", const char * symbol_name = "", const char * token = "", uint8_t depth = 0) {

      if (parseTree.is<JsonObject>()) {
        for (JsonPair element : parseTree.as<JsonObject>()) {
        if (strcmp(treeElement, "") == 0 || strcmp(treeElement, element.key().c_str()) == 0 ) {
          const char * key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!definitionJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = definitionJson["INTERPRETER"][symbol_name];
            if (!expression.isNull())
            {
              // DEBUG_ARTI("%s\n", spaces+50-depth, "Symbol ", symbol_name,  " ", expression);

              if (strcmp(expression["id"], "Program") == 0) {
                DEBUG_ARTI("%s program name %s\n", spaces+50-depth, expression["name"].as<const char *>());
                const char * program_name = value["variable"]["ID"];

                ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM",1);
                DEBUG_ARTI("%s %s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, program_name);

                this->call_stack.push(ar);

                visit(value[expression["block"].as<const char *>()], "", symbol_name, token, depth + 1);

                this->call_stack.pop();

                visitCalledAlready = true;
              }
              else if (strcmp(expression["id"], "Procedure") == 0) {
                const char * proc_name = value[expression["name"].as<const char *>()]; //as string is needed!!!
                Symbol* proc_symbol = this->global_scope->lookup(proc_name, true, true);
                DEBUG_ARTI("%s Save block of %s\n", spaces+50-depth, proc_name);
                proc_symbol->block = value[expression["block"].as<const char *>()];
                visitCalledAlready = true;
              }
              else if (strcmp(expression["id"], "ProcedureCall") == 0) {
                const char * proc_name = value[expression["name"].as<const char *>()]; //as string is needed!!!
                Symbol* proc_symbol = this->global_scope->lookup(proc_name, true, true);

                if (proc_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                ActivationRecord* ar = new ActivationRecord(proc_name, "PROCEDURE", proc_symbol->scope_level + 1);

                DEBUG_ARTI("%s %s %s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, proc_name, proc_symbol->name);

                visit(value[expression["actuals"].as<const char *>()], "", symbol_name, token, depth + 1);

                for (int i=proc_symbol->detail_scope->_symbolsIndex-1; i>=0;i--) { //backwards because popped in reversed order
                  if (strcmp(proc_symbol->detail_scope->_symbols[i]->symbol_type, "formal_parameters") == 0) { //select formal parameters
                  //{"ID":"Alpha","LPAREN":"(","actual_parameter_list*":[
                    //      {"expr*":[{"term*":[{"factor":{"INTEGER_CONST":"3"}},{}]},{"PLUS":"+"},{"term*":[{"factor":{"INTEGER_CONST":"5"}},{}]}]},{"COMMA":","},
                    //      {"expr*":[{"term*":[{"factor":{"INTEGER_CONST":"7"}},{}]},{}]}],"RPAREN":")","SEMI":";"} 
                    const char * result = calculator.pop();
                    ar->set(proc_symbol->detail_scope->_symbols[i]->name, result);
                    DEBUG_ARTI("%s %s = %s\n", spaces+50-depth, proc_symbol->detail_scope->_symbols[i]->name, result);
                  }
                }

                this->call_stack.push(ar);

                //find block of procedure... lookup procedure?
                //visit block of procedure
                // DEBUG_ARTI("%s proc block %s\n", spaces+50-depth, proc_symbol->block.as<string>().c_str());

                //Terms tovisit [{"factor":{"LPAREN":"(",
                //      "expr*":[{"term*":[{"factor":{"variable":{"ID":"a"}}}]},{"PLUS":"+"},{"term*":[{"factor":{"variable":{"ID":"b"}}}]}],
                //                          "RPAREN":")"}},{"MUL":"*"},{"factor":{"INTEGER_CONST":"2"}}] 
                //Exprs tovisit [{"term*":[{"factor":{"variable":{"ID":"a"}}}]},{"PLUS":"+"},{"term*":[{"factor":{"variable":{"ID":"b"}}}]}] 

                //{"declarations*":[
                //     {"variable_declaration":{"ID":"x","COLON":":","type_spec":{"INTEGER":"INTEGER"}}},
                //     {"SEMI":";"}],
                //"compound_statement":{
                //    "BEGIN":"BEGIN",
                //    "statement_list*":[
                //       {"statement":{
                //           "assignment_statement":{"variable":{"ID":"x"},"ASSIGN":":=",
                //                  "expr*":[{"term*":[{"factor":{"variable":{"ID":"a"}}},{"MUL":"*"},{"factor":{"INTEGER_CONST":"10"}}]},
                //                           {"PLUS":"+"},
                //                           {"term*":[{"factor":{"variable":{"ID":"b"}}},{"MUL":"*"},{"factor":{"INTEGER_CONST":"2"}}]}
                //                          ]       }
                //                     }
                //         },{"SEMI":";"}],
                //    "END":"END"       }
                //} 
                visit(proc_symbol->block, "", symbol_name, token, depth + 1);

                this->call_stack.pop();

                visitCalledAlready = true;
                } //proc_symbol != nullptr
                else {
                  DEBUG_ARTI("%s %s not found %s\n", spaces+50-depth, expression["id"].as<const char *>(), proc_name);
                }

              }
              else if (expression["id"] == "Assign") {

                visit(value, expression["value"], symbol_name, token, depth + 1);

                ActivationRecord* ar = this->call_stack.peek();
                ar->set(value["variable"]["ID"], this->calculator.pop());

                DEBUG_ARTI("%s %s := %s\n", spaces+50-depth, value["variable"]["ID"].as<const char *>(), ar->get(value["variable"]["ID"]));

                visitCalledAlready = true;
              }
              else if (strcmp(expression["id"], "Exprs") == 0 || strcmp(expression["id"], "Terms") == 0) {
                // DEBUG_ARTI("%s %s tovisit %s\n", spaces+50-depth, expression["id"].as<const char *>(), value.as<string>().c_str());
                const char * operatorx = "";
                if (value.is<JsonArray>()) {
                  JsonArray valueArray = value.as<JsonArray>();
                  if (valueArray.size() >= 1) // visit first symbol 
                    visit(valueArray[0], "", symbol_name, token, depth + 1);
                  if (valueArray.size() >= 3) { // add operator and another symbol
                    operatorx = valueArray[1].as<string>().c_str(); // as string because contains Jsonbject
                    // DEBUG_ARTI("%s %s operator %s\n", spaces+50-depth, expression["id"].as<const char *>(), operatorx.c_str());
                    visit(valueArray[2], "", symbol_name, token, depth + 1);
                  }
                  if (valueArray.size() != 1 && valueArray.size() != 3)
                    DEBUG_ARTI("%s %s array not right size ?? (%d) %s %s \n", spaces+50-depth, expression["id"].as<const char *>(), valueArray.size(), key, value.as<string>().c_str());
                }
                else
                  DEBUG_ARTI("%s %s not array?? %s %s \n", spaces+50-depth, key, expression["id"].as<const char *>(), value.as<string>().c_str());
                // DEBUG_ARTI("%s operatorx %s\n", spaces+50-depth, operatorx);
                if (strstr(operatorx, "PLUS")) {
                  const char * right = calculator.pop();
                  const char * left = calculator.pop();
                  int result = atoi(left) + atoi(right);
                  DEBUG_ARTI("%s %s + %s = %d\n", spaces+50-depth, left, right, result);
                  calculator.push("PLUS", intToCharString(result));
                }
                if (strstr(operatorx, "MUL")) {
                  const char * right = calculator.pop();
                  const char * left = calculator.pop();
                  int result = atoi(left) * atoi(right);
                  DEBUG_ARTI("%s %s * %s = %d\n", spaces+50-depth, left, right, result);
                  calculator.push("MUL", intToCharString(result));
                }
                visitCalledAlready = true;
              }
              else if (strcmp(expression["id"], "Variable") == 0) {
                ActivationRecord* ar = this->call_stack.peek();
                calculator.push(key, ar->get(value[expression["name"].as<const char *>()]));
                DEBUG_ARTI("%s %s %s %s\n", spaces+50-depth, key, value[expression["name"].as<const char *>()].as<const char *>(), ar->get(value[expression["name"].as<const char *>()]));
                visitCalledAlready = true;
              }
              else if (strcmp(expression["id"], "ForLoop") == 0) {
                DEBUG_ARTI("%s for loop\n", spaces+50-depth);

                visit(value, expression["from"], symbol_name, token, depth + 1);
                visit(value[expression["to"].as<const char *>()], "", symbol_name, token, depth + 1);
                ActivationRecord* ar = this->call_stack.peek();
                for (int i=0; i<2;i++) { //this is the current state of this project: adding for loops, of course the from and to should be derived from the code ;-)
                  DEBUG_ARTI("%s iteration %d\n", spaces+50-depth, i);
                  ar->set("y", intToCharString(i));
                  visit(value[expression["block"].as<const char *>()], "", symbol_name, token, depth + 1);
                }
                visitCalledAlready = true;
              }
            } //if expression["id"]

          } // is key is symbol_name

          // DEBUG_ARTI("%s\n", spaces+50-depth, "Object ", key, value);
          // if (key == "INTEGER_CONST" || key == "PLUS" || key == "MUL" || key == "LPAREN"  || key == "RPAREN" ) {
          if (strcmp(key, "INTEGER_CONST") == 0) {// || value == "+" || value == "*") || value == "("  || value == ")" ) {
            calculator.push(key, value.as<const char *>());
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Calculator (push) ", key, value, calculator.peek());
            visitCalledAlready = true;
          }

          if (!tokensJson[key].isNull()) { //if key is token
            token = key;
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Token ", token);
          }
          if (!visitCalledAlready)
            visit(value, "", symbol_name, token, depth + 1);
        }
        } // for (JsonPair
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
            visit(newParseTree, "", symbol_name, token, depth + 1);
          }
        }
        else { //not array
          const char * element = parseTree.as<const char *>();
          // DEBUG_ARTI("%s\n", spaces+50-depth, "not array not object but element ", element);
        }
      }
    }

};

class ARTI {
private:
  Lexer *lexer;
  Parser *parser;
  SemanticAnalyzer *semanticAnalyzer;
  TreeWalker *treeWalker;
  Interpreter *interpreter;
  char programText[1000];
public:
  ARTI() {

    // char byte = charFromProgramFile();
    // while (byte != -1) {
    //   programText += byte;
    //   DEBUG_ARTI("%c", byte);
    //   byte = charFromProgramFile();
    // }

  }

  void parse() {
    lexer = new Lexer(this->programText);
    lexer->fillTokensJson();
    parser = new Parser(this->lexer);
    parser->parse();
  }

  void walk(JsonVariant tree, char * resultString) {
    treeWalker = new TreeWalker();
    // DeserializationError err = deserializeJson(definitionJson, definitionText);
    // if (err) {
    //   DEBUG_ARTI("deserializeJson() in walk failed with code %s\n", err.c_str());
    // }
    // DEBUG_ARTI("%s\n", definitionText.c_str());
    treeWalker->walk(tree, resultString);
  }

  void analyze() {
    semanticAnalyzer = new SemanticAnalyzer();
    semanticAnalyzer->analyse();
  }

  void interpret() {
    interpreter = new Interpreter(semanticAnalyzer);
    interpreter->interpret();// interpreter.print();
  }

  #ifdef ESP32
  void openFileAndParse(const char *definitionName, const char *programName) {
      char parseOrLoad[charLength] = "NoParse";

      File definitionFile = WLED_FS.open(definitionName, "r");

      File programFile = WLED_FS.open(programName, "r");

      char parseTreeName[charLength];
      strcpy(parseTreeName, programName);
      if (strcmp(parseOrLoad, "Parse") == 0 )
        strcpy(parseTreeName, "Gen");
      strcat(parseTreeName, ".json");
      File parseTreeFile = WLED_FS.open(parseTreeName, (strcmp(parseOrLoad, "Parse")==0)?"w":"r");

      #ifdef ARTI_LOG
        char logFileName[charLength];
        strcpy(logFileName, "/");
        strcpy(logFileName, programName);
        strcat(logFileName, ".log");

        logFile = WLED_FS.open(logFileName,"w");
      #endif

      if (!programFile || !definitionFile)
      {
        DEBUG_ARTI("Files not found\n");
      }
      else {

        //read program
        uint16_t index = 0;
        while(programFile.available()){
          programText[index++] = (char)programFile.read();
        }
        programText[index] = '\0';

        //read definition
        DeserializationError err = deserializeJson(definitionJson, definitionFile);
        if (err) {
          DEBUG_ARTI("deserializeJson() in Lexer failed with code %s\n", err.c_str());
        }

        if (strcmp(parseOrLoad, "Parse") == 0) {
          this->parse();

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

        this->analyze();

      }

      programFile.close();
      definitionFile.close();
      parseTreeFile.close();
      #ifdef ARTI_LOG
        logFile.close();
      #endif

  } // openFileAndParse
  #else
    void openFileAndParse(const char *definitionName, const char *programName) {
      fstream definitionFile;
      definitionFile.open(definitionName, ios::in);

      fstream programFile;
      programFile.open(programName, ios::in);

      char parseTreeName[charLength];
      strcpy(parseTreeName, programName);
      strcat(parseTreeName, ".json");

      fstream parseTreeFile;
      parseTreeFile.open(parseTreeName, ios::out);

      #ifdef ARTI_LOG
        char logFileName[charLength];
        strcpy(logFileName, programName);
        strcat(logFileName, ".log");

        logFile = fopen (logFileName,"w");
      #endif

      if (!programFile || !definitionFile)
      {
        DEBUG_ARTI("Files not found:\n");
      }
      else {

        //read program
        programFile.read(programText, sizeof programText);
        programText[programFile.gcount()] = '\0';
        // DEBUG_ARTI("%s %d", programText, strlen(programText));

        //read definition
        DeserializationError err = deserializeJson(definitionJson, definitionFile);
        if (err) {
          DEBUG_ARTI("deserializeJson() in Lexer failed with code %s\n", err.c_str());
        }

        this->parse();

        //write parseTree
        serializeJsonPretty(parseTreeJson, parseTreeFile);

        // char resultString[standardStringLenght];
        // arti.walk(parseTreeJson.as<JsonVariant>(), resultString);
        // DEBUG_ARTI("walk result %s", resultString);

        this->analyze();

        // printf("Done!!!\n");
      }

      programFile.close();
      definitionFile.close();
      parseTreeFile.close();
      #ifdef ARTI_LOG
        // fclose (logFile); //should not be closed as still streaming...
      #endif
    } // openFileAndParse
  #endif

}; //ARTI

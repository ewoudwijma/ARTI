/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti.h
   @version 0.0.2
   @date    20211017
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
   @remarks
          - #define ARDUINOJSON_DEFAULT_NESTING_LIMIT 100 //set this in ArduinoJson!!!
   @todo
          - eexpression[""]  variables
          - *arti in segments
 */

#ifdef ESP32 //want to use a WLED variable here, but WLED_H is set in wled.h...
  #include "wled.h"
#endif

#ifdef WLED_H
  #include "src/dependencies/json/ArduinoJson-v6.h"
  // #define ARTI_DEBUG 1
  #define ARTI_DEBUGORLOG 1
  #define ARTI_MEMORY 1
#else //embedded
  #define ARTI_DEBUG 1
  #include "ArduinoJson-recent.h"
#endif

#define ARTI_ERROR 1

#ifdef ARTI_DEBUGORLOG
  #ifdef ESP32
    #define DEBUG_ARTI0(...) Serial.printf(__VA_ARGS__)
  #else
    #define DEBUG_ARTI0 printf
  #endif
#else
  #ifdef ESP32
    File logFile;
    #define DEBUG_ARTI0(...) logFile.printf(__VA_ARGS__)
  #else
    FILE * logFile;
    #define DEBUG_ARTI0(...) fprintf(logFile, __VA_ARGS__)
  #endif
#endif

#ifdef ARTI_DEBUG
    #define DEBUG_ARTI(...) DEBUG_ARTI0(__VA_ARGS__)
#else
    #define DEBUG_ARTI(...)
#endif

#ifdef ARTI_ERROR
    #define ERROR_ARTI(...) DEBUG_ARTI0(__VA_ARGS__)
#else
    #define ERROR_ARTI(...)
#endif

#ifdef ARTI_MEMORY
    #define MEMORY_ARTI(...) DEBUG_ARTI0(__VA_ARGS__)
#else
    #define MEMORY_ARTI(...)
#endif

#define charLength 30
#define arrayLength 30

#ifdef ESP32
  const char spaces[51] PROGMEM = "                                                  ";
#else
  #include <iostream>
  #include <fstream>
  #include <sstream>
  const char spaces[51]         = "                                                  ";
#endif

const char * stringOrEmpty(const char *charS)  {
  if (charS == nullptr)
    return "";
  else
    return charS;
}

enum class ErrorCode {
  UNEXPECTED_TOKEN,
  ID_NOT_FOUND,
  DUPLICATE_ID,
  NONE
};

bool errorOccurred = false;

class Token {
  private:
    uint16_t lineno;
    uint16_t column;
  public:
    char type[charLength];
    char value[charLength]; 
    
  Token(const char * type, const char * value, uint16_t lineno=0, uint16_t column=0) {
    strcpy(this->type, (type==nullptr)?"":type);
    strcpy(this->value, (value==nullptr)?"":value);
    this->lineno = lineno;
    this->column = column;
  }

  ~Token() {
    // MEMORY_ARTI("Destruct Token\n");
  }

}; //Token

class Error {
  public:
    ErrorCode error_code;
    Token *token;
    const char * message;
  public:

  Error(ErrorCode error_code=ErrorCode::NONE, Token *token = nullptr, const char * message = nullptr) {
    this->error_code = error_code;
    this->token = token;
    this->message = message;
    ERROR_ARTI("Error"); 
    if (token != nullptr)
      ERROR_ARTI(" %s %s", this->token->type, this->token->value);
    if (message != nullptr)
      ERROR_ARTI(" %s", this->message);
    // ERROR_ARTI(" %s\n", this->error_code);
    ERROR_ARTI("\n");
    errorOccurred = true;
  }

  ~Error() {
    DEBUG_ARTI("Destruct Error\n");
  }

};

class LexerError: public Error {
  public:

  LexerError(ErrorCode error_code=ErrorCode::NONE, Token *token = nullptr, const char * message = nullptr) {
    ERROR_ARTI("Lexer Error"); 
    if (token != nullptr)
      ERROR_ARTI(" %s %s", this->token->type, this->token->value);
    if (message != nullptr)
      ERROR_ARTI(" %s", this->message);
    // ERROR_ARTI(" %s\n", this->error_code);
    ERROR_ARTI("\n");
    errorOccurred = true;
  }

  ~LexerError() {
    DEBUG_ARTI("Destruct LexerError\n");
  }

};

class ParserError: public Error {
  public:

  ParserError(ErrorCode error_code=ErrorCode::NONE, Token *token = nullptr, const char * message = nullptr) {
    ERROR_ARTI("Parser Error"); 
    if (token != nullptr)
      ERROR_ARTI(" %s %s", this->token->type, this->token->value);
    if (message != nullptr)
      ERROR_ARTI(" %s", this->message);
    // ERROR_ARTI(" %s\n", this->error_code);
    ERROR_ARTI("\n");
    errorOccurred = true;
  }

  ~ParserError() {
    DEBUG_ARTI("Destruct ParserError\n");
  }

};

class SemanticError: public Error {};

class Lexer {
  private:
  public:
    const char * text;
    uint16_t pos;
    char current_char;
    uint16_t lineno;
    uint16_t column;
    JsonObject definitionJson;

  Lexer(const char * programText, JsonObject definitionJson) {
    this->text = programText;
    this->definitionJson = definitionJson;
    this->pos = 0;
    this->current_char = this->text[this->pos];
    this->lineno = 1;
    this->column = 1;
  }

  ~Lexer() {
    DEBUG_ARTI("Destruct Lexer\n");
  }

    void error() {
      ERROR_ARTI("Lexer error on %c line %u col %u\n", this->current_char, this->lineno, this->column);
      LexerError(ErrorCode::NONE, nullptr, nullptr); //LexerError x = 
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

    Token *number() {
      Token *token = new Token(nullptr, nullptr, this->lineno, this->column);

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
        strcpy(token->type, "REAL_CONST");
        strcpy(token->value, result);
      }
      else {
        result[strlen(result)] = '\0';
        strcpy(token->type, "INTEGER_CONST");
        strcpy(token->value, result);
      }

      return token;
    }

    Token *id() {
        Token *token = new Token(nullptr, nullptr, this->lineno, this->column);

        char result[charLength] = "";
        while (this->current_char != -1 && isalnum(this->current_char)) {
            result[strlen(result)] = this->current_char;
            this->advance();
        }
        result[strlen(result)] = '\0';

        char resultUpper[charLength];
        strcpy(resultUpper, result);
        strupr(resultUpper);

        // DEBUG_ARTI("upper %s [%s] [%s]\n", definitionJson["TOKENS"][resultUpper].as<const char *>(), result, resultUpper);
        if (definitionJson["TOKENS"][resultUpper].isNull()) {
            strcpy(token->type, "ID");
            strcpy(token->value, result);
        }
        else {
            strcpy(token->type, definitionJson["TOKENS"][resultUpper]);
            strcpy(token->value, resultUpper);
        }

        return token;
    }

    Token *get_next_token() {
      if (errorOccurred) return nullptr;

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

        // findLongestMatchingToken
        char token_type[charLength] = "";
        char token_value[charLength] = "";

        uint8_t longestTokenLength = 0;

        for (JsonPair tokenPair: definitionJson["TOKENS"].as<JsonObject>()) {
          const char * value = tokenPair.value();
          char currentValue[charLength];
          strncpy(currentValue, this->text + this->pos, charLength);
          currentValue[strlen(value)] = '\0';
          if (strcmp(value, currentValue) == 0 && strlen(value) > longestTokenLength) {
            strcpy(token_type, tokenPair.key().c_str());
            strcpy(token_value, value);
            longestTokenLength = strlen(value);
          }
        }

        // DEBUG_ARTI("%s\n", "get_next_token (", token_type, ") (", token_value, ")");
        if (strcmp(token_type, "") != 0 && strcmp(token_value, "") != 0) {
          // DEBUG_ARTI("%s\n", "get_next_token tvinn", token_type, token_value);
          Token *token = new Token(token_type, token_value, this->lineno, this->column);
          for (int i=0; i<strlen(token_value); i++)
            this->advance();
          return token;
        }
        else 
          this->error();

      }

      return nullptr;

    } //get_next_token

}; //Lexer

struct LexerPosition {
  uint16_t pos;
  char current_char;
  uint16_t lineno;
  uint16_t column;
  char type[charLength];
  char value[charLength];
};

#define nrOfPositions 20

class Parser {
  private:
    Lexer *lexer;
    LexerPosition positions[nrOfPositions]; //should be array of pointers but for some reason get seg fault (because a struct and not a class...)
    uint8_t positions_index = 0;
    JsonVariant parseTreeJson;

  public:
    Token *current_token;

  Parser(Lexer *lexer, JsonVariant parseTreeJson) {
    this->lexer = lexer;
    this->parseTreeJson = parseTreeJson;

    this->current_token = this->get_next_token();
  }

  ~Parser() {
    DEBUG_ARTI("Destruct Parser\n");
  }

    void parse() {
      JsonObject::iterator it = lexer->definitionJson.begin();

      DEBUG_ARTI("Parser %s %s\n", this->current_token->type, this->current_token->value);

      const char * symbol_name = it->key().c_str();
      Result result = visit(parseTreeJson, symbol_name, nullptr, lexer->definitionJson[symbol_name], 0);

      if (this->lexer->pos != strlen(this->lexer->text))
        ERROR_ARTI("Symbol %s Program not entirely parsed (%u,%u) %u of %u\n", symbol_name, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
      else if (result == Result::RESULTFAIL)
        ERROR_ARTI("Symbol %s Program parsing failed (%u,%u) %u of %u\n", symbol_name, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
      else
        DEBUG_ARTI("Symbol %s Parsed until (%u,%u) %u of %u\n", symbol_name, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
    }

    Token *get_next_token() {
      return this->lexer->get_next_token();
    }

    void error(ErrorCode error_code, Token *token) {
      ParserError(error_code, token, "ParserError"); //ParserError error = 
    }

    void eat(const char * token_type) {
      // DEBUG_ARTI("try to eat %s %s\n", this->current_token->type, token_type);
      if (strcmp(this->current_token->type, token_type) == 0) {
        if (this->current_token != nullptr) {
          delete this->current_token; this->current_token = nullptr;
        }
        this->current_token = this->get_next_token();
        // DEBUG_ARTI("eating %s -> %s %s\n", token_type, this->current_token->type, this->current_token->value);
      }
      else {
        this->error(ErrorCode::UNEXPECTED_TOKEN, this->current_token);
      }
    }

  void push_position() {
    // DEBUG_ARTI("%s\n", "push_position ", positions_index, this->lexer.pos);
    if (positions_index < nrOfPositions) {
      positions[positions_index].pos = this->lexer->pos;
      positions[positions_index].current_char = this->lexer->current_char;
      positions[positions_index].lineno = this->lexer->lineno;
      positions[positions_index].column = this->lexer->column;
      strcpy(positions[positions_index].type, this->current_token->type);
      strcpy(positions[positions_index].value, this->current_token->value);
      positions_index++;
    }
    else
      ERROR_ARTI("not enough positions %u\n", nrOfPositions);
  }

  void pop_position() {
    if (positions_index > 0) {
      positions_index--;
      // DEBUG_ARTI("%s\n", "pop_position ", positions_index, this->lexer->pos, " to ", positions[positions_index].pos);
      this->lexer->pos = positions[positions_index].pos;
      this->lexer->current_char = positions[positions_index].current_char;
      this->lexer->lineno = positions[positions_index].lineno;
      this->lexer->column = positions[positions_index].column;
      strcpy(this->current_token->type, positions[positions_index].type);
      strcpy(this->current_token->value, positions[positions_index].value);
    }
    else
      ERROR_ARTI("no positions saved\n");
  }

  enum class  Result {
    RESULTFAIL,
    RESULTSTOP,
    RESULTCONTINUE,
  };

  Result visit(JsonVariant parseTree, const char * symbol_name, const char * operatorx, JsonVariant expression, uint8_t depth = 0) {
    if (depth > 50) {
      ERROR_ARTI("Error too deep %u\n", depth);
      errorOccurred = true;
    }
    if (errorOccurred) return Result::RESULTFAIL;

    Result result = Result::RESULTCONTINUE;

    // DEBUG_ARTI("%s Visit %s %s\n", spaces+50-depth, stringOrEmpty(symbol_name), stringOrEmpty(operatorx)); //, expression.as<String>().c_str()

    if (expression.is<JsonObject>()) {
      // DEBUG_ARTI("%s visit Object %s %s %s\n", spaces+50-depth, symbol_name, operatorx, expression.as<String>().c_str());

      for (JsonPair element : expression.as<JsonObject>()) {
        const char * objectOperator = element.key().c_str();
        JsonVariant objectExpression = element.value();

        //and: see 'is array'
        if (strcmp(objectOperator, "or") == 0) {
          // DEBUG_ARTI("%s\n", "or ");
          result = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1);
          if (result != Result::RESULTFAIL) result = Result::RESULTCONTINUE;
        }
        else {
          Result resultChild = Result::RESULTCONTINUE;
          uint8_t counter = 0;
          while (resultChild == Result::RESULTCONTINUE) {
            // DEBUG_ARTI("Before %u (%u.%u) %u of %u %s\n", resultChild, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text), objectExpression.as<String>().c_str());
            resultChild = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1); //no assign to result as optional
            // DEBUG_ARTI("After %u (%u.%u) %u of %u %s\n", resultChild, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text), objectExpression.as<String>().c_str());

            if (strcmp(objectOperator, "?") == 0) { //zero or one iteration, also continue of visit not continue
              resultChild = Result::RESULTCONTINUE;
              break; 
            }
            else if (strcmp(objectOperator, "+") == 0) { //one or more iterations, stop if first visit not continue
              if (counter == 0) {
                if (resultChild != Result::RESULTCONTINUE)
                  break;
              } 
              else {
                if (resultChild != Result::RESULTCONTINUE) {
                  resultChild = Result::RESULTCONTINUE;  //always continue
                  break;
                }
              }
            }
            else if (strcmp(objectOperator, "*") == 0) { //zero or more iterations, stop if visit not continue
              if (resultChild != Result::RESULTCONTINUE) {
                resultChild = Result::RESULTCONTINUE;  //always continue
                break;
              }
            }
            else {
              ERROR_ARTI("%s %s %s\n", "undefined ", objectOperator, objectExpression.as<const char *>());
              result = Result::RESULTFAIL;
            }
            counter++;
          } //while
          result = resultChild;
        } //not or
      } //for
    }
    else { //not object
      if (expression.is<JsonArray>()) {
        // DEBUG_ARTI("%s visit Array %s %s %s\n", spaces+50-depth, symbol_name, operatorx, expression.as<String>().c_str());

        Result resultChild;

        if (operatorx == nullptr) 
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

          resultChild = visit(parseTree, symbol_name, nullptr, newExpression, depth + 1);//(operatorx == "")?"and":operatorx
          // DEBUG_ARTI("%s visited Array element %u %s \n", spaces+50-depth, resultChild, newExpression.as<String>().c_str());

          if (strcmp(operatorx, "or") == 0) {
            if (resultChild == Result::RESULTFAIL) {//if fail, go back and try another
              // result = Result::RESULTCONTINUE;
              pop_position();
            }
            else {
              result = Result::RESULTSTOP;  //Stop or continue is enough for an or
              positions_index--;
            }
          }
          else {
            if (resultChild != Result::RESULTCONTINUE) //for and, ?, + and *; each result should continue
              result = Result::RESULTFAIL;
          } 

          if (result != Result::RESULTCONTINUE) //if no reason to continue then stop
            break;
        } //for

        if (strcmp(operatorx, "or") == 0) {
          if (result == Result::RESULTCONTINUE) //still looking but nothing to look for
            result = Result::RESULTFAIL;
        } 

        // DEBUG_ARTI("%s visited Array %u %s %s %s\n", spaces+50-depth, result, symbol_name, operatorx, expression.as<String>().c_str());
      }
      else { //not array
        const char * token_type = expression;

        //if token
        if (!lexer->definitionJson["TOKENS"][token_type].isNull()) {

          if (strcmp(this->current_token->type, token_type) == 0) {
            // DEBUG_ARTI("%s visit token %s %s\n", spaces+50-depth, this->current_token->type, token_type);//, expression.as<String>().c_str());
            // if (current_token->type == "ID" || current_token->type == "INTEGER" || current_token->type == "REAL" || current_token->type == "INTEGER_CONST" || current_token->type == "REAL_CONST" || current_token->type == "ID" || current_token->type == "ID" || current_token->type == "ID") {
            if (current_token != nullptr) {
              DEBUG_ARTI("%s %s %s", spaces+50-depth, current_token->type, current_token->value);
            }

            if (symbol_name[strlen(symbol_name)-1] == '*') { //if list then add in array
              JsonArray arr = parseTree[symbol_name].as<JsonArray>();
              arr[arr.size()][current_token->type] = current_token->value; //add in last element of array
            }
            else
                parseTree[symbol_name][current_token->type] = current_token->value;

            if (strcmp(token_type, "PLUS2") == 0) { //debug for unary operators (wip)
              // DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
              // DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
            }

            eat(token_type);

            if (current_token != nullptr) {
              DEBUG_ARTI(" -> [%s %s]\n", current_token->type, current_token->value);
            }
            else
              DEBUG_ARTI("\n");
          }
          else {
            // DEBUG_ARTI("%s visit deadend %s %s\n", spaces+50-depth, this->current_token->type, token_type);//, expression.as<String>().c_str());
            // parseTree["deadend"] = token_type + "<>" + current_token->type;
            result = Result::RESULTFAIL;
          }
        }
        else { //not object, array or token but symbol
          const char * newSymbol_name = expression;
          JsonVariant newParseTree;

          DEBUG_ARTI("%s %s\n", spaces+50-depth, newSymbol_name);
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
          result = visit(newParseTree, newSymbol_name, nullptr, lexer->definitionJson[newSymbol_name], depth + 1);

          newParseTree.remove("ccc"); //remove connector

          if (result == Result::RESULTFAIL) {
            DEBUG_ARTI("%s fail %s\n", spaces+50-depth, newSymbol_name);
            newParseTree.remove(newSymbol_name); //remove result of visit

            //   DEBUG_ARTI("%s psf %s %s\n", spaces+50-depth, parseTree[symbol_name].as<const char *>(), newSymbol_name.c_str());
            if (symbol_name[strlen(symbol_name)-1] == '*') //if list then remove empty objecy
              arr.remove(arr.size()-1);
            // else
            //   parseTree.remove(newSymbol_name); //this does not change anything...
          }
          else {
            DEBUG_ARTI("%s success %s\n", spaces+50-depth, newSymbol_name);
            // DEBUG_ARTI("%s success %s %s\n", spaces+50-depth, newSymbol_name, parseTree.as<String>().c_str());
          }
          // DEBUG_ARTI("%s %s\n", spaces+50-depth, parseTree[symbol_name].as<const char *>());

        } //symbol
      } //if array
    } //if object
    // DEBUG_ARTI("%s\n", spaces+50-depth, "tokenValue ", tokenValue, isArray, isToken, isObject);
    
     return result;
  } //visit

}; //Parser

#define standardStringLenght 4000

class ScopedSymbolTable; //forward declaration

class Symbol {
  private:
  public:
  
  char symbol_type[charLength];
  char name[charLength];
  char type[charLength];
  uint8_t scope_level;
  ScopedSymbolTable* scope = nullptr;
  ScopedSymbolTable* function_scope = nullptr; //used to find the formal parameters in the scope of a function symbol

  JsonVariant block;

  Symbol(const char * symbol_type, const char * name, const char * type = nullptr) {
    strcpy(this->symbol_type, symbol_type);
    strcpy(this->name, name);
    strcpy(this->type, (type == nullptr)?"":type);
    this->scope_level = 0;
  }

  ~Symbol() {
    MEMORY_ARTI("Heap Destruct Symbol %s (%u)\n", name, esp_get_free_heap_size());
  }

}; //Symbol

#define nrOfSymbolsPerScope 20 //add checks
#define nrOfChildScope 20 //add checks

class ScopedSymbolTable {
  private:
  public:

  Symbol* symbols[nrOfSymbolsPerScope];
  uint8_t symbolsIndex = 0;
  char scope_name[charLength];
  uint8_t scope_level;
  ScopedSymbolTable *enclosing_scope;
  ScopedSymbolTable *child_scopes[nrOfChildScope];
  uint8_t child_scopesIndex = 0;

  ScopedSymbolTable(const char * scope_name, int scope_level, ScopedSymbolTable *enclosing_scope = nullptr) {
    // DEBUG_ARTI("%s\n", "ScopedSymbolTable ", scope_name, scope_level);
    strcpy(this->scope_name, scope_name);
    this->scope_level = scope_level;
    this->enclosing_scope = enclosing_scope;
  }

  ~ScopedSymbolTable() {
    for (uint8_t i=0; i<child_scopesIndex; i++) {
      delete child_scopes[i]; child_scopes[i] = nullptr;
    }
    for (uint8_t i=0; i<symbolsIndex; i++) {
      delete symbols[i]; symbols[i] = nullptr;
    }
    MEMORY_ARTI("Heap Destruct ScopedSymbolTable %s (%u)\n", scope_name, esp_get_free_heap_size());
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
    this->symbols[symbolsIndex] = symbol;
    symbolsIndex++;
  }

  Symbol* lookup(const char * name, bool current_scope_only=false) {
    // this->log("Lookup: " + name + " " + this->scope_name);
    //  'symbol' is either an instance of the Symbol class or None;
    for (uint8_t i=0; i<symbolsIndex; i++) {
      // DEBUG_ARTI("%s\n", "  symbols ", i, symbols[i]->symbol_type, symbols[i]->name, symbols[i]->type, symbols[i]->scope_level);
      if (strcmp(symbols[i]->name, name) == 0) //replace with strcmp!!!
        return symbols[i];
    }

    if (current_scope_only)
      return nullptr;
    // # recursively go up the chain and lookup the name;
    if (this->enclosing_scope != nullptr)
      return this->enclosing_scope->lookup(name);
    
    return nullptr;
  } //lookup

}; //ScopedSymbolTable

class SemanticAnalyzer {
  private:
  public:
    ScopedSymbolTable *global_scope = nullptr;
    JsonObject definitionJson;
    JsonVariant parseTreeJson;

  SemanticAnalyzer(JsonObject definitionJson, JsonVariant parseTreeJson) {
    this->definitionJson = definitionJson;
    this->parseTreeJson = parseTreeJson;
  }

  ~SemanticAnalyzer() {
    MEMORY_ARTI("Destruct SemanticAnalyzer\n");
  }

  void analyze() {
    DEBUG_ARTI("\nAnalyzer\n");
    visit(parseTreeJson);
  }

  void visit(JsonVariant parseTree, const char * treeElement = nullptr, const char * symbol_name = nullptr, const char * token = nullptr, ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) {

    if (parseTree.is<JsonObject>()) {
      // DEBUG_ARTI("%s Visit object %s %s %s\n", spaces+50-depth, stringOrEmpty(treeElement), stringOrEmpty(symbol_name), stringOrEmpty(token)); //, parseTree.as<String>().c_str()

      for (JsonPair element : parseTree.as<JsonObject>()) {
        if (treeElement == nullptr || strcmp(treeElement, element.key().c_str()) == 0 ) { //in case there are more elements in the object and you want to visit only one
          const char * key = element.key().c_str();
          JsonVariant value = element.value();

          // DEBUG_ARTI("%s Visit element %s %s\n", spaces+50-depth, key, value.as<String>().c_str());
          bool visitCalledAlready = false;

          if (!definitionJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = definitionJson["SEMANTICS"][symbol_name];
            if (!expression.isNull())
            {
              // const char * expression_id = expression["id"];
              // MEMORY_ARTI("%s Heap %s < %u\n", spaces+50-depth, expression_id, esp_get_free_heap_size());
              if (expression["id"] == "Program") {
                const char * expression_name = expression["name"];
                const char * expression_block = expression["block"];
                const char * program_name = value[expression_name];
                this->global_scope = new ScopedSymbolTable(program_name, 1, nullptr); //current_scope

                DEBUG_ARTI("%s Program %s %u %u\n", spaces+50-depth, global_scope->scope_name, global_scope->scope_level, global_scope->symbolsIndex); 

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                else {
                  visit(value[expression_block], nullptr, symbol_name, token, this->global_scope, depth + 1);
                }

                for (uint8_t i=0; i<global_scope->symbolsIndex; i++) {
                  Symbol* symbol = global_scope->symbols[i];
                  DEBUG_ARTI("%s %u %s %s.%s %s %u\n", spaces+50-depth, i, symbol->symbol_type, global_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Function") {

                //find the function name (so we must know this is a function...)
                const char * expression_name = expression["name"];//.as<const char *>();
                const char * expression_block = expression["block"];
                const char * function_name = value[expression_name];
                Symbol* function_symbol = new Symbol(expression["id"], function_name);
                current_scope->insert(function_symbol);

                DEBUG_ARTI("%s Function %s.%s\n", spaces+50-depth, current_scope->scope_name, function_name);
                ScopedSymbolTable* function_scope = new ScopedSymbolTable(function_name, current_scope->scope_level + 1, current_scope);
                current_scope->child_scopes[current_scope->child_scopesIndex++] = function_scope;
                function_symbol->function_scope = function_scope;
                // DEBUG_ARTI("%s\n", "ASSIGNING ", function_symbol->name, " " , function_scope->scope_name);

                visit(value[expression["formals"].as<const char *>()], nullptr, symbol_name, token, function_scope, depth + 1);

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, function_name, expression_block); 
                else
                  visit(value[expression_block], nullptr, symbol_name, token, function_scope, depth + 1);

                // DEBUG_ARTI("%s\n", spaces+50-depth, "end function ", symbol_name, function_scope->scope_name, function_scope->scope_level, function_scope->symbolsIndex); 

                for (uint8_t i=0; i<function_scope->symbolsIndex; i++) {
                  Symbol* symbol = function_scope->symbols[i];
                  DEBUG_ARTI("%s %u %s %s.%s %s %u\n", spaces+50-depth, i, symbol->symbol_type, function_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Var" || expression["id"] == "Formal") {
                // DEBUG_ARTI("%s var (from array) %s %s %s\n", spaces+50-depth, current_scope->scope_name.c_str(), value.as<const char *>().c_str(), key.c_str());

                if (value.is<JsonArray>()) {
                  ERROR_ARTI("Var or formal should not be an array (programming error)\n");
                  // for (JsonObject newValue: value.as<JsonArray>()) {
                  // ...
                  // }
                }
                else {
                  const char * variable_name = value[expression["name"].as<const char *>()];
                  const char * expression_type = expression["type"];
                  char param_type[charLength]; 
                  if (!value[expression_type].isNull()) {
                    serializeJson(value[expression_type], param_type); //current_scope.lookup(param.type_node.value); //need string, lookup also used to find types...
                  }
                  else
                    strcpy(param_type, "notype");
                  Symbol* var_symbol = new Symbol(expression["id"], variable_name, param_type);
                  current_scope->insert(var_symbol);
                  DEBUG_ARTI("%s Var %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, variable_name, param_type);
                }
              }
              else if (expression["id"] == "Assign") {
                const char * expression_name = expression["name"];
                const char * variable_name = value[expression_name]["ID"];

                const char * expression_value = expression["value"];
                char param_type[charLength]; 
                strcpy(param_type, "notype");

                if (!value[expression_value].isNull()) { //value assignment
                  DEBUG_ARTI("%s Assign %s = {value (not of interest during analyze...)}\n", spaces+50-depth, variable_name);
                  visit(value, expression_value, symbol_name, token, current_scope, depth + 1);
                }
                else
                  ERROR_ARTI("%s Assign %s: no value in parseTree\n", spaces+50-depth, variable_name); 

                //if variable not already defined, then add
                Symbol* var_symbol = current_scope->lookup(variable_name); //lookup here and parent scopes
                if (var_symbol == nullptr) {
                  var_symbol = new Symbol(expression["id"], variable_name, param_type);
                  current_scope->insert(var_symbol);
                  DEBUG_ARTI("%s Var (assign) %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, variable_name, param_type);
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "VarRef") {
                const char * expression_name = expression["name"];
                const char * variable_name = value[expression_name];
                Symbol* var_symbol = current_scope->lookup(variable_name); //lookup here and parent scopes
                if (var_symbol == nullptr)
                  ERROR_ARTI("%s Variable %s not found in scope %s\n", spaces+50-depth, variable_name, current_scope->scope_name); 
                visitCalledAlready = true;
              }
              else if (expression["id"] == "Call") {
                const char * function_name = value[expression["name"].as<const char *>()];

                //check if external function
                bool found = false;
                for (JsonPair externalsPair: definitionJson["EXTERNALS"].as<JsonObject>()) {
                  const char * key = externalsPair.key().c_str();
                  JsonVariant value = externalsPair.value();
                  if (strcmp(function_name, key) == 0) {
                    visit(value[expression["actuals"].as<const char *>()], nullptr, symbol_name, token, current_scope, depth + 1);
                    found = true;
                  }
                }

                if (!found) {
                  Symbol* function_symbol = current_scope->lookup(function_name); //lookup here and parent scopes
                  if (function_symbol != nullptr) {
                    visit(value[expression["actuals"].as<const char *>()], nullptr, symbol_name, token, current_scope, depth + 1);

                    visit(function_symbol->block, nullptr, symbol_name, token, current_scope, depth + 1);
                  } //function_symbol != nullptr
                  else {
                    ERROR_ARTI("%s Function %s not found in scope %s\n", spaces+50-depth, function_name, current_scope->scope_name); 
                  }
                } //external functions
                visitCalledAlready = true;
              } // if expression["id"]

              // MEMORY_ARTI("%s Heap %s > %u\n", spaces+50-depth, expression_id, esp_get_free_heap_size());
            } // if expression not null

          } // is symbol_name

          if (!this->definitionJson["TOKENS"][key].isNull()) {
            token = key;
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Token ", token);
          }
          // DEBUG_ARTI("%s Object %s %u\n", spaces+50-depth, key, value, visitCalledAlready);

          if (!visitCalledAlready)
            visit(value, nullptr, symbol_name, token, current_scope, depth + 1);

        } // key values
      }
    }
    else { //not object
      if (parseTree.is<JsonArray>()) {
        // DEBUG_ARTI("%s Visit array %s %s %s\n", spaces+50-depth, stringOrEmpty(treeElement), stringOrEmpty(symbol_name), stringOrEmpty(token)); //, parseTree.as<String>().c_str()
        for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
          // DEBUG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
          visit(newParseTree, nullptr, symbol_name, token, current_scope, depth + 1);
        }
      }
      else { //not array
        // string element = parseTree;
        // DEBUG_ARTI("%s value notnot %s\n", spaces+50-depth, parseTree.as<String>().c_str());
        // if (definitionJson["SEMANTICS"][element])
      }
    }
  } //visit

}; //SemanticAnalyzer

struct key_value
{
    char key[charLength];
    char value[charLength];
};

#define nrOfVariables 100

class ActivationRecord {
  private:
  public:
    char name[charLength];
    char type[charLength];
    int nesting_level;
    struct key_value *members;
    uint8_t membersCounter = 0;
    char lastSet[charLength];

    ActivationRecord(const char * name, const char * type, int nesting_level) {
        strcpy(this->name, name);
        strcpy(this->type, type);
        this->nesting_level = nesting_level;

        members = (struct key_value *)malloc(sizeof(struct key_value) * nrOfVariables);
    }

    ~ActivationRecord() {
      DEBUG_ARTI("Destruct activation record %s\n", name);
      free(members);
    }

    void set(const char * key, const char * value) {

      for (uint8_t i = 0; i<membersCounter; i++) {
        if (strcmp(members[i].key, key) == 0) {
          strcpy(members[i].value, value);
          strcpy(lastSet, key);
          // DEBUG_ARTI("Set %s %s\n", key, value);
          return;
        }
      }

      if (membersCounter < nrOfVariables) {
          strcpy(members[membersCounter].key, key);
          strcpy(members[membersCounter].value, value);
        strcpy(lastSet, key);
        membersCounter++;
        // DEBUG_ARTI("Set %s %s\n", key, value);
      }
      else
        ERROR_ARTI("ActivationRecord no room for new vars\n");
    }

    const char * get(const char * key) {
      for (uint8_t i = 0; i<membersCounter; i++) {
        // DEBUG_ARTI("Get %s %s %s", key, members[i].key, members[i].value);
        if (strcmp(members[i].key, key) == 0) {
          return members[i].value;
        }
      }
      return "empty";
    }
}; //ActivationRecord

#define nrOfRecords 20

class CallStack {
public:
  ActivationRecord* records[nrOfRecords];
  uint8_t recordsCounter = 0;

  CallStack() {
  }

  ~CallStack() {
    DEBUG_ARTI("Destruct callstack\n");
  }


    void push(ActivationRecord* ar) {
      if (recordsCounter < nrOfRecords) {
        // DEBUG_ARTI("%s\n", "Push ", ar->name);
        this->records[recordsCounter++] = ar;
      }
      else
        ERROR_ARTI("no space left in callstack\n");
    }

    ActivationRecord* pop() {
      if (recordsCounter > 0) {
        // DEBUG_ARTI("%s\n", "Pop ", this->peek()->name);
        return this->records[recordsCounter--];
      }
      else {
        ERROR_ARTI("no ar left on callstack\n");
        return nullptr;
      }
    }

    ActivationRecord* peek() {
        return this->records[recordsCounter-1];
    }
}; //CallStack

class ValueStack {
private:
public:
  char stack[arrayLength][charLength];
  uint8_t stack_index = 0;

  ValueStack() {
  }

  ~ValueStack() {
    DEBUG_ARTI("Destruct valueStack\n");
  }

  void push(const char * key, const char * value) {
    if (stack_index < arrayLength && value != nullptr) {
      // DEBUG_ARTI("calc push %s %s\n", key, value);
      strcpy(stack[stack_index++], value);
    }
    else {
      if (value == nullptr) {
        strcpy(stack[stack_index++], "empty");
        // ERROR_ARTI("Push value of %s is null\n", key);
      }
      else
        ERROR_ARTI("Push value stack full %u of %u\n", stack_index, arrayLength);
    }
  }

  const char * peek() {
    // DEBUG_ARTI("Calc Peek %s\n", stack[stack_index-1]);
    return stack[stack_index-1];
  }

  const char * pop() {
    if (stack_index>0) {
      stack_index--;
      return stack[stack_index];
    }
    else {
      ERROR_ARTI("Pop value stack empty\n");
    // DEBUG_ARTI("Calc Pop %s\n", stack[stack_index]);
      return "novalue";
    }
  }

}; //ValueStack

class Interpreter {
  private:
  CallStack *callStack;
  SemanticAnalyzer *analyzer;
  ValueStack *valueStack;
  const char * function_name;

  public:

  Interpreter(SemanticAnalyzer *analyzer, CallStack * callStack) {
    this->analyzer = analyzer;
    this->callStack = callStack;

    valueStack = new ValueStack();
  }

  ~Interpreter() {
    delete valueStack; valueStack = nullptr;
    DEBUG_ARTI("Destruct Interpreter\n");
  }

  void interpret(const char * function_name = nullptr) {
    this->function_name = function_name;

    if (function_name == nullptr) {
      if (this->analyzer != nullptr) { //due to undefined functions??? wip

        DEBUG_ARTI("\ninterpret %s %u %u\n", analyzer->global_scope->scope_name, analyzer->global_scope->scope_level, analyzer->global_scope->symbolsIndex); 
        for (uint8_t i=0; i<analyzer->global_scope->symbolsIndex; i++) {
          Symbol* symbol = analyzer->global_scope->symbols[i];
          DEBUG_ARTI("scope %s %s.%s %s %u\n", symbol->symbol_type, analyzer->global_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
        }

        visit(analyzer->parseTreeJson);
      }
      else
      {
        ERROR_ARTI("\nInterpret global scope is nullptr\n");
      }
    }
    else { //Call only function_name (no parameters)
      uint8_t depth = 8;
                Symbol* function_symbol = this->analyzer->global_scope->lookup(function_name);

                if (function_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                  // MEMORY_ARTI("%s Heap Call %s < %u\n", spaces+50-depth, function_name, esp_get_free_heap_size());

                  ActivationRecord* ar = new ActivationRecord(function_name, "Function", function_symbol->scope_level + 1);

                  DEBUG_ARTI("%s %s %s (%d)\n", spaces+50-depth, "Call", function_name, this->callStack->recordsCounter);

                    this->callStack->push(ar);

                    //find block of function... lookup function?
                    //visit block of function
                    // DEBUG_ARTI("%s function block %s\n", spaces+50-depth, function_symbol->block.as<String>().c_str());

                    visit(function_symbol->block, nullptr, nullptr, nullptr, this->analyzer->global_scope, depth + 1);

                    this->callStack->pop();

                    char cr[charLength] = "CallResult tbd of ";
                    strcat(cr, function_name);
                    valueStack->push("Call", cr);

                    delete ar; ar = nullptr;

                } //function_symbol != nullptr
                else {
                  DEBUG_ARTI("%s %s not found %s\n", spaces+50-depth, "Call", function_name);
                }

    }

  } //interpret

            // ["PLUS2", "factor"],
          // [ "MINUS2", "factor"],

  void visit(JsonVariant parseTree, const char * treeElement = nullptr, const char * symbol_name = nullptr, const char * token = nullptr, ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) {

    // DEBUG_ARTI("%s Visit %s %s %s\n", spaces+50-depth, stringOrEmpty(treeElement), stringOrEmpty(symbol_name), stringOrEmpty(token)); //, parseTree.as<String>().c_str()

    if (parseTree.is<JsonObject>()) {
      for (JsonPair element : parseTree.as<JsonObject>()) {
        if (treeElement == nullptr || strcmp(treeElement, element.key().c_str()) == 0 ) {
          const char * key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!this->analyzer->definitionJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = this->analyzer->definitionJson["SEMANTICS"][symbol_name];
            if (!expression.isNull())
            {
              // DEBUG_ARTI("%s Symbol %s %s\n", spaces+50-depth, symbol_name,  expression.as<std::string>().c_str());

              // const char * expression_id = expression["id"];
              // MEMORY_ARTI("%s Heap %s < %u\n", spaces+50-depth, expression_id, esp_get_free_heap_size());
              if (expression["id"] == "Program") {
                const char * expression_name = expression["name"];
                const char * expression_block = expression["block"];
                DEBUG_ARTI("%s program name %s\n", spaces+50-depth, expression_name);
                const char * program_name = value[expression_name];

                if (!value[expression_name].isNull()) {
                  ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM", 1);
                  DEBUG_ARTI("%s %s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, program_name);

                  this->callStack->push(ar);
                  if (value[expression_block].isNull())
                    ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                  else
                    visit(value[expression_block], nullptr, symbol_name, token, analyzer->global_scope, depth + 1);

                  // do not delete main stack and program ar as used in subsequent calls 
                  // this->callStack->pop();
                  // delete ar; ar = nullptr;
                }
                else
                  ERROR_ARTI("program name null\n");

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Function") {
                const char * expression_block = expression["block"];
                const char * function_name = value[expression["name"].as<const char *>()];
                Symbol* function_symbol = current_scope->lookup(function_name);
                DEBUG_ARTI("%s Save block of %s\n", spaces+50-depth, function_name);
                function_symbol->block = value[expression_block];
                visitCalledAlready = true;
              }
              else if (expression["id"] == "Call") {
                const char * function_name = value[expression["name"].as<const char *>()];

                //check if external function
                bool found = false;
                for (JsonPair externalsPair: analyzer->definitionJson["EXTERNALS"].as<JsonObject>()) {
                  if (strcmp(function_name, externalsPair.key().c_str()) == 0) {
                    uint8_t oldIndex = valueStack->stack_index;
                    uint8_t lastIndex = valueStack->stack_index;

                    visit(value[expression["actuals"].as<const char *>()], nullptr, symbol_name, token, current_scope, depth + 1);

                    bool returnFound = false;

                    #ifdef WLED_H
                      // MEMORY_ARTI("%s %s %u %u (%u)\n", spaces+50-depth, function_name, par1, par2, esp_get_free_heap_size());
                      if (strcmp(function_name, "setPixelColor") == 0)
                        strip.setPixelColor(atoi(this->valueStack->stack[lastIndex]), strip.color_wheel(atoi(this->valueStack->stack[lastIndex+1])));
                    #endif

                    #ifndef WLED_H
                      printf("%s Call %s(", spaces+50-depth, function_name);
                    #endif
                    char sep[3] = "";
                    for (JsonPair actualsPair: externalsPair.value().as<JsonObject>()) {
                      const char * name = actualsPair.key().c_str();
                      // JsonVariant type = actualsPair.value();
                      if (strcmp(name, "return") != 0) {
                        #ifndef WLED_H
                          printf("%s%s", sep, this->valueStack->stack[lastIndex++]);
                          strcpy(sep, ", ");
                        #endif
                      }
                      else
                        returnFound = true;
                    }
                    #ifndef WLED_H
                      printf(")\n");
                    #endif

                    valueStack->stack_index = oldIndex;

                    if (returnFound) {
                      char cr[charLength] = "CallResult tbd of ";
                      strcat(cr, function_name);
                      valueStack->push("Call", cr);
                    }

                    found = true;
                  }
                }

                if (!found) {
                  Symbol* function_symbol = current_scope->lookup(function_name);

                  if (function_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                    ActivationRecord* ar = new ActivationRecord(function_name, "Function", function_symbol->scope_level + 1);

                    DEBUG_ARTI("%s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), function_name);

                    uint8_t oldIndex = valueStack->stack_index;
                    uint8_t lastIndex = valueStack->stack_index;

                    visit(value[expression["actuals"].as<const char *>()], nullptr, symbol_name, token, current_scope, depth + 1);

                    for (uint8_t i=0; i<function_symbol->function_scope->symbolsIndex; i++) { //backwards because popped in reversed order
                      if (strcmp(function_symbol->function_scope->symbols[i]->symbol_type, "Formal") == 0) { //select formal parameters
                        const char * result = this->valueStack->stack[lastIndex++];
                        ar->set(function_symbol->function_scope->symbols[i]->name, result);
                        DEBUG_ARTI("%s Actual %s.%s = %s (pop)\n", spaces+50-depth, function_name, function_symbol->function_scope->symbols[i]->name, result);
                      }
                    }

                    valueStack->stack_index = oldIndex;

                    this->callStack->push(ar);

                    //find block of function... lookup function?
                    //visit block of function
                    // DEBUG_ARTI("%s function block %s\n", spaces+50-depth, function_symbol->block.as<String>().c_str());

                    visit(function_symbol->block, nullptr, symbol_name, token, function_symbol->function_scope, depth + 1);

                    this->callStack->pop();

                    delete ar; ar =  nullptr;

                    char cr[charLength] = "CallResult tbd of ";
                    strcat(cr, function_name);
                    valueStack->push("Call", cr);

                  } //function_symbol != nullptr
                  else {
                    DEBUG_ARTI("%s %s not found %s\n", spaces+50-depth, expression["id"].as<const char *>(), function_name);
                  }
                } //external functions

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Assign") {
                const char * expression_name = expression["name"];
                const char * expression_indices = expression["indices"]; //array indices
                const char * expression_value = expression["value"];
                const char * variable_name = value[expression_name]["ID"];

                if (!value[expression_value].isNull()) { //value assignment
                  visit(value, expression["value"], symbol_name, token, current_scope, depth + 1); //value pushed

                  char indices[charLength];
                  strcpy(indices, "");
                  if (!value[expression_indices].isNull()) {
                    strcat(indices, "[");
                    uint8_t oldIndex = valueStack->stack_index;
                    visit(value, expression["indices"], symbol_name, token, current_scope, depth + 1); //value pushed
                    char sep[3] = "";
                    for (uint8_t i = oldIndex; i< valueStack->stack_index; i++) {
                      strcat(indices, sep);
                      strcat(indices, this->valueStack->stack[i]);
                      strcpy(sep, ",");
                    }
                    valueStack->stack_index = oldIndex;
                    strcat(indices, "]");
                  }

                  //find variable
                  Symbol* function_symbol = current_scope->lookup(variable_name);
                  ActivationRecord* ar;

                  //check already defined in this scope


                  if (function_symbol != nullptr) { //var already exist
                    //calculate the index in the call stack to find the right ar
                    uint8_t index = this->callStack->recordsCounter - 1 - (this->callStack->peek()->nesting_level - function_symbol->scope_level);
                    //  DEBUG_ARTI("%s %s %s.%s = %s (push) %s %d-%d = %d (%d)\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, variable_name, varValue, function_symbol->name, this->callStack->peek()->nesting_level,function_symbol->scope_level, index,  this->callStack->recordsCounter); //key is variable_declaration name is ID
                    ar = this->callStack->records[index];
                  }
                  else { //var created here
                    ar = this->callStack->peek();
                  }
                  ar->set(variable_name, this->valueStack->pop());

                  DEBUG_ARTI("%s %s.%s%s := %s (pop)\n", spaces+50-depth, ar->name, variable_name, indices, ar->get(variable_name));
                }
                else {
                  ERROR_ARTI("%s Assign %s has no value\n", spaces+50-depth, expression_name);
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Exprs" || expression["id"] == "Terms") {
                // DEBUG_ARTI("%s %s tovisit %s\n", spaces+50-depth, expression["id"].as<const char *>(), value.as<String>().c_str());
                if (value.is<JsonArray>()) {
                  JsonArray valueArray = value.as<JsonArray>();

                  if (valueArray.size() >= 1) // visit first symbol 
                    visit(valueArray[0], nullptr, symbol_name, token, current_scope, depth + 1); //pushes result

                  // assuming expression contains 1 operand and 2 operators
                  if (valueArray.size() >= 3) { // add operator and another symbol
                    char operatorx[charLength];
                    serializeJson(valueArray[1], operatorx);
                    // DEBUG_ARTI("%s %s operator %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), operatorx, value.as<String>().c_str());
                    visit(valueArray[2], nullptr, symbol_name, token, current_scope, depth + 1); //pushes result

                    // DEBUG_ARTI("%s operatorx %s\n", spaces+50-depth, operatorx);
                    if (strstr(operatorx, "PLUS")) {
                      const char * right = valueStack->pop();
                      const char * left = valueStack->pop();
                      int evaluation = atoi(left) + atoi(right);
                      DEBUG_ARTI("%s %s + %s = %d\n", spaces+50-depth, left, right, evaluation);

                      char evalChar[charLength];
                      itoa(evaluation, evalChar, 10);

                      valueStack->push(operatorx, evalChar);
                    }
                    else if (strstr(operatorx, "MUL")) {
                      const char * right = valueStack->pop();
                      const char * left = valueStack->pop();
                      int evaluation = atoi(left) * atoi(right);
                      DEBUG_ARTI("%s %s * %s = %d\n", spaces+50-depth, left, right, evaluation);

                      char evalChar[charLength];
                      itoa(evaluation, evalChar, 10);

                      valueStack->push(operatorx, evalChar);
                    }
                    else if (strstr(operatorx, "SMALLER")) {
                      const char * right = valueStack->pop();
                      const char * left = valueStack->pop();
                      int evaluation = atoi(left) < atoi(right);
                      DEBUG_ARTI("%s %s < %s = %d\n", spaces+50-depth, left, right, evaluation);

                      char evalChar[charLength];
                      itoa(evaluation, evalChar, 10);

                      valueStack->push(operatorx, evalChar);
                    }
                    else if (strstr(operatorx, "GREATER")) {
                      const char * right = valueStack->pop();
                      const char * left = valueStack->pop();
                      int evaluation = atoi(left) > atoi(right);
                      DEBUG_ARTI("%s %s > %s = %d\n", spaces+50-depth, left, right, evaluation);

                      char evalChar[charLength];
                      itoa(evaluation, evalChar, 10);

                      valueStack->push(operatorx, evalChar);
                    }
                    else {
                      const char * right = valueStack->pop();
                      const char * left = valueStack->pop();
                      ERROR_ARTI("%s %s ? %s = ? (%s unknown)\n", spaces+50-depth, left, right, operatorx);
                    }
                  }

                  if (valueArray.size() != 1 && valueArray.size() != 3)
                    ERROR_ARTI("%s %s array not right size ?? (%u) %s\n", spaces+50-depth, expression["id"].as<const char *>(), valueArray.size(), key); //, value.as<String>().c_str()
                }
                else
                  ERROR_ARTI("%s %s not array?? %s\n", spaces+50-depth, key, expression["id"].as<const char *>()); //, value.as<String>().c_str()

                visitCalledAlready = true;
              }
              else if (expression["id"] == "VarRef") {
                const char * expression_name = expression["name"]; //ID (key="varref")
                const char * variable_name = value[expression_name];

                ActivationRecord* ar = this->callStack->peek();
                const char * varValue = ar->get(variable_name);

                if (strcmp(varValue, "empty") == 0) {
                  //find the scope level of the variable
                  Symbol* function_symbol = current_scope->lookup(variable_name);
                  //calculate the index in the call stack to find the right ar
                  uint8_t index = this->callStack->recordsCounter - 1 - (this->callStack->peek()->nesting_level - function_symbol->scope_level);
                  //  DEBUG_ARTI("%s %s %s.%s = %s (push) %s %d-%d = %d (%d)\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, variable_name, varValue, function_symbol->name, this->callStack->peek()->nesting_level,function_symbol->scope_level, index,  this->callStack->recordsCounter); //key is variable_declaration name is ID
                  ar = this->callStack->records[index];
                }

                if (ar != nullptr) {
                  varValue = ar->get(variable_name);

                  #ifndef ESP32 //for some weird reason this causes a crash on esp32
                    DEBUG_ARTI("%s %s %s.%s = %s (push)\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, variable_name, varValue); //key is variable_declaration name is ID
                  #endif
                  valueStack->push(variable_name, varValue);
                }
                else {
                  ERROR_ARTI("%s Var %s unknown \n", spaces+50-depth, variable_name);
                  valueStack->push(variable_name, "unknown");
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "For") {
                DEBUG_ARTI("%s For\n", spaces+50-depth);

                const char * expression_block = expression["block"];

                DEBUG_ARTI("%s from\n", spaces+50-depth);
                visit(value, expression["from"], symbol_name, token, current_scope, depth + 1); //creates the assignment
                ActivationRecord* ar = this->callStack->peek();
                const char * fromVarName = ar->lastSet;

                bool continuex = true;
                uint16_t counter = 0;
                while (continuex && counter < 1000) { //to avoid endless loops
                  DEBUG_ARTI("%s iteration\n", spaces+50-depth);

                  DEBUG_ARTI("%s check to condition\n", spaces+50-depth);
                  visit(value, expression["to"], symbol_name, token, current_scope, depth + 1); //pushes result of to

                  const char * toResult = valueStack->pop();

                  if (strcmp(toResult, "1") == 0) { //toResult is true
                    DEBUG_ARTI("%s 1 => run block\n", spaces+50-depth);
                    visit(value[expression_block], nullptr, symbol_name, token, current_scope, depth + 1);

                    DEBUG_ARTI("%s assign next value\n", spaces+50-depth);
                    visit(value[expression["increment"].as<const char *>()], nullptr, symbol_name, token, current_scope, depth + 1); //pushes increment result
                    // MEMORY_ARTI("%s Iteration %u %u\n", spaces+50-depth, counter, esp_get_free_heap_size());
                  }
                  else {
                    if (strcmp(toResult, "0") == 0) { //toResult is false
                      DEBUG_ARTI("%s 0 => end of For\n", spaces+50-depth);
                      continuex = false;
                    }
                    else { // toResult is a value (e.g. in pascal)
                      //get the variable from assignment
                      const char * varValue = ar->get(fromVarName);

                      int evaluation = atoi(varValue) <= atoi(toResult);
                      DEBUG_ARTI("%s %s.%s %s <= %s = %d\n", spaces+50-depth, ar->name, fromVarName, varValue, toResult, evaluation);

                      if (evaluation == 1) {
                        DEBUG_ARTI("%s 1 => run block\n", spaces+50-depth);
                        visit(value[expression_block], nullptr, symbol_name, token, current_scope, depth + 1);

                        //increment
                        char evalChar[charLength];
                        itoa(atoi(varValue) + 1, evalChar, 10);
                        ar->set(fromVarName, evalChar);
                      }
                      else {
                        DEBUG_ARTI("%s 0 => end of For\n", spaces+50-depth);
                        continuex = false;
                      }

                    }
                  }

                  counter++;
                };

                if (continuex)
                  ERROR_ARTI("%s too many iterations in for loop %u\n", spaces+50-depth, counter);

                visitCalledAlready = true;
              }  //if expression["id"]
              // MEMORY_ARTI("%s Heap %s > %u\n", spaces+50-depth, expression_id, esp_get_free_heap_size());
            } //if expression not null

         } // is key is symbol_name

          // DEBUG_ARTI("%s\n", spaces+50-depth, "Object ", key, value);
          // if (key == "INTEGER_CONST" || key == "PLUS" || key == "MUL" || key == "LPAREN"  || key == "RPAREN" ) {
          if (strcmp(key, "INTEGER_CONST") == 0) {// || value == "+" || value == "*") || value == "("  || value == ")" ) {
            valueStack->push("INTEGER_CONST", value);
            #ifndef ESP32  //for some weird reason this causes a crash on esp32
              DEBUG_ARTI("%s Push %s %s\n", spaces+50-depth, key, value.as<const char *>());
            #endif
            visitCalledAlready = true;
          }

          if (!analyzer->definitionJson["TOKENS"][key].isNull()) { //if key is token
            token = key;
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Token ", token);
          }
          if (!visitCalledAlready)
            visit(value, nullptr, symbol_name, token, current_scope, depth + 1);
        }
      } // for (JsonPair
    }
    else { //not object
      if (parseTree.is<JsonArray>()) {
        for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
          // DEBUG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
          visit(newParseTree, nullptr, symbol_name, token, current_scope, depth + 1);
        }
      }
      else { //not array
        // const char * element = parseTree.as<const char *>();
        // DEBUG_ARTI("%s\n", spaces+50-depth, "not array not object but element ", element);
      }
    }
  } //visit

};

#define programTextSize 1000

class ARTI {
private:
  Lexer *lexer;
  Parser *parser;
  SemanticAnalyzer *semanticAnalyzer = nullptr;
  Interpreter *interpreter;
  #ifdef ESP32
    char * programText;
    File definitionFile;
    File programFile;
    File parseTreeFile;
  #else
    char * programText;
    // char programText[programTextSize];
    std::fstream definitionFile;
    std::fstream programFile;
    std::fstream parseTreeFile;
  #endif
  uint16_t programFileSize;

  DynamicJsonDocument *definitionJson;
  DynamicJsonDocument *parseTreeJson;
  CallStack *callStack;
public:
  ARTI() {

    // char byte = charFromProgramFile();
    // while (byte != -1) {
    //   programText += byte;
    //   DEBUG_ARTI("%c", byte);
    //   byte = charFromProgramFile();
    // }

  }

  ~ARTI() {
    DEBUG_ARTI("Destruct ARTI\n");
  }

  void openFileAndParse(const char *definitionName, const char *programName) {
    MEMORY_ARTI("Heap OFP < %u (%lums) %s %s\n", esp_get_free_heap_size(), millis(), definitionName, programName);
      char parseOrLoad[charLength] = "Parse";

      //open logFile
      #ifndef ARTI_DEBUGORLOG
        char logFileName[charLength];
        #ifdef ESP32
          strcpy(logFileName, "/");
        #endif
        strcpy(logFileName, programName);
        strcat(logFileName, ".log");

        #ifdef ESP32
          #ifdef WLED_H
            logFile = WLED_FS.open(logFileName,"w");
          #else
            logFile = FS.open(logFileName,"w");
          #endif
        #else
          logFile = fopen (logFileName,"w");
        #endif
      #endif

      #ifdef ESP32
        #ifdef WLED_H
          definitionFile = WLED_FS.open(definitionName, "r");
        #else
          definitionFile = FS.open(definitionName, "r");
        #endif
      #else
        definitionFile.open(definitionName, std::ios::in);
      #endif
        // DEBUG_ARTI("def size %lu\n", definitionFile.tellg());
      MEMORY_ARTI("Heap open definition file > %u\n", esp_get_free_heap_size());
      if (!definitionFile)
        ERROR_ARTI("Definition file %s not found\n", definitionName);
      else
      {
        //open definitionFile
        definitionJson = new DynamicJsonDocument(12192);
        // mandatory tokens:
        //  "ID": "ID",
        //  "INTEGER_CONST": "INTEGER_CONST",
        //  "REAL_CONST": "REAL_CONST",


        DeserializationError err = deserializeJson(*definitionJson, definitionFile);
        if (err) {
          ERROR_ARTI("deserializeJson() of definition failed with code %s\n", err.c_str());
        }
        definitionFile.close();

        #ifdef ESP32
          #ifdef WLED_H
            programFile = WLED_FS.open(programName, "r");
          #else
            programFile = FS.open(programName, "r");
          #endif
        #else
          programFile.open(programName, std::ios::in);
        #endif
        if (!programFile)
          ERROR_ARTI("Program file %s not found\n", programName);
        else
        {
          //open programFile
          #ifdef ESP32
            programFileSize = programFile.size();
            programText = (char *)malloc(programFileSize+1);
            programFile.read((byte *)programText, programFileSize);
            programText[programFileSize] = '\0';
          #else
            programText = (char *)malloc(programFile.gcount()+1);
            programFile.read(programText, programTextSize); //sizeof programText
            programText[programFile.gcount()] = '\0';
            programFileSize = strlen(programText);
          #endif
          programFile.close();

          //open parseTreeFile
          char parseTreeName[charLength];
          strcpy(parseTreeName, programName);
          // if (strcmp(parseOrLoad, "Parse") == 0 )
          //   strcpy(parseTreeName, "Gen");
          strcat(parseTreeName, ".json");
          #ifdef ESP32
            #ifdef WLED_H
              parseTreeFile = WLED_FS.open(parseTreeName, (strcmp(parseOrLoad, "Parse")==0)?"w":"r");
            #else
              parseTreeFile = FS.open(parseTreeName, (strcmp(parseOrLoad, "Parse")==0)?"w":"r");
            #endif
            parseTreeJson = new DynamicJsonDocument(strlen(programText) * 25); //why less memory on esp32???
          #else
            parseTreeFile.open(parseTreeName, std::ios::out);
            parseTreeJson = new DynamicJsonDocument(strlen(programText) * 50);
          #endif

          MEMORY_ARTI("Heap DynamicJsonDocuments > %u (%lums)\n", esp_get_free_heap_size(), millis());

          //parse
          if (strcmp(parseOrLoad, "Parse") == 0) {

            lexer = new Lexer(this->programText, definitionJson->as<JsonObject>());
            parser = new Parser(this->lexer, parseTreeJson->as<JsonVariant>());
            parser->parse();

            delete lexer; lexer =  nullptr;
            delete parser; parser =  nullptr;

            //write parseTree
            serializeJsonPretty(*parseTreeJson,  parseTreeFile);
          }
          else
          {
            //read parseTree
            DeserializationError err = deserializeJson(*parseTreeJson, parseTreeFile);
            if (err) {
              ERROR_ARTI("deserializeJson() of parseTree failed with code %s\n", err.c_str());
            }
          }
          #ifdef ESP32
            free(programText);
          #endif

          parseTreeFile.close();

          MEMORY_ARTI("Heap parse > %u (%lums)\n", esp_get_free_heap_size(), millis());

        } //programFile
      } //definitionFilee
  } // openFileAndParse

  void analyze(const char * function_name = nullptr) {
    MEMORY_ARTI("Heap analyze < %u\n", esp_get_free_heap_size());

    //analyze
    semanticAnalyzer = new SemanticAnalyzer(definitionJson->as<JsonObject>(), parseTreeJson->as<JsonVariant>());
    semanticAnalyzer->analyze();

    callStack = new CallStack();

    MEMORY_ARTI("Heap analyze > %u (%lums)\n", esp_get_free_heap_size(), millis());
  }

  void interpret(const char * function_name = nullptr) {
    interpreter = new Interpreter(semanticAnalyzer, callStack);
    interpreter->interpret(function_name);

    delete interpreter; interpreter = nullptr;
  }

  void close() {
    delete semanticAnalyzer->global_scope; semanticAnalyzer->global_scope = nullptr;
    delete semanticAnalyzer; semanticAnalyzer = nullptr;
    delete callStack; callStack = nullptr;

    if (definitionJson != nullptr) {
      DEBUG_ARTI("def mem %u of %u %u %u\n", definitionJson->memoryUsage(), definitionJson->capacity(), definitionJson->memoryPool().capacity(), definitionJson->size());
      definitionJson->garbageCollect();
      DEBUG_ARTI("def mem %u of %u %u %u\n", definitionJson->memoryUsage(), definitionJson->capacity(), definitionJson->memoryPool().capacity(), definitionJson->size());
    }
    if (parseTreeJson != nullptr) {
      DEBUG_ARTI("par mem %u of %u %u %u\n", parseTreeJson->memoryUsage(), parseTreeJson->capacity(), parseTreeJson->memoryPool().capacity(), parseTreeJson->size());
      DEBUG_ARTI("prog size %u factor %u\n", programFileSize, parseTreeJson->memoryUsage() / programFileSize);
      parseTreeJson->garbageCollect();
      DEBUG_ARTI("par mem %u of %u %u %u\n", parseTreeJson->memoryUsage(), parseTreeJson->capacity(), parseTreeJson->memoryPool().capacity(), parseTreeJson->size());
      //199 -> 6905 (34.69)
      //469 -> 15923 (33.95)
    }
    #ifndef ARTI_DEBUGORLOG
      #ifdef ESP32
        logFile.close();
      #else
        fclose (logFile); //should not be closed as still streaming...
      #endif
    #endif

    MEMORY_ARTI("Heap close > %u (%lums)\n", esp_get_free_heap_size(), millis());
  }

}; //ARTI

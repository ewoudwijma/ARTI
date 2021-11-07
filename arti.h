/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti.h
   @version 0.0.5
   @date    20211107
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
   @remarks
          - #define ARDUINOJSON_DEFAULT_NESTING_LIMIT 100 //set this in ArduinoJson!!!, currently not necessary...
          - IF UPDATING THIS FILE IN THE WLED REPO, SEND A PULL REQUEST TO https://github.com/ewoudwijma/ARTI AS WELL!!!
   @later
          - Code improvememt
            - expression[""]  variables
            - remove std::string (now only in logging)
            - move code from interpreter to analyzer to speed up interpreting
            - initialize * to nullptr
            - See for some weird reason this causes a crash on esp32
            - remove error classes
            - Code review (memory leaks, wled: select effect multiple times causes crash)
            - Embedded: run all demos at once (not working well for some reason)
            - tried SEGENV.aux0 but that looks to be overwritten!!! (dangling pointer???)
          - Definition improvements
            - add comments
            - support string (e.g. for print)
            - add integer and real stacks
            - Add ++, --, +=, -=
            - print every x seconds (to use it in loops. e.g. to show free memory)
          - WLED improvements
            - WLED: *arti in SEGENV.data
            - wled plugin include setup and loop...
            - upload files in wled ui (instead of /edit)
            - add sliders
   @done
          - Add real
          - Add div
          - WLED: ledCount to SEGLEN (now _virtualSegmentLength not found...)
          - add second, minute, hour
   @progress
          - WLED: add more functions
          - Shrink unused parseTree levels causes crash on arduino (now if false)
   @todo
          - SetPixelColor without colorwheel
          - if function not found in arti_wled_pluging then proper error handling
          - extend errorOccurred and add warnings (continue) next to errors (stop). Include stack full/empty
          - check on return values
          - reserved words (ext functions and variables cannot be used as variables)
          - why do functions starting with color_ crash the parser and color not???
          - why is sa = sampleAvg * 256 / (256 - speed) not working (just calculates 256 - speed)
          - Error: parseTree should be array or object in pas1.pas
          - Allow deeper nesting in wled files

  */

#pragma once

#ifdef ESP32 //ESP32 is set in wled context
  #include "wled.h" //setting WLED_H, see below
#endif

// For testing porposes, definitions should not only run on Arduino but also on Windows etc. 
// Because compiling on arduino takes seriously more time than on Windows.
// The plugin.h files replace native arduino calls by windows simulated calls (e.g. setPixelColor will become printf)
#define ARTI_WLED 1
#define ARTI_PAS 2
#define ARTI_ARDUINO 3
#define ARTI_EMBEDDED 4
#define ARTI_SERIAL 5
#define ARTI_FILE 6

#define ARTI_DEFINITION ARTI_WLED // currently also pas runs fine on this as it has no own functions and variables
// #define ARTI_DEFINITION ARTI_PAS
#ifdef WLED_H // (set in wled.h) small trick to set ARDUINO using WLED context, in other contexts, set manually or find another trick
  #define ARTI_PLATFORM ARTI_ARDUINO // else on Windows/Linux/Mac...
#endif

#if ARTI_PLATFORM == ARTI_ARDUINO
  #include "wled.h"
  #include "src/dependencies/json/ArduinoJson-v6.h"
  #define ARTI_OUTPUT ARTI_SERIAL //print output to serial
  #define ARTI_ERROR 1 //shows lexer, parser, analyzer and interpreter errors
  // #define ARTI_DEBUG 1
  // #define ARTI_ANDBG 1
  // #define ARTI_RUNLOG 1 //if set on arduino this will create massive amounts of output (as ran in a loop)
  #define ARTI_MEMORY 1 //to do analyses of memory usage, trace memoryleaks
  #define ARTI_PRINT 1 //will show the printf calls
#else //embedded
  #include "dependencies/ArduinoJson-recent.h"
  #define ARTI_OUTPUT ARTI_FILE //print output to file (e.g. default.wled.log)
  #define ARTI_ERROR 1
  #define ARTI_DEBUG 1
  #define ARTI_ANDBG 1
  #define ARTI_RUNLOG 1
  #define ARTI_PRINT 1
#endif

#if ARTI_OUTPUT == ARTI_SERIAL
  #if ARTI_PLATFORM == ARTI_ARDUINO
    #define OUTPUT_ARTI(...) Serial.printf(__VA_ARGS__)
  #else
    #define OUTPUT_ARTI printf
  #endif
#else
  #if ARTI_PLATFORM == ARTI_ARDUINO
    File logFile;
    #define OUTPUT_ARTI(...) logFile.printf(__VA_ARGS__)
  #else
    FILE * logFile;
    #define OUTPUT_ARTI(...) fprintf(logFile, __VA_ARGS__)
  #endif
#endif

#ifdef ARTI_DEBUG
    #define DEBUG_ARTI(...) OUTPUT_ARTI(__VA_ARGS__)
#else
    #define DEBUG_ARTI(...)
#endif

#ifdef ARTI_ANDBG
    #define ANDBG_ARTI(...) OUTPUT_ARTI(__VA_ARGS__)
#else
    #define ANDBG_ARTI(...)
#endif

#ifdef ARTI_RUNLOG
    #define RUNLOG_ARTI(...) OUTPUT_ARTI(__VA_ARGS__)
#else
    #define RUNLOG_ARTI(...)
#endif

#ifdef ARTI_PRINT
    #define PRINT_ARTI(...) OUTPUT_ARTI(__VA_ARGS__)
#else
    #define PRINT_ARTI(...)
#endif

#ifdef ARTI_ERROR
    #define ERROR_ARTI(...) OUTPUT_ARTI(__VA_ARGS__)
#else
    #define ERROR_ARTI(...)
#endif

#ifdef ARTI_MEMORY
    #define MEMORY_ARTI(...) OUTPUT_ARTI(__VA_ARGS__)
#else
    #define MEMORY_ARTI(...)
#endif

#define charLength 30
#define fileNameLength 50
#define arrayLength 30
#if ARTI_PLATFORM != ARTI_ARDUINO || ARTI_DEFINITION != ARTI_WLED
  #define doubleNullValue -32768
#endif


#if ARTI_PLATFORM == ARTI_ARDUINO
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

#if ARTI_DEFINITION == ARTI_WLED
  #if ARTI_PLATFORM == ARTI_ARDUINO
    #include "arti_wled_plugin.h"
  #else
    #include "wled/arti_wled_plugin.h"
  #endif
#endif

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
#define ResultFail 0
#define ResultStop 2
#define ResultContinue 1

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

  bool parse() {
    JsonObject::iterator objectIterator = lexer->definitionJson.begin();

    DEBUG_ARTI("Parser %s %s\n", this->current_token->type, this->current_token->value);

    JsonObject metaData = objectIterator->value();
    const char * version = metaData["version"];
    if (strcmp(version, "0.0.5") < 0) {
      ERROR_ARTI("Parser: Version of definition file (%s) should be 0.0.5 or higher\n", version);
      return false;
    }
    else {
      const char * startSymbol = metaData["start"];
      if (startSymbol != nullptr) {
        uint8_t result = visit(parseTreeJson, startSymbol, '&', lexer->definitionJson[startSymbol], 0);

        if (this->lexer->pos != strlen(this->lexer->text)) {
          ERROR_ARTI("Symbol %s Program not entirely parsed (%u,%u) %u of %u\n", startSymbol, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
          return false;
        }
        else if (result == ResultFail) {
          ERROR_ARTI("Symbol %s Program parsing failed (%u,%u) %u of %u\n", startSymbol, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
          return false;
        }
        else
          DEBUG_ARTI("Symbol %s Parsed until (%u,%u) %u of %u\n", startSymbol, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
      }
      else
        ERROR_ARTI("Parser: No start symbol found in definition file\n");
    }
    return true;
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

  uint8_t visit(JsonVariant parseTree, const char * symbol_name, char operatorx, JsonVariant expression, uint8_t depth = 0) {
    if (depth > 50) {
      ERROR_ARTI("Error too deep %u\n", depth);
      errorOccurred = true;
    }
    if (errorOccurred) return ResultFail;

    uint8_t result = ResultContinue;

    uint8_t resultChild = ResultContinue;

    //check if unary or binary operator
    // if (expression.size() > 1) {
    //   DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
    //   DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
    // }

    if (expression.is<JsonArray>()) { //should always be the case
      for (JsonVariant arrayElement: expression.as<JsonArray>()) {

        // DEBUG_ARTI("%s Array element %s\n", spaces+50-depth, arrayElement.as<std::string>().c_str());

        JsonVariant nextExpression = arrayElement;
        const char * nextSymbol_name = symbol_name;
        JsonVariant nextParseTree = parseTree;
        JsonArray arr;

        JsonVariant symbolExpression = lexer->definitionJson[arrayElement.as<const char *>()];

        if (!symbolExpression.isNull()) { //is arrayElement a Symbol e.g. "compound" : ["CURLYOPEN", "block*", "CURLYCLOSE"],

          // DEBUG_ARTI("%s New Symbol %s %s %s\n", spaces+50-depth, parseTree.as<std::string>().c_str(), symbol_name, nextExpression.as<std::string>().c_str());
          nextSymbol_name = arrayElement.as<const char *>();
          nextExpression = symbolExpression;

          DEBUG_ARTI("%s %s %u\n", spaces+50-depth, nextSymbol_name, depth); //, parseTree.as<std::string>().c_str()

          if (parseTree.is<JsonArray>()) {

            parseTree[parseTree.size()][nextSymbol_name]["connect"] = "array";
            // DEBUG_ARTI("%s New Symbol to array %s %s %u %s\n", spaces+50-depth, symbol_name, nextSymbol_name, depth, parseTree.as<std::string>().c_str());
            nextParseTree = parseTree[parseTree.size()-1]; //nextparsetree is last element in the array (which is always an object)
          }
          else 
          { //no list, create object
            if (parseTree[symbol_name].isNull()) //no object yet
              parseTree[symbol_name]["connect"] = "object"; //make the connection, new object item

            // DEBUG_ARTI("%s New Symbol %s %s %u %s\n", spaces+50-depth, symbol_name, nextSymbol_name, depth, parseTree.as<std::string>().c_str());
            nextParseTree = parseTree[symbol_name];
          }
        }

        // DEBUG_ARTI("%s Next expression %s\n", spaces+50-depth, nextExpression.as<std::string>().c_str());

        if (operatorx == '|')
          push_position();

        if (nextExpression.is<JsonObject>()) { // e.g. {"?":["LPAREN","formals*","RPAREN"]}
          // DEBUG_ARTI("%s Visit Object %s %c %s\n", spaces+50-depth, stringOrEmpty(nextSymbol_name), operatorx, nextExpression.as<std::string>().c_str());

          JsonObject::iterator objectIterator = nextExpression.as<JsonObject>().begin();
          char objectOperator = objectIterator->key().c_str()[0];
          JsonVariant objectElement = objectIterator->value();

          if (objectElement.is<JsonArray>()) {

            if (objectOperator == '*' || objectOperator == '+') {
              nextParseTree[nextSymbol_name]["*"][0] = "multiple";
              nextParseTree = nextParseTree[nextSymbol_name]["*"];
              // DEBUG_ARTI("%s multiple / array found %s %s %s\n", spaces+50-depth, nextSymbol_name, nextExpression.as<std::string>().c_str(), nextParseTree.as<std::string>().c_str());
            }

            //and: see 'is array'
            if (objectOperator == '|') {
              // DEBUG_ARTI("%s\n", "or ");
              resultChild = visit(nextParseTree, nextSymbol_name, objectOperator, objectElement, depth + 1);
              if (resultChild != ResultFail) resultChild = ResultContinue;
            }
            else {
              uint8_t resultChild2 = ResultContinue;
              uint8_t counter = 0;
              while (resultChild2 == ResultContinue) {
                // DEBUG_ARTI("Before %u (%u.%u) %u of %u %s\n", resultChild2, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text), objectExpression.as<String>().c_str());
                resultChild2 = visit(nextParseTree, nextSymbol_name, objectOperator, objectElement, depth + 1); //no assign to result as optional
                // DEBUG_ARTI("After %u (%u.%u) %u of %u %s\n", resultChild2, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text), objectExpression.as<String>().c_str());

                if (objectOperator == '?') { //zero or one iteration, also continue of visit not continue
                  resultChild2 = ResultContinue;
                  break; 
                }
                else if (objectOperator == '+') { //one or more iterations, stop if first visit not continue
                  if (counter == 0) {
                    if (resultChild2 != ResultContinue)
                      break;
                  } 
                  else {
                    if (resultChild2 != ResultContinue) {
                      resultChild2 = ResultContinue;  //always continue
                      break;
                    }
                  }
                }
                else if (objectOperator == '*') { //zero or more iterations, stop if visit not continue
                  if (resultChild2 != ResultContinue) {
                    resultChild2 = ResultContinue;  //always continue
                    break;
                  }
                }
                else {
                  ERROR_ARTI("%s Programming error: undefined %c %s\n", spaces+50-depth, objectOperator, objectElement.as<const char *>());
                  resultChild2 = ResultFail;
                }
                counter++;
              } //while
              resultChild = resultChild2;
            } //not or
          } // element is array
          else
            ERROR_ARTI("%s Definition error: should be an array %s %c %s\n", spaces+50-depth, stringOrEmpty(nextSymbol_name), operatorx, objectElement.as<std::string>().c_str());
        }
        else if (nextExpression.is<JsonArray>()) { // e.g. ["LPAREN", "expr*", "RPAREN"]
          // DEBUG_ARTI("%s Visit Array %s %c %s\n", spaces+50-depth, stringOrEmpty(nextSymbol_name), operatorx, nextExpression.as<std::string>().c_str());
          resultChild = visit(nextParseTree, nextSymbol_name, '&', nextExpression, depth + 1); // every array element starts with '&' (operatorx is for result of all elements of array)
        }
        else if (!lexer->definitionJson["TOKENS"][nextExpression.as<const char *>()].isNull()) { // token e.g. "ID"
          const char * token_type = nextExpression;
          // DEBUG_ARTI("%s Visit Token %s %c %s\n", spaces+50-depth, stringOrEmpty(nextSymbol_name), operatorx, nextExpression.as<const char *>());
          if (strcmp(this->current_token->type, token_type) == 0) {
            // DEBUG_ARTI("%s visit token %s %s\n", spaces+50-depth, this->current_token->type, token_type);//, expression.as<String>().c_str());
            // if (current_token->type == "ID" || current_token->type == "INTEGER" || current_token->type == "REAL" || current_token->type == "INTEGER_CONST" || current_token->type == "REAL_CONST" || current_token->type == "ID" || current_token->type == "ID" || current_token->type == "ID") {
            if (current_token != nullptr) {
              DEBUG_ARTI("%s %s %s", spaces+50-depth, current_token->type, current_token->value);
            }

            if (nextParseTree.is<JsonArray>()) {
              // DEBUG_ARTI("( %s Add token in array %s %s %s %c )", spaces+50-depth, nextSymbol_name, current_token->type, current_token->value, operatorx);
              JsonArray arr = nextParseTree.as<JsonArray>();
              arr[arr.size()][current_token->type] = current_token->value; //add in last element of array
            }
            else
                nextParseTree[nextSymbol_name][current_token->type] = current_token->value;

            if (strcmp(token_type, "PLUS2") == 0) { //debug for unary operators (wip)
              // DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
              // DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
            }

            eat(token_type);

            if (current_token != nullptr) {
              DEBUG_ARTI(" -> [%s %s]", current_token->type, current_token->value);
            }
            DEBUG_ARTI(" %d\n", depth);
            resultChild = ResultContinue;
          }
          else { //deadend
            // DEBUG_ARTI("%s visit deadend %s %s %s\n", spaces+50-depth, this->current_token->type, token_type, nextParseTree.as<std::string>().c_str());//, expression.as<String>().c_str());
            resultChild = ResultFail;
          }
        } // if token
        else { //arrayElement is not a symbol, not a token, not an array and not an object
          if (lexer->definitionJson[nextExpression.as<const char *>()].isNull())
            ERROR_ARTI("%s Programming error: %s not a symbol, token, array or object in %s\n", spaces+50-depth, nextExpression.as<const char *>(), stringOrEmpty(nextSymbol_name));
          else
            ERROR_ARTI("%s Definition error: \"%s\": \"%s\" symbol should be embedded in array\n", spaces+50-depth, stringOrEmpty(nextSymbol_name), nextExpression.as<const char *>());
        } //nextExpression is not a token

        if (!symbolExpression.isNull()) { //if symbol
          nextParseTree.remove("connect"); //remove connector

          if (resultChild == ResultFail) { //remove result of visit
            nextParseTree.remove(nextSymbol_name); //remove the failed stuff

            DEBUG_ARTI("%s fail %s\n", spaces+50-depth, nextSymbol_name);
          }
          else { //success
            //make the parsetree as small as possible to let the interpreter run as fast as possible:
            //for each symbol
            //- check if * multiple is one of the elements, if only "multiple" then remove all, otherwise only multiple element
            //- check if a symbol is not used in analyzer / interpreter and has only one element: go to the parent and replace itself with its child (shrink)

            if (nextParseTree.is<JsonObject>()) { //Symbols trees are always objects e.g. {"term": {"factor": {"varref": {..}},"*": ["multiple"]}}
              for (JsonPair symbolObjectPair : nextParseTree.as<JsonObject>()) {
                const char * symbolObjectKey = symbolObjectPair.key().c_str();
                JsonVariant symbolObjectValue = symbolObjectPair.value();

                if (symbolObjectValue.is<JsonObject>()) { // e.g. {"term":{"factor":{"varref":{"ID":"ledCount"}},"*":["multiple"]}}
                  for (JsonPair symbolObjectObject : symbolObjectValue.as<JsonObject>()) {

                    if (symbolObjectObject.value().is<JsonArray>()) {
                      JsonArray symbolObjectObjectArray = symbolObjectObject.value().as<JsonArray>();
                      for (JsonArray::iterator it=symbolObjectObjectArray.begin(); it!=symbolObjectObjectArray.end(); ++it) {
                        if ((*it) == "multiple") {
                          DEBUG_ARTI("%s remove multiple key\n", spaces+50-depth);
                          symbolObjectObjectArray.remove(it);
                        }
                      }
                      if (symbolObjectObjectArray.size() == 0) {
                        DEBUG_ARTI("%s remove multiple empty\n", spaces+50-depth);
                        symbolObjectValue.remove("*");
                      }
                    }
                  } //for symbol object objects
                } //if symbol object has object

                //symbolObjectKey should be a symbol on itself and the value must consist of one element
                if (false && !lexer->definitionJson[symbolObjectKey].isNull() && symbolObjectValue.size()==1) { //disabled as causing crash on Arduino

                  bool found = false;
                  for (JsonPair semanticsPair : lexer->definitionJson["SEMANTICS"].as<JsonObject>()) {
                    const char * semanticsKey = semanticsPair.key().c_str();
                    JsonVariant semanticsValue = semanticsPair.value();

                    // DEBUG_ARTI("%s semantics %s", spaces+50-depth, semanticsKey);
                    if (strcmp(symbolObjectKey, semanticsKey) == 0){
                      found = true;
                      break;
                    }
                    for (JsonPair semanticsVariables : semanticsValue.as<JsonObject>()) {
                      JsonVariant variableValue = semanticsVariables.value();
                      // DEBUG_ARTI(" %s", variableValue.as<std::string>().c_str());
                      if (variableValue.is<const char *>() && strcmp(symbolObjectKey, variableValue.as<const char *>()) == 0) {
                        found = true;
                        break;
                      }
                    }
                    // DEBUG_ARTI("\n");
                  }

                  if (!found) { // not used in analyzer / interpreter
                    DEBUG_ARTI("%s symbol to shrink %s\n", spaces+50-depth, symbolObjectKey);
                    if (parseTree.is<JsonObject>()) {
                      JsonObject parseTreeObject = parseTree.as<JsonObject>();
                      for (JsonObject::iterator it=parseTreeObject.begin(); it!=parseTreeObject.end(); ++it) {
                        const char * parseTreeObjectKey = it->key().c_str();
                        JsonVariant parseTreeObjectValue = it->value();
                        if (parseTreeObjectValue == nextParseTree) //find the right element to replace
                        {
                          parseTree[parseTreeObjectKey] = symbolObjectValue;
                          // DEBUG_ARTI("%s Shrink object %s %s\n", spaces+50-depth, symbolObjectKey, symbolObjectValue.as<std::string>().c_str());
                          // DEBUG_ARTI("%s Shrink object %s %s\n", spaces+50-depth, symbolObjectKey, nextParseTree.as<std::string>().c_str());
                          // DEBUG_ARTI("%s Shrink object %s %s\n", spaces+50-depth, symbolObjectKey, parseTree.as<std::string>().c_str());
                        }
                      }
                    }
                    else if (parseTree.is<JsonArray>()) {
                      JsonArray parseTreeArray = parseTree.as<JsonArray>();
                      for (int i=0; i<parseTreeArray.size();i++) {
                        if (parseTreeArray[i] == nextParseTree) { //find the right element to replace
                          parseTreeArray[i] = symbolObjectValue;
                          // DEBUG_ARTI("%s Shrink array %s %s\n", spaces+50-depth, symbolObjectKey, symbolObjectValue.as<std::string>().c_str());
                          // DEBUG_ARTI("%s Shrink array %s %s\n", spaces+50-depth, symbolObjectKey, nextParseTree.as<std::string>().c_str());
                          // DEBUG_ARTI("%s Shrink array %s %s\n", spaces+50-depth, symbolObjectKey, parseTree.as<std::string>().c_str());
                        }
                      }
                    }
                    // DEBUG_ARTI("%s symbol to shrink done %s %s\n", spaces+50-depth, symbolObjectKey, symbolObjectValue.as<std::string>().c_str());
                  }
                } // symbolObjectKey should by a symbol on itself and value is one element
              } //for symbol objects
            } //if symbol has object

            DEBUG_ARTI("%s success %s\n", spaces+50-depth, nextSymbol_name);
          }
        } // if symbol

        //determine result of arrayElement
        if (operatorx == '|') {
          if (resultChild == ResultFail) {//if fail, go back and try another
            // result = ResultContinue;
            pop_position();
          }
          else {
            result = ResultStop;  //Stop or continue is enough for an or
            positions_index--;
          }
        }
        else {
          if (resultChild != ResultContinue) //for and, ?, + and *; each result should continue
            result = ResultFail;
        } 

        if (result != ResultContinue) //if no reason to continue then stop
          break;

      } //for arrayelement

      if (operatorx == '|') {
        if (result != ResultStop) //still looking but nothing to look for
          result = ResultFail;
      }
    }
    else { //should never happen
      ERROR_ARTI("%s Programming error: no array %s %c %s\n", spaces+50-depth, stringOrEmpty(symbol_name), operatorx, expression.as<std::string>().c_str());
    }

    return result;

  } //visit

}; //Parser

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
    // ANDBG_ARTI("%s\n", "ScopedSymbolTable ", scope_name, scope_level);
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
      ANDBG_ARTI("Log scope Insert %s\n", symbol->name.c_str());
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
      // ANDBG_ARTI("%s\n", "  symbols ", i, symbols[i]->symbol_type, symbols[i]->name, symbols[i]->type, symbols[i]->scope_level);
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
    delete global_scope; global_scope = nullptr;
  }

  void analyze() {
    ANDBG_ARTI("\nAnalyzer\n");
    visit(parseTreeJson);
  }

  void visit(JsonVariant parseTree, const char * treeElement = nullptr, ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) {

    if (parseTree.is<JsonObject>()) {
      // ANDBG_ARTI("%s Visit object %s %u %s (%u)\n", spaces+50-depth, stringOrEmpty(treeElement), parseTree.size(), parseTree.as<std::string>().c_str(), depth);

      for (JsonPair parseTreePair : parseTree.as<JsonObject>()) {
        const char * key = parseTreePair.key().c_str();
        JsonVariant value = parseTreePair.value();
        if (treeElement == nullptr || strcmp(treeElement, key) == 0 ) { //in case there are more elements in the object and you want to visit only one

          // ANDBG_ARTI("%s Visit object element %s %s\n", spaces+50-depth, key, value.as<std::string>().c_str());
          bool visitCalledAlready = false;

          if (!definitionJson[key].isNull()) { //if key is symbol_name
            JsonVariant expression = definitionJson["SEMANTICS"][key];
            if (!expression.isNull())
            {
              // const char * expression_id = expression["id"];
              // MEMORY_ARTI("%s Heap %s < %u\n", spaces+50-depth, expression_id, esp_get_free_heap_size());
              if (expression["id"] == "Program") {
                const char * expression_name = expression["name"];
                const char * expression_block = expression["block"];
                const char * program_name = value[expression_name];
                this->global_scope = new ScopedSymbolTable(program_name, 1, nullptr); //current_scope

                ANDBG_ARTI("%s Program %s %u %u\n", spaces+50-depth, global_scope->scope_name, global_scope->scope_level, global_scope->symbolsIndex); 

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                else {
                  visit(value[expression_block], nullptr, this->global_scope, depth + 1);
                }

                #ifdef ARTI_DEBUG
                  for (uint8_t i=0; i<global_scope->symbolsIndex; i++) {
                    Symbol* symbol = global_scope->symbols[i];
                    ANDBG_ARTI("%s %u %s %s.%s %s %u\n", spaces+50-depth, i, symbol->symbol_type, global_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                  }
                #endif

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Function") {

                //find the function name (so we must know this is a function...)
                const char * expression_name = expression["name"];//.as<const char *>();
                const char * expression_block = expression["block"];
                const char * function_name = value[expression_name];
                Symbol* function_symbol = new Symbol(expression["id"], function_name);
                current_scope->insert(function_symbol);

                ANDBG_ARTI("%s Function %s.%s\n", spaces+50-depth, current_scope->scope_name, function_name);
                ScopedSymbolTable* function_scope = new ScopedSymbolTable(function_name, current_scope->scope_level + 1, current_scope);
                current_scope->child_scopes[current_scope->child_scopesIndex++] = function_scope;
                function_symbol->function_scope = function_scope;
                // ANDBG_ARTI("%s\n", "ASSIGNING ", function_symbol->name, " " , function_scope->scope_name);

                visit(value[expression["formals"].as<const char *>()], nullptr, function_scope, depth + 1);

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, function_name, expression_block); 
                else
                  visit(value[expression_block], nullptr, function_scope, depth + 1);

                // ANDBG_ARTI("%s\n", spaces+50-depth, "end function ", symbol_name, function_scope->scope_name, function_scope->scope_level, function_scope->symbolsIndex); 

                #ifdef ARTI_DEBUG
                  for (uint8_t i=0; i<function_scope->symbolsIndex; i++) {
                    Symbol* symbol = function_scope->symbols[i];
                    ANDBG_ARTI("%s %u %s %s.%s %s %u\n", spaces+50-depth, i, symbol->symbol_type, function_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                  }
                #endif

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Var" || expression["id"] == "Formal" || expression["id"] == "Assign" || expression["id"] == "VarRef") {
                const char * expression_name = expression["name"];
                const char * expression_type = expression["type"];

                const char * variable_name = value[expression_name];
                if (expression["id"] == "Assign")
                  variable_name = value[expression_name]["ID"];

                char param_type[charLength]; 
                if (!value[expression_type].isNull()) {
                  serializeJson(value[expression_type], param_type); //current_scope.lookup(param.type_node.value); //need string, lookup also used to find types...
                }
                else
                  strcpy(param_type, "notype");

                //check if external variable
                bool found = false;
                for (JsonPair externalsPair: definitionJson["EXTERNALS"].as<JsonObject>()) {
                  if (strcmp(variable_name, externalsPair.key().c_str()) == 0) {
                    // visit(value[expression["actuals"].as<const char *>()], nullptr, current_scope, depth + 1);
                    ANDBG_ARTI("%s Ext Variable found %s (%u)\n", spaces+50-depth, variable_name, depth);
                    found = true;
                  }
                }

                if (!found) {
                  if (expression["id"] == "VarRef") {
                    Symbol* var_symbol = current_scope->lookup(variable_name); //lookup here and parent scopes
                    if (var_symbol == nullptr)
                      ERROR_ARTI("%s VarRef %s not found in scope %s\n", spaces+50-depth, variable_name, current_scope->scope_name); 
                    else
                      ANDBG_ARTI("%s VarRef found %s (%u)\n", spaces+50-depth, variable_name, depth);
                  }
                  else { //assign and var/formal
                    //if variable not already defined, then add
                    // if (expression["id"] == "Formal")
                    //   ANDBG_ARTI("%s Formal found %s (%u)\n", spaces+50-depth, variable_name, depth);
                    Symbol* var_symbol = current_scope->lookup(variable_name); //lookup here and parent scopes
                    if (expression["id"] != "Assign" || var_symbol == nullptr) { //only assign needs to check if not exists
                      var_symbol = new Symbol(expression["id"], variable_name, param_type);
                      current_scope->insert(var_symbol);
                      ANDBG_ARTI("%s %s %s.%s of %s\n", spaces+50-depth, expression["id"].as<const char *>(), current_scope->scope_name, variable_name, param_type);
                    }

                  }
                }

                if (expression["id"] == "Assign") {
                  const char * expression_value = expression["value"];

                  if (!value[expression_value].isNull()) { //value assignment
                    ANDBG_ARTI("%s Assign %s = (%u)\n", spaces+50-depth, variable_name, depth);
                    visit(value, expression_value, current_scope, depth + 1); // check  the variables in the value
                  }
                  else
                    ERROR_ARTI("%s Assign %s: no value in parseTree\n", spaces+50-depth, variable_name); 
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Call") {
                const char * function_name = value[expression["name"].as<const char *>()];

                //check if external function
                bool found = false;
                for (JsonPair externalsPair: definitionJson["EXTERNALS"].as<JsonObject>()) {
                  const char * key = externalsPair.key().c_str();
                  // JsonVariant value = externalsPair.value();
                  if (strcmp(function_name, key) == 0) {
                    ANDBG_ARTI("%s extfunc %s\n", spaces+50-depth, function_name);//value.as<std::string>().c_str(), expression["actuals"].as<const char *>()
                    visit(value[expression["actuals"].as<const char *>()], nullptr, current_scope, depth + 1);
                    found = true;
                  }
                }

                if (!found) {
                  Symbol* function_symbol = current_scope->lookup(function_name); //lookup here and parent scopes
                  if (function_symbol != nullptr) {
                    visit(value[expression["actuals"].as<const char *>()], nullptr, current_scope, depth + 1);

                    visit(function_symbol->block, nullptr, current_scope, depth + 1);
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
            // ANDBG_ARTI("%s Token %s\n", spaces+50-depth, key);
            visitCalledAlready = true;
          }
          // ANDBG_ARTI("%s Object %s %u\n", spaces+50-depth, key, value, visitCalledAlready);

          if (!visitCalledAlready) //parseTreeObject.size() != 1 && 
            visit(value, nullptr, current_scope, depth + 1);

        } // key values
      } ///for elements in object

    }
    else { //not object
      if (parseTree.is<JsonArray>()) {
        // ANDBG_ARTI("%s Visit array %s %s (%u)\n", spaces+50-depth, stringOrEmpty(treeElement), parseTree.as<std::string>().c_str(), depth);
        for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
          // ANDBG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
          visit(newParseTree, nullptr, current_scope, depth + 1);
        }
      }
      else { //not array
        // string element = parseTree;
        ERROR_ARTI("%s Error: parseTree should be array or object %s %s (%u)\n", spaces+50-depth, stringOrEmpty(treeElement), parseTree.as<std::string>().c_str(), depth);
        // if (definitionJson["SEMANTICS"][element])
      }
    }
  } //visit

}; //SemanticAnalyzer

struct charKeyValue
{
    char key[charLength];
    char value[charLength];
};

struct doubleKeyValue
{
    char key[charLength];
    double value;
};

#define nrOfVariables 20

class ActivationRecord {
  private:
  public:
    char name[charLength];
    char type[charLength];
    int nesting_level;
    struct charKeyValue *charMembers;
    uint8_t charMembersCounter = 0;
    struct doubleKeyValue *doubleMembers;
    uint8_t doubleMembersCounter = 0;
    char lastSet[charLength];

    ActivationRecord(const char * name, const char * type, int nesting_level) {
        strcpy(this->name, name);
        strcpy(this->type, type);
        this->nesting_level = nesting_level;

        charMembers = (struct charKeyValue *)malloc(sizeof(struct charKeyValue) * nrOfVariables);
        doubleMembers = (struct doubleKeyValue *)malloc(sizeof(struct doubleKeyValue) * nrOfVariables);
    }

    ~ActivationRecord() {
      RUNLOG_ARTI("Destruct activation record %s\n", name);
      free(charMembers);
      free(doubleMembers);
    }

    void set(const char * key, const char * value) {

      for (uint8_t i = 0; i < charMembersCounter; i++) {
        if (strcmp(charMembers[i].key, key) == 0) {
          strcpy(charMembers[i].value, value);
          strcpy(lastSet, key);
          // RUNLOG_ARTI("Set %s %s\n", key, value);
          return;
        }
      }

      if (charMembersCounter < nrOfVariables) {
          strcpy(charMembers[charMembersCounter].key, key);
          strcpy(charMembers[charMembersCounter].value, value);
        strcpy(lastSet, key);
        charMembersCounter++;
        // RUNLOG_ARTI("Set %s %s\n", key, value);
      }
      else
        ERROR_ARTI("ActivationRecord no room for new vars\n");
    }

    void set(const char * key, double value) {

      for (uint8_t i = 0; i < doubleMembersCounter; i++) {
        if (strcmp(doubleMembers[i].key, key) == 0) {
          doubleMembers[i].value = value;
          strcpy(lastSet, key);
          // RUNLOG_ARTI("Set %s %s\n", key, value);
          return;
        }
      }

      if (doubleMembersCounter < nrOfVariables) {
          strcpy(doubleMembers[doubleMembersCounter].key, key);
          doubleMembers[doubleMembersCounter].value = value;
        strcpy(lastSet, key);
        doubleMembersCounter++;
        // RUNLOG_ARTI("Set %s %s\n", key, value);
      }
      else
        ERROR_ARTI("ActivationRecord no room for new vars\n");
    }

    const char * getChar(const char * key) {
      for (uint8_t i = 0; i < charMembersCounter; i++) {
        // RUNLOG_ARTI("Get %s %s %s", key, members[i].key, members[i].value);
        if (strcmp(charMembers[i].key, key) == 0) {
          return charMembers[i].value;
        }
      }
      return "empty";
    }

    double getDouble(const char * key) {
      for (uint8_t i = 0; i < doubleMembersCounter; i++) {
        // RUNLOG_ARTI("Get %s %s %s", key, members[i].key, members[i].value);
        if (strcmp(doubleMembers[i].key, key) == 0) {
          return doubleMembers[i].value;
        }
      }
      return doubleNullValue;
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
    RUNLOG_ARTI("Destruct callstack\n");
  }


    void push(ActivationRecord* ar) {
      if (recordsCounter < nrOfRecords) {
        // RUNLOG_ARTI("%s\n", "Push ", ar->name);
        this->records[recordsCounter++] = ar;
      }
      else
        ERROR_ARTI("no space left in callstack\n");
    }

    ActivationRecord* pop() {
      if (recordsCounter > 0) {
        // RUNLOG_ARTI("%s\n", "Pop ", this->peek()->name);
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
  char charStack[arrayLength][charLength];
  double doubleStack[arrayLength];
  uint8_t stack_index = 0;

  ValueStack() {
  }

  ~ValueStack() {
    RUNLOG_ARTI("Destruct valueStack\n");
  }

  void push(const char * value) {
    if (stack_index >= arrayLength)
      ERROR_ARTI("Push charStack full %u of %u\n", stack_index, arrayLength);
    else if (value == nullptr) {
      strcpy(charStack[stack_index++], "empty");
      ERROR_ARTI("Push null pointer on double stack\n");
    }
    else
      // RUNLOG_ARTI("calc push %s %s\n", key, value);
      strcpy(charStack[stack_index++], value);
  }

  void push(double value) {
    if (stack_index >= arrayLength)
      ERROR_ARTI("Push doubleStack full %u of %u\n", stack_index, arrayLength);
    else if (value == doubleNullValue)
      ERROR_ARTI("Push null value on double stack\n");
    else
      // RUNLOG_ARTI("calc push %s %s\n", key, value);
      doubleStack[stack_index++] = value;
  }

  const char * peekChar() {
    // RUNLOG_ARTI("Calc Peek %s\n", charStack[stack_index-1]);
    return charStack[stack_index-1];
  }

  double peekDouble() {
    // RUNLOG_ARTI("Calc Peek %s\n", doubleStack[stack_index-1]);
    return doubleStack[stack_index-1];
  }

  const char * popChar() {
    if (stack_index>0) {
      stack_index--;
      return charStack[stack_index];
    }
    else {
      ERROR_ARTI("Pop value stack empty\n");
    // RUNLOG_ARTI("Calc Pop %s\n", charStack[stack_index]);
      return "novalue";
    }
  }

  double popDouble() {
    if (stack_index>0) {
      stack_index--;
      return doubleStack[stack_index];
    }
    else {
      ERROR_ARTI("Pop doubleStack empty\n");
    // RUNLOG_ARTI("Calc Pop %s\n", doubleStack[stack_index]);
      return -1;
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
    RUNLOG_ARTI("Destruct Interpreter\n");
  }

  bool interpret(const char * function_name = nullptr) {
    this->function_name = function_name;

    if (function_name == nullptr) {
      if (this->analyzer != nullptr) { //due to undefined functions??? wip

        RUNLOG_ARTI("\ninterpret %s %u %u\n", analyzer->global_scope->scope_name, analyzer->global_scope->scope_level, analyzer->global_scope->symbolsIndex); 

        visit(analyzer->parseTreeJson);
      }
      else
      {
        ERROR_ARTI("\nInterpret global scope is nullptr\n");
        return false;
      }
    }
    else { //Call only function_name (no parameters)
      uint8_t depth = 8;
      Symbol* function_symbol = this->analyzer->global_scope->lookup(function_name);

      if (function_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

        // MEMORY_ARTI("%s Heap Call %s < %u\n", spaces+50-depth, function_name, esp_get_free_heap_size());

        ActivationRecord* ar = new ActivationRecord(function_name, "Function", function_symbol->scope_level + 1);

        RUNLOG_ARTI("%s %s %s (%u)\n", spaces+50-depth, "Call", function_name, this->callStack->recordsCounter);

        this->callStack->push(ar);

        //find block of function... lookup function?
        //visit block of function
        // RUNLOG_ARTI("%s function block %s\n", spaces+50-depth, function_symbol->block.as<String>().c_str());

        visit(function_symbol->block, nullptr, this->analyzer->global_scope, depth + 1);

        this->callStack->pop();

        delete ar; ar = nullptr;

      } //function_symbol != nullptr
      else {
        RUNLOG_ARTI("%s %s not found %s\n", spaces+50-depth, "Call", function_name);
      }

    }
    return true;

  } //interpret

            // ["PLUS2", "factor"],
          // [ "MINUS2", "factor"],

  void visit(JsonVariant parseTree, const char * treeElement = nullptr, ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) {

    // RUNLOG_ARTI("%s Visit %s %s (%u)\n", spaces+50-depth, stringOrEmpty(treeElement), parseTree.as<std::string>().c_str(), depth);

    if (depth >= 50) {
      ERROR_ARTI("Error too deep %u\n", depth);
      errorOccurred = true;
    }
    if (errorOccurred) return;

    if (parseTree.is<JsonObject>()) {
      for (JsonPair parseTreePair : parseTree.as<JsonObject>()) {
        const char * key = parseTreePair.key().c_str();
        JsonVariant value = parseTreePair.value();
        if (treeElement == nullptr || strcmp(treeElement, key) == 0 ) {

          // RUNLOG_ARTI("%s Visit object element %s\n", spaces+50-depth, key); //, value.as<std::string>().c_str()

          // return;

          bool visitCalledAlready = false;

          if (!this->analyzer->definitionJson[key].isNull()) { //if key is symbol_name
            JsonVariant expression = this->analyzer->definitionJson["SEMANTICS"][key];
            if (!expression.isNull())
            {
              // RUNLOG_ARTI("%s Symbol %s %s\n", spaces+50-depth, symbol_name,  expression.as<std::string>().c_str());

              // const char * expression_id = expression["id"];
              // MEMORY_ARTI("%s Heap %s < %u\n", spaces+50-depth, expression_id, esp_get_free_heap_size());
              if (expression["id"] == "Program") {
                const char * expression_name = expression["name"];
                const char * expression_block = expression["block"];
                RUNLOG_ARTI("%s program name %s\n", spaces+50-depth, expression_name);
                const char * program_name = value[expression_name];

                if (!value[expression_name].isNull()) {
                  ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM", 1);
                  RUNLOG_ARTI("%s %s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, program_name);

                  this->callStack->push(ar);
                  if (value[expression_block].isNull())
                    ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                  else
                    visit(value[expression_block], nullptr, analyzer->global_scope, depth + 1);

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
                RUNLOG_ARTI("%s Save block of %s\n", spaces+50-depth, function_name);
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

                    visit(value[expression["actuals"].as<const char *>()], nullptr, current_scope, depth + 1);

                    double returnValue = doubleNullValue;

                    #if ARTI_DEFINITION == ARTI_WLED
                      returnValue = strip.arti_wled_functions(function_name, valueStack->doubleStack[oldIndex], (valueStack->stack_index - oldIndex>1)?valueStack->doubleStack[oldIndex+1]:doubleNullValue, (valueStack->stack_index - oldIndex>2)?valueStack->doubleStack[oldIndex+2]:doubleNullValue);
                    #endif

                    #if ARTI_PLATFORM != ARTI_ARDUINO
                      uint8_t lastIndex = oldIndex;
                      RUNLOG_ARTI("%s Call %s(", spaces+50-depth, function_name);
                      char sep[3] = "";
                      for (JsonPair actualsPair: externalsPair.value().as<JsonObject>()) {
                        const char * name = actualsPair.key().c_str();
                        // JsonVariant type = actualsPair.value();
                        if (strcmp(name, "return") != 0) {
                          RUNLOG_ARTI("%s%f", sep, valueStack->doubleStack[lastIndex++]);
                          strcpy(sep, ", ");
                        }
                        else {
                          // strcat(returnValue, "CallResult tbd of ");
                          // strcat(returnValue, function_name);
                        }
                      }
                      if ( returnValue != doubleNullValue)
                        RUNLOG_ARTI(") = %f\n", returnValue);
                      else
                        RUNLOG_ARTI(")\n");
                    #endif

                    valueStack->stack_index = oldIndex;

                    if (returnValue != doubleNullValue) {
                      valueStack->push(returnValue);
                    }

                    found = true;
                  }
                }

                if (!found) { //not an external function
                  Symbol* function_symbol = current_scope->lookup(function_name);

                  if (function_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                    ActivationRecord* ar = new ActivationRecord(function_name, "Function", function_symbol->scope_level + 1);

                    RUNLOG_ARTI("%s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), function_name);

                    uint8_t oldIndex = valueStack->stack_index;
                    uint8_t lastIndex = valueStack->stack_index;

                    visit(value[expression["actuals"].as<const char *>()], nullptr, current_scope, depth + 1);

                    for (uint8_t i=0; i<function_symbol->function_scope->symbolsIndex; i++) { //backwards because popped in reversed order
                      if (strcmp(function_symbol->function_scope->symbols[i]->symbol_type, "Formal") == 0) { //select formal parameters
                        //determine type, for now assume double
                        double result = valueStack->doubleStack[lastIndex++];
                        ar->set(function_symbol->function_scope->symbols[i]->name, result);
                        RUNLOG_ARTI("%s Actual %s.%s = %f (pop %u)\n", spaces+50-depth, function_name, function_symbol->function_scope->symbols[i]->name, result, valueStack->stack_index);
                      }
                    }

                    valueStack->stack_index = oldIndex;

                    this->callStack->push(ar);

                    //find block of function... lookup function?
                    //visit block of function
                    // RUNLOG_ARTI("%s function block %s\n", spaces+50-depth, function_symbol->block.as<String>().c_str());

                    visit(function_symbol->block, nullptr, function_symbol->function_scope, depth + 1);

                    this->callStack->pop();

                    delete ar; ar =  nullptr;

                    //tbd if syntax supports returnvalue
                    // char callResult[charLength] = "CallResult tbd of ";
                    // strcat(callResult, function_name);
                    // valueStack->push(callResult);

                  } //function_symbol != nullptr
                  else {
                    //this should move to analyze and should abort the programm!!!
                    RUNLOG_ARTI("%s %s not found %s\n", spaces+50-depth, expression["id"].as<const char *>(), function_name);
                  }
                } //external functions

                visitCalledAlready = true;
              }
              else if (expression["id"] == "VarRef" || expression["id"] == "Assign") { //get or set a variable
                const char * expression_name = expression["name"]; //ID (key="varref")
                const char * variable_name = value[expression_name];

                //array indices
                const char * expression_indices = expression["indices"];
                char indices[charLength];
                strcpy(indices, "");
                uint8_t oldIndex = valueStack->stack_index;
                if (expression_indices != nullptr) {
                  strcpy(indices, "");
                  if (!value[expression_indices].isNull()) {
                    strcat(indices, "[");

                    visit(value, expression["indices"], current_scope, depth + 1); //value pushed

                    char sep[3] = "";
                    for (uint8_t i = oldIndex; i< valueStack->stack_index; i++) {
                      strcat(indices, sep);
                      char itoaChar[charLength];
                      itoa(valueStack->doubleStack[i], itoaChar, 10);
                      strcat(indices, itoaChar);
                      strcpy(sep, ",");
                    }

                    strcat(indices, "]");
                  }
                }

                if (expression["id"] == "Assign") {
                  variable_name = value[expression_name]["ID"];

                  const char * expression_value = expression["value"];
                  if (!value[expression_value].isNull()) { //value assignment
                    visit(value, expression["value"], current_scope, depth + 1); //value pushed
                  }
                  else {
                    ERROR_ARTI("%s Assign %s has no value\n", spaces+50-depth, expression_name);
                    valueStack->push(doubleNullValue);
                  }
                }

                //check if external variable
                bool found = false;
                for (JsonPair externalsPair: analyzer->definitionJson["EXTERNALS"].as<JsonObject>()) {
                  if (strcmp(variable_name, externalsPair.key().c_str()) == 0) {

                    double returnValue = doubleNullValue;

                    if (expression["id"] == "VarRef") { //get the value

                      #if ARTI_DEFINITION == ARTI_WLED
                        returnValue = strip.arti_wled_get_variables(variable_name, (valueStack->stack_index - oldIndex>0)?valueStack->doubleStack[oldIndex]:doubleNullValue, (valueStack->stack_index - oldIndex>1)?valueStack->doubleStack[oldIndex+1]:doubleNullValue);
                      #endif
                valueStack->stack_index = oldIndex;

                      if (returnValue != doubleNullValue) {
                        valueStack->push(returnValue);
                        RUNLOG_ARTI("%s %s ext.%s = %f (push %u)\n", spaces+50-depth, expression["id"].as<const char *>(), variable_name, returnValue, valueStack->stack_index); //key is variable_declaration name is ID
                      }
                      else
                        ERROR_ARTI("%s Error: %s ext.%s no value\n", spaces+50-depth, expression["id"].as<const char *>(), variable_name);
                    }
                    else { //assign: set the external value...
                      returnValue = valueStack->popDouble(); //result of visit value
                      #if ARTI_DEFINITION == ARTI_WLED
                        strip.arti_wled_set_variables(returnValue, variable_name, (valueStack->stack_index - oldIndex>0)?valueStack->doubleStack[oldIndex]:doubleNullValue, (valueStack->stack_index - oldIndex>1)?valueStack->doubleStack[oldIndex+1]:doubleNullValue);
                      #endif
                      RUNLOG_ARTI("%s %s set ext.%s%s = %f (%u)\n", spaces+50-depth, expression["id"].as<const char *>(), variable_name, indices, returnValue, valueStack->stack_index);
                valueStack->stack_index = oldIndex;
                    }
                    found = true;
                  }
                }

                if (!found) { //not external, get er set the variable
                  Symbol* variable_symbol = current_scope->lookup(variable_name);
                  ActivationRecord* ar;

                  //check already defined in this scope

                  if (variable_symbol != nullptr) { //var already exist
                    //calculate the index in the call stack to find the right ar
                    uint8_t index = this->callStack->recordsCounter - 1 - (this->callStack->peek()->nesting_level - variable_symbol->scope_level);
                    //  RUNLOG_ARTI("%s %s %s.%s = %s (push) %s %d-%d = %d (%d)\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, variable_name, varValue, variable_symbol->name, this->callStack->peek()->nesting_level,variable_symbol->scope_level, index,  this->callStack->recordsCounter); //key is variable_declaration name is ID
                    ar = this->callStack->records[index];
                  }
                  else { //var created here
                    ar = this->callStack->peek();
                  }

                  if (ar != nullptr) {
                    if (expression["id"] == "VarRef") { //get the value
                      //determine type, for now assume double
                      double varValue = ar->getDouble(variable_name);

                      valueStack->push(varValue);
                      #if ARTI_PLATFORM != ARTI_ARDUINO  //for some weird reason this causes a crash on esp32
                        RUNLOG_ARTI("%s %s %s.%s = %f (push %u)\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, variable_name, varValue, valueStack->stack_index); //key is variable_declaration name is ID
                      #endif
                    }
                    else { //assign: set the value 
                      ar->set(variable_name, valueStack->popDouble()); // pushed by visit value
                      valueStack->stack_index = oldIndex;

                      RUNLOG_ARTI("%s %s.%s%s := %f (pop %u)\n", spaces+50-depth, ar->name, variable_name, indices, ar->getDouble(variable_name), valueStack->stack_index);
                    }
                  }
                  else { //unknown variable
                    ERROR_ARTI("%s %s %s unknown \n", spaces+50-depth, expression["id"].as<const char *>(), variable_name);
                    valueStack->push(doubleNullValue);
                  }
                } // ! founnd
                visitCalledAlready = true;
              }
              else if (expression["id"] == "Expr" || expression["id"] == "Term") {
                // RUNLOG_ARTI("%s %s tovisit %s\n", spaces+50-depth, expression["id"].as<const char *>(), value.as<std::string>().c_str());

                uint8_t oldIndex = valueStack->stack_index;

                visit(value, nullptr, current_scope, depth + 1); //pushes results

                if (valueStack->stack_index - oldIndex == 3) {
                  // RUNLOG_ARTI("%s %s visited (%u)\n", spaces+50-depth, expression["id"].as<const char *>(), valueStack->stack_index - oldIndex);
                  double left = valueStack->doubleStack[oldIndex];
                  const char * operatorx = valueStack->charStack[oldIndex + 1];
                  double right = valueStack->doubleStack[oldIndex + 2];

                  valueStack->stack_index = oldIndex;
  
                  double evaluation = 0;
                  if (strcmp(operatorx, "+") == 0)
                    evaluation = left + right;
                  else if (strcmp(operatorx, "-") == 0)
                    evaluation = left - right;
                  else if (strcmp(operatorx, "*") == 0)
                    evaluation = left * right;
                  else if (strcmp(operatorx, "/") == 0)
                    evaluation = left / right;
                  else if (strcmp(operatorx, "==") == 0)
                    evaluation = left == right;
                  else if (strcmp(operatorx, "!=") == 0)
                    evaluation = left != right;
                  else if (strcmp(operatorx, "<") == 0)
                    evaluation = left < right;
                  else if (strcmp(operatorx, "<=") == 0)
                    evaluation = left <= right;
                  else if (strcmp(operatorx, ">") == 0)
                    evaluation = left > right;
                  else if (strcmp(operatorx, ">=") == 0)
                    evaluation = left >= right;

                  RUNLOG_ARTI("%s %f %s %f = %f (push %u)\n", spaces+50-depth, left, operatorx, right, evaluation, valueStack->stack_index+1);
                  valueStack->push(evaluation);
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "For") {
                RUNLOG_ARTI("%s For (%u)\n", spaces+50-depth, valueStack->stack_index);

                const char * expression_block = expression["block"];

                RUNLOG_ARTI("%s from\n", spaces+50-depth);
                visit(value, expression["from"], current_scope, depth + 1); //creates the assignment
                ActivationRecord* ar = this->callStack->peek();
                const char * fromVarName = ar->lastSet;

                bool continuex = true;
                uint16_t counter = 0;
                while (continuex && counter < 1000) { //to avoid endless loops
                  RUNLOG_ARTI("%s iteration\n", spaces+50-depth);

                  RUNLOG_ARTI("%s check to condition\n", spaces+50-depth);
                  visit(value, expression["condition"], current_scope, depth + 1); //pushes result of to

                  double conditionResult = valueStack->popDouble();

                  RUNLOG_ARTI("%s conditionResult (pop %u)\n", spaces+50-depth, valueStack->stack_index);

                  if (conditionResult == 1) { //conditionResult is true
                    RUNLOG_ARTI("%s 1 => run block\n", spaces+50-depth);
                    visit(value[expression_block], nullptr, current_scope, depth + 1);

                    RUNLOG_ARTI("%s assign next value\n", spaces+50-depth);
                    visit(value[expression["increment"].as<const char *>()], nullptr, current_scope, depth + 1); //pushes increment result
                    // MEMORY_ARTI("%s Iteration %u %u\n", spaces+50-depth, counter, esp_get_free_heap_size());
                  }
                  else {
                    if (conditionResult == 0) { //conditionResult is false
                      RUNLOG_ARTI("%s 0 => end of For\n", spaces+50-depth);
                      continuex = false;
                    }
                    else { // conditionResult is a value (e.g. in pascal)
                      //get the variable from assignment
                      double varValue = ar->getDouble(fromVarName);

                      double evaluation = varValue <= conditionResult;
                      RUNLOG_ARTI("%s %s.%s %f <= %f = %f\n", spaces+50-depth, ar->name, fromVarName, varValue, conditionResult, evaluation);

                      if (evaluation == 1) {
                        RUNLOG_ARTI("%s 1 => run block\n", spaces+50-depth);
                        visit(value[expression_block], nullptr, current_scope, depth + 1);

                        //increment
                        ar->set(fromVarName, varValue + 1);
                      }
                      else {
                        RUNLOG_ARTI("%s 0 => end of For\n", spaces+50-depth);
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
              else if (expression["id"] == "If") {
                RUNLOG_ARTI("%s If (%u)\n", spaces+50-depth, valueStack->stack_index);

                RUNLOG_ARTI("%s if condition \n", spaces+50-depth);
                visit(value, expression["condition"], current_scope, depth + 1);

                double conditionResult = valueStack->popDouble();

                RUNLOG_ARTI("%s (pop %u)\n", spaces+50-depth, valueStack->stack_index);

                if (conditionResult == 1) //conditionResult is true
                  visit(value, expression["true"], current_scope, depth + 1);
                else
                  visit(value, expression["false"], current_scope, depth + 1);

                // ActivationRecord* ar = this->callStack->peek();
                // const char * fromVarName = ar->lastSet;

                visitCalledAlready = true;
              }  //if expression["id"]

              // MEMORY_ARTI("%s Heap %s > %u\n", spaces+50-depth, expression_id, esp_get_free_heap_size());
            } //if expression not null

          } // is key is symbol_name

          const char * valueStr = value.as<std::string>().c_str();

          if (strcmp(key, "INTEGER_CONST") == 0 || strcmp(key, "REAL_CONST") == 0) {
            valueStack->push(atof(valueStr));
            #if ARTI_PLATFORM != ARTI_ARDUINO  //for some weird reason this causes a crash on esp32
              RUNLOG_ARTI("%s %s %s (Push %u)\n", spaces+50-depth, key, valueStr, valueStack->stack_index);
            #endif
            visitCalledAlready = true;
          }

          // RUNLOG_ARTI("%s\n", spaces+50-depth, "Object ", key, value);
          if (strcmp(valueStr, "+") == 0 || strcmp(valueStr, "-") == 0 || strcmp(valueStr, "*") == 0 || strcmp(valueStr, "/") == 0 || 
                          strcmp(valueStr, "==") == 0 || strcmp(valueStr, "!=-") == 0 || 
                          strcmp(valueStr, ">") == 0 || strcmp(valueStr, ">=") == 0 || strcmp(valueStr, "<") == 0 || strcmp(valueStr, "<=") == 0) {
            valueStack->push(valueStr);
            #if ARTI_PLATFORM != ARTI_ARDUINO  //for some weird reason this causes a crash on esp32
              RUNLOG_ARTI("%s %s %s (Push %u)\n", spaces+50-depth, key, valueStr, valueStack->stack_index);
            #endif
            visitCalledAlready = true;
          }

          if (!analyzer->definitionJson["TOKENS"][key].isNull()) { //if key is token
            // RUNLOG_ARTI("%s Token %s\n", spaces+50-depth, key);
            visitCalledAlready = true;
          }

          if (!visitCalledAlready)
            visit(value, nullptr, current_scope, depth + 1);
        } // if treeelement
      } // for (JsonPair)
    }
    else { //not object
      if (parseTree.is<JsonArray>()) {
        for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
          // RUNLOG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
          visit(newParseTree, nullptr, current_scope, depth + 1);
        }
      }
      else { //not array
        // const char * element = parseTree.as<const char *>();
        // RUNLOG_ARTI("%s\n", spaces+50-depth, "not array not object but element ", element);
      }
    }
  } //visit

}; //interpreter

#define programTextSize 1000

class ARTI {
private:
  Lexer *lexer;
  Parser *parser;
  SemanticAnalyzer *semanticAnalyzer = nullptr;
  Interpreter *interpreter = nullptr;
  // Interpreter2 *interpreter2 = nullptr;
  char * programText;
  #if ARTI_PLATFORM == ARTI_ARDUINO
    File definitionFile;
    File programFile;
  #else
    std::fstream definitionFile;
    std::fstream programFile;
  #endif
  uint16_t programFileSize;

  DynamicJsonDocument *definitionJsonDoc = nullptr;
  DynamicJsonDocument *parseTreeJsonDoc = nullptr;
  CallStack *callStack = nullptr;
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

  bool openFileAndParse(const char *definitionName, const char *programName) {
    MEMORY_ARTI("Heap OFP < %u (%lums) %s %s\n", esp_get_free_heap_size(), millis(), definitionName, programName);
      char parseOrLoad[charLength] = "Parse";

      //open logFile
      #if ARTI_OUTPUT == ARTI_FILE
        char logFileName[fileNameLength];
        #if ARTI_PLATFORM == ARTI_ARDUINO
          strcpy(logFileName, "/");
        #endif
        strcpy(logFileName, programName);
        strcat(logFileName, ".log");

        #if ARTI_PLATFORM == ARTI_ARDUINO
          #if ARTI_DEFINITION == ARTI_WLED
            logFile = WLED_FS.open(logFileName,"w");
          #else
            logFile = FS.open(logFileName,"w");
          #endif
        #else
          logFile = fopen (logFileName,"w");
        #endif
      #endif

      #if ARTI_PLATFORM == ARTI_ARDUINO
        #if ARTI_DEFINITION == ARTI_WLED
          definitionFile = WLED_FS.open(definitionName, "r");
        #else
          definitionFile = FS.open(definitionName, "r");
        #endif
      #else
        definitionFile.open(definitionName, std::ios::in);
      #endif
        // DEBUG_ARTI("def size %lu\n", definitionFile.tellg());
      MEMORY_ARTI("Heap open definition file > %u\n", esp_get_free_heap_size());
      if (!definitionFile) {
        ERROR_ARTI("Definition file %s not found\n", definitionName);
        return false;
      }
      else
      {
        //open definitionFile
        definitionJsonDoc = new DynamicJsonDocument(12192);

        // mandatory tokens:
        //  "ID": "ID",
        //  "INTEGER_CONST": "INTEGER_CONST",
        //  "REAL_CONST": "REAL_CONST",


        DeserializationError err = deserializeJson(*definitionJsonDoc, definitionFile);
        if (err) {
          ERROR_ARTI("deserializeJson() of definition failed with code %s\n", err.c_str());
          return false;
        }
        definitionFile.close();

        #if ARTI_PLATFORM == ARTI_ARDUINO
          #if ARTI_DEFINITION == ARTI_WLED
            programFile = WLED_FS.open(programName, "r");
          #else
            programFile = FS.open(programName, "r");
          #endif
        #else
          programFile.open(programName, std::ios::in);
        #endif
        if (!programFile) {
          ERROR_ARTI("Program file %s not found\n", programName);
          return  false;
        }
        else
        {
          //open programFile
          #if ARTI_PLATFORM == ARTI_ARDUINO
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

          char parseTreeName[fileNameLength];
          strcpy(parseTreeName, programName);
          // if (strcmp(parseOrLoad, "Parse") == 0 )
          //   strcpy(parseTreeName, "Gen");
          strcat(parseTreeName, ".json");
          #if ARTI_PLATFORM == ARTI_ARDUINO
            parseTreeJsonDoc = new DynamicJsonDocument(strlen(programText) * 50); //less memory on arduino: 32 vs 64 bit?
          #else
            parseTreeJsonDoc = new DynamicJsonDocument(strlen(programText) * 100);
          #endif

          MEMORY_ARTI("Heap DynamicJsonDocuments > %u (%lums)\n", esp_get_free_heap_size(), millis());

          //parse
          if (strcmp(parseOrLoad, "Parse") == 0) {

            lexer = new Lexer(this->programText, definitionJsonDoc->as<JsonObject>());
            parser = new Parser(this->lexer, parseTreeJsonDoc->as<JsonVariant>());
            if (!parser->parse())
              return false;;

            DEBUG_ARTI("par mem %u of %u %u %u %u %u\n", parseTreeJsonDoc->memoryUsage(), parseTreeJsonDoc->capacity(), parseTreeJsonDoc->memoryPool().capacity(), parseTreeJsonDoc->size(), parseTreeJsonDoc->overflowed(), parseTreeJsonDoc->nesting());
            DEBUG_ARTI("prog size %u factor %u\n", programFileSize, parseTreeJsonDoc->memoryUsage() / programFileSize);
            // parseTreeJsonDoc->garbageCollect();
            // DEBUG_ARTI("par mem %u of %u %u %u %u %u\n", parseTreeJsonDoc->memoryUsage(), parseTreeJsonDoc->capacity(), parseTreeJsonDoc->memoryPool().capacity(), parseTreeJsonDoc->size(), parseTreeJsonDoc->overflowed(), parseTreeJsonDoc->nesting());
            //199 -> 6905 (34.69)
            //469 -> 15923 (33.95)

            delete lexer; lexer =  nullptr;
            delete parser; parser =  nullptr;

            //write parseTree
            #if ARTI_PLATFORM == ARTI_ARDUINO
              // File parseTreeFile;
              // #if ARTI_DEFINITION == ARTI_WLED
              //   parseTreeFile = WLED_FS.open(parseTreeName, (strcmp(parseOrLoad, "Parse")==0)?"w":"r");
              // #else
              //   parseTreeFile = FS.open(parseTreeName, (strcmp(parseOrLoad, "Parse")==0)?"w":"r");
              // #endif
              // serializeJsonPretty(*parseTreeJsonDoc,  parseTreeFile);
              // parseTreeFile.close();
            #else
              std::fstream parseTreeFile;
              parseTreeFile.open(parseTreeName, std::ios::out);
              serializeJsonPretty(*parseTreeJsonDoc,  parseTreeFile);
              parseTreeFile.close();
            #endif
          }
          else
          {
            //read parseTree
            // DeserializationError err = deserializeJson(*parseTreeJsonDoc, parseTreeFile);
            // if (err) {
            //   ERROR_ARTI("deserializeJson() of parseTree failed with code %s\n", err.c_str());
            //   return false;
            // }
          }

          #if ARTI_PLATFORM == ARTI_ARDUINO //not on windows???
            free(programText);
          #endif

          MEMORY_ARTI("Heap parse > %u (%lums)\n", esp_get_free_heap_size(), millis());

        } //programFile
      } //definitionFilee
    return true;
  } // openFileAndParse

  bool analyze(const char * function_name = nullptr) {
    MEMORY_ARTI("Heap analyze < %u (%lums)\n", esp_get_free_heap_size(), millis());

    if (parseTreeJsonDoc == nullptr || parseTreeJsonDoc->isNull()) {
      ERROR_ARTI("Analyze: No parsetree created\n");
      return false;
    }
    else {
      //analyze
      semanticAnalyzer = new SemanticAnalyzer(definitionJsonDoc->as<JsonObject>(), parseTreeJsonDoc->as<JsonVariant>());
      semanticAnalyzer->analyze();

      callStack = new CallStack();
      interpreter = new Interpreter(semanticAnalyzer, callStack);
      // interpreter2 = new Interpreter2(semanticAnalyzer, callStack);

      MEMORY_ARTI("Heap analyze > %u (%lums)\n", esp_get_free_heap_size(), millis());
    }

    //flush does not seem to work... further testing needed
    #if ARTI_OUTPUT == ARTI_FILE
      #if ARTI_PLATFORM == ARTI_ARDUINO
        logFile.flush();
      #else
        fflush(logFile);
      #endif
    #endif
    return true;
  }

  bool interpret(const char * function_name = nullptr) {
    if (parseTreeJsonDoc == nullptr || parseTreeJsonDoc->isNull()) {
      ERROR_ARTI("Interpret %s: No parsetree created\n", function_name);
      return false;
    }
    else {
      interpreter->interpret(function_name);
      // interpreter2->interpret2(function_name);
    }
    return true;
  }

  void close() {
    if (semanticAnalyzer != nullptr) {delete semanticAnalyzer; semanticAnalyzer = nullptr;}
    if (callStack != nullptr) {delete callStack; callStack = nullptr;}
    if (interpreter != nullptr) {delete interpreter; interpreter = nullptr;}
    // if (interpreter2 != nullptr) {delete interpreter2; interpreter2 = nullptr;}

    if (definitionJsonDoc != nullptr) {
      DEBUG_ARTI("def mem %u of %u %u %u %u %u\n", definitionJsonDoc->memoryUsage(), definitionJsonDoc->capacity(), definitionJsonDoc->memoryPool().capacity(), definitionJsonDoc->size(), definitionJsonDoc->overflowed(), definitionJsonDoc->nesting());
      delete definitionJsonDoc; definitionJsonDoc = nullptr;
    }

    if (parseTreeJsonDoc != nullptr) {
      DEBUG_ARTI("def mem %u of %u %u %u %u %u\n", parseTreeJsonDoc->memoryUsage(), parseTreeJsonDoc->capacity(), parseTreeJsonDoc->memoryPool().capacity(), parseTreeJsonDoc->size(), parseTreeJsonDoc->overflowed(), parseTreeJsonDoc->nesting());
      delete parseTreeJsonDoc; parseTreeJsonDoc = nullptr;
    }

    #if ARTI_OUTPUT == ARTI_FILE
      #if ARTI_PLATFORM == ARTI_ARDUINO
        logFile.close();
      #else
        fclose(logFile);
      #endif
    #endif

    MEMORY_ARTI("Heap close > %u (%lums)\n", esp_get_free_heap_size(), millis());
  }

}; //ARTI

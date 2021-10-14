/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti.h
   @version 0.0.1
   @date    20211014
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
   @remarks
          - #define ARDUINOJSON_DEFAULT_NESTING_LIMIT 100 //set this in ArduinoJson!!!
   @todo
          - eexpression[""]  variables
          - *arti in segments
          - chack all uint8_t's
 */

#ifdef ESP32 //want to use a WLED variable here, but WLED_H is set in wled.h...
  #include "wled.h"
#endif

#ifdef WLED_H
  #include "src/dependencies/json/ArduinoJson-v6.h"
  #define ARTI_DEBUGORLOG 1
#else //embedded
  #define ARTI_DEBUG1 1
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

#ifdef ARTI_DEBUG1
    #define DEBUG_ARTI(...) DEBUG_ARTI0(__VA_ARGS__)
#else
    #define DEBUG_ARTI(...)
#endif

#ifdef ARTI_ERROR
    #define ERROR_ARTI(...) DEBUG_ARTI0(__VA_ARGS__)
#else
    #define ERROR_ARTI(...)
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
    uint16_t positions_index = 0;
    JsonVariant parseTreeJson;

  public:
    Token *current_token;

    Parser(Lexer *lexer, JsonVariant parseTreeJson) {
      this->lexer = lexer;
      this->parseTreeJson = parseTreeJson;

      this->current_token = this->get_next_token();
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
          // DEBUG_ARTI("%s visit element %s\n", spaces+50-depth, parseTree.as<String>().c_str());
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

  Symbol(const char * symbol_type, const char * name, const char * type = nullptr) {
    strcpy(this->symbol_type, symbol_type);
    strcpy(this->name, name);
    strcpy(this->type, (type == nullptr)?"":type);
    this->scope_level = 0;
  }
}; //Symbol

#define nrOfSymbolsPerScope 20 //add checks
#define nrOfChildScope 20 //add checks

class ScopedSymbolTable {
  private:
  public:

  Symbol* _symbols[nrOfSymbolsPerScope];
  uint16_t _symbolsIndex = 0;
  char scope_name[charLength];
  uint8_t scope_level;
  ScopedSymbolTable *enclosing_scope;
  ScopedSymbolTable *child_scopes[nrOfChildScope];
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
    JsonObject definitionJson;
    JsonVariant parseTreeJson;

    SemanticAnalyzer(JsonObject definitionJson, JsonVariant parseTreeJson) {
      this->definitionJson = definitionJson;
      this->parseTreeJson = parseTreeJson;
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
              if (expression["id"] == "Program") {
                const char * expression_name = expression["name"];
                const char * expression_block = expression["block"];
                const char * program_name = value[expression_name];
                this->global_scope = new ScopedSymbolTable(program_name, 1, nullptr); //current_scope

                DEBUG_ARTI("%s Program %s %u %u\n", spaces+50-depth, global_scope->scope_name, global_scope->scope_level, global_scope->_symbolsIndex); 

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                else {
                 // current_scope->child_scopes[current_scope->child_scopesIndex++] = this->global_scope;
                  // current_scope = global_scope;
                  visit(value[expression_block], nullptr, symbol_name, token, this->global_scope, depth + 1);
                }

                for (uint8_t i=0; i<global_scope->_symbolsIndex; i++) {
                  Symbol* symbol = global_scope->_symbols[i];
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
                function_symbol->detail_scope = function_scope;
                // DEBUG_ARTI("%s\n", "ASSIGNING ", function_symbol->name, " " , function_scope->scope_name);

                // current_scope = function_scope;
                visit(value[expression["formals"].as<const char *>()], nullptr, symbol_name, token, function_scope, depth + 1);

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, function_name, expression_block); 
                else
                  visit(value[expression_block], nullptr, symbol_name, token, function_scope, depth + 1);

                // DEBUG_ARTI("%s\n", spaces+50-depth, "end function ", symbol_name, function_scope->scope_name, function_scope->scope_level, function_scope->_symbolsIndex); 

                for (uint8_t i=0; i<function_scope->_symbolsIndex; i++) {
                  Symbol* symbol = function_scope->_symbols[i];
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
                  const char * param_name = value[expression["name"].as<const char *>()];
                  const char * expression_type = expression["type"];//.as<const char *>();
                  char param_type[charLength]; 
                  if (!value[expression_type].isNull()) {
                    serializeJson(value[expression_type], param_type); //current_scope.lookup(param.type_node.value); //need string
                  }
                  else
                    strcpy(param_type, "notype");
                  Symbol* var_symbol = new Symbol(expression["id"], param_name, param_type);
                  current_scope->insert(var_symbol);
                  DEBUG_ARTI("%s Var %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                }
              }
              else if (expression["id"] == "Assign") {
                const char * expression_name = expression["name"];
                JsonVariant left = value[expression_name]["ID"];

                const char * expression_value = expression["value"];
                char param_type[charLength]; 
                strcpy(param_type, "notype");

                // JsonVariant right = value[expression_value];

                if (!value[expression_value].isNull()) { //value assignment
                  DEBUG_ARTI("%s Assign %s = {value (not of interest during analyze...)}\n", spaces+50-depth, left.as<const char *>());
                  visit(value, expression_value, symbol_name, token, current_scope, depth + 1);
                }
                else
                  ERROR_ARTI("%s Assign %s: no value in parseTree\n", spaces+50-depth, left.as<const char *>()); 

                //is it already a defined var, if not add
                Symbol* var_symbol = current_scope->lookup(left.as<const char *>()); //lookup here and parent scopes
                if (var_symbol == nullptr) {
                  const char * param_name = left;
                  if (param_name != nullptr) {
                    var_symbol = new Symbol(expression["id"], param_name, param_type);
                    current_scope->insert(var_symbol);
                    DEBUG_ARTI("%s Var (assign) %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                  }
                  else
                    DEBUG_ARTI("%s Var (not assign) %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                }

                visitCalledAlready = true;
              }
            } // is expression["id"]

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
    char * key;
    char * value;
};

#define nrOfVariables 100

class ActivationRecord {
  private:
  public:
    char name[charLength];
    char type[charLength];
    int nesting_level;
    struct key_value *members;
    uint16_t membersCounter = 0;
    char lastSet[charLength];

    ActivationRecord(const char * name, const char * type, int nesting_level) {
        strcpy(this->name, name);
        strcpy(this->type, type);
        this->nesting_level = nesting_level;

        members = (struct key_value *)malloc(sizeof(struct key_value) * nrOfVariables);
    }

    ~ActivationRecord() {
      DEBUG_ARTI("Free activation record %s\n", name);
      free(members);
    }

    void set(const char * key, const char * value) {

      for (uint16_t i = 0; i<membersCounter; i++) {
        if (strcmp(members[i].key, key) == 0) {
          members[i].value = strdup(value);
          strcpy(lastSet, key);
          // DEBUG_ARTI("Set %s %s\n", key, value);
          return;
        }
      }

      if (membersCounter < nrOfVariables) {
        members[membersCounter].key = strdup(key);
        members[membersCounter].value = strdup(value);
        strcpy(lastSet, key);
        membersCounter++;
        // DEBUG_ARTI("Set %s %s\n", key, value);
      }
      else
        ERROR_ARTI("ActivationRecord no room for new vars\n");
    }

    const char * get(const char * key) {
      for (uint16_t i = 0; i<membersCounter; i++) {
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
        ERROR_ARTI("Push stack full %u of %u\n", stack_index, arrayLength);
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
      ERROR_ARTI("Pop stack empty\n");
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

  public:

  Interpreter(SemanticAnalyzer *analyzer) {
    this->analyzer = analyzer;

    callStack = new CallStack();
    valueStack = new ValueStack();
  }

  void interpret(const char * function_name = nullptr) {
    if (this->analyzer != nullptr) { //due to undefined functions??? wip

      DEBUG_ARTI("\ninterpret %s %u %u %u\n", analyzer->global_scope->scope_name, analyzer->global_scope->scope_level, analyzer->global_scope->_symbolsIndex, analyzer->global_scope->child_scopesIndex); 
      for (int i=0; i<analyzer->global_scope->_symbolsIndex; i++) {
        Symbol* symbol = analyzer->global_scope->_symbols[i];
        DEBUG_ARTI("scope %s %s.%s %s %u\n", symbol->symbol_type, analyzer->global_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
      }

      visit(analyzer->parseTreeJson);
    }
    else
    {
      ERROR_ARTI("\nInterpret global scope is nullptr\n");
    }

  }

            // ["PLUS2", "factor"],
          // [ "MINUS2", "factor"],

    void visit(JsonVariant parseTree, const char * treeElement = nullptr, const char * symbol_name = nullptr, const char * token = nullptr, uint8_t depth = 0) {

      // DEBUG_ARTI("%s Visit %s %s %s\n", spaces+50-depth, stringOrEmpty(treeElement), stringOrEmpty(symbol_name), stringOrEmpty(token)); //, parseTree.as<String>().c_str()

      if (parseTree.is<JsonObject>()) {
        for (JsonPair element : parseTree.as<JsonObject>()) {
        if (treeElement == nullptr || strcmp(treeElement, element.key().c_str()) == 0 ) {
          const char * key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!this->analyzer->definitionJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = this->analyzer->definitionJson["INTERPRETER"][symbol_name];
            if (!expression.isNull())
            {
              // DEBUG_ARTI("%s\n", spaces+50-depth, "Symbol ", symbol_name,  " ", expression);

              if (expression["id"] == "Program") {
                const char * expression_name = expression["name"];
                const char * expression_block = expression["block"];
                DEBUG_ARTI("%s program name %s\n", spaces+50-depth, expression_name);
                const char * program_name = value[expression_name];

                if (!value[expression_name].isNull()) {
                  ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM", 1);
                  DEBUG_ARTI("%s %s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, program_name);

                  this->callStack->push(ar);
                }
                else
                  ERROR_ARTI("program name null\n");

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                else
                  visit(value[expression_block], nullptr, symbol_name, token, depth + 1);

                if (!value[expression_name].isNull()) {
                  this->callStack->pop();
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Function") {
                const char * expression_block = expression["block"];
                const char * function_name = value[expression["name"].as<const char *>()];
                Symbol* function_symbol = this->analyzer->global_scope->lookup(function_name, true, true);
                DEBUG_ARTI("%s Save block of %s\n", spaces+50-depth, function_name);
                function_symbol->block = value[expression_block];
                visitCalledAlready = true;
              }
              else if (expression["id"] == "Call") {
                const char * function_name = value[expression["name"].as<const char *>()];
                Symbol* function_symbol = this->analyzer->global_scope->lookup(function_name, true, true);

                if (function_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                  ActivationRecord* ar = new ActivationRecord(function_name, "Function", function_symbol->scope_level + 1);

                  DEBUG_ARTI("%s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), function_name);

                  uint8_t oldIndex = valueStack->stack_index;
                  uint8_t lastIndex = valueStack->stack_index;

                  visit(value[expression["actuals"].as<const char *>()], nullptr, symbol_name, token, depth + 1);

                  for (int i=0; i<function_symbol->detail_scope->_symbolsIndex; i++) { //backwards because popped in reversed order
                    if (strcmp(function_symbol->detail_scope->_symbols[i]->symbol_type, "Formal") == 0) { //select formal parameters
                      const char * result = this->valueStack->stack[lastIndex++];
                      ar->set(function_symbol->detail_scope->_symbols[i]->name, result);
                      DEBUG_ARTI("%s Actual %s.%s = %s (pop)\n", spaces+50-depth, function_name, function_symbol->detail_scope->_symbols[i]->name, result);
                    }
                  }

                  valueStack->stack_index = oldIndex;

                  if (strcmp(function_name, "setPixelColor") == 0) {
                    DEBUG_ARTI("%s special function  %s\n", spaces+50-depth, function_name);
                    #ifdef WLED_H
                      uint16_t par1 = atoi(ar->get(function_symbol->detail_scope->_symbols[0]->name));
                      uint16_t par2 = atoi(ar->get(function_symbol->detail_scope->_symbols[1]->name));
                      Serial.printf("setPixelColor %u %u\n", par1, par2);
                      strip.setPixelColor(par1, strip.color_wheel(par2));

                    #endif
                  }
                  else {
                    this->callStack->push(ar);

                    //find block of function... lookup function?
                    //visit block of function
                    // DEBUG_ARTI("%s function block %s\n", spaces+50-depth, function_symbol->block.as<String>().c_str());

                    visit(function_symbol->block, nullptr, symbol_name, token, depth + 1);

                    this->callStack->pop();

                    char cr[charLength] = "CallResult tbd of ";
                    strcat(cr, function_name);
                    valueStack->push("Call", cr);

                  }

                  visitCalledAlready = true;
                } //function_symbol != nullptr
                else {
                  DEBUG_ARTI("%s %s not found %s\n", spaces+50-depth, expression["id"].as<const char *>(), function_name);
                }

              }
              else if (expression["id"] == "Assign") {
                const char * expression_value = expression["value"];
                const char * expression_indices = expression["indices"]; //array indices
                const char * expression_name = expression["name"];

                if (!value[expression_value].isNull()) { //value assignment
                  visit(value, expression["value"], symbol_name, token, depth + 1); //value pushed

                  char indices[charLength];
                  strcpy(indices, "");
                  if (!value[expression_indices].isNull()) {
                    strcat(indices, "[");
                    uint8_t oldIndex = valueStack->stack_index;
                    visit(value, expression["indices"], symbol_name, token, depth + 1); //value pushed
                    char sep[3] = "";
                    for (uint8_t i = oldIndex; i< valueStack->stack_index; i++) {
                      strcat(indices, sep);
                      strcat(indices, this->valueStack->stack[i]);
                      strcpy(sep, ",");
                    }
                    valueStack->stack_index = oldIndex;
                    strcat(indices, "]");
                  }

                  ActivationRecord* ar = this->callStack->peek();
                  ar->set(value[expression_name]["ID"], this->valueStack->pop());

                  DEBUG_ARTI("%s %s.%s%s := %s (pop)\n", spaces+50-depth, ar->name, value[expression_name]["ID"].as<const char *>(), indices, ar->get(value[expression_name]["ID"]));
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Exprs" || expression["id"] == "Terms") {
                // DEBUG_ARTI("%s %s tovisit %s\n", spaces+50-depth, expression["id"].as<const char *>(), value.as<String>().c_str());
                if (value.is<JsonArray>()) {
                  JsonArray valueArray = value.as<JsonArray>();

                  if (valueArray.size() >= 1) // visit first symbol 
                    visit(valueArray[0], nullptr, symbol_name, token, depth + 1); //pushes result

                  // assuming expression contains 1 operand and 2 operators
                  if (valueArray.size() >= 3) { // add operator and another symbol
                    char operatorx[charLength];
                    serializeJson(valueArray[1], operatorx);
                    // DEBUG_ARTI("%s %s operator %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), operatorx, value.as<String>().c_str());
                    visit(valueArray[2], nullptr, symbol_name, token, depth + 1); //pushes result

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
              else if (expression["id"] == "Var") {
                const char * expression_name = expression["name"]; //ID (key="varref")
                const char * variable_name = value[expression_name];

                ActivationRecord* ar = this->callStack->peek();
                const char * varValue = ar->get(variable_name);

                if (strcmp(varValue, "empty") == 0) {
                  //find the scope level of the variable (is this not working from global down instead from current up??? in other words global vars with same name precede local vars...)
                  Symbol* function_symbol = this->analyzer->global_scope->lookup(variable_name, true, true);
                  //calculate the index in the call stack to find the right ar
                  uint8_t index = this->callStack->recordsCounter - 1 - (this->callStack->peek()->nesting_level - function_symbol->scope_level);
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
                visit(value, expression["from"], symbol_name, token, depth + 1); //creates the assignment
                ActivationRecord* ar = this->callStack->peek();
                const char * fromVarName = ar->lastSet;

                bool continuex = true;
                uint16_t counter = 0;
                while (continuex && counter < 1000) { //to avoid endless loops
                  DEBUG_ARTI("%s iteration\n", spaces+50-depth);

                  DEBUG_ARTI("%s check to condition\n", spaces+50-depth);
                  visit(value, expression["to"], symbol_name, token, depth + 1); //pushes result of to

                  const char * toResult = valueStack->pop();

                  if (strcmp(toResult, "1") == 0) { //toResult is true
                    DEBUG_ARTI("%s 1 => run block\n", spaces+50-depth);
                    visit(value[expression_block], nullptr, symbol_name, token, depth + 1);

                    DEBUG_ARTI("%s assign next value\n", spaces+50-depth);
                    visit(value[expression["increment"].as<const char *>()], nullptr, symbol_name, token, depth + 1); //pushes increment result
                    // #ifdef ESP32
                    //   Serial.printf("Iteration %u\n", counter);
                    // #endif
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
                        visit(value[expression_block], nullptr, symbol_name, token, depth + 1);

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
              }
            } //if expression["id"]
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
            visit(value, nullptr, symbol_name, token, depth + 1);
        }
        } // for (JsonPair
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
            visit(newParseTree, nullptr, symbol_name, token, depth + 1);
          }
        }
        else { //not array
          // const char * element = parseTree.as<const char *>();
          // DEBUG_ARTI("%s\n", spaces+50-depth, "not array not object but element ", element);
        }
      }
    }

};

#define programTextSize 1000

class ARTI {
private:
  Lexer *lexer;
  Parser *parser;
  SemanticAnalyzer *semanticAnalyzer = nullptr;
  TreeWalker *treeWalker;
  Interpreter *interpreter;
  #ifdef ESP32
    char * programText;
    File definitionFile;
    File programFile;
    File parseTreeFile;
  #else
    char programText[programTextSize];
    std::fstream definitionFile;
    std::fstream programFile;
    std::fstream parseTreeFile;
  #endif
  uint16_t programFileSize;

  DynamicJsonDocument *definitionJson;
  DynamicJsonDocument *parseTreeJson;
public:
  ARTI() {

    // char byte = charFromProgramFile();
    // while (byte != -1) {
    //   programText += byte;
    //   DEBUG_ARTI("%c", byte);
    //   byte = charFromProgramFile();
    // }

  }

  void openFileAndParse(const char *definitionName, const char *programName) {
    // #ifdef ESP32
    //   Serial.printf("begin millis %lu\n", millis());
    // #endif
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
          logFile = WLED_FS.open(logFileName,"w");
        #else
          logFile = fopen (logFileName,"w");
        #endif
      #endif

      #ifdef ESP32
        definitionFile = WLED_FS.open(definitionName, "r");
        DEBUG_ARTI("RAM left %u\n", esp_get_free_heap_size());
      #else
        definitionFile.open(definitionName, std::ios::in);
        // DEBUG_ARTI("def size %lu\n", definitionFile.tellg());
      #endif
      if (!definitionFile)
        ERROR_ARTI("Definition file %s not found\n", definitionName);
      else
      {
        //open definitionFile
        definitionJson = new DynamicJsonDocument(8192);
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
          programFile = WLED_FS.open(programName, "r");
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
            programFile.read((uint8_t*)programText, programFileSize);
            programText[programFileSize] = '\0';
          #else
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
            parseTreeFile = WLED_FS.open(parseTreeName, (strcmp(parseOrLoad, "Parse")==0)?"w":"r");
            parseTreeJson = new DynamicJsonDocument(strlen(programText) * 25); //why less memory on esp32???
          #else
            parseTreeFile.open(parseTreeName, std::ios::out);
            parseTreeJson = new DynamicJsonDocument(strlen(programText) * 50);
          #endif

    // #ifdef ESP32
    //   Serial.printf("loaded millis %lu\n", millis());
    // #endif

          //parse
          if (strcmp(parseOrLoad, "Parse") == 0) {

            lexer = new Lexer(this->programText, definitionJson->as<JsonObject>());
            parser = new Parser(this->lexer, parseTreeJson->as<JsonVariant>());
            parser->parse();

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

    // #ifdef ESP32
    //   Serial.printf("parsed millis %lu\n", millis());
    // #endif
          // char resultString[standardStringLenght];
          // // strcpy(resultString, "");
          // arti.walk(parseTreeJson->as<JsonVariant>(), resultString);
            // DEBUG_ARTI("walk result %s", resultString);

          //analyze
          semanticAnalyzer = new SemanticAnalyzer(definitionJson->as<JsonObject>(), parseTreeJson->as<JsonVariant>());
          semanticAnalyzer->analyze();
    // #ifdef ESP32
    //   Serial.printf("analyzed millis %lu\n", millis());
    // #endif

        } //programFile
      } //definitionFilee

  } // openFileAndParse

    void walk(JsonVariant tree, char * resultString) {
    treeWalker = new TreeWalker();
    // DeserializationError err = deserializeJson(definitionJson, definitionText);
    // if (err) {
    //   DEBUG_ARTI("deserializeJson() in walk failed with code %s\n", err.c_str());
    // }
    // DEBUG_ARTI("%s\n", definitionText.c_str());
    treeWalker->walk(tree, resultString);
  }

  void interpret(const char * function_name = nullptr) {
    interpreter = new Interpreter(semanticAnalyzer);
    interpreter->interpret(function_name);
    //   #ifdef ESP32
    //   Serial.printf("interpreted millis %lu\n", millis());
    // #endif
}

  void close() {
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
    // #ifdef ESP32
    //   Serial.printf("closed millis %lu\n", millis());
    // #endif
  }

}; //ARTI

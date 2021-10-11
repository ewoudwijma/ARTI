#define ARTI_DEBUG1 1
#define ARTI_ERROR 1
// #define ARTI_DEBUGORLOG 1

// #define ARDUINOJSON_DEFAULT_NESTING_LIMIT 100 //set this in ArduinoJson!!!

#define charLength 30
#define arrayLength 30

using namespace std;

#ifdef WLED_H
  #include "src/dependencies/json/ArduinoJson-v6.h"
#else
  #include "ArduinoJson-recent.h"
#endif

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
    #define DEBUG_ARTI1(...) DEBUG_ARTI0(__VA_ARGS__)
#else
    #define DEBUG_ARTI1(...)
#endif

#ifdef ARTI_ERROR
    #define ERROR_ARTI(...) DEBUG_ARTI0(__VA_ARGS__)
#else
    #define ERROR_ARTI(...)
#endif

#ifndef ESP32
  #include <iostream>
  #include <fstream>
  #include <sstream>
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

#ifdef ESP32
  const char spaces[51] PROGMEM = "                                                  ";
#else
  const char spaces[51]         = "                                                  ";
#endif
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
      // DEBUG_ARTI1("%s\n", "Lexer init");
      this->text = programText;
      this->definitionJson = definitionJson;
      this->pos = 0;
      this->current_char = this->text[this->pos];
      this->lineno = 1;
      this->column = 1;
    }

    void error() {
      ERROR_ARTI("Lexer error on %c line %d col %d\n", this->current_char, this->lineno, this->column);
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

      // DEBUG_ARTI1("%s\n", "Number!!! ", token->type, token->value);
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

        // DEBUG_ARTI1("upper %s [%s] [%s]\n", definitionJson["TOKENS"][resultUpper].as<const char *>(), result, resultUpper);
        if (definitionJson["TOKENS"][resultUpper].isNull()) {
            // DEBUG_ARTI1("%s\n", "  id empty ");
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
        // DEBUG_ARTI1("get_next_token %c\n", this->current_char);
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

        // DEBUG_ARTI1("%s\n", "get_next_token (", token_type, ") (", token_value, ")");
        if (strcmp(token_type, "") != 0 && strcmp(token_value, "") != 0) {
          // DEBUG_ARTI1("%s\n", "get_next_token tvinn", token_type, token_value);
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

      DEBUG_ARTI1("Parser %s %s\n", this->current_token->type, this->current_token->value);

      const char * symbol_name = it->key().c_str();
      Result result = visit(parseTreeJson, symbol_name, nullptr, lexer->definitionJson[symbol_name], 0);

      if (this->lexer->pos != strlen(this->lexer->text))
        ERROR_ARTI("Symbol %s Program not entirely parsed (%d,%d) %d of %d\n", symbol_name, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
      else if (result == Result::RESULTFAIL)
        ERROR_ARTI("Symbol %s Program parsing failed (%d,%d) %d of %d\n", symbol_name, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
      else
        DEBUG_ARTI1("Symbol %s Parsed until (%d,%d) %d of %d\n", symbol_name, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text));
    }

    Token *get_next_token() {
      return this->lexer->get_next_token();
    }

    void error(ErrorCode error_code, Token *token) {
      ParserError(error_code, token, "ParserError"); //ParserError error = 
    }

    void eat(const char * token_type) {
      // DEBUG_ARTI1("try to eat %s %s\n", this->current_token->type, token_type);
      if (strcmp(this->current_token->type, token_type) == 0) {
        this->current_token = this->get_next_token();
        // DEBUG_ARTI1("eating %s -> %s %s\n", token_type, this->current_token->type, this->current_token->value);
      }
      else {
        this->error(ErrorCode::UNEXPECTED_TOKEN, this->current_token);
      }
    }

  void push_position() {
    // DEBUG_ARTI1("%s\n", "push_position ", positions_index, this->lexer.pos);
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
      ERROR_ARTI("not enough positions %d\n", nrOfPositions);
  }

  void pop_position() {
    if (positions_index > 0) {
      positions_index--;
      // DEBUG_ARTI1("%s\n", "pop_position ", positions_index, this->lexer->pos, " to ", positions[positions_index].pos);
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

  Result visit(JsonVariant parseTree, const char * symbol_name, const char * operatorx, JsonVariant expression, int depth = 0) {
    if (depth > 50) {
      ERROR_ARTI("Error too deep %d\n", depth);
      errorOccurred = true;
    }
    if (errorOccurred) return Result::RESULTFAIL;

    Result result = Result::RESULTCONTINUE;

    // DEBUG_ARTI1("%s Visit %s %s\n", spaces+50-depth, stringOrEmpty(symbol_name), stringOrEmpty(operatorx)); //, expression.as<String>().c_str()

    if (expression.is<JsonObject>()) {
      // DEBUG_ARTI1("%s visit Object %s %s %s\n", spaces+50-depth, symbol_name, operatorx, expression.as<String>().c_str());

      for (JsonPair element : expression.as<JsonObject>()) {
        const char * objectOperator = element.key().c_str();
        JsonVariant objectExpression = element.value();

        //and: see 'is array'
        if (strcmp(objectOperator, "or") == 0) {
          // DEBUG_ARTI1("%s\n", "or ");
          result = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1);
          if (result != Result::RESULTFAIL) result = Result::RESULTCONTINUE;
        }
        else {
          Result resultChild = Result::RESULTCONTINUE;
          uint8_t counter = 0;
          while (resultChild == Result::RESULTCONTINUE) {
            // DEBUG_ARTI1("Before %d (%d,%d) %d of %d %s\n", resultChild, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text), objectExpression.as<String>().c_str());
            resultChild = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1); //no assign to result as optional
            // DEBUG_ARTI1("After %d (%d,%d) %d of %d %s\n", resultChild, this->lexer->lineno, this->lexer->column, this->lexer->pos, strlen(this->lexer->text), objectExpression.as<String>().c_str());

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
        // DEBUG_ARTI1("%s visit Array %s %s %s\n", spaces+50-depth, symbol_name, operatorx, expression.as<String>().c_str());

        Result resultChild;

        if (operatorx == nullptr) 
          operatorx = "and";

        //check if unary or binary operator
        // if (expression.size() > 1) {
        //   DEBUG_ARTI1("%s\n", "array multiple 1 ", parseTree);
        //   DEBUG_ARTI1("%s\n", "array multiple 2 ", expression);
        // }

        for (JsonVariant newExpression: expression.as<JsonArray>()) {
          //Save current position, in case some of the expressions in the or array go wrong (deadend), go back to the saved position and try the next
          if (strcmp(operatorx , "or") == 0)
            push_position();

          resultChild = visit(parseTree, symbol_name, nullptr, newExpression, depth + 1);//(operatorx == "")?"and":operatorx
          // DEBUG_ARTI1("%s visited Array element %d %s \n", spaces+50-depth, resultChild, newExpression.as<String>().c_str());

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

        // DEBUG_ARTI1("%s visited Array %d %s %s %s\n", spaces+50-depth, result, symbol_name, operatorx, expression.as<String>().c_str());
      }
      else { //not array
        const char * token_type = expression;

        //if token
        if (!lexer->definitionJson["TOKENS"][token_type].isNull()) {

          if (strcmp(this->current_token->type, token_type) == 0) {
            // DEBUG_ARTI1("%s visit token %s %s\n", spaces+50-depth, this->current_token->type, token_type);//, expression.as<String>().c_str());
            // if (current_token->type == "ID" || current_token->type == "INTEGER" || current_token->type == "REAL" || current_token->type == "INTEGER_CONST" || current_token->type == "REAL_CONST" || current_token->type == "ID" || current_token->type == "ID" || current_token->type == "ID") {
            if (current_token != nullptr) {
              DEBUG_ARTI1("%s %s %s", spaces+50-depth, current_token->type, current_token->value);
            }

            if (symbol_name[strlen(symbol_name)-1] == '*') { //if list then add in array
              JsonArray arr = parseTree[symbol_name].as<JsonArray>();
              arr[arr.size()][current_token->type] = current_token->value; //add in last element of array
            }
            else
                parseTree[symbol_name][current_token->type] = current_token->value;

            if (strcmp(token_type, "PLUS2") == 0) { //debug for unary operators (wip)
              // DEBUG_ARTI1("%s\n", "array multiple 1 ", parseTree);
              // DEBUG_ARTI1("%s\n", "array multiple 2 ", expression);
            }

            eat(token_type);

            if (current_token != nullptr) {
              DEBUG_ARTI1(" -> [%s %s]\n", current_token->type, current_token->value);
            }
            else
              DEBUG_ARTI1("\n");
          }
          else {
            // DEBUG_ARTI1("%s visit deadend %s %s\n", spaces+50-depth, this->current_token->type, token_type);//, expression.as<String>().c_str());
            // parseTree["deadend"] = token_type + "<>" + current_token->type;
            result = Result::RESULTFAIL;
          }
        }
        else { //not object, array or token but symbol
          const char * newSymbol_name = expression;
          JsonVariant newParseTree;

          DEBUG_ARTI1("%s %s\n", spaces+50-depth, newSymbol_name);
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

          // DEBUG_ARTI1("%s %s\n", spaces+50-depth, newSymbol_name);
          result = visit(newParseTree, newSymbol_name, nullptr, lexer->definitionJson[newSymbol_name], depth + 1);

          newParseTree.remove("ccc"); //remove connector

          if (result == Result::RESULTFAIL) {
            DEBUG_ARTI1("%s fail %s\n", spaces+50-depth, newSymbol_name);
            newParseTree.remove(newSymbol_name); //remove result of visit

            //   DEBUG_ARTI1("%s psf %s %s\n", spaces+50-depth, parseTree[symbol_name].as<const char *>(), newSymbol_name.c_str());
            if (symbol_name[strlen(symbol_name)-1] == '*') //if list then remove empty objecy
              arr.remove(arr.size()-1);
            // else
            //   parseTree.remove(newSymbol_name); //this does not change anything...
          }
          else {
            DEBUG_ARTI1("%s success %s\n", spaces+50-depth, newSymbol_name);
            // DEBUG_ARTI1("%s success %s %s\n", spaces+50-depth, newSymbol_name, parseTree.as<string>().c_str());
          }
          // DEBUG_ARTI1("%s %s\n", spaces+50-depth, parseTree[symbol_name].as<const char *>());

        } //symbol
      } //if array
    } //if object
    // DEBUG_ARTI1("%s\n", spaces+50-depth, "tokenValue ", tokenValue, isArray, isToken, isObject);
    
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
      DEBUG_ARTI1("\nWalker\n");
      visit(tree, resultString);
    }

    void visit(JsonVariant parseTree, char * resultString, uint8_t depth = 0) {

      if (parseTree.is<JsonObject>()) {
        concatenate(resultString, spaces, depth); concatenate(resultString,"{\n");
        for (JsonPair element : parseTree.as<JsonObject>()) {
          const char * key = element.key().c_str();
          JsonVariant value = element.value();
          concatenate(resultString, spaces, depth); concatenate(resultString, key); concatenate(resultString, "=\n");
          // DEBUG_ARTI1("%s Visit object %s %s\n", spaces+50-depth, symbol_name.c_str(), token.c_str());
          visit(value, resultString, depth + 1);
        } // key values
        concatenate(resultString, spaces, depth); concatenate(resultString,"}\n");
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          concatenate(resultString, spaces, depth); concatenate(resultString,"[\n");
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI1("%s Visit array %s %s\n", spaces+50-depth, symbol_name.c_str(), token.c_str());
            // DEBUG_ARTI1("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
            visit(newParseTree, resultString, depth + 1);
          }
          concatenate(resultString, spaces, depth); concatenate(resultString,"]\n");
        }
        else { //not array
          const char * temp = parseTree.as<const char *>();
          concatenate(resultString, spaces, depth); concatenate(resultString, temp); concatenate(resultString,  "\n");
          // strcat(resultString, parseTree.as<const char *>()); strcat(resultString, "\n");
          // DEBUG_ARTI1("%s visit element %s\n", spaces+50-depth, parseTree.as<String>().c_str());
        }
      }
      // DEBUG_ARTI1("%s", localResult);
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
  int scope_level;
  ScopedSymbolTable *enclosing_scope;
  ScopedSymbolTable *child_scopes[nrOfChildScope];
  uint16_t child_scopesIndex = 0;

  ScopedSymbolTable(const char * scope_name, int scope_level, ScopedSymbolTable *enclosing_scope = nullptr) {
    // DEBUG_ARTI1("%s\n", "ScopedSymbolTable ", scope_name, scope_level);
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
      DEBUG_ARTI1("Log scope Insert %s\n", symbol->name.c_str());
    #endif
    symbol->scope_level = this->scope_level;
    symbol->scope = this;
    this->_symbols[_symbolsIndex] = symbol;
    _symbolsIndex++;
  }

  Symbol* lookup(const char * name, bool current_scope_only=false, bool child_scopes_included=false) {
    // this->log("Lookup: " + name + " " + this->scope_name);
    // DEBUG_ARTI1("%s\n", "lookup ", name, this->scope_name, _symbolsIndex, child_scopesIndex);
    //  'symbol' is either an instance of the Symbol class or None;
    for (int i=0; i<_symbolsIndex; i++) {
      // DEBUG_ARTI1("%s\n", "  symbols ", i, _symbols[i]->symbol_type, _symbols[i]->name, _symbols[i]->type, _symbols[i]->scope_level);
      if (strcmp(_symbols[i]->name, name) == 0) //replace with strcmp!!!
        return _symbols[i];
    }

    if (child_scopes_included) {
      for (int i=0; i<this->child_scopesIndex;i++) {
        // DEBUG_ARTI1("%s\n", "  detail ", i, this->child_scopes[i]->scope_name);
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

    // DEBUG_ARTI1("%s\n", spaces+50-depth, "show ", this->scope_name, " " , this->scope_level);
    for (int i=0; i<_symbolsIndex; i++) {
      // DEBUG_ARTI1("%s\n", spaces+50-depth, "-symbols ", i, _symbols[i]->symbol_type, _symbols[i]->name, _symbols[i]->type);
    }

      for (int i=0; i<this->child_scopesIndex;i++) {
        // DEBUG_ARTI1("%s\n", spaces+50-depth, "-detail ", i, this->child_scopes[i]->scope_name);
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
      DEBUG_ARTI1("\nAnalyzer\n");
      visit(parseTreeJson);
    }

    void visit(JsonVariant parseTree, const char * treeElement = nullptr, const char * symbol_name = nullptr, const char * token = nullptr, ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) {

      if (parseTree.is<JsonObject>()) {
        // DEBUG_ARTI1("%s Visit object %s %s %s\n", spaces+50-depth, stringOrEmpty(treeElement), stringOrEmpty(symbol_name), stringOrEmpty(token)); //, parseTree.as<String>().c_str()

        for (JsonPair element : parseTree.as<JsonObject>()) {
        if (treeElement == nullptr || strcmp(treeElement, element.key().c_str()) == 0 ) {
          const char * key = element.key().c_str();
          JsonVariant value = element.value();

          // DEBUG_ARTI1("%s Visit element %s %s\n", spaces+50-depth, key, value.as<string>().c_str());
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

                DEBUG_ARTI1("%s Program %s %d %d\n", spaces+50-depth, global_scope->scope_name, global_scope->scope_level, global_scope->_symbolsIndex); 

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                else {
                 // current_scope->child_scopes[current_scope->child_scopesIndex++] = this->global_scope;
                  // current_scope = global_scope;
                  visit(value[expression_block], nullptr, symbol_name, token, this->global_scope, depth + 1);
                }

                for (int i=0; i<global_scope->_symbolsIndex; i++) {
                  Symbol* symbol = global_scope->_symbols[i];
                  DEBUG_ARTI1("%s %d %s %s.%s %s %d\n", spaces+50-depth, i, symbol->symbol_type, global_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Function") {

                //find the function name (so we must know this is a function...)
                const char * expression_name = expression["name"];//.as<const char *>();
                const char * expression_block = expression["block"];
                const char * proc_name = value[expression_name];
                Symbol* proc_symbol = new Symbol(expression["id"], proc_name);
                current_scope->insert(proc_symbol);

                DEBUG_ARTI1("%s Function %s.%s\n", spaces+50-depth, current_scope->scope_name, proc_name);
                ScopedSymbolTable* function_scope = new ScopedSymbolTable(proc_name, current_scope->scope_level + 1, current_scope);
                current_scope->child_scopes[current_scope->child_scopesIndex++] = function_scope;
                proc_symbol->detail_scope = function_scope;
                // DEBUG_ARTI1("%s\n", "ASSIGNING ", proc_symbol->name, " " , function_scope->scope_name);

                // current_scope = function_scope;
                visit(value[expression["formals"].as<const char *>()], nullptr, symbol_name, token, function_scope, depth + 1);

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, proc_name, expression_block); 
                else
                  visit(value[expression_block], nullptr, symbol_name, token, function_scope, depth + 1);

                // DEBUG_ARTI1("%s\n", spaces+50-depth, "end proc ", symbol_name, function_scope->scope_name, function_scope->scope_level, function_scope->_symbolsIndex); 

                for (int i=0; i<function_scope->_symbolsIndex; i++) {
                  Symbol* symbol = function_scope->_symbols[i];
                  DEBUG_ARTI1("%s %d %s %s.%s %s %d\n", spaces+50-depth, i, symbol->symbol_type, function_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Var" || expression["id"] == "Formal") {
                // DEBUG_ARTI1("%s var (from array) %s %s %s\n", spaces+50-depth, current_scope->scope_name.c_str(), value.as<const char *>().c_str(), key.c_str());
                //can be expression or array of expressions
                if (value.is<JsonArray>()) {
                  ERROR_ARTI("Var or formal should not be an array %s\n", value.as<string>().c_str());
                  // for (JsonObject newValue: value.as<JsonArray>()) {
                  //   const char * param_name = newValue[expression["name"].as<const char *>()];
                  //   char param_type[charLength]; strcpy(param_type, newValue[expression["type"].as<const char *>()].as<string>().c_str());//current_scope.lookup(param.type_node.value); //need string
                  //   Symbol* var_symbol = new Symbol(expression["id"], param_name, param_type);
                  //   current_scope->insert(var_symbol);
                  //   DEBUG_ARTI1("%s var (from array) %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                  // }
                }
                else {
                  const char * param_name = value[expression["name"].as<const char *>()];
                  const char * expression_type = expression["type"];//.as<const char *>();
                  char param_type[charLength]; 
                  if (!value[expression_type].isNull())
                    strcpy(param_type, value[expression_type].as<string>().c_str());//current_scope.lookup(param.type_node.value); //need string
                  else
                    strcpy(param_type, "notype");
                  Symbol* var_symbol = new Symbol(expression["id"], param_name, param_type);
                  current_scope->insert(var_symbol);
                  DEBUG_ARTI1("%s Var %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
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
                  DEBUG_ARTI1("%s Assign %s = {value (not of interest during analyze...) %s}\n", spaces+50-depth, left.as<const char *>(), value[expression_value].as<string>().c_str());
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
                    DEBUG_ARTI1("%s Var (assign) %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                  }
                  else
                    DEBUG_ARTI1("%s Var (not assign) %s.%s of %s\n", spaces+50-depth, current_scope->scope_name, param_name, param_type);
                }

                visitCalledAlready = true;
              }
            } // is expression["id"]

          } // is symbol_name

          if (!this->definitionJson["TOKENS"][key].isNull()) {
            token = key;
            // DEBUG_ARTI1("%s\n", spaces+50-depth, "Token ", token);
          }
          // DEBUG_ARTI1("%s Object %s %d\n", spaces+50-depth, key, value, visitCalledAlready);

          if (!visitCalledAlready)
            visit(value, nullptr, symbol_name, token, current_scope, depth + 1);

        } // key values
        }
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          // DEBUG_ARTI1("%s Visit array %s %s %s\n", spaces+50-depth, stringOrEmpty(treeElement), stringOrEmpty(symbol_name), stringOrEmpty(token)); //, parseTree.as<String>().c_str()
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI1("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
            visit(newParseTree, nullptr, symbol_name, token, current_scope, depth + 1);
          }
        }
        else { //not array
          // string element = parseTree;
          // DEBUG_ARTI1("%s value notnot %s\n", spaces+50-depth, parseTree.as<string>().c_str());
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
      mem[key] = value;
    }

    const char * get(const char * key) {
      if (mem[key].isNull())
        return nullptr;
      else
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
      // DEBUG_ARTI1("%s\n", "Push ", ar->name);
        this->_records[_recordsCounter++] = ar;
    }

    ActivationRecord* pop() {
      // DEBUG_ARTI1("%s\n", "Pop ", this->peek()->name);
        return this->_records[_recordsCounter--];
    }

    ActivationRecord* peek() {
        return this->_records[_recordsCounter-1];
    }
}; //CallStack

class Calculator {
private:
public:
  char stack[arrayLength][charLength];
  uint8_t stack_index = 0;

  Calculator() {
  }

  void push(const char * key, const char * value) {
    if (stack_index < arrayLength && value != nullptr) {
      // DEBUG_ARTI1("calc push %s %s\n", key, value);
      strcpy(stack[stack_index++], value);
    }
    else
      ERROR_ARTI("Calc stack full %d of %d or value is null\n", stack_index, arrayLength);
  }

  const char * peek() {
    // DEBUG_ARTI1("Calc Peek %s\n", stack[stack_index-1]);
    return stack[stack_index-1];
  }

  const char * pop() {
    if (stack_index>0) {
      stack_index--;
      return stack[stack_index];
    }
    else {
      ERROR_ARTI("Calc stack empty\n");
    // DEBUG_ARTI1("Calc Pop %s\n", stack[stack_index]);
      return "novalue";
    }
  }

}; //Calculator

class Interpreter {
  private:
  CallStack *call_stack;
  SemanticAnalyzer *analyzer;
  Calculator *calculator;

  public:

  Interpreter(SemanticAnalyzer *analyzer) {
    this->analyzer = analyzer;

    call_stack = new CallStack();
    calculator = new Calculator();
  }

  void interpret() {
    if (this->analyzer != nullptr) { //due to undefined functions??? wip

      DEBUG_ARTI1("\ninterpret %s %d %d %d\n", analyzer->global_scope->scope_name, analyzer->global_scope->scope_level, analyzer->global_scope->_symbolsIndex, analyzer->global_scope->child_scopesIndex); 
      for (int i=0; i<analyzer->global_scope->_symbolsIndex; i++) {
        Symbol* symbol = analyzer->global_scope->_symbols[i];
        DEBUG_ARTI1("scope %s %s.%s %s %d\n", symbol->symbol_type, analyzer->global_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
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

      // DEBUG_ARTI1("%s Visit %s %s %s\n", spaces+50-depth, stringOrEmpty(treeElement), stringOrEmpty(symbol_name), stringOrEmpty(token)); //, parseTree.as<String>().c_str()

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
              // DEBUG_ARTI1("%s\n", spaces+50-depth, "Symbol ", symbol_name,  " ", expression);

              if (expression["id"] == "Program") {
                const char * expression_name = expression["name"];
                const char * expression_block = expression["block"];
                DEBUG_ARTI1("%s program name %s\n", spaces+50-depth, expression_name);
                const char * program_name = value[expression_name];

                if (!value[expression_name].isNull()) {
                  ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM",1);
                  DEBUG_ARTI1("%s %s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, program_name);

                  this->call_stack->push(ar);
                }
                else
                  ERROR_ARTI("program name null\n");

                if (value[expression_block].isNull())
                  ERROR_ARTI("%s Program %s: no block %s in parseTree\n", spaces+50-depth, program_name, expression_block); 
                else
                  visit(value[expression_block], nullptr, symbol_name, token, depth + 1);

                if (!value[expression_name].isNull()) {
                  this->call_stack->pop();
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Function") {
                const char * expression_block = expression["block"];
                const char * proc_name = value[expression["name"].as<const char *>()]; //as string is needed!!!
                Symbol* proc_symbol = this->analyzer->global_scope->lookup(proc_name, true, true);
                DEBUG_ARTI1("%s Save block of %s\n", spaces+50-depth, proc_name);
                proc_symbol->block = value[expression_block];
                visitCalledAlready = true;
              }
              else if (expression["id"] == "Call") {
                const char * proc_name = value[expression["name"].as<const char *>()]; //as string is needed!!!
                Symbol* proc_symbol = this->analyzer->global_scope->lookup(proc_name, true, true);

                if (proc_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                  ActivationRecord* ar = new ActivationRecord(proc_name, "Function", proc_symbol->scope_level + 1);

                  DEBUG_ARTI1("%s %s %s %s %s\n", spaces+50-depth, expression["id"].as<const char *>(), ar->name, proc_name, proc_symbol->name);

                  visit(value[expression["actuals"].as<const char *>()], nullptr, symbol_name, token, depth + 1);

                  for (int i=proc_symbol->detail_scope->_symbolsIndex-1; i>=0;i--) { //backwards because popped in reversed order
                    if (strcmp(proc_symbol->detail_scope->_symbols[i]->symbol_type, "Formal") == 0) { //select formal parameters
                      const char * result = calculator->pop();
                      ar->set(proc_symbol->detail_scope->_symbols[i]->name, result);
                      DEBUG_ARTI1("%s Actual %s = %s (pop)\n", spaces+50-depth, proc_symbol->detail_scope->_symbols[i]->name, result);
                    }
                  }

                  this->call_stack->push(ar);

                  //find block of function... lookup function?
                  //visit block of function
                  // DEBUG_ARTI1("%s proc block %s\n", spaces+50-depth, proc_symbol->block.as<string>().c_str());

                  visit(proc_symbol->block, nullptr, symbol_name, token, depth + 1);

                  this->call_stack->pop();

                  char cr[charLength] = "CallResult tbd of ";
                  strcat(cr, proc_name);
                  calculator->push("Call", cr);

                  visitCalledAlready = true;
                } //proc_symbol != nullptr
                else {
                  DEBUG_ARTI1("%s %s not found %s\n", spaces+50-depth, expression["id"].as<const char *>(), proc_name);
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
                    uint8_t oldIndex = calculator->stack_index;
                    visit(value, expression["indices"], symbol_name, token, depth + 1); //value pushed
                    char sep[3] = "";
                    for (uint8_t i = oldIndex; i<=calculator->stack_index; i++) {
                      strcat(indices, sep);
                      strcat(indices, this->calculator->stack[i]);
                      strcpy(sep, ",");
                    }
                    for (uint8_t i = oldIndex; i<=calculator->stack_index; i++) {
                      this->calculator->pop();
                    }
                    strcat(indices, "]");
                  }

                  ActivationRecord* ar = this->call_stack->peek();
                  ar->set(value[expression_name]["ID"], this->calculator->pop());

                  DEBUG_ARTI1("%s %s%s := %s (pop)\n", spaces+50-depth, value[expression_name]["ID"].as<const char *>(), indices, ar->get(value[expression_name]["ID"]));
                }

                visitCalledAlready = true;
              }
              else if (expression["id"] == "Exprs" || expression["id"] == "Terms") {
                // DEBUG_ARTI1("%s %s tovisit %s\n", spaces+50-depth, expression["id"].as<const char *>(), value.as<string>().c_str());
                const char * operatorx = nullptr;
                if (value.is<JsonArray>()) {
                  JsonArray valueArray = value.as<JsonArray>();
                  if (valueArray.size() >= 1) // visit first symbol 
                    visit(valueArray[0], nullptr, symbol_name, token, depth + 1);
                  if (valueArray.size() >= 3) { // add operator and another symbol
                    operatorx = valueArray[1].as<string>().c_str(); // as string because contains Jsonbject
                    // DEBUG_ARTI1("%s %s operator %s\n", spaces+50-depth, expression["id"].as<const char *>(), operatorx.c_str());
                    visit(valueArray[2], nullptr, symbol_name, token, depth + 1);
                  }
                  if (valueArray.size() != 1 && valueArray.size() != 3)
                    ERROR_ARTI("%s %s array not right size ?? (%d) %s %s \n", spaces+50-depth, expression["id"].as<const char *>(), valueArray.size(), key, value.as<string>().c_str());
                }
                else
                  ERROR_ARTI("%s %s not array?? %s %s \n", spaces+50-depth, key, expression["id"].as<const char *>(), value.as<string>().c_str());
                // DEBUG_ARTI1("%s operatorx %s\n", spaces+50-depth, operatorx);
                if (operatorx != nullptr && strstr(operatorx, "PLUS")) {
                  const char * right = calculator->pop();
                  const char * left = calculator->pop();
                  int result = atoi(left) + atoi(right);
                  DEBUG_ARTI1("%s %s + %s = %d\n", spaces+50-depth, left, right, result);

                  char resultChar[charLength];
                  itoa(result, resultChar, 10);

                  calculator->push("PLUS", resultChar);
                }
                if (operatorx != nullptr && strstr(operatorx, "MUL")) {
                  const char * right = calculator->pop();
                  const char * left = calculator->pop();
                  int result = atoi(left) * atoi(right);
                  DEBUG_ARTI1("%s %s * %s = %d\n", spaces+50-depth, left, right, result);

                  char resultChar[charLength];
                  itoa(result, resultChar, 10);

                  calculator->push("MUL", resultChar);
                }
                visitCalledAlready = true;
              }
              else if (expression["id"] == "Var") {
                ActivationRecord* ar = this->call_stack->peek();
                const char * expression_name = expression["name"];
                const char * nameValue = ar->get(value[expression_name]);

                #ifndef ESP32 //for some weird reason this causes a crash on esp32
                  DEBUG_ARTI1("%s %s %s = %s (push)\n", spaces+50-depth, key, value[expression_name].as<const char *>(), nameValue); //key is variable_declaration name is ID
                #endif
                calculator->push(key, nameValue);

                visitCalledAlready = true;
              }
              else if (expression["id"] == "ForLoop") {
                DEBUG_ARTI1("%s for loop\n", spaces+50-depth);

                const char * expression_block = expression["block"];
                visit(value, expression["from"], symbol_name, token, depth + 1);
                visit(value[expression["to"].as<const char *>()], nullptr, symbol_name, token, depth + 1);
                ActivationRecord* ar = this->call_stack->peek();
                for (int i=0; i<2;i++) { //this is the current state of this project: adding for loops, of course the from and to should be derived from the code ;-)
                  DEBUG_ARTI1("%s iteration %d\n", spaces+50-depth, i);

                  char resultChar[charLength];
                  itoa(i, resultChar, 10);

                  ar->set("y", resultChar);
                  visit(value[expression_block], nullptr, symbol_name, token, depth + 1);
                }
                visitCalledAlready = true;
              }
            } //if expression["id"]
         } // is key is symbol_name

          // DEBUG_ARTI1("%s\n", spaces+50-depth, "Object ", key, value);
          // if (key == "INTEGER_CONST" || key == "PLUS" || key == "MUL" || key == "LPAREN"  || key == "RPAREN" ) {
          if (strcmp(key, "INTEGER_CONST") == 0) {// || value == "+" || value == "*") || value == "("  || value == ")" ) {
            calculator->push(key, value.as<const char *>());
            DEBUG_ARTI1("%s Push %s %s %s\n", spaces+50-depth, key, value.as<const char *>(), calculator->peek());
            visitCalledAlready = true;
          }

          if (!analyzer->definitionJson["TOKENS"][key].isNull()) { //if key is token
            token = key;
            // DEBUG_ARTI1("%s\n", spaces+50-depth, "Token ", token);
          }
          if (!visitCalledAlready)
            visit(value, nullptr, symbol_name, token, depth + 1);
        }
        } // for (JsonPair
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI1("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
            visit(newParseTree, nullptr, symbol_name, token, depth + 1);
          }
        }
        else { //not array
          // const char * element = parseTree.as<const char *>();
          // DEBUG_ARTI1("%s\n", spaces+50-depth, "not array not object but element ", element);
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
    fstream definitionFile;
    fstream programFile;
    fstream parseTreeFile;
  #endif

  DynamicJsonDocument *definitionJson;
  DynamicJsonDocument *parseTreeJson;
public:
  ARTI() {

    // char byte = charFromProgramFile();
    // while (byte != -1) {
    //   programText += byte;
    //   DEBUG_ARTI1("%c", byte);
    //   byte = charFromProgramFile();
    // }

  }

  void openFileAndParse(const char *definitionName, const char *programName) {
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
        DEBUG_ARTI1("RAM left %d", esp_get_free_heap_size());
      #else
        definitionFile.open(definitionName, ios::in);
        // DEBUG_ARTI1("def size %d\n", definitionFile.tellg());
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
          programFile.open(programName, ios::in);
        #endif
        if (!programFile)
          ERROR_ARTI("Program file %s not found\n", programName);
        else
        {
          //open programFile
          #ifdef ESP32
            uint16_t programFileSize = programFile.size();
            programText = (char *)malloc(programFileSize+1);
            programFile.read((uint8_t*)programText, programFileSize);
            programText[programFileSize] = '\0';
          #else
            programFile.read(programText, programTextSize); //sizeof programText
            programText[programFile.gcount()] = '\0';
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
          #else
            parseTreeFile.open(parseTreeName, ios::out);
          #endif

          parseTreeJson = new DynamicJsonDocument(strlen(programText) * 50);

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

          // char resultString[standardStringLenght];
          // // strcpy(resultString, "");
          // arti.walk(parseTreeJson->as<JsonVariant>(), resultString);
            // DEBUG_ARTI1("walk result %s", resultString);

          //analyze
          semanticAnalyzer = new SemanticAnalyzer(definitionJson->as<JsonObject>(), parseTreeJson->as<JsonVariant>());
          semanticAnalyzer->analyze();

        } //programFile
      } //definitionFilee

  } // openFileAndParse

    void walk(JsonVariant tree, char * resultString) {
    treeWalker = new TreeWalker();
    // DeserializationError err = deserializeJson(definitionJson, definitionText);
    // if (err) {
    //   DEBUG_ARTI1("deserializeJson() in walk failed with code %s\n", err.c_str());
    // }
    // DEBUG_ARTI1("%s\n", definitionText.c_str());
    treeWalker->walk(tree, resultString);
  }

  void interpret() {
    interpreter = new Interpreter(semanticAnalyzer);
    interpreter->interpret();// interpreter.print();
  }

  void close() {
          DEBUG_ARTI1("def mem %d of %d %d %d\n", definitionJson->memoryUsage(), definitionJson->capacity(), definitionJson->memoryPool().capacity(), definitionJson->size());
          DEBUG_ARTI1("par mem %d of %d %d %d\n", parseTreeJson->memoryUsage(), parseTreeJson->capacity(), parseTreeJson->memoryPool().capacity(), parseTreeJson->size());
          DEBUG_ARTI1("prog size %d factor %d\n", strlen(programText), parseTreeJson->memoryUsage() / strlen(programText));
          //199 -> 6905 (34.69)
          //469 -> 15923 (33.95)
      #ifndef ARTI_DEBUGORLOG
        #ifdef ESP32
          logFile.close();
        #else
          fclose (logFile); //should not be closed as still streaming...
        #endif
      #endif
  }

}; //ARTI

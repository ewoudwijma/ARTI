/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti.h
   @version 0.2.1
   @date    20211212
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
   @remarks
          - #define ARDUINOJSON_DEFAULT_NESTING_LIMIT 100 //set this in ArduinoJson!!!, currently not necessary...
          - IF UPDATING THIS FILE IN THE WLED REPO, SEND A PULL REQUEST TO https://github.com/ewoudwijma/ARTI AS WELL!!!
   @later
          - Code improvememt
            - remove std::string (now only in logging)
            - See 'for some weird reason this causes a crash on esp32'
            - check why column/lineno not correct
          - Definition improvements
            - support string (e.g. for print)
            - add integer and string stacks
            - Add unary operators
            - Add ++, --
            - print every x seconds (to use it in loops. e.g. to show free memory)
            - reserved words (ext functions and variables cannot be used as variables)
            - check on return values
            - arrays (indices) for varref
          - WLED improvements
            - rename program to sketch?
   @done
          - add button to show(edit) wled file in wled segments
          - upload files in wled ui (instead of /edit)
          - arti_wled include setup and loop...
          - update images to arti repo
          - add upload presets button...
          - underscores in ids 
          - assigned variables global?
   @done?
   @progress
          - SetPixelColor without colorwheel
          - extend errorOccurred and add warnings (continue) next to errors (stop). Include stack full/empty
          - WLED: *arti in SEGENV.data: not working well as change mode will free(data)
          - move code from interpreter to analyzer to speed up interpreting
   @todo
          - check why statement is not 'shrinked'
          - minimize value["x"]
          - make default work in js
  */

#pragma once

#define ARTI_SERIAL 1
#define ARTI_FILE 2

#if ARTI_PLATFORM == ARTI_ARDUINO //defined in arti_definition.h e.g. arti_wled.h!
  #include "wled.h"  
  #include "src/dependencies/json/ArduinoJson-v6.h"

  File logFile;

  #define ARTI_ERRORWARNING 1 //shows lexer, parser, analyzer and interpreter errors
  // #define ARTI_DEBUG 1
  // #define ARTI_ANDBG 1
  // #define ARTI_RUNLOG 1 //if set on arduino this will create massive amounts of output (as ran in a loop)
  #define ARTI_MEMORY 1 //to do analyses of memory usage, trace memoryleaks (works only on arduino)
  #define ARTI_PRINT 1 //will show the printf calls

  const char spaces[51] PROGMEM = "                                                  ";
  #define FREE_SIZE esp_get_free_heap_size()
#else //embedded
  #include "dependencies/ArduinoJson-recent.h"

  FILE * logFile; // FILE needed to use in fprintf (std stream does not work)

  #define ARTI_ERRORWARNING 1
  #define ARTI_DEBUG 1
  #define ARTI_ANDBG 1
  #define ARTI_RUNLOG 1
  #define ARTI_MEMORY 1
  #define ARTI_PRINT 1

  #include <math.h>
  #include <iostream>
  #include <fstream>
  #include <sstream>

  const char spaces[51]         = "                                                  ";
  #define FREE_SIZE (unsigned int)0
#endif

bool logToFile = true; //print output to file (e.g. default.wled.log)

void artiPrintf(char const * format, ...)
{
  va_list argp;

  va_start(argp, format);

  if (!logToFile)
  {
    vprintf(format, argp);
  }
  else
  {
    #if ARTI_PLATFORM == ARTI_ARDUINO
      // rocket science here! As logfile.printf causes crashes we create our own printf here
      // logFile.printf(format, argp);
      for (int i = 0; i < strlen(format); i++) 
      {
        if (format[i] == '%') 
        {
          switch (format[i+1]) 
          {
            case 's':
              logFile.print(va_arg(argp, const char *));
              break;
            case 'u':
              logFile.print(va_arg(argp, unsigned int));
              break;
            case 'c':
              logFile.print((char)va_arg(argp, int));
              break;
            case '%':
              logFile.print("%"); // in case of %%
              break;
            default:
              va_arg(argp, int);
            // logFile.print(x);
              logFile.print(format[i]);
              logFile.print(format[i+1]);
          }
          i++;
        } 
        else 
        {
          logFile.print(format[i]);
        }
      }

    #else
      vfprintf(logFile, format, argp);
    #endif
  }
  va_end(argp);
}

#ifdef ARTI_DEBUG
    #define DEBUG_ARTI(...) artiPrintf(__VA_ARGS__)
#else
    #define DEBUG_ARTI(...)
#endif

#ifdef ARTI_ANDBG
    #define ANDBG_ARTI(...) artiPrintf(__VA_ARGS__)
#else
    #define ANDBG_ARTI(...)
#endif

#ifdef ARTI_RUNLOG
  #define RUNLOG_ARTI(...) artiPrintf(__VA_ARGS__)
#else
    #define RUNLOG_ARTI(...)
#endif

#ifdef ARTI_PRINT
    #define PRINT_ARTI(...) artiPrintf(__VA_ARGS__)
#else
    #define PRINT_ARTI(...)
#endif

#ifdef ARTI_ERRORWARNING
    #define ERROR_ARTI(...) artiPrintf(__VA_ARGS__)
    #define WARNING_ARTI(...) artiPrintf(__VA_ARGS__)
#else
    #define ERROR_ARTI(...)
    #define WARNING_ARTI(...)
#endif

#ifdef ARTI_MEMORY
    #define MEMORY_ARTI(...) artiPrintf(__VA_ARGS__)
#else
    #define MEMORY_ARTI(...)
#endif

#define charLength 30
#define fileNameLength 50
#define arrayLength 30

#define doubleNull -32768

const char * stringOrEmpty(const char *charS)  {
  if (charS == nullptr)
    return "";
  else
    return charS;
}

//define strupr as only supported in windows toolchain
char* strupr(char* s)
{
    char* tmp = s;

    for (;*tmp;++tmp) {
        *tmp = toupper((unsigned char) *tmp);
    }

    return s;
}

enum Operators
{
  F_integerConstant,
  F_realConstant,
  F_plus,
  F_minus,
  F_multiplication,
  F_division,
  F_modulo,
  F_bitShiftLeft,
  F_bitShiftRight,
  F_equal,
  F_notEqual,
  F_lessThen,
  F_lessThenOrEqual,
  F_greaterThen,
  F_greaterThenOrEqual,
  F_and,
  F_or
};

const char * operatorToString(uint8_t key)
{
  switch (key) {
  case F_integerConstant:
    return "integer";
    break;
  case F_realConstant:
    return "real";
    break;
  case F_plus:
    return "+";
    break;
  case F_minus:
    return "-";
    break;
  case F_multiplication:
    return "*";
    break;
  case F_division:
    return "/";
    break;
  case F_modulo:
    return "%";
    break;
  case F_bitShiftLeft:
    return "<<";
    break;
  case F_bitShiftRight:
    return ">>";
    break;
  case F_equal:
    return "==";
    break;
  case F_notEqual:
    return "!=";
    break;
  case F_lessThen:
    return "<";
    break;
  case F_lessThenOrEqual:
    return "<=>";
    break;
  case F_greaterThen:
    return ">";
    break;
  case F_greaterThenOrEqual:
    return ">=";
    break;
  case F_and:
    return "&&";
    break;
  case F_or:
    return "||";
    break;
  }
  return "unknown key";
}

enum Constructs
{
  F_Program,
  F_Function,
  F_Call,
  F_Var,
  F_Assign,
  F_Formal,
  F_VarRef,
  F_For,
  F_If,
  F_Expr,
  F_Term
};

const char * constructToString(uint8_t key)
{
  switch (key) {
  case F_Program:
    return "program";
    break;
  case F_Function:
    return "function";
    break;
  case F_Call:
    return "call";
    break;
  case F_Var:
    return "variable";
    break;
  case F_Assign:
    return "assign";
    break;
  case F_Formal:
    return "formal";
    break;
  case F_VarRef:
    return "varref";
    break;
  case F_For:
    return "for";
    break;
  case F_If:
    return "if";
    break;
  case F_Expr:
    return "expr";
    break;
  case F_Term:
    return "term";
    break;
  }
  return "unknown key";
}

uint8_t stringToConstruct(const char * construct)
{
  if (strcmp(construct, "program") == 0)
    return F_Program;
  else if (strcmp(construct, "function") == 0)
    return F_Function;
  else if (strcmp(construct, "call") == 0)
    return F_Call;
  else if (strcmp(construct, "variable") == 0)
    return F_Var;
  else if (strcmp(construct, "assign") == 0)
    return F_Assign;
  else if (strcmp(construct, "formal") == 0)
    return F_Formal;
  else if (strcmp(construct, "varref") == 0)
    return F_VarRef;
  else if (strcmp(construct, "for") == 0)
    return F_For;
  else if (strcmp(construct, "if") == 0)
    return F_If;
  else if (strcmp(construct, "expr") == 0)
    return F_Expr;
  else if (strcmp(construct, "term") == 0)
    return F_Term;
  return 255;
}

bool errorOccurred = false;

struct Token {
    uint16_t lineno;
    uint16_t column;
    char type[charLength];
    char value[charLength]; 
};

struct LexerPosition {
  uint16_t pos;
  char current_char;
  uint16_t lineno;
  uint16_t column;
  char type[charLength];
  char value[charLength];
};

#define nrOfPositions 20

class Lexer {
  private:
  public:
    const char * text;
    uint16_t pos;
    char current_char;
    uint16_t lineno;
    uint16_t column;
    JsonObject definitionJson;
    Token current_token;
    LexerPosition positions[nrOfPositions]; //should be array of pointers but for some reason get seg fault (because a struct and not a class...)
    uint8_t positions_index = 0;

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

    void skip_whitespace() {
      while (this->current_char != -1 && isspace(this->current_char))
        this->advance();
    }

    void skip_comment(const char * endTokens) {
      while (strncmp(this->text + this->pos, endTokens, strlen(endTokens)) != 0)
        this->advance();
      for (int i=0; i<strlen(endTokens); i++)
        this->advance();
    }

    void number() 
    {
      current_token.lineno = this->lineno;
      current_token.column = this->column;
      strcpy(current_token.type, "");
      strcpy(current_token.value, "");

      char result[charLength] = "";
      while (this->current_char != -1 && isdigit(this->current_char)) {
        result[strlen(result)] = this->current_char;
        this->advance();
      }
      if (this->current_char == '.') 
      {
        result[strlen(result)] = this->current_char;
        this->advance();

        while (this->current_char != -1 && isdigit(this->current_char)) 
        {
          result[strlen(result)] = this->current_char;
          this->advance();
        }

        result[strlen(result)] = '\0';
        strcpy(current_token.type, "REAL_CONST");
        strcpy(current_token.value, result);
      }
      else 
      {
        result[strlen(result)] = '\0';
        strcpy(current_token.type, "INTEGER_CONST");
        strcpy(current_token.value, result);
      }

    }

    void id() 
    {
      current_token.lineno = this->lineno;
      current_token.column = this->column;
      strcpy(current_token.type, "");
      strcpy(current_token.value, "");

        char result[charLength] = "";
        while (this->current_char != -1 && (isalnum(this->current_char) || this->current_char == '_')) 
        {
            result[strlen(result)] = this->current_char;
            this->advance();
        }
        result[strlen(result)] = '\0';

        char resultUpper[charLength];
        strcpy(resultUpper, result);
        strupr(resultUpper);

        if (definitionJson["TOKENS"][resultUpper].isNull()) 
        {
            strcpy(current_token.type, "ID");
            strcpy(current_token.value, result);
        }
        else {
            strcpy(current_token.type, definitionJson["TOKENS"][resultUpper]);
            strcpy(current_token.value, resultUpper);
        }
    }

    void get_next_token() 
    {
      current_token.lineno = this->lineno;
      current_token.column = this->column;
      strcpy(current_token.type, "");
      strcpy(current_token.value, "");

      if (errorOccurred) return;

      while (this->current_char != -1 && this->pos <= strlen(this->text) - 1 && !errorOccurred) 
      {
        if (isspace(this->current_char)) {
          this->skip_whitespace();
          continue;
        }

        if (strncmp(this->text + this->pos, "/*", 2) == 0) {
          this->advance();
          skip_comment("*/");
          continue;
        }

        if (strncmp(this->text + this->pos, "//", 2) == 0) {
          this->advance();
          skip_comment("\n");
          continue;
        }

        if (isalpha(this->current_char)) {
          this->id();
          return;
        }
        
        if (isdigit(this->current_char)) {
          this->number();
          return;
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

        if (strcmp(token_type, "") != 0 && strcmp(token_value, "") != 0) 
        {
          strcpy(current_token.type, token_type);
          strcpy(current_token.value, token_value);
          for (int i=0; i<strlen(token_value); i++)
            this->advance();
          return;
        }
        else {
          ERROR_ARTI("Lexer error on %c line %u col %u\n", this->current_char, this->lineno, this->column);
          errorOccurred = true;
        }
      }
    } //get_next_token

  void eat(const char * token_type) {
    // DEBUG_ARTI("try to eat %s %s\n", lexer->current_token.type, token_type);
    if (strcmp(current_token.type, token_type) == 0) {
      get_next_token();
      // DEBUG_ARTI("eating %s -> %s %s\n", token_type, lexer->current_token.type, lexer->current_token.value);
    }
    else {
      ERROR_ARTI("Lexer Error: Unexpected token %s %s\n", current_token.type, current_token.value);
      errorOccurred = true;
    }
  }

  void push_position() {
    if (positions_index < nrOfPositions) {
      positions[positions_index].pos = this->pos;
      positions[positions_index].current_char = this->current_char;
      positions[positions_index].lineno = this->lineno;
      positions[positions_index].column = this->column;
      strcpy(positions[positions_index].type, current_token.type);
      strcpy(positions[positions_index].value, current_token.value);
      positions_index++;
    }
    else
      ERROR_ARTI("not enough positions %u\n", nrOfPositions);
  }

  void pop_position() {
    if (positions_index > 0) {
      positions_index--;
      this->pos = positions[positions_index].pos;
      this->current_char = positions[positions_index].current_char;
      this->lineno = positions[positions_index].lineno;
      this->column = positions[positions_index].column;
      strcpy(current_token.type, positions[positions_index].type);
      strcpy(current_token.value, positions[positions_index].value);
    }
    else
      ERROR_ARTI("no positions saved\n");
  }

}; //Lexer

#define ResultFail 0
#define ResultStop 2
#define ResultContinue 1

class ScopedSymbolTable; //forward declaration

class Symbol {
  private:
  public:
  
  uint8_t symbol_type;
  char name[charLength];
  uint8_t type;
  uint8_t scope_level;
  uint8_t scope_index;
  ScopedSymbolTable* scope = nullptr;
  ScopedSymbolTable* function_scope = nullptr; //used to find the formal parameters in the scope of a function symbol

  JsonVariant block;

  Symbol(uint8_t symbol_type, const char * name, uint8_t type = 9) {
    this->symbol_type = symbol_type;
    strcpy(this->name, name);
    this->type = type;
    this->scope_level = 0;
  }

  ~Symbol() {
    MEMORY_ARTI("Destruct Symbol %s (%u)\n", name, FREE_SIZE);
  }

}; //Symbol

#define nrOfSymbolsPerScope 30
#define nrOfChildScope 10 //add checks

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
    MEMORY_ARTI("Destruct ScopedSymbolTable %s (%u)\n", scope_name, FREE_SIZE);
  }

  void init_builtins() {
        // this->insert(BuiltinTypeSymbol('INTEGER'));
        // this->insert(BuiltinTypeSymbol('REAL'));
  }

  void insert(Symbol* symbol) 
  {
    #ifdef _SHOULD_LOG_SCOPE
      ANDBG_ARTI("Log scope Insert %s\n", symbol->name.c_str());
    #endif
    symbol->scope_level = this->scope_level;
    symbol->scope = this;
    symbol->scope_index = symbolsIndex;
    if (symbolsIndex < nrOfSymbolsPerScope)
      this->symbols[symbolsIndex++] = symbol;
    else
      ERROR_ARTI("ScopedSymbolTable %s symbols full (%d)", scope_name, nrOfSymbolsPerScope);
  }

  Symbol* lookup(const char * name, bool current_scope_only=false) 
  {
    for (uint8_t i=0; i<symbolsIndex; i++) {
      if (strcmp(symbols[i]->name, name) == 0)
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

#define nrOfVariables 20

class ActivationRecord 
{
  private:
  public:
    char name[charLength];
    char type[charLength];
    int nesting_level;
    // char charMembers[charLength][nrOfVariables];
    double doubleMembers[nrOfVariables];
    char lastSet[charLength];
    uint8_t lastSetIndex;

    ActivationRecord(const char * name, const char * type, int nesting_level) 
    {
        strcpy(this->name, name);
        strcpy(this->type, type);
        this->nesting_level = nesting_level;
    }

    ~ActivationRecord() 
    {
      RUNLOG_ARTI("Destruct activation record %s\n", name);
    }

    // void set(uint8_t index,  const char * value) 
    // {
    //   lastSetIndex = index;
    //   strcpy(charMembers[index], value);
    // }

    void set(uint8_t index, double value) 
    {
      lastSetIndex = index;
      doubleMembers[index] = value;
    }

    // const char * getChar(uint8_t index) 
    // {
    //   return charMembers[index];
    // }

    double getDouble(uint8_t index) 
    {
      return doubleMembers[index];
    }

}; //ActivationRecord

#define nrOfRecords 20

class CallStack {
public:
  ActivationRecord* records[nrOfRecords];
  uint8_t recordsCounter = 0;

  CallStack() {
  }

  ~CallStack() 
  {
    RUNLOG_ARTI("Destruct callstack\n");
  }

  void push(ActivationRecord* ar) 
  {
    if (recordsCounter < nrOfRecords) 
    {
      // RUNLOG_ARTI("%s\n", "Push ", ar->name);
      this->records[recordsCounter++] = ar;
    }
    else 
    {
      errorOccurred = true;
      ERROR_ARTI("no space left in callstack\n");
    }
  }

  ActivationRecord* pop()
  {
    if (recordsCounter > 0)
    {
      // RUNLOG_ARTI("%s\n", "Pop ", this->peek()->name);
      return this->records[recordsCounter--];
    }
    else 
    {
      ERROR_ARTI("no ar left on callstack\n");
      return nullptr;
    }
  }

  ActivationRecord* peek() 
  {
    return this->records[recordsCounter-1];
  }
}; //CallStack

class ValueStack 
{
private:
public:
  // char charStack[arrayLength][charLength]; //currently only doubleStack used.
  double doubleStack[arrayLength];
  uint8_t stack_index = 0;

  ValueStack() 
  {
  }

  ~ValueStack() 
  {
    RUNLOG_ARTI("Destruct valueStack\n");
  }

  // void push(const char * value) {
  //   if (stack_index >= arrayLength)
  //     ERROR_ARTI("Push charStack full %u of %u\n", stack_index, arrayLength);
  //   else if (value == nullptr) {
  //     strcpy(charStack[stack_index++], "empty");
  //     ERROR_ARTI("Push null pointer on double stack\n");
  //   }
  //   else
  //     // RUNLOG_ARTI("calc push %s %s\n", key, value);
  //     strcpy(charStack[stack_index++], value);
  // }

  void push(double value) 
  {
    if (stack_index >= arrayLength) 
    {
      ERROR_ARTI("Push doubleStack full %u of %u\n", stack_index, arrayLength);
      errorOccurred = true;
    }
    else if (value == doubleNull)
      ERROR_ARTI("Push null value on double stack\n");
    else
      // RUNLOG_ARTI("calc push %s %s\n", key, value);
      doubleStack[stack_index++] = value;
  }

  // const char * peekChar() {
  //   // RUNLOG_ARTI("Calc Peek %s\n", charStack[stack_index-1]);
  //   return charStack[stack_index-1];
  // }

  double peekDouble() 
  {
    // RUNLOG_ARTI("Calc Peek %s\n", doubleStack[stack_index-1]);
    return doubleStack[stack_index-1];
  }

  // const char * popChar() {
  //   if (stack_index>0) {
  //     stack_index--;
  //     return charStack[stack_index];
  //   }
  //   else {
  //     ERROR_ARTI("Pop value stack empty\n");
        // errorOccurred = true;
//   // RUNLOG_ARTI("Calc Pop %s\n", charStack[stack_index]);
  //     return "novalue";
  //   }
  // }

  double popDouble() 
  {
    if (stack_index>0) 
    {
      stack_index--;
      return doubleStack[stack_index];
    }
    else 
    {
      ERROR_ARTI("Pop doubleStack empty\n");
    // RUNLOG_ARTI("Calc Pop %s\n", doubleStack[stack_index]);
      errorOccurred = true;
      return -1;
    }
  }

}; //ValueStack

#define programTextSize 5000

class ARTI {
private:
  Lexer *lexer = nullptr;

  DynamicJsonDocument *definitionJsonDoc = nullptr;
  DynamicJsonDocument *parseTreeJsonDoc = nullptr;
  JsonObject definitionJson;
  JsonVariant parseTreeJson;

  ScopedSymbolTable *global_scope = nullptr;
  CallStack *callStack = nullptr;
  ValueStack *valueStack = nullptr;

  uint8_t stages = 5; //for debugging: 1:parseFile, 2:Lexer, 3:parse, 4:analyze, 5:interpret should be 5 if no debugging

  char logFileName[fileNameLength];

public:
  ARTI() 
  {
    // MEMORY_ARTI("new Arti < %u\n", FREE_SIZE); //logfile not open here
  }

  ~ARTI() 
  {
    MEMORY_ARTI("Destruct ARTI\n");
  }

  //defined in arti_definition.h e.g. arti_wled.h!
  double arti_external_function(uint8_t function, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull, double par4 = doubleNull, double par5 = doubleNull);
  double arti_get_external_variable(uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
  void arti_set_external_variable(double value, uint8_t variable, double par1 = doubleNull, double par2 = doubleNull, double par3 = doubleNull);
  bool loop(); 
  
  uint8_t parse(JsonVariant parseTree, const char * symbol_name, char operatorx, JsonVariant expression, uint8_t depth = 0) 
  {
    if (depth > 50) 
    {
      ERROR_ARTI("Error: Parse recursion level too deep at %s (%u)\n", parseTree.as<std::string>().c_str(), depth);
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
      for (JsonVariant arrayElement: expression.as<JsonArray>()) 
      {
        JsonVariant nextExpression = arrayElement;
        const char * nextSymbol_name = symbol_name;
        JsonVariant nextParseTree = parseTree;
        JsonArray arr;

        JsonVariant symbolExpression = lexer->definitionJson[arrayElement.as<const char *>()];

        if (!symbolExpression.isNull()) //is arrayElement a Symbol e.g. "compound" : ["CURLYOPEN", "block*", "CURLYCLOSE"],
        {
          nextSymbol_name = arrayElement.as<const char *>();
          nextExpression = symbolExpression;

          // DEBUG_ARTI("%s %s %u\n", spaces+50-depth, nextSymbol_name, depth); //, parseTree.as<std::string>().c_str()

          if (parseTree.is<JsonArray>()) 
          {
            parseTree[parseTree.size()][nextSymbol_name]["connect"] = "array";
            nextParseTree = parseTree[parseTree.size()-1]; //nextparsetree is last element in the array (which is always an object)
          }
          else //no list, create object
          { 
            if (parseTree[symbol_name].isNull()) //no object yet
              parseTree[symbol_name]["connect"] = "object"; //make the connection, new object item

            nextParseTree = parseTree[symbol_name];
          }
        }

        if (operatorx == '|')
          lexer->push_position();

        if (nextExpression.is<JsonObject>()) // e.g. {"?":["LPAREN","formals*","RPAREN"]}
        {
          JsonObject::iterator objectIterator = nextExpression.as<JsonObject>().begin();
          char objectOperator = objectIterator->key().c_str()[0];
          JsonVariant objectElement = objectIterator->value();

          if (objectElement.is<JsonArray>()) 
          {
            if (objectOperator == '*' || objectOperator == '+') 
            {
              nextParseTree[nextSymbol_name]["*"][0] = "multiple";
              nextParseTree = nextParseTree[nextSymbol_name]["*"];
            }

            //and: see 'is array'
            if (objectOperator == '|') 
            {
              resultChild = parse(nextParseTree, nextSymbol_name, objectOperator, objectElement, depth + 1);
              if (resultChild != ResultFail) resultChild = ResultContinue;
            }
            else 
            {
              uint8_t resultChild2 = ResultContinue;
              uint8_t counter = 0;
              while (resultChild2 == ResultContinue) 
              {
                resultChild2 = parse(nextParseTree, nextSymbol_name, objectOperator, objectElement, depth + 1); //no assign to result as optional

                if (objectOperator == '?') { //zero or one iteration, also continue if parse not continue
                  resultChild2 = ResultContinue;
                  break; 
                }
                else if (objectOperator == '+') { //one or more iterations, stop if first parse not continue
                  if (counter == 0) {
                    if (resultChild2 != ResultContinue)
                      break;
                  } 
                  else 
                  {
                    if (resultChild2 != ResultContinue) 
                    {
                      resultChild2 = ResultContinue;  //always continue
                      break;
                    }
                  }
                }
                else if (objectOperator == '*') //zero or more iterations, stop if parse not continue
                {
                  if (resultChild2 != ResultContinue) 
                  {
                    resultChild2 = ResultContinue;  //always continue
                    break;
                  }
                }
                else 
                {
                  ERROR_ARTI("%s Programming error: undefined %c %s\n", spaces+50-depth, objectOperator, objectElement.as<std::string>().c_str());
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
        else if (nextExpression.is<JsonArray>()) // e.g. ["LPAREN", "expr*", "RPAREN"]
        {
          resultChild = parse(nextParseTree, nextSymbol_name, '&', nextExpression, depth + 1); // every array element starts with '&' (operatorx is for result of all elements of array)
        }
        else if (!lexer->definitionJson["TOKENS"][nextExpression.as<const char *>()].isNull()) // token e.g. "ID"
        {
          const char * token_type = nextExpression;
          if (strcmp(lexer->current_token.type, token_type) == 0) 
          {
            if (strcmp(lexer->current_token.type, "") != 0) 
              DEBUG_ARTI("%s %s %s", spaces+50-depth, lexer->current_token.type, lexer->current_token.value);

            if (nextParseTree.is<JsonArray>()) 
            {
              JsonArray arr = nextParseTree.as<JsonArray>();
              arr[arr.size()][lexer->current_token.type] = lexer->current_token.value; //add in last element of array
            }
            else
                nextParseTree[nextSymbol_name][lexer->current_token.type] = lexer->current_token.value;

            if (strcmp(token_type, "PLUS2") == 0) { //debug for unary operators (wip)
              // DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
              // DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
            }

            lexer->eat(token_type);

            if (strcmp(lexer->current_token.type, "") != 0) 
              DEBUG_ARTI(" -> [%s %s]", lexer->current_token.type, lexer->current_token.value);

            DEBUG_ARTI(" %d\n", depth);
            resultChild = ResultContinue;
          }
          else //deadend
          {
            resultChild = ResultFail;
          }
        } // if token
        else //arrayElement is not a symbol, not a token, not an array and not an object
        {
          if (lexer->definitionJson[nextExpression.as<const char *>()].isNull())
            ERROR_ARTI("%s Programming error: %s not a symbol, token, array or object in %s\n", spaces+50-depth, nextExpression.as<std::string>().c_str(), stringOrEmpty(nextSymbol_name));
          else
            ERROR_ARTI("%s Definition error: \"%s\": \"%s\" symbol should be embedded in array\n", spaces+50-depth, stringOrEmpty(nextSymbol_name), nextExpression.as<std::string>().c_str());
        } //nextExpression is not a token

        if (!symbolExpression.isNull()) //if symbol
        {
          nextParseTree.remove("connect"); //remove connector

          if (resultChild == ResultFail) { //remove result of parse
            nextParseTree.remove(nextSymbol_name); //remove the failed stuff

            // DEBUG_ARTI("%s fail %s\n", spaces+50-depth, nextSymbol_name);
          }
          else //success
          {
            //parseTree optimization moved to optimize function

            DEBUG_ARTI("%s found %s\n", spaces+50-depth, nextSymbol_name);
          }
        } // if symbol

        //determine result of arrayElement
        if (operatorx == '|') {
          if (resultChild == ResultFail) {//if fail, go back and try another
            // result = ResultContinue;
            lexer->pop_position();
          }
          else {
            result = ResultStop;  //Stop or continue is enough for an or
            lexer->positions_index--;
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

  } //parse

  bool analyze(JsonVariant parseTree, const char * treeElement = nullptr, ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) 
  {
    // ANDBG_ARTI("%s Depth %u %s\n", spaces+50-depth, depth, parseTree.as<std::string>().c_str());
    if (depth > 24) //otherwise stack canary errors on Arduino (value determined after testing, should be revisited)
    {
      ERROR_ARTI("Error: Analyze recursion level too deep at %s (%u)\n", parseTree.as<std::string>().c_str(), depth);
      errorOccurred = true;
    }
    if (errorOccurred) return false;

    if (parseTree.is<JsonObject>()) 
    {
      for (JsonPair parseTreePair : parseTree.as<JsonObject>()) 
      {
        const char * key = parseTreePair.key().c_str();
        JsonVariant value = parseTreePair.value();
        if (treeElement == nullptr || strcmp(treeElement, key) == 0 ) //in case there are more elements in the object and you want to analyze only one
        {
          bool visitedAlready = false;

          if (!definitionJson[key].isNull()) //if key is symbol_name
          {
            uint8_t construct = stringToConstruct(key);

            switch (construct) 
            {
              case F_Program: {
                const char * program_name = value["ID"];
                global_scope = new ScopedSymbolTable(program_name, 1, nullptr); //current_scope

                ANDBG_ARTI("%s Program %s %u %u\n", spaces+50-depth, global_scope->scope_name, global_scope->scope_level, global_scope->symbolsIndex); 

                if (value["ID"].isNull()) {
                  ERROR_ARTI("program name null\n");
                  errorOccurred = true;
                }
                if (value["block"].isNull()) {
                  ERROR_ARTI("%s Program %s: no block in parseTree\n", spaces+50-depth, program_name); 
                  errorOccurred = true;
                }
                else {
                  analyze(value["block"], nullptr, global_scope, depth + 1);
                }

                #ifdef ARTI_DEBUG
                  for (uint8_t i=0; i<global_scope->symbolsIndex; i++) {
                    Symbol* symbol = global_scope->symbols[i];
                    ANDBG_ARTI("%s %u %s %s.%s of %u (%u)\n", spaces+50-depth, i, constructToString(symbol->symbol_type), global_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                  }
                #endif

                visitedAlready = true;
                break;
              }
              case F_Function: {

                //find the function name (so we must know this is a function...)
                const char * function_name = value["ID"];
                Symbol* function_symbol = new Symbol(construct, function_name);
                current_scope->insert(function_symbol);

                ANDBG_ARTI("%s Function %s.%s\n", spaces+50-depth, current_scope->scope_name, function_name);
                ScopedSymbolTable* function_scope = new ScopedSymbolTable(function_name, current_scope->scope_level + 1, current_scope);
                if (current_scope->child_scopesIndex < nrOfChildScope)
                  current_scope->child_scopes[current_scope->child_scopesIndex++] = function_scope;
                else
                  ERROR_ARTI("ScopedSymbolTable %s childs full (%d)", current_scope->scope_name, nrOfChildScope);
                function_symbol->function_scope = function_scope;

                if (!value["formals"].isNull())
                  analyze(value["formals"], nullptr, function_scope, depth + 1);

                if (value["block"].isNull())
                  ERROR_ARTI("%s Function %s: no block in parseTree\n", spaces+50-depth, function_name); 
                else
                  analyze(value["block"], nullptr, function_scope, depth + 1);

                #ifdef ARTI_DEBUG
                  for (uint8_t i=0; i<function_scope->symbolsIndex; i++) {
                    Symbol* symbol = function_scope->symbols[i];
                    ANDBG_ARTI("%s %u %s %s.%s of %u (%u)\n", spaces+50-depth, i, constructToString(symbol->symbol_type), function_scope->scope_name, symbol->name, symbol->type, symbol->scope_level); 
                  }
                #endif

                visitedAlready = true;
                break;
              }
              case F_Var:
              case F_Formal:
              case F_Assign:
              case F_VarRef: {
                const char * variable_name = value["ID"];
                if (construct == F_Assign)
                  variable_name = value["varref"]["ID"];

                char param_type[charLength]; 
                if (!value["type"].isNull()) 
                  serializeJson(value["type"], param_type); //current_scope.lookup(param.type_node.value); //need string, lookup also used to find types...
                else
                  strcpy(param_type, "notype");

                //check if external variable
                bool found = false;
                uint8_t index = 0;
                for (JsonPair externalsPair: definitionJson["EXTERNALS"].as<JsonObject>()) {
                  if (strcmp(variable_name, externalsPair.key().c_str()) == 0) {
                    if (construct == F_Assign)
                      value["varref"]["external"] = index; //add external index to parseTree
                    else
                      value["external"] = index; //add external index to parseTree
                    ANDBG_ARTI("%s Ext Variable found %s (%u) %s\n", spaces+50-depth, variable_name, depth, key);
                    found = true;
                  }
                  index++;
                }

                if (!found) {
                  Symbol* var_symbol = current_scope->lookup(variable_name); //lookup here and parent scopes
                  if (construct == F_VarRef) 
                  {
                    if (var_symbol == nullptr)
                      ERROR_ARTI("%s VarRef %s ID not found in scope of %s\n", spaces+50-depth, variable_name, current_scope->scope_name); 
                    else 
                    {
                      value["level"] = var_symbol->scope_level;
                      value["index"] = var_symbol->scope_index;
                      ANDBG_ARTI("%s VarRef found %s.%s (%u)\n", spaces+50-depth, var_symbol->scope->scope_name, variable_name, depth);
                    }
                  }
                  else //assign and var/formal
                  {
                    //if variable not already defined, then add
                    if (construct != F_Assign || var_symbol == nullptr) { //only assign needs to check if not exists
                      var_symbol = new Symbol(construct, variable_name, 9); // no type support yet
                      if (construct == F_Assign)
                        global_scope->insert(var_symbol); // assigned variables are global scope
                      else
                        current_scope->insert(var_symbol);
                      ANDBG_ARTI("%s %s %s.%s of %s\n", spaces+50-depth, key, var_symbol->scope->scope_name, variable_name, param_type);
                    }
                    else if (construct != F_Assign && var_symbol != nullptr)
                      ERROR_ARTI("%s %s Duplicate ID %s.%s\n", spaces+50-depth, key, var_symbol->scope->scope_name, variable_name); 

                    if (construct == F_Assign) 
                    {
                      value["varref"]["level"] = var_symbol->scope_level;
                      value["varref"]["index"] = var_symbol->scope_index;;
                    }
                  }
                }

                if (construct == F_Assign) 
                {
                  ANDBG_ARTI("%s %s %s = (%u)\n", spaces+50-depth, key, variable_name, depth);

                  if (!value["expr"].isNull()) 
                  {
                    analyze(value, "expr", current_scope, depth + 1);
                  }
                  else
                    ERROR_ARTI("%s %s %s: no expr in parseTree\n", spaces+50-depth, key, variable_name); 

                  if (!value["assignoperator"].isNull()) 
                  {
                    JsonObject asop = value["assignoperator"];
                    JsonObject::iterator asopBegin = asop.begin();
                    ANDBG_ARTI("%s asop %s\n", spaces+50-depth, asopBegin->value().as<std::string>().c_str());
                    if (strcmp(asopBegin->value(), "+=") == 0) 
                      value["assignoperator"] = F_plus;
                    else if (strcmp(asopBegin->value(), "-=") == 0) 
                      value["assignoperator"] = F_minus;
                    else if (strcmp(asopBegin->value(), "*=") == 0) 
                      value["assignoperator"] = F_multiplication;
                    else if (strcmp(asopBegin->value(), "/=") == 0) 
                      value["assignoperator"] = F_division;
                  }
                }

                if (!value["indices"].isNull()) {
                  analyze(value, "indices", current_scope, depth + 1);
                }

                visitedAlready = true;
                break;
              }
              case F_Call: {
                const char * function_name = value["ID"];

                //check if external function
                bool found = false;
                uint8_t index = 0;
                for (JsonPair externalsPair: definitionJson["EXTERNALS"].as<JsonObject>()) 
                {
                  if (strcmp(function_name, externalsPair.key().c_str()) == 0) 
                  {
                    ANDBG_ARTI("%s Ext Function found %s (%u)\n", spaces+50-depth, function_name, depth);
                    value["external"] = index; //add external index to parseTree 
                    if (!value["actuals"].isNull())
                      analyze(value["actuals"], nullptr, current_scope, depth + 1);
                    found = true;
                  }
                  index++;
                }

                if (!found) 
                {
                  Symbol* function_symbol = current_scope->lookup(function_name); //lookup here and parent scopes
                  if (function_symbol != nullptr) 
                  {
                    if (!value["actuals"].isNull())
                      analyze(value["actuals"], nullptr, current_scope, depth + 1);

                    analyze(function_symbol->block, nullptr, current_scope, depth + 1);
                  } //function_symbol != nullptr
                  else 
                  {
                    ERROR_ARTI("%s Function %s not found in scope of %s\n", spaces+50-depth, function_name, current_scope->scope_name); 
                  }
                } //external functions

                visitedAlready = true;
              } // case
              break;
            } //switch

          } // is symbol_name

          else if (!this->definitionJson["TOKENS"][key].isNull()) 
          {
            const char * valueStr = value;

            if (strcmp(key, "INTEGER_CONST") == 0)
              parseTree["operator"] = F_integerConstant;
            else if (strcmp(key, "REAL_CONST") == 0)
              parseTree["operator"] = F_realConstant;
            else if (strcmp(valueStr, "+") == 0)
              parseTree["operator"] = F_plus;
            else if (strcmp(valueStr, "-") == 0)
              parseTree["operator"] = F_minus;
            else if (strcmp(valueStr, "*") == 0)
              parseTree["operator"] = F_multiplication;
            else if (strcmp(valueStr, "/") == 0)
              parseTree["operator"] = F_division;
            else if (strcmp(valueStr, "%") == 0)
              parseTree["operator"] = F_modulo;
            else if (strcmp(valueStr, "<<") == 0)
              parseTree["operator"] = F_bitShiftLeft;
            else if (strcmp(valueStr, ">>") == 0)
              parseTree["operator"] = F_bitShiftRight;
            else if (strcmp(valueStr, "==") == 0)
              parseTree["operator"] = F_equal;
            else if (strcmp(valueStr, "!=") == 0)
              parseTree["operator"] = F_notEqual;
            else if (strcmp(valueStr, ">") == 0)
              parseTree["operator"] = F_greaterThen;
            else if (strcmp(valueStr, ">=") == 0)
              parseTree["operator"] = F_greaterThenOrEqual;
            else if (strcmp(valueStr, "<") == 0)
              parseTree["operator"] = F_lessThen;
            else if (strcmp(valueStr, "<=") == 0)
              parseTree["operator"] = F_lessThenOrEqual;
            else if (strcmp(valueStr, "&&") == 0)
              parseTree["operator"] = F_and;
            else if (strcmp(valueStr, "||") == 0)
              parseTree["operator"] = F_or;

            visitedAlready = true;
          }

          if (!visitedAlready && value.size() > 0) // if size == 0 then injected key/value like operator
            analyze(value, nullptr, current_scope, depth + 1);

        } // key values
      } ///for elements in object

    }
    else if (parseTree.is<JsonArray>()) 
    {
      for (JsonVariant newParseTree: parseTree.as<JsonArray>()) 
      {
        analyze(newParseTree, nullptr, current_scope, depth + 1);
      }
    }
    else //not array
    {
      // string element = parseTree;
      //for some weird reason this causes a crash on esp32
      // ERROR_ARTI("%s Error: parseTree should be array or object %s (%u)\n", spaces+50-depth, parseTree.as<std::string>().c_str(), depth);
    }

    return !errorOccurred;
  } //analyze

  //https://dev.to/lefebvre/compilers-106---optimizer--ig8
  bool optimize(JsonVariant parseTree, uint8_t depth = 0) 
  {
    if (parseTree.is<JsonObject>()) 
    {
      //make the parsetree as small as possible to let the interpreter run as fast as possible:

      for (JsonPair parseTreePair : parseTree.as<JsonObject>()) 
      {
        const char * key = parseTreePair.key().c_str();
        JsonVariant value = parseTreePair.value();

        bool visitedAlready = false;

        // object:
        // if key is symbol and value is object then optimize object after that if object empty then remove key
        // else key is !ID and value is string then remove key
        // else key is * and value is array then optimize array after that if array empty then remove key *

        // array
        // if element is multiple then remove

        if (!definitionJson[key].isNull() && value.is<JsonObject>()) //if key is symbol_name
        {
          optimize(value, depth + 1);

          if (value.size() == 0) 
          {
            // DEBUG_ARTI("%s remove key without value %s (%u)\n", spaces+50-depth, key, depth);
            parseTree.remove(key);
          }
          else if (value.size() == 1) //try to shrink
          {
            //- check if a symbol is not used in analyzer / interpreter and has only one element: go to the parent and replace itself with its child (shrink)

            JsonObject::iterator objectIterator = value.as<JsonObject>().begin();

            if (!definitionJson[objectIterator->key().c_str()].isNull()) 
            {
              // //symbolObjectKey should be a symbol on itself and the value must consist of one element
              // bool found = false;
              // for (JsonPair semanticsPair : definitionJson["SEMANTICS"].as<JsonObject>()) 
              // {
              //   const char * semanticsKey = semanticsPair.key().c_str();
              //   JsonVariant semanticsValue = semanticsPair.value();

              //   if (strcmp(objectIterator->key().c_str(), semanticsKey) == 0){
              //     found = true;
              //     break;
              //   }
              //   for (JsonPair semanticsVariables : semanticsValue.as<JsonObject>()) {
              //     JsonVariant variableValue = semanticsVariables.value();
              //     if (variableValue.is<const char *>() && strcmp(objectIterator->key().c_str(), variableValue.as<const char *>()) == 0) {
              //       found = true;
              //       break;
              //     }
              //   }
              // }
              
              if (stringToConstruct(objectIterator->key().c_str()) == 255)
              // if (false && !found) // not used in analyzer / interpreter
              {
                DEBUG_ARTI("%s symbol to shrink %s in %s = %s from %s\n", spaces+50-depth, objectIterator->key().c_str(), key, objectIterator->value().as<std::string>().c_str(), parseTree[key].as<std::string>().c_str());
                parseTree[key] = objectIterator->value();
              }
            } // symbolObjectKey should by a symbol on itself and value is one element
          } // symbolObjectKey should by a symbol on itself and value is one element

          visitedAlready = true;
        }
        else if (!this->definitionJson["TOKENS"][key].isNull()) {
          const char * valueStr = value;
          if (strcmp(key, "ID") == 0 || strcmp(key, "INTEGER_CONST") == 0 || strcmp(key, "REAL_CONST") == 0 || 
                          strcmp(key, "INTEGER") == 0 || strcmp(key, "REAL") == 0 || 
                          strcmp(valueStr, "+") == 0 || strcmp(valueStr, "-") == 0 || strcmp(valueStr, "*") == 0 || strcmp(valueStr, "/") == 0 || strcmp(valueStr, "%") == 0 || 
                          strcmp(valueStr, "+=") == 0 || strcmp(valueStr, "-=") == 0 || 
                          strcmp(valueStr, "*=") == 0 || strcmp(valueStr, "/=") == 0 || 
                          strcmp(valueStr, "<<") == 0 || strcmp(valueStr, ">>") == 0 || 
                          strcmp(valueStr, "==") == 0 || strcmp(valueStr, "!=") == 0 || 
                          strcmp(valueStr, "&&") == 0 || strcmp(valueStr, "||") == 0 || 
                          strcmp(valueStr, ">") == 0 || strcmp(valueStr, ">=") == 0 || strcmp(valueStr, "<") == 0 || strcmp(valueStr, "<=") == 0) {
          }
          else 
          {
            // DEBUG_ARTI("%s remove key/value %s %s (%u)\n", spaces+50-depth, key, valueStr, depth);
            parseTree.remove(key);
          }

          visitedAlready = true;
        }
        else if (strcmp(key, "*") == 0 ) 
        {
          optimize(value, depth + 1);

          if (value.size() == 0)
            parseTree.remove(key);

          visitedAlready = true;
        }
        else
          DEBUG_ARTI("%s unknown status for %s %s (%u)\n", spaces+50-depth, key, value.as<std::string>().c_str(), depth);

          if (!visitedAlready && value.size() > 0) // if size == 0 then injected key/value like operator
            optimize(value, depth + 1);

      } ///for elements in object

    }
    else if (parseTree.is<JsonArray>())
    {
      JsonArray parseTreeArray = parseTree.as<JsonArray>();
      
      uint8_t arrayIndex = 0;
      for (JsonArray::iterator it = parseTreeArray.begin(); it != parseTreeArray.end(); ++it) 
      {
        JsonVariant element = *it;

        if (element == "multiple") 
        {
          // DEBUG_ARTI("%s remove array element 'multiple' of %s array (%u)\n", spaces+50-depth, element.as<std::string>().c_str(), arrayIndex);
          parseTreeArray.remove(it);
        }
        else if (it->size() == 0)
        {
          // DEBUG_ARTI("%s remove array element {} of %s array (%u)\n", spaces+50-depth, element.as<std::string>().c_str(), arrayIndex);
          parseTreeArray.remove(it); //remove {} elements (added by * arrays, don't know where added)
        }
        else 
          optimize(*it, depth + 1);

        arrayIndex++;
      }

      // for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
      //   DEBUG_ARTI("%s Array element %s\n", spaces+50-depth, newParseTree.as<std::string>().c_str());
      //   if (newParseTree.is<JsonObject>() || newParseTree.is<JsonArray>())
      //     optimize(newParseTree, depth + 1);
      //   else
      //     ERROR_ARTI("%s Error: parseTree not object %s (%u)\n", spaces+50-depth, newParseTree.as<std::string>().c_str(), depth);
      // }
    }
    else { //not array
      // string element = parseTree;
      //for some weird reason this causes a crash on esp32
      ERROR_ARTI("%s Error: parseTree should be array or object %s (%u)\n", spaces+50-depth, parseTree.as<std::string>().c_str(), depth);
    }

    return !errorOccurred;
  } //optimize

            // ["PLUS2", "factor"],
          // [ "MINUS2", "factor"],

  bool interpret(JsonVariant parseTree, const char * treeElement = nullptr, ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) 
  {
    // RUNLOG_ARTI("%s Interpret %s %s (%u)\n", spaces+50-depth, stringOrEmpty(treeElement), parseTree.as<std::string>().c_str(), depth);

    if (depth >= 50) 
    {
      ERROR_ARTI("Error: Interpret recursion level too deep at %s (%u)\n", parseTree.as<std::string>().c_str(), depth);
      errorOccurred = true;
    }
    if (errorOccurred) return false;

    if (parseTree.is<JsonObject>()) 
    {
      for (JsonPair parseTreePair : parseTree.as<JsonObject>()) 
      {
        const char * key = parseTreePair.key().c_str();
        JsonVariant value = parseTreePair.value();
        if (treeElement == nullptr || strcmp(treeElement, key) == 0) 
        {
          // RUNLOG_ARTI("%s Interpret object element %s\n", spaces+50-depth, key); //, value.as<std::string>().c_str()

          bool visitedAlready = false;

          if (!this->definitionJson[key].isNull()) { //if key is symbol_name
            uint8_t construct = stringToConstruct(key);

            // RUNLOG_ARTI("%s Symbol %s\n", spaces+50-depth, symbol_name);

            switch (construct)
            {
              case F_Program: 
              {
                const char * program_name = value["ID"];
                RUNLOG_ARTI("%s program %s\n", spaces+50-depth, program_name);

                ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM", 1);

                this->callStack->push(ar);

                interpret(value["block"], nullptr, global_scope, depth + 1);

                // do not delete main stack and program ar as used in subsequent calls 
                // this->callStack->pop();
                // delete ar; ar = nullptr;

                visitedAlready = true;
                break;
              }
              case F_Function: 
              {
                const char * function_name = value["ID"];
                Symbol* function_symbol = current_scope->lookup(function_name);
                RUNLOG_ARTI("%s Save block of %s\n", spaces+50-depth, function_name);
                if (function_symbol != nullptr)
                  function_symbol->block = value["block"];
                else
                  ERROR_ARTI("%s Function %s: not found\n", spaces+50-depth, function_name); 

                visitedAlready = true;
                break;
              }
              case F_Call: 
              {
                const char * function_name = value["ID"];

                //check if external function
                if (!value["external"].isNull()) {
                  uint8_t oldIndex = valueStack->stack_index;

                  if (!value["actuals"].isNull())
                    interpret(value["actuals"], nullptr, current_scope, depth + 1);

                  double returnValue = doubleNull;

                  returnValue = arti_external_function(value["external"], valueStack->doubleStack[oldIndex]
                                                                        , (valueStack->stack_index - oldIndex>1)?valueStack->doubleStack[oldIndex+1]:doubleNull
                                                                        , (valueStack->stack_index - oldIndex>2)?valueStack->doubleStack[oldIndex+2]:doubleNull
                                                                        , (valueStack->stack_index - oldIndex>3)?valueStack->doubleStack[oldIndex+3]:doubleNull
                                                                        , (valueStack->stack_index - oldIndex>4)?valueStack->doubleStack[oldIndex+4]:doubleNull);

                  #if ARTI_PLATFORM != ARTI_ARDUINO // because arduino runs the code instead of showing the code
                    uint8_t lastIndex = oldIndex;
                    RUNLOG_ARTI("%s Call %s(", spaces+50-depth, function_name);
                    char sep[3] = "";
                    for (int i = oldIndex; i< valueStack->stack_index; i++) {
                      RUNLOG_ARTI("%s%f", sep, valueStack->doubleStack[i]);
                      strcpy(sep, ", ");
                    }
                    if ( returnValue != doubleNull)
                      RUNLOG_ARTI(") = %f\n", returnValue);
                    else
                      RUNLOG_ARTI(")\n");
                  #endif

                  valueStack->stack_index = oldIndex;

                  if (returnValue != doubleNull)
                    valueStack->push(returnValue);

                }
                else { //not an external function
                  Symbol* function_symbol = current_scope->lookup(function_name);

                  if (function_symbol != nullptr) //calling undefined function: pre-defined functions e.g. print
                  {
                    ActivationRecord* ar = new ActivationRecord(function_name, "Function", function_symbol->scope_level + 1);

                    RUNLOG_ARTI("%s %s %s\n", spaces+50-depth, key, function_name);

                    uint8_t oldIndex = valueStack->stack_index;
                    uint8_t lastIndex = valueStack->stack_index;

                    if (!value["actuals"].isNull())
                      interpret(value["actuals"], nullptr, current_scope, depth + 1);

                    for (uint8_t i=0; i<function_symbol->function_scope->symbolsIndex; i++) //backwards because popped in reversed order
                    {
                      if (function_symbol->function_scope->symbols[i]->symbol_type == F_Formal) //select formal parameters
                      {
                        //determine type, for now assume double
                        double result = valueStack->doubleStack[lastIndex++];
                        ar->set(function_symbol->function_scope->symbols[i]->scope_index, result);
                        RUNLOG_ARTI("%s Actual %s.%s = %f (pop %u)\n", spaces+50-depth, function_name, function_symbol->function_scope->symbols[i]->name, result, valueStack->stack_index);
                      }
                    }

                    valueStack->stack_index = oldIndex;

                    this->callStack->push(ar);

                    interpret(function_symbol->block, nullptr, function_symbol->function_scope, depth + 1);

                    this->callStack->pop();

                    delete ar; ar =  nullptr;

                    //tbd if syntax supports returnvalue
                    // char callResult[charLength] = "CallResult tbd of ";
                    // strcat(callResult, function_name);
                    // valueStack->push(callResult);

                  } //function_symbol != nullptr
                  else
                    RUNLOG_ARTI("%s %s not found %s\n", spaces+50-depth, key, function_name);
                } //external functions

                visitedAlready = true;
                break;
              }
              case F_VarRef:
              case F_Assign: //get or set a variable
              {
                const char * variable_name = value["ID"];
                uint8_t variable_level = value["level"];
                uint8_t variable_index = value["index"];
                uint8_t variable_external = value["external"];

                uint8_t oldIndex = valueStack->stack_index;

                //array indices
                char indices[charLength]; //used in RUNLOG_ARTI only
                strcpy(indices, "");
                if (!value["indices"].isNull()) {
                  strcat(indices, "[");

                  interpret(value, "indices", current_scope, depth + 1); //values of indices pushed

                  char sep[3] = "";
                  for (uint8_t i = oldIndex; i< valueStack->stack_index; i++) {
                    strcat(indices, sep);
                    char itoaChar[charLength];
                    // itoa(valueStack->doubleStack[i], itoaChar, 10);
                    snprintf(itoaChar, sizeof(itoaChar), "%f", valueStack->doubleStack[i]);
                    strcat(indices, itoaChar);
                    strcpy(sep, ",");
                  }

                  strcat(indices, "]");
                }

                if (construct == F_Assign) 
                {
                  variable_name = value["varref"]["ID"];
                  variable_level = value["varref"]["level"];
                  variable_index = value["varref"]["index"];
                  variable_external = value["varref"]["external"];

                  if (!value["expr"].isNull()) //value assignment
                    interpret(value, "expr", current_scope, depth + 1); //value pushed
                  else 
                  {
                    ERROR_ARTI("%s Assign %s has no expr\n", spaces+50-depth, variable_name);
                    valueStack->push(doubleNull);
                  }
                }

                //check if external variable
                if (!value["external"].isNull() || !value["varref"]["external"].isNull()) //added by Analyze
                {
                  double returnValue = doubleNull;

                  if (construct == F_VarRef) { //get the value

                    returnValue = arti_get_external_variable(variable_external, (valueStack->stack_index - oldIndex>0)?valueStack->doubleStack[oldIndex]:doubleNull, (valueStack->stack_index - oldIndex>1)?valueStack->doubleStack[oldIndex+1]:doubleNull);

              valueStack->stack_index = oldIndex;

                    if (returnValue != doubleNull) 
                    {
                      valueStack->push(returnValue);
                      RUNLOG_ARTI("%s %s ext.%s = %f (push %u)\n", spaces+50-depth, key, variable_name, returnValue, valueStack->stack_index); //key is variable_declaration name is ID
                    }
                    else
                      ERROR_ARTI("%s Error: %s ext.%s no value\n", spaces+50-depth, key, variable_name);
                  }
                  else //assign: set the external value...
                  {
                    returnValue = valueStack->popDouble(); //result of interpret value

                    arti_set_external_variable(returnValue, variable_external, (valueStack->stack_index - oldIndex>0)?valueStack->doubleStack[oldIndex]:doubleNull, (valueStack->stack_index - oldIndex>1)?valueStack->doubleStack[oldIndex+1]:doubleNull);

                    RUNLOG_ARTI("%s %s set ext.%s%s = %f (%u)\n", spaces+50-depth, key, variable_name, indices, returnValue, valueStack->stack_index);
              valueStack->stack_index = oldIndex;
                  }
                }
                else //not external, get er set the variable
                {
                  // Symbol* variable_symbol = current_scope->lookup(variable_name);
                  ActivationRecord* ar;

                  //check already defined in this scope

                  // RUNLOG_ARTI("%s levels %u-%u\n", spaces+50-depth, variable_level,  variable_index );
                  if (variable_level != 0) { //var already exist
                    //calculate the index in the call stack to find the right ar
                    uint8_t index = this->callStack->recordsCounter - 1 - (this->callStack->peek()->nesting_level - variable_level);
                    //  RUNLOG_ARTI("%s %s %s.%s = %s (push) %s %d-%d = %d (%d)\n", spaces+50-depth, key, ar->name, variable_name, varValue, variable_symbol->name, this->callStack->peek()->nesting_level,variable_symbol->scope_level, index,  this->callStack->recordsCounter); //key is variable_declaration name is ID
                    ar = this->callStack->records[index];
                  }
                  else { //var created here
                    ar = this->callStack->peek();
                  }

                  if (ar != nullptr) {
                    if (construct == F_VarRef) { //get the value
                      //determine type, for now assume double
                      double varValue = ar->getDouble(variable_index);

                      valueStack->push(varValue);
                      #if ARTI_PLATFORM != ARTI_ARDUINO  //for some weird reason this causes a crash on esp32
                        RUNLOG_ARTI("%s %s %s.%s = %f (push %u) %u-%u\n", spaces+50-depth, key, ar->name, variable_name, varValue, valueStack->stack_index, variable_level, variable_index); //key is variable_declaration name is ID
                      #endif
                    }
                    else { //assign: set the value 

                      if (!value["assignoperator"].isNull()) 
                      {
                        switch (value["assignoperator"].as<uint8_t>()) 
                        {
                          case F_plus:
                            ar->set(variable_index, ar->getDouble(variable_index) + valueStack->popDouble());
                            break;
                          case F_minus:
                            ar->set(variable_index, ar->getDouble(variable_index) - valueStack->popDouble());
                            break;
                          case F_multiplication:
                            ar->set(variable_index, ar->getDouble(variable_index) * valueStack->popDouble());
                            break;
                          case F_division: {
                            double divisor = valueStack->popDouble();
                            if (divisor == 0)
                            {
                              divisor = 1;
                              ERROR_ARTI("%s /= division by 0 not possible, divisor ignored for %f\n", spaces+50-depth, ar->getDouble(variable_index));
                            }
                            ar->set(variable_index, ar->getDouble(variable_index) / divisor);
                            break;
                          }
                        }

                        RUNLOG_ARTI("%s %s.%s%s %s= %f (pop %u) %u-%u\n", spaces+50-depth, ar->name, variable_name, indices, operatorToString(value["assignoperator"]), ar->getDouble(variable_index), valueStack->stack_index, variable_level, variable_index);
                      }
                      else 
                      {
                        ar->set(variable_index, valueStack->popDouble());
                        RUNLOG_ARTI("%s %s.%s%s := %f (pop %u) %u-%u\n", spaces+50-depth, ar->name, variable_name, indices, ar->getDouble(variable_index), valueStack->stack_index, variable_level, variable_index);
                      }
                      valueStack->stack_index = oldIndex;
                    }
                  }
                  else { //unknown variable
                    ERROR_ARTI("%s %s %s unknown\n", spaces+50-depth, key, variable_name);
                    valueStack->push(doubleNull);
                  }
                } // ! founnd
                visitedAlready = true;
                break;
              }
              case F_Expr:
              case F_Term: 
              {
                // RUNLOG_ARTI("%s %s interpret < %s\n", spaces+50-depth, key, value.as<std::string>().c_str());

                uint8_t oldIndex = valueStack->stack_index;

                interpret(value, nullptr, current_scope, depth + 1); //pushes results

                // RUNLOG_ARTI("%s %s interpret > (%u - %u = %u)\n", spaces+50-depth, key, valueStack->stack_index, oldIndex, valueStack->stack_index - oldIndex);

                // always 3, 5, 7 ... values
                if (valueStack->stack_index - oldIndex >= 3) 
                {
                  double left = valueStack->doubleStack[oldIndex];
                  for (int i = 3; i <= valueStack->stack_index - oldIndex; i += 2)
                  {
                    uint8_t operatorx = valueStack->doubleStack[oldIndex + i - 2];
                    double right = valueStack->doubleStack[oldIndex + i - 1];

                    double evaluation = 0;

                    switch (operatorx) {
                      case F_plus: 
                        evaluation = left + right;
                        break;
                      case F_minus: 
                        evaluation = left - right;
                        break;
                      case F_multiplication: 
                        evaluation = left * right;
                        break;
                      case F_division: {
                        if (right == 0)
                        {
                          right = 1;
                          ERROR_ARTI("%s division by 0 not possible, divisor ignored for %f\n", spaces+50-depth, left);
                        }
                        evaluation = left / right;
                        break;
                      }
                      case F_modulo: {
                        if (right == 0) {
                          evaluation = left;
                          ERROR_ARTI("%s mod 0 not possible, mod ignored %f\n", spaces+50-depth, left);
                        }
                        else 
                          evaluation = fmod(left, right);
                        break;
                      }
                      case F_bitShiftLeft: 
                        evaluation = (int)left << (int)right; //only works on integers
                        break;
                      case F_bitShiftRight: 
                        evaluation = (int)left >> (int)right; //only works on integers
                        break;
                      case F_equal: 
                        evaluation = left == right;
                        break;
                      case F_notEqual: 
                        evaluation = left != right;
                        break;
                      case F_lessThen: 
                        evaluation = left < right;
                        break;
                      case F_lessThenOrEqual: 
                        evaluation = left <= right;
                        break;
                      case F_greaterThen: 
                        evaluation = left > right;
                        break;
                      case F_greaterThenOrEqual: 
                        evaluation = left >= right;
                        break;
                      case F_and: 
                        evaluation = left && right;
                        break;
                      case F_or: 
                        evaluation = left || right;
                        break;
                      default:
                        ERROR_ARTI("%s Programming error: unknown operator %u\n", spaces+50-depth, operatorx);
                    }

                    RUNLOG_ARTI("%s %f %s %f = %f (push %u)\n", spaces+50-depth, left, operatorToString(operatorx), right, evaluation, valueStack->stack_index+1);

                    left = evaluation;

                  }

                  valueStack->stack_index = oldIndex;
    
                  valueStack->push(left);
                }

                visitedAlready = true;
                break;
              }
              case F_For: 
              {
                RUNLOG_ARTI("%s For (%u)\n", spaces+50-depth, valueStack->stack_index);

                interpret(value, "assign", current_scope, depth + 1); //creates the assignment
                ActivationRecord* ar = this->callStack->peek();

                bool continuex = true;
                uint16_t counter = 0;
                while (continuex && counter < 2000) //to avoid endless loops
                {
                  RUNLOG_ARTI("%s iteration\n", spaces+50-depth);

                  RUNLOG_ARTI("%s check to condition\n", spaces+50-depth);
                  interpret(value, "expr", current_scope, depth + 1); //pushes result of to

                  double conditionResult = valueStack->popDouble();

                  RUNLOG_ARTI("%s conditionResult (pop %u)\n", spaces+50-depth, valueStack->stack_index);

                  if (conditionResult == 1) { //conditionResult is true
                    RUNLOG_ARTI("%s 1 => run block\n", spaces+50-depth);
                    interpret(value["block"], nullptr, current_scope, depth + 1);

                    RUNLOG_ARTI("%s assign next value\n", spaces+50-depth);
                    interpret(value["increment"], nullptr, current_scope, depth + 1); //pushes increment result
                    // MEMORY_ARTI("%s Iteration %u %u\n", spaces+50-depth, counter, FREE_SIZE);
                  }
                  else 
                  {
                    if (conditionResult == 0) { //conditionResult is false
                      RUNLOG_ARTI("%s 0 => end of For\n", spaces+50-depth);
                      continuex = false;
                    }
                    else // conditionResult is a value (e.g. in pascal)
                    {
                      //get the variable from assignment
                      double varValue = ar->getDouble(ar->lastSetIndex);

                      double evaluation = varValue <= conditionResult;
                      RUNLOG_ARTI("%s %s.(%u) %f <= %f = %f\n", spaces+50-depth, ar->name, ar->lastSetIndex, varValue, conditionResult, evaluation);

                      if (evaluation == 1) 
                      {
                        RUNLOG_ARTI("%s 1 => run block\n", spaces+50-depth);
                        interpret(value["block"], nullptr, current_scope, depth + 1);

                        //increment
                        ar->set(ar->lastSetIndex, varValue + 1);
                      }
                      else 
                      {
                        RUNLOG_ARTI("%s 0 => end of For\n", spaces+50-depth);
                        continuex = false;
                      }
                    }
                  }
                  counter++;
                };

                if (continuex)
                  ERROR_ARTI("%s too many iterations in for loop %u\n", spaces+50-depth, counter);

                visitedAlready = true;
                break;
              }  // case
              case F_If: 
              {
                RUNLOG_ARTI("%s If (%u)\n", spaces+50-depth, valueStack->stack_index);

                RUNLOG_ARTI("%s if condition\n", spaces+50-depth);
                interpret(value, "expr", current_scope, depth + 1);

                double conditionResult = valueStack->popDouble();

                RUNLOG_ARTI("%s (pop %u)\n", spaces+50-depth, valueStack->stack_index);

                if (conditionResult == 1) //conditionResult is true
                  interpret(value, "block", current_scope, depth + 1);
                else
                  interpret(value, "elseBlock", current_scope, depth + 1);

                visitedAlready = true;
                break;
              }  // case
            }

          } // is key is symbol_name

          else if (!definitionJson["TOKENS"][key].isNull()) //if key is token
          {
            // RUNLOG_ARTI("%s Token %s %s %s\n", spaces+50-depth, key, valueStr, parseTree.as<std::string>().c_str());

            if (!parseTree["operator"].isNull())
            {
              const char * valueStr = value;

              switch (parseTree["operator"].as<uint8_t>()) 
              {
                case F_integerConstant:
                case F_realConstant:
                  valueStack->push(atof(valueStr)); //push value
                  #if ARTI_PLATFORM != ARTI_ARDUINO  //for some weird reason this causes a crash on esp32
                    RUNLOG_ARTI("%s %s %s (Push %u)\n", spaces+50-depth, key, valueStr, valueStack->stack_index);
                  #endif
                  break;
                default:
                  valueStack->push(parseTree["operator"].as<uint8_t>()); // push Operator index
                  #if ARTI_PLATFORM != ARTI_ARDUINO  //for some weird reason this causes a crash on esp32
                    RUNLOG_ARTI("%s %s %s (Push %u)\n", spaces+50-depth, key, valueStr, valueStack->stack_index);
                  #endif
              }
            }

            visitedAlready = true;
          }

          if (!visitedAlready && value.size() > 0) // if size == 0 then injected key/value like operator
            interpret(value, nullptr, current_scope, depth + 1);
        } // if treeelement
      } // for (JsonPair)
    }
    else if (parseTree.is<JsonArray>()) 
    {
      for (JsonVariant newParseTree: parseTree.as<JsonArray>()) 
      {
        // RUNLOG_ARTI("%s\n", spaces+50-depth, "Array ", parseTree[i], "  ";
        interpret(newParseTree, nullptr, current_scope, depth + 1);
      }
    }
    else { //not array
      ERROR_ARTI("%s Error: parseTree should be array or object %s (%u)\n", spaces+50-depth, parseTree.as<std::string>().c_str(), depth);
    }

    return !errorOccurred;
  } //interpret

  void closeLog() {
    //arduino stops log here
    #if ARTI_PLATFORM == ARTI_ARDUINO
      if (logToFile) 
      {
        logFile.close();
        logToFile = false;
      }
    #endif
  }

  bool setup(const char *definitionName, const char *programName)
  {
    errorOccurred = false;
    logToFile = true;
    //open logFile
    if (logToFile)
    {
      #if ARTI_PLATFORM == ARTI_ARDUINO
        strcpy(logFileName, "/");
      #endif
      strcpy(logFileName, programName);
      strcat(logFileName, ".log");

      #if ARTI_PLATFORM == ARTI_ARDUINO
        logFile = LITTLEFS.open(logFileName,"w");
      #else
        logFile = fopen (logFileName,"w");
      #endif
    }

    MEMORY_ARTI("setup %u bytes free\n", FREE_SIZE);

    if (stages < 1) {close(); return true;}
    bool loadParseTreeFile = false;

    #if ARTI_PLATFORM == ARTI_ARDUINO
      File definitionFile;
      definitionFile = LITTLEFS.open(definitionName, "r");
    #else
      std::fstream definitionFile;
      definitionFile.open(definitionName, std::ios::in);
    #endif

    MEMORY_ARTI("open %s %u ✓\n", definitionName, FREE_SIZE);

    if (!definitionFile) {
      ERROR_ARTI("Definition file %s not found. Press Download wled.json\n", definitionName);
      closeLog();
      return false;
    }
    
    //open definitionFile
    #if ARTI_PLATFORM == ARTI_ARDUINO
      definitionJsonDoc = new DynamicJsonDocument(8192); //currently 5335
    #else
      definitionJsonDoc = new DynamicJsonDocument(16384); //currently 9521
    #endif

    // mandatory tokens:
    //  "ID": "ID",
    //  "INTEGER_CONST": "INTEGER_CONST",
    //  "REAL_CONST": "REAL_CONST",

    MEMORY_ARTI("definitionTree %u => %u ✓\n", (unsigned int)definitionJsonDoc->capacity(), FREE_SIZE); //unsigned int needed when running embedded to suppress warnings

    DeserializationError err = deserializeJson(*definitionJsonDoc, definitionFile);
    if (err) {
      ERROR_ARTI("deserializeJson() of definition failed with code %s\n", err.c_str());
      closeLog();
      return false;
    }
    definitionFile.close();
    definitionJson = definitionJsonDoc->as<JsonObject>();

    JsonObject::iterator objectIterator = definitionJson.begin();
    JsonObject metaData = objectIterator->value();
    const char * version = metaData["version"];
    if (strcmp(version, "0.2.1") < 0) {
      ERROR_ARTI("Version of definition.json file (%s) should be 0.2.1 or higher. Press Download wled.json\n", version);
      closeLog();
      return false;
    }
    const char * startSymbol = metaData["start"];
    if (startSymbol == nullptr) {
      ERROR_ARTI("Setup Error: No start symbol found in definition file\n");
      closeLog();
      return false;
    }

    #if ARTI_PLATFORM == ARTI_ARDUINO
      File programFile;
      programFile = LITTLEFS.open(programName, "r");
    #else
      std::fstream programFile;
      programFile.open(programName, std::ios::in);
    #endif
    MEMORY_ARTI("open %s %u ✓\n", programName, FREE_SIZE);
    if (!programFile) {
      ERROR_ARTI("Program file %s not found\n", programName);
      closeLog();
      return  false;
    }

    //open programFile
    char * programText;
    uint16_t programFileSize;
    #if ARTI_PLATFORM == ARTI_ARDUINO
      programFileSize = programFile.size();
      programText = (char *)malloc(programFileSize+1);
      programFile.read((byte *)programText, programFileSize);
      programText[programFileSize] = '\0';
    #else
      programText = (char *)malloc(programTextSize);
      programFile.read(programText, programTextSize);
      DEBUG_ARTI("programFile size %lu bytes\n", programFile.gcount());
      programText[programFile.gcount()] = '\0';
      programFileSize = strlen(programText);
    #endif
    programFile.close();

    char parseTreeName[fileNameLength];
    strcpy(parseTreeName, programName);
    // if (loadParseTreeFile)
    //   strcpy(parseTreeName, "Gen");
    strcat(parseTreeName, ".json");
    #if ARTI_PLATFORM == ARTI_ARDUINO
      parseTreeJsonDoc = new DynamicJsonDocument(32768); // current largest (Subpixel) 5624 //strlen(programText) * 50); //less memory on arduino: 32 vs 64 bit?
    #else
      parseTreeJsonDoc = new DynamicJsonDocument(65536);  // current largest (Subpixel) 7926 //strlen(programText) * 100
    #endif

    MEMORY_ARTI("parseTree %u => %u ✓\n", (unsigned int)parseTreeJsonDoc->capacity(), FREE_SIZE);

    //parse

    #ifdef ARTI_DEBUG // only read write file if debug is on
      #if ARTI_PLATFORM == ARTI_ARDUINO
        File parseTreeFile;
        parseTreeFile = LITTLEFS.open(parseTreeName, loadParseTreeFile?"r":"w");
      #else
        std::fstream parseTreeFile;
        parseTreeFile.open(parseTreeName, loadParseTreeFile?std::ios::in:std::ios::out);
      #endif
    #endif

    if (stages < 1) {close(); return true;}

    if (!loadParseTreeFile) {
      parseTreeJson = parseTreeJsonDoc->as<JsonVariant>();

      lexer = new Lexer(programText, definitionJson);
      lexer->get_next_token();

      if (stages < 2) {close(); return true;}

      uint8_t result = parse(parseTreeJson, startSymbol, '&', lexer->definitionJson[startSymbol], 0);

      if (this->lexer->pos != strlen(this->lexer->text)) {
        ERROR_ARTI("Symbol %s Program not entirely parsed (%u,%u) %u of %u\n", startSymbol, this->lexer->lineno, this->lexer->column, this->lexer->pos, (unsigned int)strlen(this->lexer->text));
        closeLog();
        return false;
      }
      else if (result == ResultFail) {
        ERROR_ARTI("Symbol %s Program parsing failed (%u,%u) %u of %u\n", startSymbol, this->lexer->lineno, this->lexer->column, this->lexer->pos, (unsigned int)strlen(this->lexer->text));
        closeLog();
        return false;
      }
      else
        DEBUG_ARTI("Symbol %s Parsed until (%u,%u) %u of %u\n", startSymbol, this->lexer->lineno, this->lexer->column, this->lexer->pos, (unsigned int)strlen(this->lexer->text));

      MEMORY_ARTI("definitionTree %u / %u%% (%u %u %u)\n", (unsigned int)definitionJsonDoc->memoryUsage(), 100 * definitionJsonDoc->memoryUsage() / definitionJsonDoc->capacity(), (unsigned int)definitionJsonDoc->size(), definitionJsonDoc->overflowed(), (unsigned int)definitionJsonDoc->nesting());
      MEMORY_ARTI("parseTree      %u / %u%% (%u %u %u)\n", (unsigned int)parseTreeJsonDoc->memoryUsage(), 100 * parseTreeJsonDoc->memoryUsage() / parseTreeJsonDoc->capacity(), (unsigned int)parseTreeJsonDoc->size(), parseTreeJsonDoc->overflowed(), (unsigned int)parseTreeJsonDoc->nesting());
      parseTreeJsonDoc->garbageCollect();
      MEMORY_ARTI("garbageCollect %u / %u%%\n", (unsigned int)parseTreeJsonDoc->memoryUsage(), 100 * parseTreeJsonDoc->memoryUsage() / parseTreeJsonDoc->capacity());
      //199 -> 6905 (34.69)
      //469 -> 15923 (33.95)

      delete lexer; lexer =  nullptr;
    }
    else
    {
      // read parseTree
      #ifdef ARTI_DEBUG // only write file if debug is on
        DeserializationError err = deserializeJson(*parseTreeJsonDoc, parseTreeFile);
        if (err) {
          ERROR_ARTI("deserializeJson() of parseTree failed with code %s\n", err.c_str());
          closeLog();
          return false;
        }
      #endif
    }
    #if ARTI_PLATFORM == ARTI_ARDUINO //not on windows as cause crash???
      free(programText);
    #endif

    MEMORY_ARTI("parse %u ✓\n", FREE_SIZE);

    if (stages < 3) {close(); return true;}

    DEBUG_ARTI("\nOptimizer\n");
    if (!optimize(parseTreeJson)) 
    {
      ERROR_ARTI("Optimize failed\n");
      closeLog();
      return false;
    }

    MEMORY_ARTI("optimize %u ✓\n", FREE_SIZE);

    if (stages < 4) {close(); return true;}

    ANDBG_ARTI("\nAnalyzer\n");
    if (!analyze(parseTreeJson)) 
    {
      ERROR_ARTI("Analyze failed\n");
      closeLog();
      return false;
    }

    MEMORY_ARTI("analyze %u ✓\n", FREE_SIZE);

    #ifdef ARTI_DEBUG // only write parseTree file if debug is on
      if (!loadParseTreeFile)
        serializeJsonPretty(*parseTreeJsonDoc,  parseTreeFile);
      parseTreeFile.close();
    #endif

    if (stages < 5) {close(); return true;}

    //interpret main
    callStack = new CallStack();
    valueStack = new ValueStack();

    if (global_scope != nullptr) //due to undefined functions??? wip
    { 
      RUNLOG_ARTI("\ninterpret %s %u %u\n", global_scope->scope_name, global_scope->scope_level, global_scope->symbolsIndex); 

      if (!interpret(parseTreeJson)) {
        ERROR_ARTI("Interpret main failed\n");
        closeLog();
        return false;
      }
    }
    else
    {
      ERROR_ARTI("\nInterpret global scope is nullptr\n");
      closeLog();
      return false;
    }

    MEMORY_ARTI("Interpret main %u ✓\n", FREE_SIZE);
 
    closeLog();

    return !errorOccurred;
  } // setup

  void close() {
    MEMORY_ARTI("closing Arti %u\n", FREE_SIZE);

    if (callStack != nullptr) {delete callStack; callStack = nullptr;}
    if (valueStack != nullptr) {delete valueStack; valueStack = nullptr;}
    if (global_scope != nullptr) {delete global_scope; global_scope = nullptr;}

    if (definitionJsonDoc != nullptr) {
      MEMORY_ARTI("definitionJson  %u / %u%% (%u %u %u)\n", (unsigned int)definitionJsonDoc->memoryUsage(), 100 * definitionJsonDoc->memoryUsage() / definitionJsonDoc->capacity(), (unsigned int)definitionJsonDoc->size(), definitionJsonDoc->overflowed(), (unsigned int)definitionJsonDoc->nesting());
      delete definitionJsonDoc; definitionJsonDoc = nullptr;
    }

    if (parseTreeJsonDoc != nullptr) {
      MEMORY_ARTI("parseTree       %u / %0u%% (%u %u %u)\n", (unsigned int)parseTreeJsonDoc->memoryUsage(), 100 * parseTreeJsonDoc->memoryUsage() / parseTreeJsonDoc->capacity(), (unsigned int)parseTreeJsonDoc->size(), parseTreeJsonDoc->overflowed(), (unsigned int)parseTreeJsonDoc->nesting());
      delete parseTreeJsonDoc; parseTreeJsonDoc = nullptr;
    }

    MEMORY_ARTI("closed Arti %u ✓\n", FREE_SIZE);

    //non arduino stops log here
    #if ARTI_PLATFORM == ARTI_ARDUINO
      LITTLEFS.remove(logFileName); //cleanup the /edit folder a bit
    #else
      if (logToFile)
      {
        fclose(logFile);
        logToFile = false;
      }
    #endif
  }
}; //ARTI
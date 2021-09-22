#define ARTI_DEBUG 1

using namespace std;

#ifdef WLED_H
  #include "src/dependencies/json/ArduinoJson-v6.h"
#else
  #include "ArduinoJson-recent.h"
#endif

#ifdef ESP32
  #define string String
  #define substr(x,y) substring(x, x+y)
#endif

#ifdef ARTI_DEBUG
  #ifdef ESP32
    #define DEBUG_ARTI(x...) Serial.print("["); Serial.printf(x);Serial.print("]")
  #else
    #define DEBUG_ARTI printf
  #endif
#else
  #define DEBUG_ARTI(x...)
#endif

string stringToUpper(string &s)
{
  string t = s;
   for(unsigned int i = 0; i < s.length(); i++)
  {
    t[i] = toupper(s[i]);
  }
  return t;
}

// void print (string line) {
//   // DEBUG_ARTI("%s\n", line);
// }

int stringToInt(string value) {
  #ifdef ESP32
    return value.toInt();
  #else
    return stoi(value);
  #endif
}

string intToString(int value) {
  #ifdef ESP32
    return String(value);
  #else
    return to_string(value);
  #endif
}

//https://ruslanspivak.com/lsbasi-part19/

enum class ErrorCode {
  UNEXPECTED_TOKEN,
  ID_NOT_FOUND,
  DUPLICATE_ID,
  NONE
};

DynamicJsonDocument tokensJson(2048);
DynamicJsonDocument definitionJson(8192);
DynamicJsonDocument parseTreeJson(16384);

string spaces = "                                                                                      ";

class Token {
  private:
    uint16_t lineno;
    uint16_t column;
  public:
    string type;
    string value; 
    Token() {
      this->type = "NONE";
      this->value = "";
      this->lineno = 0;
      this->column = 0;
    }
    Token(string type, string value, uint16_t lineno=0, uint16_t column=0) {
      this->type = type;
      this->value = value;
      this->lineno = lineno;
      this->column = column;
    }
}; //Token

class Error {
  public:
    ErrorCode error_code;
    Token token;
    string message;
  public:
    Error(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE","", 0, 0), string message="") {
      this->error_code = error_code;
      this->token = token;
      this->message = "typeid(this).name()" + message;
      // DEBUG_ARTI("%s\n", "Error", this->token.type, " " <<this->token.value, this->message); //this->error_code, 
    }
};

class LexerError: public Error {
  public:
    LexerError(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE","", 0, 0), string message="") {
      // DEBUG_ARTI("%s\n", "LexerError", this->token.type, " " <<this->token.value, this->message); //this->error_code, 
      throw("LexerError");
    }
};
class ParserError: public Error {
  public:
    ParserError(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE","", 0, 0), string message="") {
      // DEBUG_ARTI("%s\n", "ParserError", this->token.type, " " <<this->token.value, this->message); //this->error_code, 
      throw("ParserError");
    }
};
class SemanticError: public Error {};

class Lexer {
  private:
    string text;
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
    Lexer(string definitionText, string programText) {
      // DEBUG_ARTI("%s\n", "Lexer init");
      this->text = programText;
      this->pos = 0;
      this->current_char = this->text[this->pos];
      this->lineno = 1;
      this->column = 1;

      DeserializationError err = deserializeJson(definitionJson, definitionText);
      if (err) {
        DEBUG_ARTI("deserializeJson() failed with code %s\n", err.c_str());
      }
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
      string message = "";
      LexerError x = LexerError(ErrorCode::NONE, Token(), message);
      // LexerError x = LexerError(string(""),Token(),string(""));
      DEBUG_ARTI("%s\n", "LexerError");
    }

    void advance() {
      if (this->current_char == '\n') {
        this->lineno += 1;
        this->column = 0;
      }
      this->pos++;

      if (this->pos > this->text.length() - 1)
        this->current_char = -1;
      else {
        this->current_char = this->text[this->pos];
        this->column++;
      }
    }

    char peek(uint8_t ahead) {
      uint16_t peek_pos = this->pos + ahead;

      if (peek_pos > this->text.length() - 1)
        return -1;
      else 
        this->text[peek_pos];
    }

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

      string result = "";
      while (this->current_char != -1 && isdigit(this->current_char)) {
        result += this->current_char;
        this->advance();
      }
      if (this->current_char == '.') {
        result += this->current_char;
        this->advance();

        while (this->current_char != -1 && isdigit(this->current_char)) {
          result += this->current_char;
          this->advance();
        }

        token.type = "REAL_CONST";
        token.value = result;
      }
      else {
        token.type = "INTEGER_CONST";
        token.value = result;
      }

      // DEBUG_ARTI("%s\n", "Number!!! ", token.type, token.value);
      return token;
    }

    Token id() {
        Token token = Token("NONE", "", this->lineno, this->column);

        string value = "";
        while (this->current_char != -1 && isalnum(this->current_char)) {
            value += this->current_char;
            this->advance();
        }

        // string token_type = tokensJson[stringToUpper(value)];
        // DEBUG_ARTI("%s\n", "id ", token_type, token_type.size(), value);
        if (tokensJson[stringToUpper(value)].isNull()) {
            // DEBUG_ARTI("%s\n", "  id empty ");
            token.type = "ID";
            token.value = value;
        }
        else {
            token.type = tokensJson[stringToUpper(value)].as<string>();
            token.value = stringToUpper(value);
        }

        return token;
    }

    Token get_next_token() {

      while (this->current_char != -1 && this->pos <= this->text.length() - 1) {
        // DEBUG_ARTI("%s\n", "get_next_token ", this->current_char);
        if (isspace(this->current_char)) {
          this->skip_whitespace();
          continue;
        }

        if (this->current_char == '{') {
          this->advance();
          this->skip_comment();
          continue;
        }

        if (isalpha(this->current_char)) {
          return this->id();
        }

        if (isdigit(this->current_char)) {
          return this->number();
        }

        // findLongestMatchingToken(tokensJson, 1);
        string token_value = "";
        string token_type = "";
          // token_value = token_value.append(1,this->current_char);

        uint8_t longestTokenLength = 0;

        for (JsonPair tokenPair: tokensJson.as<JsonObject>()) {
          string value = tokenPair.value().as<const char*>();
          string current_string = this->text.substr(this->pos, value.length());
          // DEBUG_ARTI("%s\n", value, token_value);
          if (value == current_string && value.length() > longestTokenLength) {
            token_value = value;
            token_type = tokenPair.key().c_str();
            longestTokenLength = value.length();
          }
        }

        // DEBUG_ARTI("%s\n", "get_next_token (", token_type, ") (", token_value, ")");
        if (token_type != "") {
          // DEBUG_ARTI("%s\n", "get_next_token tvinn", token_type, token_value);
          Token token = Token(token_type, token_value, this->lineno, this->column);
          for (int i=0; i<token_value.length();i++)
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
  string type;
  string value;
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

    string parse() {
      JsonObject definitionObject = definitionJson.as<JsonObject>();
      JsonObject::iterator it = definitionObject.begin();

      DEBUG_ARTI("Parser %s %s\n", this->current_token.type.c_str(), this->current_token.value.c_str());

      string symbol_name = it->key().c_str();
      Result result = visit(parseTreeJson.as<JsonVariant>(), symbol_name, "", definitionJson[symbol_name.c_str()], 0);

      if (result == Result::RESULTFAIL)
        DEBUG_ARTI("Program parsing failed (start=%s)\n", symbol_name.c_str());

      string parseTreeString = "";
      serializeJsonPretty(parseTreeJson, parseTreeString);
      return parseTreeString;
    }

    Token get_next_token() {
      return this->lexer->get_next_token();
    }

    void error(ErrorCode error_code, Token token) {
      ParserError error = ParserError(error_code, token, "");
    }

    void eat(string token_type) {
      // DEBUG_ARTI("%s\n", "try to eat ", this->current_token.type, "-", token_type);
      if (this->current_token.type == token_type) {
        this->current_token = this->get_next_token();
        // DEBUG_ARTI("%s\n", "eating ", token_type, " -> ", this->current_token.type, this->current_token.value);
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
    positions[positions_index].value = this->current_token.value;
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
      this->current_token.value = positions[positions_index].value;
    }
  }

  enum class  Result {
    RESULTFAIL,
    RESULTSTOP,
    RESULTCONTINUE,
  };

  Result visit(JsonVariant parseTree, string symbol_name, string operatorx, JsonVariant expression, int depth = 0) {
    Result result = Result::RESULTCONTINUE;

    if (expression.is<JsonObject>()) {
      // DEBUG_ARTI("%s parseObject %s\n", spaces.substr(0,depth).c_str(), operatorx.c_str());//, expression.as<string>().c_str());

      for (JsonPair element : expression.as<JsonObject>()) {
        string objectOperator = element.key().c_str();
        JsonVariant objectExpression = element.value();
        if (objectOperator == "*") {
          // DEBUG_ARTI("%s\n", "zero or more");
          visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1); //no assign to result as optional
        }
        else if (objectOperator == "?") {
          // DEBUG_ARTI("%s\n", "zero or one (optional) ");
          visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1); //no assign to result as optional
        }
        else if (objectOperator == "+") {
          // DEBUG_ARTI("%s\n", "one or more ");
          result = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1);
        }
        else if (objectOperator == "or") {
          // DEBUG_ARTI("%s\n", "or ");
          result = visit(parseTree, symbol_name, objectOperator, objectExpression, depth + 1);
          if (result != Result::RESULTFAIL) result = Result::RESULTCONTINUE;
        }
        else {
          DEBUG_ARTI("%s %s %s\n", "undefined ", objectOperator.c_str(), objectExpression.as<string>().c_str());//.as<char*>()
          result = Result::RESULTFAIL;
        }
      }
    }
    else { //not object
      if (expression.is<JsonArray>()) {
        // bool failThis = false;
        Result resultChild;
        // DEBUG_ARTI("%s parseArray %s\n", spaces.substr(0,depth).c_str(), operatorx.c_str());//, expression.as<string>().c_str());
        if (operatorx == "") 
          operatorx = "and";

        //check if unary or binary operator
        // if (expression.size() > 1) {
        //   DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
        //   DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
        // }

        for (JsonVariant newExpression: expression.as<JsonArray>()) {
          //Save current position, in case some of the expressions in the or array go wrong (deadend), go back to the saved position and try the next
          if (operatorx == "or")
            push_position();

          resultChild = visit(parseTree, symbol_name, "", newExpression, depth + 1);//(operatorx == "")?"and":operatorx

          if (operatorx == "*") resultChild = Result::RESULTCONTINUE; //0 or more always succesful

          if ((operatorx != "or") && resultChild != Result::RESULTCONTINUE) //result should be continue for and, *, +, ?
            result = Result::RESULTFAIL;
          if ((operatorx == "or") && resultChild != Result::RESULTFAIL) //Stop or continue is enough for an or
            result = Result::RESULTSTOP;

          if (operatorx == "or" && resultChild == Result::RESULTFAIL) //if fail, go back and try another
            pop_position();

          if (result != Result::RESULTCONTINUE) 
            break;
        }
        if ((operatorx == "or") && result == Result::RESULTCONTINUE) //still looking but nothing to look for
          result = Result::RESULTFAIL;
      }
      else { //not array
        string tokenType = expression.as<string>();

        if (!tokensJson[tokenType].isNull()) {

          if (this->current_token.type == tokenType) {
            // DEBUG_ARTI("%s parseTOken %s\n", spaces.substr(0,depth).c_str(), operatorx.c_str());//, expression.as<string>().c_str());
            // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "istoken ok ", tokenType, tokenValue, current_token.type, current_token.value);
            // if (current_token.type == "ID" || current_token.type == "INTEGER" || current_token.type == "REAL" || current_token.type == "INTEGER_CONST" || current_token.type == "REAL_CONST" || current_token.type == "ID" || current_token.type == "ID" || current_token.type == "ID") {
            DEBUG_ARTI("%s %s %s\n", spaces.substr(0,depth).c_str(), current_token.type.c_str(), current_token.value.c_str());

            if (symbol_name[symbol_name.length()-1] == '*') { //if list then add in array
              JsonArray arr = parseTree[symbol_name].as<JsonArray>();
              arr[arr.size()][current_token.type] = current_token.value; //add in last element of array
            }
            else
                parseTree[symbol_name][current_token.type] = current_token.value;

            if (tokenType == "PLUS2") { //debug for unary operators (wip)
              // DEBUG_ARTI("%s\n", "array multiple 1 ", parseTree);
              // DEBUG_ARTI("%s\n", "array multiple 2 ", expression);
            }
            eat(tokenType);
          }
          else {
            // parseTree["deadend"] = tokenType + "<>" + current_token.type;
            result = Result::RESULTFAIL;
          }
        }
        else { //not object, array or token but symbol
          string newSymbol_name = expression;
          JsonVariant newParseTree;

          DEBUG_ARTI("%s %s\n", spaces.substr(0,depth).c_str(), newSymbol_name.c_str());
          // DEBUG_ARTI("%s %s %s\n", spaces.substr(0,depth).c_str(), parseTree[symbol_name].as<string>().c_str(), newSymbol_name.c_str());
          JsonArray arr;
          if (symbol_name[symbol_name.length()-1] == '*') { //if list then create/get array
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

          // DEBUG_ARTI("%s %s\n", spaces.substr(0,depth).c_str(), newSymbol_name.c_str());
          result = visit(newParseTree, newSymbol_name, "", definitionJson[newSymbol_name], depth + 1);

          newParseTree.remove("ccc"); //remove connector

          if (result == Result::RESULTFAIL) {
            newParseTree.remove(newSymbol_name); //remove result of visit

          //   DEBUG_ARTI("%s psf %s %s\n", spaces.substr(0,depth).c_str(), parseTree[symbol_name].as<string>().c_str(), newSymbol_name.c_str());
          if (symbol_name[symbol_name.length()-1] == '*') //if list then remove empty objecy
            arr.remove(arr.size()-1);
          // else
          //   parseTree.remove(newSymbol_name); //this does not change anything...
          } //f
          // DEBUG_ARTI("%s %s\n", spaces.substr(0,depth).c_str(), parseTree[symbol_name].as<string>().c_str());

        }
      } //if array
    } //if object
    // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "tokenValue ", tokenValue, isArray, isToken, isObject);
    
     return result;
  } //visit

}; //Parser

class ScopedSymbolTable; //forward declaration

class Symbol {
  private:
  public:
  
  string name;
  string type;
  uint8_t scope_level;
  string symbol_type;
  ScopedSymbolTable* scope = nullptr;
  ScopedSymbolTable* detail_scope = nullptr;

  JsonVariant block;

  Symbol(string symbol_type, string name, string type = "") {
    this->symbol_type = symbol_type;
    this->name = name;
    this->type = type;
    this->scope_level = 0;
  }
}; //Symbol


class ScopedSymbolTable {
  private:
  public:

  Symbol* _symbols[100];
  uint16_t _symbolsIndex = 0;
  string scope_name;
  int scope_level;
  ScopedSymbolTable *enclosing_scope;
  ScopedSymbolTable *child_scopes[100];
  uint16_t child_scopesIndex = 0;

  ScopedSymbolTable(string scope_name, int scope_level, ScopedSymbolTable *enclosing_scope = nullptr) {
    // DEBUG_ARTI("%s\n", "ScopedSymbolTable ", scope_name, scope_level);
    this->scope_name = scope_name;
    this->scope_level = scope_level;
    this->enclosing_scope = enclosing_scope;
  }

  void init_builtins() {
        // this->insert(BuiltinTypeSymbol('INTEGER'));
        // this->insert(BuiltinTypeSymbol('REAL'));
  }

  #define _SHOULD_LOG_SCOPE
  void log(string msg) {
    #ifdef _SHOULD_LOG_SCOPE
      // DEBUG_ARTI("%s\n", "Log scope ", msg);
    #endif
  }

  void insert(Symbol* symbol) {
    this->log("Insert: " + symbol->name);
    symbol->scope_level = this->scope_level;
    symbol->scope = this;
    this->_symbols[_symbolsIndex] = symbol;
    _symbolsIndex++;
  }

  Symbol* lookup(string name, bool current_scope_only=false, bool child_scopes_included=false) {
    // this->log("Lookup: " + name + " " + this->scope_name);
    // DEBUG_ARTI("%s\n", "lookup ", name, this->scope_name, _symbolsIndex, child_scopesIndex);
    //  'symbol' is either an instance of the Symbol class or None;
    for (int i=0; i<_symbolsIndex; i++) {
      // DEBUG_ARTI("%s\n", "  symbols ", i, _symbols[i]->symbol_type, _symbols[i]->name, _symbols[i]->type, _symbols[i]->scope_level);
      if (_symbols[i]->name == name)
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

    // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "show ", this->scope_name, " " , this->scope_level);
    for (int i=0; i<_symbolsIndex; i++) {
      // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "-symbols ", i, _symbols[i]->symbol_type, _symbols[i]->name, _symbols[i]->type);
    }

      for (int i=0; i<this->child_scopesIndex;i++) {
        // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "-detail ", i, this->child_scopes[i]->scope_name);
        this->child_scopes[i]->show(depth + 1);
      }
  }
}; //ScopedSymbolTable

class TreeWalker {
  private:
  public:
    TreeWalker() {
    }

    void walk(JsonVariant tree) {
      DEBUG_ARTI("\nWalker\n");
      visit(tree);
    }

    void visit(JsonVariant parseTree, string symbol_name = "", string token = "", uint8_t depth = 0) {


      if (parseTree.is<JsonObject>()) {
        for (JsonPair element : parseTree.as<JsonObject>()) {
          string key = element.key().c_str();
          JsonVariant value = element.value();
          symbol_name = key;
          DEBUG_ARTI("%s Visit object %s %s\n", spaces.substr(0,depth).c_str(), symbol_name.c_str(), token.c_str());
          visit(value, symbol_name, token, depth + 1);
        } // key values
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            DEBUG_ARTI("%s Visit array %s %s\n", spaces.substr(0,depth).c_str(), symbol_name.c_str(), token.c_str());
            // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Array ", parseTree[i], "  ";
            visit(newParseTree, symbol_name, token, depth + 1);
          }
        }
        else { //not array
          DEBUG_ARTI("%s visit element %s\n", spaces.substr(0,depth).c_str(), parseTree.as<string>().c_str());
        }
      }
    } //visit

}; //TreeWalker


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

    void visit(JsonVariant parseTree, string symbol_name = "", string token = "", ScopedSymbolTable* current_scope = nullptr, uint8_t depth = -1) {

      // DEBUG_ARTI("%s Visit %s %s\n", spaces.substr(0,depth).c_str(), symbol_name.c_str(), token.c_str());

      if (parseTree.is<JsonObject>()) {
        for (JsonPair element : parseTree.as<JsonObject>()) {
          string key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!definitionJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = definitionJson["SEMANTICS"][symbol_name];
            if (!expression.isNull())
            {
              // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Symbol ", symbol_name,  " ", expression);
              // JsonObject expressionObject = expression.as<JsonObject>();
              // JsonObject::iterator it = expressionObject.begin();
              // string keykey = it->key().c_str();

              if (expression == "Program") {
                string program_name = value["variable"]["ID"];
                this->global_scope = new ScopedSymbolTable(program_name, 1, nullptr); //current_scope

                DEBUG_ARTI("%s Program %s %d %d\n", spaces.substr(0,depth).c_str(), global_scope->scope_name.c_str(), global_scope->scope_level, global_scope->_symbolsIndex); 

                // current_scope->child_scopes[current_scope->child_scopesIndex++] = this->global_scope;
                // current_scope = global_scope;
                visit(value["block"], symbol_name, token, this->global_scope, depth + 1);

                for (int i=0; i<global_scope->_symbolsIndex; i++) {
                  Symbol* symbol = global_scope->_symbols[i];
                  DEBUG_ARTI("%s %d %s %s %s %d\n", spaces.substr(0,depth).c_str(), i, symbol->symbol_type.c_str(), symbol->name.c_str(), symbol->type.c_str(), symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (expression == "Procedure") {

                //find the procedure name (so we must know this is a procedure...)
                string proc_name = value["ID"];
                Symbol* proc_symbol = new Symbol(symbol_name, proc_name);
                current_scope->insert(proc_symbol);

                DEBUG_ARTI("%s procedure %s %s\n", spaces.substr(0,depth).c_str(), current_scope->scope_name.c_str(), proc_name.c_str());
                ScopedSymbolTable* procedure_scope = new ScopedSymbolTable(proc_name, current_scope->scope_level + 1, current_scope);
                current_scope->child_scopes[current_scope->child_scopesIndex++] = procedure_scope;
                proc_symbol->detail_scope = procedure_scope;
                // DEBUG_ARTI("%s\n", "ASSIGNING ", proc_symbol->name, " " , procedure_scope->scope_name);

                // current_scope = procedure_scope;
                visit(value["formal_parameter_list*"], symbol_name, token, procedure_scope, depth + 1);

                visit(value["block"], symbol_name, token, procedure_scope, depth + 1);

                // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "end proc ", symbol_name, procedure_scope->scope_name, procedure_scope->scope_level, procedure_scope->_symbolsIndex); 

                for (int i=0; i<procedure_scope->_symbolsIndex; i++) {
                  Symbol* symbol = procedure_scope->_symbols[i];
                  DEBUG_ARTI("%s %d %s %s %s %d\n", spaces.substr(0,depth).c_str(), i, symbol->symbol_type.c_str(), symbol->name.c_str(), symbol->type.c_str(), symbol->scope_level); 
                }

                visitCalledAlready = true;
              }
              else if (expression == "VarSymbol") {
                // DEBUG_ARTI("%s var (from array) %s %s %s\n", spaces.substr(0,depth).c_str(), current_scope->scope_name.c_str(), value.as<string>().c_str(), key.c_str());
                //can be expression or array of expressions
                if (value.is<JsonArray>()) {
                  for (JsonObject newValue: value.as<JsonArray>()) {
                    string param_name = newValue["ID"];
                    string param_type = newValue["type_spec"];//current_scope.lookup(param.type_node.value);
                    Symbol* var_symbol = new Symbol(symbol_name, param_name, param_type);
                    current_scope->insert(var_symbol);
                    DEBUG_ARTI("%s var (from array) %s.%s of %s\n", spaces.substr(0,depth).c_str(), current_scope->scope_name.c_str(), param_name.c_str(), param_type.c_str());
                  }
                }
                else {
                  string param_name = value["ID"];
                  string param_type = value["type_spec"];//current_scope.lookup(param.type_node.value);
                  Symbol* var_symbol = new Symbol(symbol_name, param_name, param_type);
                  current_scope->insert(var_symbol);
                  DEBUG_ARTI("%s var %s.%s of %s\n", spaces.substr(0,depth).c_str(), current_scope->scope_name.c_str(), param_name.c_str(), param_type.c_str());
                }
              }
              else if (expression == "Assign") {
                JsonVariant left = value["variable"]["ID"];
                // JsonVariant right = value["expr*"];
                DEBUG_ARTI("%s Assign %s =\n", spaces.substr(0,depth).c_str(), left.as<string>().c_str());

                DynamicJsonDocument newx(1024);
                JsonObject newObject = newx.createNestedObject();
                newObject["expr*"] = value["expr*"];

                visit(newObject, symbol_name, token, current_scope, depth + 1);
                visitCalledAlready = true;
              }
            } // is expression

          } // is symbol_name

          if (!tokensJson[key].isNull()) {
            token = key;
            // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Token ", token);
          }
          // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Object ", key, value);

          if (!visitCalledAlready)
            visit(value, symbol_name, token, current_scope, depth + 1);

        } // key values
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Array ", parseTree[i], "  ";
            visit(newParseTree, symbol_name, token, current_scope, depth + 1);
          }
        }
        else { //not array
          // string element = parseTree;
          // DEBUG_ARTI("%s value notnot %s\n", spaces.substr(0,depth).c_str(), element.c_str());
          // if (definitionJson["SEMANTICS"][element])
        }
      }
    } //visit

}; //SemanticAnalyzer

class ActivationRecord {
  private:
  public:
    string name;
    string type;
    int nesting_level;
    DynamicJsonDocument *members;
    JsonObject mem;

    ActivationRecord(string name, string type, int nesting_level) {
        this->name = name;
        this->type = type;
        this->nesting_level = nesting_level;
        this->members = new DynamicJsonDocument(1024);
        this->mem = this->members->createNestedObject();
    }

    void set(string key, string value) {
      // JsonObject mem = members->as<JsonObject>();
      mem[key] = value;
      // cout <<"Set ", key, value, mem);
        // this->members[key] = value;
    }
    string get(string key) {
        // return this->members.get(key);
      // JsonObject mem = members->as<JsonObject>();
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
    string stack[100];
    uint8_t stack_index = 0;
  public:
  Calculator() {

  }

  void push(string key, string value) {
    // DEBUG_ARTI("calc push %s %s\n", key.c_str(), value.c_str());
      stack[stack_index++] = value;
    return;
    // bool doneSomething = true;
    if (key == "INTEGER_CONST") {
      stack[stack_index++] = value;
    }
    else if (key == "variable") {
      stack[stack_index++] = value;
    }
    else if (key == "PLUS") {
      stack[stack_index-2] = intToString(stringToInt(stack[stack_index-2]) + stringToInt(stack[stack_index-1]));
      stack_index--;
    }
    else if (key == "MUL") {
      stack[stack_index-2] = intToString(stringToInt(stack[stack_index-2]) * stringToInt(stack[stack_index-1]));
      stack_index--;
    }
    // else
    //   doneSomething = false;

    // if (doneSomething)
    //   DEBUG_ARTI("%s\n", "Push ", key, value, stack[stack_index-1]);
  }

  string peek() {
    // DEBUG_ARTI("Peek %s\n", stack[stack_index-1].c_str());
    return stack[stack_index-1];
  }

  string pop() {
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

      DEBUG_ARTI("\ninterpret %s %d %d %d\n", global_scope->scope_name.c_str(), global_scope->scope_level, global_scope->_symbolsIndex, global_scope->child_scopesIndex); 
      for (int i=0; i<global_scope->_symbolsIndex; i++) {
        Symbol* symbol = global_scope->_symbols[i];
        DEBUG_ARTI("scope %s %s %s %d\n", symbol->symbol_type.c_str(), symbol->name.c_str(), symbol->type.c_str(), symbol->scope_level); 
      }
    }
    else
      DEBUG_ARTI("Interpret global scope is nullptr\n");

    visit(parseTreeJson.as<JsonVariant>());
  }

            // ["PLUS2", "factor"],
          // [ "MINUS2", "factor"],

  // JsonObject objectFromKeyValue(string key, JsonVariant value) {
  //   DynamicJsonDocument newx(1024);
  //   JsonObject newObject = newx.createNestedObject();
  //   newObject[key] = value[key];

  //   return newObject;

  // }

    void visit(JsonVariant parseTree, string symbol_name = "", string token = "", uint8_t depth = 0) {

      if (parseTree.is<JsonObject>()) {
        for (JsonPair element : parseTree.as<JsonObject>()) {
          string key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!definitionJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = definitionJson["INTERPRETER"][symbol_name];
            if (!expression.isNull())
            {
              // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Symbol ", symbol_name,  " ", expression);

              if (expression == "Program") {
                string program_name = value["variable"]["ID"];

                ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM",1);
                DEBUG_ARTI("%s %s %s %s\n", spaces.substr(0,depth).c_str(), expression.as<string>().c_str(), ar->name.c_str(), program_name.c_str());

                this->call_stack.push(ar);

                visit(value["block"], symbol_name, token, depth + 1);

                this->call_stack.pop();

                visitCalledAlready = true;
              }
              else if (expression == "Procedure") {
                string proc_name = value["ID"];
                Symbol* proc_symbol = this->global_scope->lookup(proc_name, true, true);
                DEBUG_ARTI("%s Save block of %s\n", spaces.substr(0,depth).c_str(), proc_name.c_str());
                proc_symbol->block = value["block"];
                visitCalledAlready = true;
              }
              else if (expression == "ProcedureCall") {
                string proc_name = value["ID"];
                Symbol* proc_symbol = this->global_scope->lookup(proc_name, true, true);

                if (proc_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                ActivationRecord* ar = new ActivationRecord(proc_name, "PROCEDURE", proc_symbol->scope_level + 1);

                DEBUG_ARTI("%s %s %s %s %s\n", spaces.substr(0,depth).c_str(), expression.as<string>().c_str(), ar->name.c_str(), proc_name.c_str(), proc_symbol->name.c_str());

                visit(value["actual_parameter_list*"], symbol_name, token, depth + 1);

                for (int i=proc_symbol->detail_scope->_symbolsIndex-1; i>=0;i--) { //backwards because popped in reversed order
                  if (proc_symbol->detail_scope->_symbols[i]->symbol_type == "formal_parameters") { //select formal parameters
                  //{"ID":"Alpha","LPAREN":"(","actual_parameter_list*":[
                    //      {"expr*":[{"term*":[{"factor":{"INTEGER_CONST":"3"}},{}]},{"PLUS":"+"},{"term*":[{"factor":{"INTEGER_CONST":"5"}},{}]}]},{"COMMA":","},
                    //      {"expr*":[{"term*":[{"factor":{"INTEGER_CONST":"7"}},{}]},{}]}],"RPAREN":")","SEMI":";"} 
                    string result = calculator.pop();
                    ar->set(proc_symbol->detail_scope->_symbols[i]->name, result.c_str());
                    DEBUG_ARTI("%s %s = %s\n", spaces.substr(0,depth).c_str(), proc_symbol->detail_scope->_symbols[i]->name.c_str(), result.c_str());
                  }
                }

                this->call_stack.push(ar);

                //find block of procedure... lookup procedure?
                //visit block of procedure
                // DEBUG_ARTI("%s proc block %s\n", spaces.substr(0,depth).c_str(), proc_symbol->block.as<string>().c_str());

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
                visit(proc_symbol->block, symbol_name, token, depth + 1);

                this->call_stack.pop();

                visitCalledAlready = true;
                } //proc_symbol != nullptr
                else {
                  DEBUG_ARTI("%s %s not found %s\n", spaces.substr(0,depth).c_str(), expression.as<string>().c_str(), proc_name.c_str());
                }

              }
              else if (expression == "Assign") {
                DynamicJsonDocument newx(1024);
                JsonObject newObject = newx.createNestedObject();
                newObject["expr*"] = value["expr*"];

                visit(newObject, symbol_name, token, depth + 1);

                ActivationRecord* ar = this->call_stack.peek();
                ar->set(value["variable"]["ID"], this->calculator.pop());

                DEBUG_ARTI("%s %s := %s\n", spaces.substr(0,depth).c_str(), value["variable"]["ID"].as<string>().c_str(), ar->get(value["variable"]["ID"]).c_str());

                visitCalledAlready = true;
              }
              else if (expression == "Exprs" || expression == "Terms") {
                // DEBUG_ARTI("%s %s tovisit %s\n", spaces.substr(0,depth).c_str(), expression.as<string>().c_str(), value.as<string>().c_str());
                string operatorx = "";
                if (value.is<JsonArray>()) {
                  JsonArray valueArray = value.as<JsonArray>();
                  if (valueArray.size() >= 1) // visit first symbol 
                    visit(valueArray[0], symbol_name, token, depth + 1);
                  if (valueArray.size() >= 3) { // add operator and another symbol
                    operatorx = valueArray[1].as<string>();
                    // DEBUG_ARTI("%s %s operator %s\n", spaces.substr(0,depth).c_str(), expression.as<string>().c_str(), operatorx.c_str());
                    visit(valueArray[2], symbol_name, token, depth + 1);
                  }
                  if (valueArray.size() != 1 && valueArray.size() != 3)
                    DEBUG_ARTI("%s %s array not right size ?? (%d) %s %s \n", spaces.substr(0,depth).c_str(), expression.as<string>().c_str(), valueArray.size(), key.c_str(), value.as<string>().c_str());
                }
                else
                  DEBUG_ARTI("%s %s not array?? %s %s \n", spaces.substr(0,depth).c_str(), key.c_str(), expression.as<string>().c_str(), value.as<string>().c_str());
                if (operatorx.find("PLUS") != string::npos) {
                  string right = calculator.pop();
                  string left = calculator.pop();
                  int result = stringToInt(left) + stringToInt(right);
                  DEBUG_ARTI("%s %s + %s = %d\n", spaces.substr(0,depth).c_str(), left.c_str(), right.c_str(), result);
                  calculator.push("PLUS", intToString(result));
                }
                if (operatorx.find("MUL") != string::npos) {
                  string right = calculator.pop();
                  string left = calculator.pop();
                  int result = stringToInt(left) * stringToInt(right);
                  DEBUG_ARTI("%s %s * %s = %d\n", spaces.substr(0,depth).c_str(), left.c_str(), right.c_str(), result);
                  calculator.push("MUL", intToString(result));
                }
                visitCalledAlready = true;
              }
              else if (expression == "Variable") {
                ActivationRecord* ar = this->call_stack.peek();
                calculator.push(key, ar->get(value["ID"]));
                DEBUG_ARTI("%s %s %s %s\n", spaces.substr(0,depth).c_str(), key.c_str(), value["ID"].as<string>().c_str(), ar->get(value["ID"]).c_str());
                visitCalledAlready = true;
              }
              else if (expression == "ForLoop") {
                DEBUG_ARTI("%s for loop\n", spaces.substr(0,depth).c_str());

                DynamicJsonDocument newx(1024);
                JsonObject newObject = newx.createNestedObject();
                newObject["assignment_statement"] = value["assignment_statement"];

                visit(newObject, symbol_name, token, depth + 1);
                visit(value["expr*"], symbol_name, token, depth + 1);
                ActivationRecord* ar = this->call_stack.peek();
                for (int i=0; i<2;i++) { //this is the current state of this project: adding for loops, of course the from and to should be derived from the code ;-)
                  DEBUG_ARTI("%s iteration %d\n", spaces.substr(0,depth).c_str(), i);
                  ar->set("y", intToString(i));
                  visit(value["compound_statement"], symbol_name, token, depth + 1);
                }
                visitCalledAlready = true;
              }
            } //if expression

          } // is key is symbol_name

          // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Object ", key, value);
          // if (key == "INTEGER_CONST" || key == "PLUS" || key == "MUL" || key == "LPAREN"  || key == "RPAREN" ) {
          if (key == "INTEGER_CONST") {// || value == "+" || value == "*") || value == "("  || value == ")" ) {
            calculator.push(key, value.as<string>());
            // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Calculator (push) ", key, value, calculator.peek());
            visitCalledAlready = true;
          }

          if (!tokensJson[key].isNull()) { //if key is token
            token = key;
            // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Token ", token);
          }
          if (!visitCalledAlready)
            visit(value, symbol_name, token, depth + 1);

        } // key values
      }
      else { //not object
        if (parseTree.is<JsonArray>()) {
          for (JsonVariant newParseTree: parseTree.as<JsonArray>()) {
            // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "Array ", parseTree[i], "  ";
            visit(newParseTree, symbol_name, token, depth + 1);
          }
          // cout);
        }
        else { //not array
          string element = parseTree;
          // DEBUG_ARTI("%s\n", spaces.substr(0,depth).c_str(), "not array not object but element ", element);
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
  string definitionText;
  string programText;
public:
  ARTI(string definitionText, string programText) {
    this->definitionText = definitionText;
    this->programText = programText;
  }

  string parse() {

    string parseTreeText;
    lexer = new Lexer(definitionText, programText);
    lexer->fillTokensJson();
    parser = new Parser(lexer);
    parseTreeText = parser->parse();
    return parseTreeText;
  }

  void walk() {
    treeWalker = new TreeWalker();
    treeWalker->walk(parseTreeJson.as<JsonVariant>());
  }
  void analyze() {
    semanticAnalyzer = new SemanticAnalyzer();
    semanticAnalyzer->analyse();
  }

  void interpret() {
    interpreter = new Interpreter(semanticAnalyzer);
    interpreter->interpret();// interpreter.print();
  }

}; //ARTI

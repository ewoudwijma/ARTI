#include <iostream>
#include <string>
#include <fstream>
#include "ArduinoJson-v6.18.3.h"
#include <sstream>

using namespace std;

string stringToUpper(string &s)
{
  string t = s;
   for(unsigned int i = 0; i < s.length(); i++)
  {
    t[i] = toupper(s[i]);
  }
  return t;
}

void print (string line) {
  cout << line << endl;
}

//https://ruslanspivak.com/lsbasi-part19/

enum class ErrorCode {
  UNEXPECTED_TOKEN,
  ID_NOT_FOUND,
  DUPLICATE_ID,
  NONE
};

DynamicJsonDocument bnfJson(20480);
DynamicJsonDocument tokenTypeJson(20480);

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
      this->message = typeid(this).name() + message;
      // cout << "Error" << " " << this->token.type << " " <<this->token.value << " " << this->message << endl; //this->error_code << 
    }
};

class LexerError: public Error {
  public:
    LexerError(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE","", 0, 0), string message="") {
      cout << "LexerError" << " " << this->token.type << " " <<this->token.value << " " << this->message << endl; //this->error_code << 
      throw("LexerError");
    }
};
class ParserError: public Error {
  public:
    ParserError(ErrorCode error_code=ErrorCode::NONE, Token token=Token("NONE","", 0, 0), string message="") {
      cout << "ParserError" << " " << this->token.type << " " <<this->token.value << " " << this->message << endl; //this->error_code << 
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
    Lexer(string text) {
      cout << "Lexer init" << endl;
      this->text = text;
      this->pos = 0;
      this->current_char = this->text[this->pos];
      this->lineno = 1;
      this->column = 1;
    }

    void fillTokenTypeJson(JsonObject tree) {

      //system tokens
      tokenTypeJson["ID"] = "ID";
      tokenTypeJson["INTEGER_CONST"] = "INTEGER_CONST";
      tokenTypeJson["REAL_CONST"] = "REAL_CONST";
      // tokenTypeJson["EOF"]  = "EOF";

      if (tree["TOKENS"].is<JsonObject>()) {
        for (JsonPair element : tree["TOKENS"].as<JsonObject>()) {
          string type = element.key().c_str();
          string value = element.value();
          // cout << type << " " << value << endl;
          tokenTypeJson[type] = value;

        }
        // fstream parseTreeFile;
        // parseTreeFile.open("tokenTypeJson.json", ios::out);
        // serializeJsonPretty(tokenTypeJson, parseTreeFile);
        // parseTreeFile.close();
      }
    } //fillTokenTypeJson

    void error() {
      cout << "Lexer error on " << this->current_char << " line: " << this->lineno <<  " column: " <<  this->column << endl;
      string message = "";
      LexerError x = LexerError(ErrorCode::NONE, Token(), message);
      // LexerError x = LexerError(string(""),Token(),string(""));
      cout << "LexerError" << endl;
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

      // cout << "Number!!! " << token.type << " " << token.value << endl;
      return token;
    }

    Token id() {
        Token token = Token("NONE", "", this->lineno, this->column);

        string value = "";
        while (this->current_char != -1 && isalnum(this->current_char)) {
            value += this->current_char;
            this->advance();
        }

        // string token_type = tokenTypeJson[stringToUpper(value)];
        // cout << "id " << token_type << " " << token_type.size() << " " << value << endl;
        if (tokenTypeJson[stringToUpper(value)].isNull()) {
            // cout << "  id empty " << endl;
            token.type = "ID";
            token.value = value;
        }
        else {
            token.type = tokenTypeJson[stringToUpper(value)].as<string>();
            token.value = stringToUpper(value);
        }

        return token;
    }

    Token get_next_token() {

      while (this->current_char != -1 && this->pos <= this->text.length() - 1) {
        // cout << "get_next_token " << this->current_char << endl;
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

        // findLongestMatchingToken(tokenTypeJson, 1);
        string token_value = "";
        string token_type = "";
          // token_value = token_value.append(1,this->current_char);

        uint8_t longestTokenLength = 0;

        JsonObject root = tokenTypeJson.as<JsonObject>();
        for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it) {
          string value = it->value().as<const char*>();
          string current_string = this->text.substr(this->pos, value.length());
          // cout << value << " " << token_value << endl;
          if (value == current_string && value.length() > longestTokenLength) {
            token_value = value;
            token_type = it->key().c_str();
            longestTokenLength = value.length();
          }
        }

        // cout << "get_next_token (" << token_type << ") (" << token_value << ")" << endl;
        if (token_type != "") {
          // cout << "get_next_token tvinn" << token_type << " " << token_value << endl;
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
    Lexer lexer;
    LexerPosition positions[100]; //should be array of pointers but for some reason get seg fault
    uint16_t positions_index = 0;

  public:
    Token current_token;

    Parser(Lexer lexer) {
      this->lexer = lexer;
      this->current_token = this->get_next_token();
      cout << "Parser init " << this->current_token.type << ": " << this->current_token.value << endl;
    }

    Token get_next_token() {
      return this->lexer.get_next_token();
    }

    void error(ErrorCode error_code, Token token) {
      ParserError error = ParserError(error_code, token, "");
    }

    void eat(string token_type) {
      // cout << "try to eat " << this->current_token.type << "-" << token_type << endl;
      if (this->current_token.type == token_type) {
        this->current_token = this->get_next_token();
        cout << "eating " << token_type << " -> " << this->current_token.type << " " << this->current_token.value << endl;
      }
      else {
        this->error(ErrorCode::UNEXPECTED_TOKEN, this->current_token);
      }
    }

  void push_position() {
    // cout << "push_position " << positions_index << " " << this->lexer.pos << endl;
    uint16_t indpositions_indexexio = positions_index%100;
    positions[positions_index].pos = this->lexer.pos;
    positions[positions_index].current_char = this->lexer.current_char;
    positions[positions_index].lineno = this->lexer.lineno;
    positions[positions_index].column = this->lexer.column;
    positions[positions_index].type = this->current_token.type;
    positions[positions_index].value = this->current_token.value;
    positions_index++;
  }

  void pop_position() {
    if (positions_index > 0) {
      positions_index--;
      // cout << "pop_position " << positions_index << " " << this->lexer.pos << " to " << positions[positions_index].pos << endl;
      uint16_t index = positions_index%100;
      this->lexer.pos = positions[positions_index].pos;
      this->lexer.current_char = positions[positions_index].current_char;
      this->lexer.lineno = positions[positions_index].lineno;
      this->lexer.column = positions[positions_index].column;
      this->current_token.type = positions[positions_index].type;
      this->current_token.value = positions[positions_index].value;
    }
  }

  enum class  Result {
    RESULTFAIL,
    RESULTSTOP,
    RESULTCONTINUE,
  };

    Result parseExpression(JsonVariant tree, string operatorx, JsonVariant expression, int depth = 0) {
      string spaces(depth*2, ' ');
      string resultString = expression;
      // serializeJson(expression, resultString);
      Result result = Result::RESULTCONTINUE;

      if (expression.is<JsonObject>()) {
        resultString = "parse Object: " + operatorx + " " + resultString;
        // cout << spaces << resultString << " " << expressionObject.size() << endl;

              for (JsonPair element : expression.as<JsonObject>()) {
                string objectOperator = element.key().c_str();
                JsonVariant objectExpression = element.value();
                if (objectOperator == "*") {
                  // cout << "zero or more" << endl;
                  parseExpression(tree, objectOperator, objectExpression, depth + 1); //no assign to result as optional
                }
                else if (objectOperator == "?") {
                  // cout << "zero or one (optional) " << endl;
                  parseExpression(tree, objectOperator, objectExpression, depth + 1); //no assign to result as optional
                }
                else if (objectOperator == "+") {
                  // cout << "one or more " << endl;
                  result = parseExpression(tree, objectOperator, objectExpression, depth + 1);
                }
                else if (objectOperator == "or") {
                  // cout << "or " << endl;
                  result = parseExpression(tree, objectOperator, objectExpression, depth + 1);
                  if (result != Result::RESULTFAIL) result = Result::RESULTCONTINUE;
                }
                else {
                  cout << "undefined " << objectOperator << " : " << objectExpression << endl;//.as<char*>()
                  result = Result::RESULTFAIL;
                }
              }
      }
      else { //not object
        if (expression.is<JsonArray>()) {
          bool failThis = false;
          Result resultChild;
          resultString = "parse Array: " + operatorx + " " + resultString;
          // cout << spaces << resultString << " " << expression.size() << " " << depth << endl;
          if (operatorx == "") 
            operatorx = "and";

          //check if unary or binary operator
          // if (expression.size() > 1) {
          //   cout << "array multiple 1 " << tree << endl;
          //   cout << "array multiple 2 " << expression << endl;
          // }

          for (int i=0; i < expression.size() && result == Result::RESULTCONTINUE; i++) {
            //Save current position, in case some of the expressions in the or array go wrong (deadend), go back to the saved position and try the next
            if (operatorx == "or")
              push_position();

            resultChild = parseExpression(tree, "", expression[i], depth + 1);//(operatorx == "")?"and":operatorx

            if (operatorx == "*") resultChild = Result::RESULTCONTINUE; //0 or more always succesful

            if ((operatorx != "or") && resultChild != Result::RESULTCONTINUE) //result should be continue for and, *, +, ?
              result = Result::RESULTFAIL;
            if ((operatorx == "or") && resultChild != Result::RESULTFAIL) //Stop or continue is enough for an or
              result = Result::RESULTSTOP;

            if (operatorx == "or" && resultChild == Result::RESULTFAIL) //if fail, go back and try another
              pop_position();
          }
          if ((operatorx == "or") && result == Result::RESULTCONTINUE) //still looking but nothing to look for
            result = Result::RESULTFAIL;
        }
        else { //not array
          string tokenType = expression.as<string>();

          if (!tokenTypeJson[tokenType].isNull()) {
            if (this->current_token.type == tokenType) {
              // cout << spaces << "istoken ok " << tokenType << " " << tokenValue << " " << current_token.type << " " << current_token.value << endl;
              // if (current_token.type == "ID" || current_token.type == "INTEGER" || current_token.type == "REAL" || current_token.type == "INTEGER_CONST" || current_token.type == "REAL_CONST" || current_token.type == "ID" || current_token.type == "ID" || current_token.type == "ID") {
                tree[current_token.type] = current_token.value;
                // string output = parseTreeJson;
                // //serializeJson(parseTreeJson, output);
                // cout <<" result " << output << endl;
              // }
              if (tokenType == "PLUS2") { //debug for unary operators (wip)
                cout << "array multiple 1 " << tree << endl;
                cout << "array multiple 2 " << expression << endl;
              }
              eat(tokenType);
            }
            else {
              // tree["deadend"] = tokenType + "<>" + current_token.type;
              result = Result::RESULTFAIL;
            }
            resultString = "parse Token: " + operatorx + " " + resultString;
            // cout << spaces << resultString << " " << tree << endl;
          }
          else { //not object, array or token but symbol
            string symbol_name = expression;
            resultString = "parse Symbol: " + operatorx + " " + resultString;
            // cout << spaces << "parseExpression isSymbol " << operatorx << " " << expression << endl;
            result = parseSymbol(tree, symbol_name, depth + 1);
          }

        }
      }
      // cout << spaces << "tokenValue " << tokenValue << isArray << isToken << isObject << endl;
      if (result==Result::RESULTFAIL) {
        resultString = "Fail " + resultString;
        // tree["failexp"] = resultString;
      }
      else if (result==Result::RESULTSTOP)
        resultString = "Stop " + resultString;
      else
        resultString = "Continue " + resultString;

      // if (result!=Result::RESULTFAIL)
      //   cout << spaces << "} " << resultString << " " << depth << endl;
      
      return result;
    } //parseExpression

    Result parseSymbol(JsonVariant tree, string symbol_name, int depth = 0) {

      string spaces(depth*2, ' ');
      string resultString = "parse Symbol " + symbol_name;
      // cout << spaces << resultString << endl;

      // cout << spaces << "{" << endl;
      Result result = Result::RESULTFAIL;

      DynamicJsonDocument expressionTreeDoc(20480);
      JsonObject expressionTree = expressionTreeDoc.createNestedObject();
      // expressionTree["name"] = symbol_name;

      JsonVariant expression = bnfJson[symbol_name];
      result = parseExpression(expressionTree, "", expression, depth + 1);

      if (result != Result::RESULTFAIL) {
        //repeated symbol: check if expression is already array, if not make array of it
        if (tree.containsKey(symbol_name)) {
          if (tree[symbol_name].is<JsonObject>()) {
            DynamicJsonDocument arrayDoc(20480);
            JsonArray array = arrayDoc.createNestedArray();
            array.add(tree[symbol_name]);
            array.add(expressionTree);
            tree[symbol_name] = array; //now it is an array
            // cout << spaces << "isObject (now array) " << tree << endl;
          }
          else if (tree[symbol_name].is<JsonArray>()) {
            tree[symbol_name].add(expressionTree);
            cout << spaces << "isArray (PROGRAMMER TEST!!!)" << tree << endl;
          }
          else
            cout << "Error check it..." << endl;

          // tree[symbol_name+"1"] = expressionTree;
          // cout << spaces << "  add new1 " << tree << endl;
        }
        else {
          tree[symbol_name] = expressionTree;
          // cout << spaces << "  add new " << tree << endl;
        }
      }
      if (result==Result::RESULTFAIL) {
        resultString = "Fail " + resultString;
        // tree["failsym"] = resultString;
      }
      else if (result==Result::RESULTSTOP)
        resultString = "Stop " + resultString;
      else
        resultString = "Continue " + resultString;

      // if (result!=Result::RESULTFAIL)
      //   cout << spaces << "} " << resultString << endl;

      return result;
    } //parseSymbol

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
    cout << "ScopedSymbolTable " << scope_name << " " << scope_level << endl;
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
      cout << "Log scope " << msg << endl;
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
    // cout << "lookup " << name << " " << this->scope_name << " " << _symbolsIndex << " " << child_scopesIndex << endl;
    //  'symbol' is either an instance of the Symbol class or None;
    for (int i=0; i<_symbolsIndex; i++) {
      // cout << "  symbols " << i << " " << _symbols[i]->symbol_type << " " << _symbols[i]->name << " " << _symbols[i]->type << " " << _symbols[i]->scope_level << endl;
      if (_symbols[i]->name == name)
        return _symbols[i];
    }

    if (child_scopes_included) {
      for (int i=0; i<this->child_scopesIndex;i++) {
        // cout << "  detail " << i << " " << this->child_scopes[i]->scope_name << endl;
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

    string spaces(depth*2, ' ');

    cout << spaces << "show " << this->scope_name << " "  << this->scope_level << endl;
    for (int i=0; i<_symbolsIndex; i++) {
      cout << spaces << "-symbols " << i << " " << _symbols[i]->symbol_type << " " << _symbols[i]->name << " " << _symbols[i]->type << endl;
    }

      for (int i=0; i<this->child_scopesIndex;i++) {
        cout << spaces << "-detail " << i << " " << this->child_scopes[i]->scope_name << endl;
        this->child_scopes[i]->show(depth + 1);
      }
  }
}; //ScopedSymbolTable

class SemanticAnalyzer {
  private:
  public:
    ScopedSymbolTable *global_scope = nullptr;

    void visit(JsonVariant tree, string symbol_name = "", string token = "", ScopedSymbolTable* current_scope = nullptr, uint8_t depth = 0) {
      string spaces(depth*2, ' ');

      if (tree.is<JsonObject>()) {
        for (JsonPair element : tree.as<JsonObject>()) {
          string key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!bnfJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = bnfJson["SEMANTICS"][symbol_name];
            if (!expression.isNull())
            {
              cout << spaces << "Symbol " << symbol_name <<  " " << expression << endl;
              // JsonObject expressionObject = expression.as<JsonObject>();
              // JsonObject::iterator it = expressionObject.begin();
              // string keykey = it->key().c_str();

              if (expression == "Program") {
                string program_name = value["variable"]["ID"];
                this->global_scope = new ScopedSymbolTable(program_name, 1, nullptr); //current_scope
                // current_scope->child_scopes[current_scope->child_scopesIndex++] = this->global_scope;
                // current_scope = global_scope;
                visit(value["block"], symbol_name, token, this->global_scope, depth + 1);

                cout << spaces << "End prog " << global_scope->scope_name << " " << global_scope->scope_level << " " << global_scope->_symbolsIndex << endl; 
                for (int i=0; i<global_scope->_symbolsIndex; i++) {
                  Symbol* symbol = global_scope->_symbols[i];
                  cout << spaces << i << " " << symbol->symbol_type << " " << symbol->name << " " << symbol->type << " " << symbol->scope_level << endl; 
                }

                visitCalledAlready = true;
              }
              else if (expression == "Procedure") {

                //find the procedure name (so we must know this is a procedure...)
                string proc_name = value["ID"];
                Symbol* proc_symbol = new Symbol(symbol_name, proc_name);
                current_scope->insert(proc_symbol);

                cout << spaces << "insert proc " << current_scope->scope_name << " " << proc_name << endl;
                ScopedSymbolTable* procedure_scope = new ScopedSymbolTable(proc_name, current_scope->scope_level + 1, current_scope);
                current_scope->child_scopes[current_scope->child_scopesIndex++] = procedure_scope;
                proc_symbol->detail_scope = procedure_scope;
                cout << "ASSIGNING " << proc_symbol->name << " "  << procedure_scope->scope_name << endl;

                // current_scope = procedure_scope;
                visit(value["formal_parameter_list"], symbol_name, token, procedure_scope, depth + 1);

                visit(value["block"], symbol_name, token, procedure_scope, depth + 1);

                cout << spaces << "end proc " << symbol_name << " " << procedure_scope->scope_name << " " << procedure_scope->scope_level << " " << procedure_scope->_symbolsIndex << endl; 
                for (int i=0; i<procedure_scope->_symbolsIndex; i++) {
                  Symbol* symbol = procedure_scope->_symbols[i];
                  cout << spaces << i << " " << symbol->symbol_type << " " << symbol->name << " " << symbol->type << " " << symbol->scope_level << endl; 
                }

                visitCalledAlready = true;
              }
              else if (expression == "VarSymbol") {
                //can be expression or array of expressions
                if (value.is<JsonArray>()) {
                  for (int i=0; i < value.size(); i++) {

                    string param_name = value[i]["ID"];
                    string param_type = value[i]["type_spec"];//current_scope.lookup(param.type_node.value);
                    Symbol* var_symbol = new Symbol(symbol_name, param_name, param_type);
                    current_scope->insert(var_symbol);
                    cout << spaces << "  insert var (from array) " << current_scope->scope_name << " "<< param_name << " " << param_type << endl;
                  }
                }
                else {
                  string param_name = value["ID"];
                  string param_type = value["type_spec"];//current_scope.lookup(param.type_node.value);
                  Symbol* var_symbol = new Symbol(symbol_name, param_name, param_type);
                  current_scope->insert(var_symbol);
                  cout << spaces << "  insert var " << current_scope->scope_name << " "<< param_name << " " << param_type << endl;
                }
              }
              else if (expression == "Assign") {
                JsonVariant left = value["variable"]["ID"];
                JsonVariant right = value["expr"];
                cout << spaces << "Assign " << left << " " << right << endl;
                visit(value["expr"], symbol_name, token, current_scope, depth + 1);
                visitCalledAlready = true;
              }
            } // is expression
          } // is symbol_name

          if (!tokenTypeJson[key].isNull()) {
            token = key;
            // cout << spaces << "Token " << token << endl;
          }
          // cout << spaces << "Object " << key << value << endl;
          if (!visitCalledAlready)
            visit(value, symbol_name, token, current_scope, depth + 1);

        } // key values
      }
      else { //not object
        if (tree.is<JsonArray>()) {
          for (int i=0; i < tree.size(); i++) {
            // cout << spaces << "Array " << tree[i] << "  ";
            visit(tree[i], symbol_name, token, current_scope, depth + 1);
          }
          // cout << endl;
        }
        else { //not array
          string element = tree;
          // cout << spaces << "Value " << element << endl;
          // if (bnfJson["SEMANTICS"][element])
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
        this->members = new DynamicJsonDocument(1000);
        this->mem = this->members->createNestedObject();
    }

    void set(string key, string value) {
      // JsonObject mem = members->as<JsonObject>();
      mem[key] = value;
      // cout <<"Set " << key << " " << value << " " << mem << endl;
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
      cout << "Push " << ar->name << endl;
        this->_records[_recordsCounter++] = ar;
    }

    ActivationRecord* pop() {
      cout << "Pop " << this->peek()->name << endl;
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
    // bool doneSomething = true;
    if (key == "INTEGER_CONST") {
      stack[stack_index++] = value;
    }
    else if (key == "variable") {
      stack[stack_index++] = value;
    }
    else if (key == "PLUS") {
      stack[stack_index-2] = to_string(stoi(stack[stack_index-2]) + stoi(stack[stack_index-1]));
      stack_index--;
    }
    else if (key == "MUL") {
      stack[stack_index-2] = to_string(stoi(stack[stack_index-2]) * stoi(stack[stack_index-1]));
      stack_index--;
    }
    // else
    //   doneSomething = false;

    // if (doneSomething)
    //   cout << "Push " << key << " " << value << " " << stack[stack_index-1] << endl;
  }

  string peek() {
    // cout << "Peek  " << stack[stack_index-1] << endl;
    return stack[stack_index-1];
  }

  string pop() {
    stack_index--;
    // cout << "Pop  " << stack[stack_index] << endl;
    return stack[stack_index];
  }

}; //Calculator

class Interpreter {
  private:
  JsonVariant tree;
  CallStack call_stack;
  ScopedSymbolTable *global_scope;
  Calculator calculator;

  public:

  Interpreter(JsonVariant tree) {
    this->tree = tree;
  }

  void interpret(ScopedSymbolTable *global_scope) {

    this->global_scope = global_scope;

    if (global_scope != nullptr) { //due to undefined procedures??? wip

    cout << "interpret " << global_scope->scope_name << " " << global_scope->scope_level << " " << global_scope->_symbolsIndex << " " << global_scope->child_scopesIndex << endl; 
    for (int i=0; i<global_scope->_symbolsIndex; i++) {
      Symbol* symbol = global_scope->_symbols[i];
      cout << i << " " << symbol->symbol_type << " " << symbol->name << " " << symbol->type << " " << symbol->scope_level << endl; 
    }
    }
    else
      cout << "Interpret global scope is nullptr" << endl;

    visit(tree);
  }

            // ["PLUS2", "factor"],
          // [ "MINUS2", "factor"],

  JsonObject objectFromKeyValue(string key, JsonVariant value) {
    DynamicJsonDocument newx(1000);
    JsonObject newObject = newx.createNestedObject();
    newObject[key] = value[key];

    return newObject;

  }

    void visit(JsonVariant tree, string symbol_name = "", string token = "", uint8_t depth = 0) {
      string spaces(depth*2, ' ');

      if (tree.is<JsonObject>()) {
        for (JsonPair element : tree.as<JsonObject>()) {
          string key = element.key().c_str();
          JsonVariant value = element.value();

          bool visitCalledAlready = false;

          if (!bnfJson[key].isNull()) { //if key is symbol_name
            symbol_name = key;
            JsonVariant expression = bnfJson["INTERPRETER"][symbol_name];
            if (!expression.isNull())
            {
              // cout << spaces << "Symbol " << symbol_name <<  " " << expression << endl;

              if (expression == "Program") {
                string program_name = value["variable"]["ID"];

                ActivationRecord* ar = new ActivationRecord(program_name, "PROGRAM",1);
                cout << spaces << expression << " " << ar->name << " " << program_name << endl;

                this->call_stack.push(ar);

                visit(value["block"], symbol_name, token, depth + 1);

                this->call_stack.pop();

                visitCalledAlready = true;
              }
              else if (expression == "Procedure") {
                string proc_name = value["ID"];
                Symbol* proc_symbol = this->global_scope->lookup(proc_name, true, true);
                cout << spaces << expression << " " << proc_name << endl;
                proc_symbol->block = value["block"];
                visitCalledAlready = true;
              }
              else if (expression == "ProcedureCall") {
                string proc_name = value["ID"];
                Symbol* proc_symbol = this->global_scope->lookup(proc_name, true, true);

                if (proc_symbol != nullptr) { //calling undefined function: pre-defined functions e.g. print

                ActivationRecord* ar = new ActivationRecord(proc_name, "PROCEDURE", proc_symbol->scope_level + 1);

                cout << spaces << expression << " " << ar->name << " " << proc_name << " " << proc_symbol->name << endl;

                for (int i=0; i<proc_symbol->detail_scope->_symbolsIndex;i++) {
                  if (proc_symbol->detail_scope->_symbols[i]->symbol_type == "formal_parameters") { //select formal parameters
                    JsonVariant valueExpr;
                    string result;
                    if (value["expr"].is<JsonArray>()) {
                      DynamicJsonDocument newx(1000);
                      JsonObject newObject = newx.createNestedObject();
                      newObject["expr"] = value["expr"][i];

                      visit(newObject, symbol_name, token, depth + 1);
                      valueExpr = value["expr"][i];
                      result = calculator.pop();
                    }
                    else {
                      visit(value["expr"], symbol_name, token, depth + 1);
                      valueExpr = value["expr"];
                      result = calculator.pop();
                    }
                    ar->set(proc_symbol->detail_scope->_symbols[i]->name, result);
                    cout << spaces << "FP Assignment (pop) " << ar->name << " " << proc_symbol->detail_scope->_symbols[i]->name << " " << result << endl;
                  }
                }

                this->call_stack.push(ar);

                //find block of procedure... lookup procedure?
                //visit block of procedure
                // cout << spaces << "procedure block " << proc_symbol->block << endl;

                visit(proc_symbol->block, symbol_name, token, depth + 1);

                this->call_stack.pop();

                visitCalledAlready = true;
                } //proc_symbol != nullptr
                else
                  cout << spaces << expression << " not found " << proc_name << endl;

              }
              else if (expression == "Assign") {
                visit(value["expr"], symbol_name, token, depth + 1);

                ActivationRecord* ar = this->call_stack.peek();
                ar->set(value["variable"]["ID"], this->calculator.pop());

                cout << spaces << "Assign (pop) " << key << " " << value["variable"]["ID"] << " " << ar->get(value["variable"]["ID"]) << endl;

                visitCalledAlready = true;
              }
              else if (expression == "Variable") {
                ActivationRecord* ar = this->call_stack.peek();
                calculator.push(key, ar->get(value["ID"]));
                cout << spaces << "Calculator (push) " << key << " " << value["ID"] << " " << ar->get(value["ID"]) << " " << calculator.peek() << endl;
                visitCalledAlready = true;
              }
              else if (expression == "ForLoop") {
                cout << spaces << expression << " " << value << endl;

    DynamicJsonDocument newx(1000);
    JsonObject newObject = newx.createNestedObject();
    newObject["assignment_statement"] = value["assignment_statement"];

                visit(newObject, symbol_name, token, depth + 1);
                visit(value["expr"], symbol_name, token, depth + 1);
                ActivationRecord* ar = this->call_stack.peek();
                for (int i=0; i<10;i++) {
                  ar->set("y", to_string(i));
                  visit(value["compound_statement"], symbol_name, token, depth + 1);
                }
                visitCalledAlready = true;
              }
            } //if expression

          } // is key is symbol_name

          // cout << spaces << "Object " << key << value << endl;
          // if (key == "INTEGER_CONST" || key == "PLUS" || key == "MUL" || key == "LPAREN"  || key == "RPAREN" ) {
          if (key == "INTEGER_CONST" || value == "+" || value == "*" || value == "("  || value == ")" ) {
            calculator.push(key, value.as<string>());
            cout << spaces << "Calculator (push) " << key << " " << value << " " << calculator.peek() << endl;
            visitCalledAlready = true;
          }

          if (!tokenTypeJson[key].isNull()) { //if key is token
            token = key;
            // cout << spaces << "Token " << token << endl;
          }
          if (!visitCalledAlready)
            visit(value, symbol_name, token, depth + 1);

        } // key values
      }
      else { //not object
        if (tree.is<JsonArray>()) {
          for (int i=0; i < tree.size(); i++) {
            // cout << spaces << "Array " << tree[i] << "  ";
            visit(tree[i], symbol_name, token, depth + 1);
          }
          // cout << endl;
        }
        else { //not array
          string element = tree;
          // cout << spaces << "not array not object but element " << element << endl;
        }
      }
    }

};


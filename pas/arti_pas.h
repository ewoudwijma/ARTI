/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti_pas_plugin.h
   @version 0.2.3
   @date    20220103
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
 */

#pragma once

// For testing porposes, definitions should not only run on Arduino but also on Windows etc. 
// Because compiling on arduino takes seriously more time than on Windows.
// The plugin.h files replace native arduino calls by windows simulated calls (e.g. setPixelColor will become printf)
#define ARTI_ARDUINO 1
#define ARTI_EMBEDDED 2
#define ARTI_PLATFORM ARTI_EMBEDDED

#include "../arti.h"

//make sure the numbers here correspond to the order in which these functions are defined in wled.json!!
enum Externals
{
  F_printf
};

float ARTI::arti_external_function(uint8_t function, float par1, float par2, float par3, float par4, float par5) 
{
  switch (function)
  {
    case F_printf: {
      if (par3 == floatNull) {
        if (par2 == floatNull) {
          PRINT_ARTI("%s(%f)\n", "printf", par1);
        }
        else
          PRINT_ARTI("%s(%f, %f)\n", "printf", par1, par2);
      }
      else
        PRINT_ARTI("%s(%f, %f, %f)\n", "printf", par1, par2, par3);
      return floatNull;
    }
  }

  ERROR_ARTI("Error: arti_external_function: %u not implemented\n", function);
  return function;
}

float ARTI::arti_get_external_variable(uint8_t variable, float par1, float par2, float par3) 
{
  return floatNull;
}

void ARTI::arti_set_external_variable(float value, uint8_t variable, float par1, float par2, float par3) {
}

bool ARTI::loop() {
  //pas example has no loop function

  uint8_t depth = 8;

  const char * function_name = "loop";
  Symbol* function_symbol = global_scope->lookup(function_name);

  if (function_symbol != nullptr) //calling undefined function: pre-defined functions e.g. print
  {
    ActivationRecord* ar = new ActivationRecord(function_name, "Function", function_symbol->scope_level + 1);

    RUNLOG_ARTI("%s %s %s (%u)\n", spaces+50-depth, "Call", function_name, this->callStack->recordsCounter);

    this->callStack->push(ar);

    interpret(function_symbol->block, nullptr, global_scope, depth + 1);

    this->callStack->pop();

    delete ar; ar = nullptr;
  }
  else 
  {
    ERROR_ARTI("%s loop not found\n", spaces+50-depth);
    return false;
  }

  return true;
} // loop

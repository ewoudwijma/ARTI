/*
   @title   Arduino Real Time Interpreter (ARTI)
   @file    arti_test.cpp
   @version 0.1.1
   @date    20211129
   @author  Ewoud Wijma
   @repo    https://github.com/ewoudwijma/ARTI
 */

#include "arti_pas.h"

void execute(const char *definitionName, const char *programName) 
{
  ARTI *arti = new ARTI();

  printf("open %s and %s\n", definitionName, programName);

  if (arti->setup(definitionName, programName)) 
  {
    if (strstr(definitionName, "wled")) 
    {
      uint8_t nrOfTimes = 2;
  
      for (uint8_t i=0; i<nrOfTimes; i++)
        arti->loop();
    }
  }
  else
    printf("setup fail\n");

  arti->close();
  printf("done\n");
}

int main() {
  
  execute("pas.json", "Examples/pas1.pas");

}

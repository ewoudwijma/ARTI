#include "FX.h"

#include "src/dependencies/ARTI/arti.h"
ARTI * arti = nullptr;


uint16_t WS2812FX::mode_customEffect(void) {
  // ARTI* arti = nullptr;

  static bool succesful;
  static int previousMillis;

  if (SEGENV.call == 0) {
    // arti = reinterpret_cast<ARTI*>(SEGENV.data);

    Serial.println();
    if (arti != nullptr) {
      MEMORY_ARTI("Heap delete old Arti %u\n", esp_get_free_heap_size());
      arti->close();
      delete arti; arti = nullptr;
    }

    MEMORY_ARTI("Heap new Arti < %u\n", esp_get_free_heap_size());

    // if (!SEGENV.allocateData(sizeof(ARTI))) return mode_static();  // We use this method for allocating memory for static variables.
    // arti = reinterpret_cast<ARTI*>(SEGENV.data);

    arti = new ARTI();

    char programFileName[charLength] = "/";
    if (SEGMENT.name != nullptr)
      strcat(programFileName, SEGMENT.name);
    else
      strcat(programFileName, "default");
    strcat(programFileName, ".wled");

    if (arti->openFileAndParse("/wled.json", programFileName)) {
      if (arti->analyze()) {
        if (arti->interpret())
          succesful = true;
        else
          succesful = false;
      }
      else
        succesful = false;
    }
    else 
      succesful = false;

    if (!succesful) {
      ERROR_ARTI("not succesful\n");
      return mode_blink();
    }
  }
  else {
    // arti = reinterpret_cast<ARTI*>(SEGENV.data);

    if (succesful) {// && SEGENV.call < 250 for each frame
      if (esp_get_free_heap_size() <= 30000) {
        ERROR_ARTI("Not enough free heap (%u <= 30000)\n", esp_get_free_heap_size());
        succesful = false;
      }
      else {
        // if (millis() - previousMillis > 5000) { //tried SEGENV.aux0 but that looks to be overwritten!!! (dangling pointer???)
        //   previousMillis = millis();
        //   MEMORY_ARTI("Heap renderFrame %u\n", esp_get_free_heap_size());
        // }
        arti->interpret("renderFrame");
      }
    }
    else 
    {
      return mode_blink();

      if (arti != nullptr) { // if not enough free mem to continue
        MEMORY_ARTI("Heap delete Arti < %u\n", esp_get_free_heap_size());
        arti->close();
        delete arti; arti = nullptr;
        MEMORY_ARTI("Heap delete Arti > %u\n", esp_get_free_heap_size());
      }
    }
  }

  return 0; //as fast as possible

}

// platformio.ini:

[env:soundReactive_esp32dev]
board = esp32dev
platform = espressif32@3.3.2
; build_type = debug ; debug can cause crashes otherwise not occurring!!!
upload_speed = 921600
build_unflags = ${common.build_unflags}
build_flags = ${common.build_flags_esp32} -D WLED_RELEASE_NAME=ESP32 -D WLED_DISABLE_MQTT
  ; -D USERMOD_FOUR_LINE_DISPLAY
  ; -D USERMOD_MODE_SORT
  ; -D USERMOD_ROTARY_ENCODER_UI
  ; -D USERMOD_DALLASTEMPERATURE
lib_deps = ${esp32.lib_deps}
  ; https://github.com/olikraus/U8g2_Arduino
; board_build.partitions = tools/WLED_ESP32_16MB.csv
board_build.partitions = tools/WLED_ESP32_4MB_1MB_FS.csv
monitor_filters = esp32_exception_decoder


///FX.cpp:

#include "FX.h"

#include "arti.h"
ARTI * arti = nullptr;


uint16_t WS2812FX::mode_2DCenterBars(void) {              // Written by Scott Marley Adapted by  Spiro-C..

  // ARTI* arti = nullptr;

  if (SEGENV.call == 0) {

    // arti = reinterpret_cast<ARTI*>(SEGENV.data);

    Serial.println();
    if (arti != nullptr) {
      Serial.printf("Heap delete old Arti %u\n", esp_get_free_heap_size());
      arti->close();
      delete arti; arti = nullptr;
    }

    printf("Heap new Arti < %u\n", esp_get_free_heap_size());

    // if (!SEGENV.allocateData(sizeof(ARTI))) return mode_static();  // We use this method for allocating memory for static variables.
    // arti = reinterpret_cast<ARTI*>(SEGENV.data);

    arti = new ARTI();

    char programFileName[charLength] = "/";
    if (SEGMENT.name != nullptr)
      strcat(programFileName, SEGMENT.name);
    else
      strcat(programFileName, "wled1"); //default
    strcat(programFileName, ".wled");

    arti->openFileAndParse("/wled.json", programFileName);  

    arti->analyze();
    arti->interpret();
  }
  else {
    // arti = reinterpret_cast<ARTI*>(SEGENV.data);

    if (esp_get_free_heap_size() > 30000) {//SEGENV.call < 250 && for each frame
      arti->interpret("renderFrame");
    }
    else if (arti != nullptr) { // if not enough free mem to continue
      Serial.printf("Heap delete Arti < %u\n", esp_get_free_heap_size());
      arti->close();
      delete arti; arti = nullptr;
      Serial.printf("Heap delete Arti > %u\n", esp_get_free_heap_size());
    }
    else { // do nothing
    }
  }

  return FRAMETIME;

} // mode_2DCenterBars()


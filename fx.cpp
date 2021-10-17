// platformio.ini:

[env:soundReactive_esp32dev]
board = esp32dev
platform = espressif32@3.3.2
upload_speed = 921600
build_unflags = ${common.build_unflags}
build_flags = ${common.build_flags_esp32} -D WLED_RELEASE_NAME=ESP32 -D WLED_DISABLE_MQTT
  ; -D USERMOD_FOUR_LINE_DISPLAY
  ; -D USERMOD_MODE_SORT
  ; -D USERMOD_ROTARY_ENCODER_UI
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
  // static unsigned long resetMillis; //triggers reset if more than 3 seconds from millis()
  // if (millis() - resetMillis > 3000) {
  //   resetMillis = millis();

  if (SEGENV.call == 0 || SEGMENT.intensity != SEGENV.aux0) {
    SEGENV.aux0 = SEGMENT.intensity; //intensity determines which programFile to load

    Serial.println();
    if (arti != nullptr) {
      Serial.printf("Heap destruct old Arti %u\n", esp_get_free_heap_size());
      arti->close();
      delete arti; arti = nullptr;
    }

    if (esp_get_free_heap_size() > 30000) {

      Serial.printf("Heap new Arti < %u\n", esp_get_free_heap_size());
      arti = new ARTI();
      Serial.printf("Heap new Arti > %u\n", esp_get_free_heap_size());

      uint8_t nrOfEffects = 2;
      uint8_t effectNr = SEGMENT.intensity % ((uint8_t)((double)SEGMENT.intensity / (double)nrOfEffects));
      Serial.printf("effect %u", effectNr);
      if (effectNr == 0)
        arti->openFileAndParse("/wled.json", "/wled2.wled");  
      else
        arti->openFileAndParse("/wled.json", "/wled1.wled");  

      Serial.printf("Heap parse > %u\n", esp_get_free_heap_size());

      arti->analyze();
      arti->interpret();
    }
  }
  else if (esp_get_free_heap_size() > 30000) {//SEGENV.call < 250 && for each frame
        arti->interpret("renderFrame");
  }
  else if (arti != nullptr) { // if not enough free mem to continue
        Serial.printf("Heap destruct Arti < %u\n", esp_get_free_heap_size());
        arti->close();
        delete arti; arti = nullptr;
        Serial.printf("Heap destruct Arti > %u\n", esp_get_free_heap_size());
  }
  else { // do nothing
  }

  return FRAMETIME;
} // mode_2DCenterBars()


// this should work... (See AuroraWave) ... but it does crash
  ARTI* arti;

  if (SEGENV.call == 0) { //effect starts
    Serial.printf("sizeof arti %u\n", sizeof(ARTI));

    if (!SEGENV.allocateData(sizeof(ARTI))) return mode_static();  // We use this method for allocating memory for static variables.
      arti = reinterpret_cast<ARTI*>(SEGENV.data);          // Because 'static' doesn't work with SEGMENTS.

    arti[0].openFileAndParse("/wled.json", "/wled1.wled");
    arti[0].interpret();
    arti[0].close();
  }
  else {
      arti = reinterpret_cast<ARTI*>(SEGENV.data);          // Because 'static' doesn't work with SEGMENTS.
  }

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

  if (arti == nullptr)
     arti = new ARTI();

  if (SEGENV.call == 0) { //effect starts

      arti->openFileAndParse("/wled.json", "/wled1.wled");
      arti->interpret();
      arti->close();
  }
  else {

  }

  return FRAMETIME;
} // mode_2DCenterBars()

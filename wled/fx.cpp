#include "FX.h"

#include "src/dependencies/arti/arti.h"
ARTI * arti;



//Adding ARTI to this structure seems to be needed to make the pointers used in ARTI survive in subsequent calls of mode_customEffect
//  otherwise: Interpret renderFrame: No parsetree created
//  initially added parseTreeJsonDoc in this struct to save it explicitly but that was not needed
// maybe because this struct is not deleted
// typedef struct ArtiWrapper {
//   ARTI * arti;
// } artiWrapper;

uint16_t WS2812FX::mode_customEffect(void) {

  // //brightpulse
  // uint8_t lum = constrain(sampleAvg * 256.0 / (256.0 - SEGMENT.speed), 0, 255);
  // fill(color_blend(SEGCOLOR(1), SEGCOLOR(0), lum));

  // return FRAMETIME;

  // float t = (sin((float)millis() / 1000.) + 1.0) / 2.;            // Make a slow sine wave and convert output range from -1.0 and 1.0 to between 0 and 1.0.
  // t = t * (float)SEGLEN;                                        // Now map to the length of the strand.
 
  // for (int i = 0; i < SEGLEN; i++)
  // {
  //   float diff = abs(t - (float)i);                               // Get difference between t and current location. Greater distance = lower brightness.
  //   if (diff > 2.0) diff = 2.0;                                   // Let's not overflow.
  //   float bri = 256 - diff * 128;                                 // Scale the brightness to up to 255. Closer = brighter.
  //   leds[i] = CHSV(0, 255, (uint8_t)bri);
  // }
 
  // setPixels(leds);

  // return FRAMETIME;

  
  // //Random
  // for (int i = 0; i < ledCount; i = i + 1) {
  //   uint16_t color = random16();
  //   setPixelColor(i%ledCount, color_wheel(color%256));
  // }

  // return 0;

  // //Kitt

  // static int pixelCounter;
  // static int goingUp;

  // if (pixelCounter > (ledCount-5)) {
  //   goingUp = 0;
  // }
  // if (pixelCounter == 0) {
  //   goingUp = 1;
  // }
  // setPixelColor(pixelCounter%ledCount, color_wheel(pixelCounter%256));
  // if (goingUp) {
  //   setPixelColor((pixelCounter-5)%ledCount, CRGB::Black);
  //   pixelCounter = pixelCounter + 1;
  // }
  // else {
  //   setPixelColor((pixelCounter+5)%ledCount, CRGB::Black);
  //   pixelCounter = pixelCounter - 1;
  // }

  // return 0;

  // ArtiWrapper* artiWrapper = reinterpret_cast<ArtiWrapper*>(SEGENV.data);
  
  //tbd: move statics to SEGMENT.data
  static bool succesful;
  static bool notEnoughHeap;

  static char previousEffect[charLength];
  if (SEGENV.call == 0)
    strcpy(previousEffect, ""); //force init

  char currentEffect[charLength];
  strcpy(currentEffect, (SEGMENT.name != nullptr)?SEGMENT.name:"default"); //note: switching preset with segment name to preset without does not clear the SEGMENT.name variable, but not gonna solve here ;-)

  if (strcmp(previousEffect, currentEffect) != 0) {
    strcpy(previousEffect, currentEffect);

    // if (artiWrapper != nullptr && artiWrapper->arti != nullptr) {
    if (arti != nullptr) {
      arti->close();
      delete arti; arti = nullptr;
    }

    // if (!SEGENV.allocateData(sizeof(ArtiWrapper))) return mode_static();  // We use this method for allocating memory for static variables.
    // artiWrapper = reinterpret_cast<ArtiWrapper*>(SEGENV.data);
    arti = new ARTI();

    char programFileName[charLength];
    strcpy(programFileName, "/");
    strcat(programFileName, currentEffect);
    strcat(programFileName, ".wled");

    succesful = arti->setup("/wled.json", programFileName);

    if (!succesful) {
      ERROR_ARTI("Setup not succesful\n");
    }
  }
  else {

    if (succesful) {// && SEGENV.call < 250 for each frame
      if (esp_get_free_heap_size() <= 30000) {
        ERROR_ARTI("Not enough free heap (%u <= 30000)\n", esp_get_free_heap_size());
        notEnoughHeap = true;
        succesful = false;
      }
      else {
        // static int previousMillis;
        // if (millis() - previousMillis > 5000) { //tried SEGENV.aux0 but that looks to be overwritten!!! (dangling pointer???)
        //   previousMillis = millis();
        //   MEMORY_ARTI("Heap renderFrame %u\n", esp_get_free_heap_size());
        // }
        arti->loop();
      }
    }
    else 
    {
      if (notEnoughHeap && esp_get_free_heap_size() > 30000) {
        ERROR_ARTI("Again enough free heap, restart effect (%u > 30000)\n", esp_get_free_heap_size());
        succesful = true;
        notEnoughHeap = false;
        strcpy(previousEffect, ""); // force new create
      }
      else {
        return mode_blink();
      }
    }
  }

  return FRAMETIME;
}
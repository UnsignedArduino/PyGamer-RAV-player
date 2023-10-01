#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include "rav_player.h"
#include <Adafruit_Arcada.h>

void halt() {
  Serial.println("Halting");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  while (true) {
    ;
  }
}

void setup() {
  if (!RAVinit()) {
    halt();
  }
}

void loop() {
  char pathBuf[MAX_PATH_LEN + 1] = {};
  if (!RAVPickFile(pathBuf, MAX_PATH_LEN + 1)) {
    return;
  }
  RAVPlayFile(pathBuf);
}

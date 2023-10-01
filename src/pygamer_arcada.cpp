#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>

byte battPercent(Adafruit_Arcada arcada) {
  float voltage = arcada.readBatterySensor();
  const byte voltCounts = 21;
  // LiPo voltage is not linear
  // https://www.desmos.com/calculator/hrzzbwffct
  const byte lipoPercents[voltCounts] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50,
                                         45,  40, 35, 30, 25, 20, 15, 10, 5,  0};
  const float lipoVolts[voltCounts] = {4.2,  4.15, 4.11, 4.08, 4.02, 3.98, 3.95, 3.91, 3.87, 3.85, 3.84,
                                       3.82, 3.8,  3.79, 3.77, 3.75, 3.73, 3.71, 3.69, 3.61, 3.27};
  for (byte i = 0; i < voltCounts; i++) {
    if (voltage > lipoVolts[i]) {
      return lipoPercents[i];
    }
  }
  return lipoPercents[voltCounts - 1];
}

void waitForPress(Adafruit_Arcada arcada) {
  while (arcada.readButtons() == 0) {
    ;
  }
}

void waitForRelease(Adafruit_Arcada arcada) {
  while (arcada.readButtons() != 0) {
    ;
  }
}

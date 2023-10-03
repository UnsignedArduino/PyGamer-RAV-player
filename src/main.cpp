#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include <RAVCodec.h>

Adafruit_Arcada arcada;

RAVCodec::RAVCodec codec(&arcada);

const uint8_t MAX_PATH_SIZE = 255;

void halt() {
  Serial.println("Halting");
  digitalWrite(LED_BUILTIN, HIGH);
  while (true) {
    ;
  }
}

uint8_t battPercent() {
  float voltage = arcada.readBatterySensor();
  const uint8_t voltCounts = 21;
  // LiPo voltage is not linear
  // https://www.desmos.com/calculator/hrzzbwffct
  const uint8_t lipoPercents[voltCounts] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50,
                                            45,  40, 35, 30, 25, 20, 15, 10, 5,  0};
  const float lipoVolts[voltCounts] = {4.2,  4.15, 4.11, 4.08, 4.02, 3.98, 3.95, 3.91, 3.87, 3.85, 3.84,
                                       3.82, 3.8,  3.79, 3.77, 3.75, 3.73, 3.71, 3.69, 3.61, 3.27};
  for (uint8_t i = 0; i < voltCounts; i++) {
    if (voltage > lipoVolts[i]) {
      return lipoPercents[i];
    }
  }
  return lipoPercents[voltCounts - 1];
}

void waitForPress() {
  while (arcada.readButtons() == 0) {
    ;
  }
}

void waitForRelease() {
  while (arcada.readButtons() != 0) {
    ;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("RAV player");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!arcada.arcadaBegin()) {
    Serial.println("Failed to start Arcada");
    halt();
  }

  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.display->fillScreen(ARCADA_BLACK);

  if (!arcada.filesysBegin(ARCADA_FILESYS_SD)) {
    Serial.println("Could not find SD filesystem");
    arcada.display->fillScreen(ARCADA_RED);
    arcada.haltBox("Could not find SD card! "
                   "Make sure there is a card plugged in that is formatted with FAT.");
  } else {
    Serial.println("Found SD filesystem");
  }

  analogWriteResolution(8);
}

void loop() {
  static char path[MAX_PATH_SIZE];
  if (!arcada.chooseFile("/", path, MAX_PATH_SIZE, "rav")) {
    Serial.println("Failed to choose file!");
    return;
  }
  Serial.printf("Choose %s\n", path);
  waitForRelease();
  if (codec.open(path)) {
    Serial.println("Decoder successfully opened!");
    codec.readCurrentFrame();
  } else {
    Serial.println("Decoder failed to open!");
  }
  codec.close();
}

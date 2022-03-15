#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include "rav_codec.h"
#include "rav_player.h"

Adafruit_Arcada arcada;

bool volatile playSamples = false;
unsigned long volatile sampleIdx = 0;

byte volume = 16;
const byte MAX_VOLUME = 128;

bool RAVinit() {
  Serial.begin(9600);
  Serial.println("RAV player init");
  if (!arcada.arcadaBegin()) {
    Serial.println("Failed to start Arcada");
    return false;
  }

  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.display->fillScreen(ARCADA_BLACK);

  if (!arcada.filesysBegin(ARCADA_FILESYS_SD)) {
    Serial.println("Could not find SD filesystem");
    arcada.display->fillScreen(ARCADA_RED);
    arcada.haltBox("Could not find SD card! " \
                   "Make sure there is a card plugged in that is formatted with FAT.");
  } else {
    Serial.println("Found SD filesystem");
  }

  analogWriteResolution(8);

  Serial.println("Init success");
  return true;
}

bool RAVPickFile(char* result, path_size_t resultSize) {
  Serial.println("Choosing file");
  if (!arcada.chooseFile("/", result, resultSize, "rav")) {
    Serial.println("Failed to choose file");
    return false;
  }
  waitForRelease();
  Serial.print("Choose file: ");
  Serial.println(result);
  return true;
}

void RAVPlayFile(char* path) {
  Serial.print("Playing file: ");
  Serial.println(path);

  if (!RAVCodecEnter(path)) {
    Serial.println("Failed to start codec");
    return;
  }

  playSamples = true;
  sampleIdx = 0;

  arcada.timerCallback(SAMPLE_RATE, playNextSample);
  arcada.enableSpeaker(true);

  while (true) {
    unsigned long start_time = millis();
    if (!RAVCodecDecodeFrame()) {
      Serial.println("Stopping decoder, EOF");
      break;
    }
    sampleIdx = 0;
    while (millis() - start_time < FRAME_LENGTH) {
      ;
    }
  }

  arcada.timerStop();
  arcada.enableSpeaker(false);
  RAVCodecExit();
}

void playNextSample() {
  if (!playSamples || sampleIdx == SAMPLES_PER_FRAME) {
    return;
  }
  byte sample = map(frameSamples[sampleIdx], 0, 255, 0, volume);
  analogWrite(A0, sample);
  analogWrite(A1, sample);
  sampleIdx ++;
}

void waitForRelease() {
  while (arcada.readButtons() != 0) {
    ;
  }
}
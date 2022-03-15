#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include <JPEGDEC.h>
#include "rav_codec.h"
#include "rav_player.h"

Adafruit_Arcada arcada;
JPEGDEC jpeg;

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

  arcada.display->setTextColor(ARCADA_WHITE, ARCADA_BLACK);
  arcada.display->setTextSize(1);
  arcada.display->setTextWrap(true);

  arcada.timerCallback(SAMPLE_RATE, playNextSample);
  arcada.enableSpeaker(true);

  while (true) {
    unsigned long start_time = millis();
    if (!RAVCodecDecodeFrame()) {
      Serial.println("Stopping decoder, EOF");
      break;
    }
    sampleIdx = 0;
    arcada.display->startWrite();
    if (jpeg.openRAM(RAVCodecJPEGImage, RAVCodecJPEGsize, JPEGDraw)) {
      if (!jpeg.decode((ARCADA_TFT_WIDTH - jpeg.getWidth()) / 2, 
                       (ARCADA_TFT_HEIGHT - jpeg.getHeight()) / 2,
                       0)) {
        Serial.println("Failed to decode JPEG");
      }
      jpeg.close();
    }
    arcada.display->endWrite();
    arcada.display->setCursor(0, ARCADA_TFT_HEIGHT - 8);
    const byte timeBufSize = 12;
    char timeBuf[timeBufSize] = {};
    formatFrameAsTime(RAVCodecCurrFrame, timeBuf, timeBufSize);
    arcada.display->print(timeBuf);
    arcada.display->print("/");
    formatFrameAsTime(RAVCodecMaxFrame, timeBuf, timeBufSize);
    arcada.display->print(timeBuf);
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
    analogWrite(A0, 0);
    analogWrite(A1, 0);
    return;
  }
  byte sample = map(RAVCodecFrameSamples[sampleIdx], 0, 255, 0, volume);
  analogWrite(A0, sample);
  analogWrite(A1, sample);
  sampleIdx ++;
}

int JPEGDraw(JPEGDRAW *draw) {
  arcada.display->dmaWait();
  arcada.display->setAddrWindow(draw->x, draw->y, draw->iWidth, draw->iHeight);
  arcada.display->writePixels(draw->pPixels, draw->iWidth * draw->iHeight, true, false);
  return 1;
}

void formatFrameAsTime(unsigned long f, char* result, byte resultSize) {
  unsigned long secs = f / VIDEO_FPS;
  unsigned int h = secs / 3600;
  secs = secs % 3600;
  unsigned int m = secs / 60;
  unsigned int s = secs % 60;
  if (h == 0) {
    snprintf(result, resultSize, "%02d:%02d", m, s);
  } else {
    snprintf(result, resultSize, "%02ld:%02d:%02d", h, m, s);
  }
}

void waitForRelease() {
  while (arcada.readButtons() != 0) {
    ;
  }
}

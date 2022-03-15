#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include "rav_player.h"
#include "rav_codec.h"

char* RAVCodecPath;
File RAVCodecFile;
unsigned long RAVCodecCurrFrame;
sample_t RAVCodecFrameSamples[SAMPLES_PER_FRAME];

bool RAVCodecEnter(char* path) {
  RAVCodecPath = path;
  RAVCodecCurrFrame = 0;
  memset(RAVCodecFrameSamples, 0, SAMPLES_PER_FRAME * sizeof(sample_t));
  Serial.print("Opening file ");
  Serial.println(RAVCodecFile);
  RAVCodecFile = arcada.open(RAVCodecPath);
  if (!RAVCodecFile) {
    Serial.println("Failed to open file");
    return false;
  }
  Serial.println("Success init codec");
  return true;
}

bool RAVCodecDecodeFrame() {
  if (_RAVCodecAtEOF()) {
    return false;
  }
  // Current frame number
  RAVCodecCurrFrame = _RAVCodecReadULong();
  #if defined(DEBUG_FRAME)
  Serial.print("Frame #");
  Serial.println(RAVCodecCurrFrame);
  #endif
  // Frame length
  #if defined(DEBUG_FRAME)
  Serial.print("Frame len: ");
  Serial.println(_RAVCodecReadULong());
  #else
  _RAVCodecReadULong();
  #endif
  // Audio length
  #if defined(DEBUG_FRAME)
  Serial.print("Audio len: ");
  Serial.println(_RAVCodecReadULong());
  #else
  _RAVCodecReadULong();
  #endif
  // Audio
  _RAVCodecReadAudio();
  // Video frame length
  unsigned long jpegLen = _RAVCodecReadULong();
  #if defined(DEBUG_FRAME)
  Serial.print("JPEG len: ");
  Serial.println(jpegLen);
  #endif
  // Video
  _RAVCodecReadVideo(jpegLen);
  // Frame length
  #if defined(DEBUG_FRAME)
  Serial.print("Frame len ");
  Serial.println(_RAVCodecReadULong());
  #else
  _RAVCodecReadULong();
  #endif
  return true;
}

void RAVCodecExit() {
  RAVCodecFile.close();
}

void _RAVCodecReadAudio() {
  RAVCodecFile.read(RAVCodecFrameSamples, SAMPLES_PER_FRAME);
}

void _RAVCodecReadVideo(unsigned long JPEGlen) {
  // For now we don't decode audio
  RAVCodecFile.seekCur(JPEGlen);
}

unsigned long _RAVCodecReadULong() {
  ULongAsBytes ulb;
  RAVCodecFile.read(ulb.as_array, sizeof(unsigned long));
  return ulb.as_ulong;
}

bool _RAVCodecAtEOF() {
  return RAVCodecFile.position() >= RAVCodecFile.size() - 1;
}

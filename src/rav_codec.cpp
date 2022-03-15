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

char* _path;
File _file;
unsigned long _currFrame;
sample_t frameSamples[SAMPLES_PER_FRAME];

bool RAVCodecEnter(char* path) {
  _path = path;
  _currFrame = 0;
  memset(frameSamples, 0, SAMPLES_PER_FRAME * sizeof(sample_t));
  Serial.print("Opening file ");
  Serial.println(_file);
  _file = arcada.open(_path);
  if (!_file) {
    Serial.println("Failed to open file");
    return false;
  }
  Serial.println("Success init codec");
  return true;
}

bool RAVCodecDecodeFrame() {
  if (atEOF()) {
    return false;
  }
  // Current frame number
  _currFrame = _readULong();
  #if defined(DEBUG_FRAME)
  Serial.print("Frame #");
  Serial.println(_currFrame);
  #endif
  // Frame length
  #if defined(DEBUG_FRAME)
  Serial.print("Frame len: ");
  Serial.println(_readULong());
  #else
  _readULong();
  #endif
  // Audio length
  #if defined(DEBUG_FRAME)
  Serial.print("Audio len: ");
  Serial.println(_readULong());
  #else
  _readULong();
  #endif
  // Audio
  _readAudio();
  // Video frame length
  unsigned long jpegLen = _readULong();
  #if defined(DEBUG_FRAME)
  Serial.print("JPEG len: ");
  Serial.println(jpegLen);
  #endif
  // Video
  _readVideo(jpegLen);
  // Frame length
  #if defined(DEBUG_FRAME)
  Serial.print("Frame len ");
  Serial.println(_readULong());
  #else
  _readULong();
  #endif
  return true;
}

void RAVCodecExit() {
  _file.close();
}

void _readAudio() {
  _file.read(frameSamples, SAMPLES_PER_FRAME);
}

void _readVideo(unsigned long JPEGlen) {
  // For now we don't decode audio
  _file.seekCur(JPEGlen);
}

unsigned long _readULong() {
  ULongAsBytes ulb;
  _file.read(ulb.as_array, sizeof(unsigned long));
  return ulb.as_ulong;
}

bool atEOF() {
  return _file.position() >= _file.size() - 1;
}

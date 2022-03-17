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
unsigned long RAVCodecMaxFrame;
sample_t RAVCodecFrameSamples[SAMPLES_PER_FRAME];
byte RAVCodecJPEGImage[MAX_JPEG_SIZE];
unsigned long RAVCodecJPEGsize;

bool RAVCodecEnter(char* path) {
  RAVCodecPath = path;
  RAVCodecCurrFrame = 0;
  RAVCodecMaxFrame = 0;
  memset(RAVCodecFrameSamples, 0, SAMPLES_PER_FRAME * sizeof(sample_t));
  RAVCodecJPEGsize = 0;
  memset(RAVCodecJPEGImage, 0, MAX_JPEG_SIZE * sizeof(byte));
  Serial.print("Opening file ");
  Serial.println(RAVCodecFile);
  RAVCodecFile = arcada.open(RAVCodecPath);
  Serial.println();
  if (!RAVCodecFile) {
    Serial.println("Failed to open file");
    return false;
  }
  RAVCodecFile.seekEnd();
  RAVCodecFile.seekCur(-4);
  unsigned long frameLen = _RAVCodecReadULong();
  Serial.print("Last frame length: ");
  Serial.println(frameLen);
  // Seek negative (backwards)
  // -4 for the last frame length, frame length itself, 
  // and -8 to get to the beginning of the frame
  // Then we can read the frame number and get the last frame
  RAVCodecFile.seekCur((long)(4 + frameLen + 8 + 8) * (long)-1);
  RAVCodecMaxFrame = _RAVCodecReadULong();
  RAVCodecFile.seek(0);
  Serial.print("Last frame: ");
  Serial.println(RAVCodecMaxFrame);
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
  RAVCodecJPEGsize = _RAVCodecReadULong();
  #if defined(DEBUG_FRAME)
  Serial.print("JPEG len: ");
  Serial.println(RAVCodecJPEGsize);
  #endif
  // Video
  _RAVCodecReadVideo(RAVCodecJPEGsize);
  // Frame length
  #if defined(DEBUG_FRAME)
  Serial.print("Frame len ");
  Serial.println(_RAVCodecReadULong());
  #else
  _RAVCodecReadULong();
  #endif
  return true;
}

void RAVCodecSeekFramesCur(long changeBy) {
  Serial.print("Seeking ");
  Serial.print(changeBy);
  Serial.println(" frames");
  unsigned long targetFrame = constrain((long)RAVCodecCurrFrame + changeBy, 0, RAVCodecMaxFrame - 1);
  Serial.print("Seeking to ");
  Serial.print(targetFrame);
  Serial.print("/");
  Serial.println(RAVCodecMaxFrame);
  if (targetFrame == 0) {
    RAVCodecSeekToFirstFrame();
    return;
  } else if (targetFrame >= RAVCodecMaxFrame - VIDEO_FPS) {
    RAVCodecSeekToLastFrame();
    return;
  } else {
    if (RAVCodecCurrFrame < targetFrame) {
      while (RAVCodecCurrFrame < targetFrame) {
        // Current frame number
        RAVCodecCurrFrame = _RAVCodecReadULong();
        // Frame length
        unsigned long frameLen = _RAVCodecReadULong();
        // Skip:
        // Audio length
        // Audio
        // Video frame length
        // Video
        RAVCodecFile.seekCur(frameLen + 8);
        // Frame length
        _RAVCodecReadULong();
        Serial.print("Frame #");
        Serial.println(RAVCodecCurrFrame);
      }
    } else {
      while (RAVCodecCurrFrame > targetFrame) {
        // Go back 4 bytes to read the current frame's length
        RAVCodecFile.seekCur(-4);
        unsigned long frameLen = _RAVCodecReadULong();
        // Seek negative (backwards)
        // -4 for the last frame length, frame length itself, 
        // and -8 to get to the beginning of the frame
        // Then we can read the frame number and get the last frame
        RAVCodecFile.seekCur((long)(4 + frameLen + 8 + 8) * (long)-1);
        // Read current frame
        RAVCodecCurrFrame = _RAVCodecReadULong();
        // Seek backwards 4 bytes to offset reading the current frame
        RAVCodecFile.seekCur(-4);
        Serial.print("Frame #");
        Serial.println(RAVCodecCurrFrame);
      }
    }
  }
}

void RAVCodecSeekToFirstFrame() {
  RAVCodecFile.seek(0);
}

void RAVCodecSeekToLastFrame() {
  RAVCodecFile.seekEnd();
  RAVCodecFile.seekCur(-4);
  unsigned long frameLen = _RAVCodecReadULong();
  // Seek negative (backwards)
  // -4 for the last frame length, frame length itself, 
  // and -8 to get to the beginning of the frame
  // Then we can read the frame number and get the last frame
  RAVCodecFile.seekCur((long)(4 + frameLen + 8 + 8) * (long)-1);
}

void RAVCodecExit() {
  RAVCodecFile.close();
}

void _RAVCodecReadAudio() {
  RAVCodecFile.read(RAVCodecFrameSamples, SAMPLES_PER_FRAME);
}

void _RAVCodecReadVideo(unsigned long JPEGlen) {
  memset(RAVCodecJPEGImage, 0, MAX_JPEG_SIZE * sizeof(byte));
  if (JPEGlen > MAX_JPEG_SIZE) {
    Serial.print("JPEG size ");
    Serial.print(JPEGlen);
    Serial.println(" too big!");
    return;
  }
  // // For now we don't decode audio
  // RAVCodecFile.seekCur(JPEGlen);
  RAVCodecFile.read(RAVCodecJPEGImage, JPEGlen);
}

unsigned long _RAVCodecReadULong() {
  ULongAsBytes ulb;
  RAVCodecFile.read(ulb.as_array, sizeof(unsigned long));
  return ulb.as_ulong;
}

bool _RAVCodecAtEOF() {
  return RAVCodecFile.position() >= RAVCodecFile.size() - 1;
}

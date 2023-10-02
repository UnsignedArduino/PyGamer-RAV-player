#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include "rav_codec.h"
#include "rav_player.h"
#include <Adafruit_Arcada.h>

char* RAVCodecPath;
File RAVCodecFile;
uint32_t RAVCodecCurrFrame;
sample_t RAVCodecFrameSamples[SAMPLES_PER_FRAME];
uint16_t* RAVCodecImage;
uint32_t RAVCodecImageSize;

uint32_t RAVCodecMaxFrame;
uint8_t RAVCodecSampleSize;
uint32_t RAVCodecSampleRate;
uint8_t RAVCodecFPS;
uint16_t RAVCodecFrameWidth;
uint16_t RAVCodecFrameHeight;

bool RAVCodecEnter(char* path) {
  RAVCodecPath = path;
  RAVCodecCurrFrame = 0;
  RAVCodecMaxFrame = 0;
  memset(RAVCodecFrameSamples, 0, SAMPLES_PER_FRAME * sizeof(sample_t));
  RAVCodecImage = NULL;
  RAVCodecImageSize = 0;
  Serial.print("Opening file ");
  Serial.println(RAVCodecFile);
  RAVCodecFile = arcada.open(RAVCodecPath);
  Serial.println();
  if (!RAVCodecFile) {
    Serial.println("Failed to open file");
    return false;
  }
  RAVCodecMaxFrame = _RAVCodecReadULong();
  RAVCodecSampleSize = RAVCodecFile.read();
  RAVCodecSampleRate = _RAVCodecReadULong();
  RAVCodecFPS = RAVCodecFile.read();
  RAVCodecFrameWidth = _RAVCodecReadUInt();
  RAVCodecFrameHeight = _RAVCodecReadUInt();
  Serial.printf("Frame count: %d\nSample size: (bits) %d\nSample rate: %d\n", RAVCodecMaxFrame, RAVCodecSampleSize,
                RAVCodecSampleRate);
  Serial.printf("FPS: %d\nWidth: %d\nHeight: %d\n", RAVCodecFPS, RAVCodecFrameWidth, RAVCodecFrameHeight);
  RAVCodecImageSize = RAVCodecFrameWidth * RAVCodecFrameHeight * sizeof(uint16_t);
  RAVCodecImage = (uint16_t*)malloc(RAVCodecImageSize);
  if (RAVCodecImage == NULL) {
    Serial.println("Failed to allocate frame buffer!");
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
  const uint32_t vidSize = _RAVCodecReadULong();
#if defined(DEBUG_FRAME)
  Serial.print("Video len: ");
  Serial.println(vidSize);
#endif
  // Video
  _RAVCodecReadVideo(vidSize);
// Frame length
#if defined(DEBUG_FRAME)
  Serial.print("Frame len ");
  Serial.println(_RAVCodecReadULong());
#else
  _RAVCodecReadULong();
#endif
  return true;
}

void RAVCodecSeekFramesCur(long changeBy, bool showFramesLeft, Adafruit_Arcada arcada) {
  Serial.print("Seeking ");
  Serial.print(changeBy);
  Serial.println(" frames");
  uint32_t targetFrame = constrain((long)RAVCodecCurrFrame + changeBy, 0, RAVCodecMaxFrame - 1);
  uint32_t framesLeft = abs((long)targetFrame - (long)RAVCodecCurrFrame);
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
        uint32_t frameLen = _RAVCodecReadULong();
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
        if (showFramesLeft && framesLeft % 10 == 0) {
          const uint8_t MESSAGE_LEN = 32;
          char message[MESSAGE_LEN] = {};
          snprintf(message, MESSAGE_LEN, "Seeking... (%lu)", framesLeft);
          arcada.infoBox(message, 0);
        }
        framesLeft--;
      }
    } else {
      while (RAVCodecCurrFrame > targetFrame) {
        // Go back 4 uint8_ts to read the current frame's length
        RAVCodecFile.seekCur(-4);
        uint32_t frameLen = _RAVCodecReadULong();
        // Seek negative (backwards)
        // -4 for the last frame length, frame length itself,
        // and -8 to get to the beginning of the frame
        // Then we can read the frame number and get the last frame
        RAVCodecFile.seekCur((int32_t)(4 + frameLen + 8 + 8) * (int32_t)-1);
        // Read current frame
        RAVCodecCurrFrame = _RAVCodecReadULong();
        // Seek backwards 4 uint8_ts to offset reading the current frame
        RAVCodecFile.seekCur(-4);
        Serial.print("Frame #");
        Serial.println(RAVCodecCurrFrame);
        if (showFramesLeft && framesLeft % 10 == 0) {
          const uint8_t MESSAGE_LEN = 32;
          char message[MESSAGE_LEN] = {};
          snprintf(message, MESSAGE_LEN, "Seeking... (%lu)", framesLeft);
          arcada.infoBox(message, 0);
        }
        framesLeft--;
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
  uint32_t frameLen = _RAVCodecReadULong();
  // Seek negative (backwards)
  // -4 for the last frame length, frame length itself,
  // and -8 to get to the beginning of the frame
  // Then we can read the frame number and get the last frame
  RAVCodecFile.seekCur((int32_t)(4 + frameLen + 8 + 8) * (int32_t)-1);
}

void RAVCodecExit() {
  RAVCodecFile.close();
  free(RAVCodecImage);
}

void _RAVCodecReadAudio() {
  RAVCodecFile.read(RAVCodecFrameSamples, SAMPLES_PER_FRAME);
}

void _RAVCodecReadVideo(uint32_t vidLen) {
  RAVCodecFile.read(RAVCodecImage, vidLen);
}

uint32_t _RAVCodecReadULong() {
  ULongAsBytes ulb;
  RAVCodecFile.read(ulb.as_array, sizeof(uint32_t));
  return ulb.as_ulong;
}

uint16_t _RAVCodecReadUInt() {
  UIntAsBytes uib;
  RAVCodecFile.read(uib.as_array, sizeof(uint16_t));
  return uib.as_uint;
}

bool _RAVCodecAtEOF() {
  return RAVCodecFile.position() >= RAVCodecFile.size() - 1;
}

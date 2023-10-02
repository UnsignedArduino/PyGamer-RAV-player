#pragma once

#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>

// #define DEBUG_FRAME

typedef uint8_t sample_t;

const uint32_t SAMPLE_RATE = 16000;
const uint8_t SAMPLE_SIZE = 1;
const uint8_t CHANNEL_COUNT = 1;

const uint8_t VIDEO_FPS = 10;
const uint32_t FRAME_LENGTH = 1000 / VIDEO_FPS;

const uint32_t SAMPLES_PER_FRAME = SAMPLE_RATE / VIDEO_FPS;

// clang-format off
union ULongAsBytes {
  uint8_t as_array[sizeof(uint32_t)];
  uint32_t as_ulong;
};

union UIntAsBytes {
  uint8_t as_array[sizeof(uint16_t)];
  uint16_t as_uint;
};
// clang-format on

extern char* RAVCodecPath;
extern File RAVCodecFile;
extern uint32_t RAVCodecCurrFrame;
extern sample_t RAVCodecFrameSamples[SAMPLES_PER_FRAME];
extern uint16_t* RAVCodecImage;
extern uint32_t RAVCodecImageSize;

extern uint32_t RAVCodecMaxFrame;
extern uint8_t RAVCodecSampleSize;
extern uint32_t RAVCodecSampleRate;
extern uint8_t RAVCodecFPS;
extern uint16_t RAVCodecFrameWidth;
extern uint16_t RAVCodecFrameHeight;

bool RAVCodecEnter(char* path);
bool RAVCodecDecodeFrame();
void RAVCodecSeekFramesCur(long changeBy, bool showFramesLeft, Adafruit_Arcada arcada);
void RAVCodecSeekToFirstFrame();
void RAVCodecSeekToLastFrame();
void RAVCodecExit();

void _RAVCodecReadAudio();
void _RAVCodecReadVideo(uint32_t vidLen);

uint32_t _RAVCodecReadULong();
uint16_t _RAVCodecReadUInt();
bool _RAVCodecAtEOF();

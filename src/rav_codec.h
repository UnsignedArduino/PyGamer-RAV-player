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

typedef byte sample_t;

const unsigned long SAMPLE_RATE = 16000;
const byte SAMPLE_SIZE = 1;
const byte CHANNEL_COUNT = 1;

const byte VIDEO_FPS = 10;
const unsigned long FRAME_LENGTH = 1000 / VIDEO_FPS;

const unsigned long SAMPLES_PER_FRAME = SAMPLE_RATE / VIDEO_FPS;

const unsigned long MAX_JPEG_SIZE = 32768;

union ULongAsBytes {
  byte as_array[sizeof(unsigned long)];
  unsigned long as_ulong;
};

extern char* RAVCodecPath;
extern File RAVCodecFile;
extern unsigned long RAVCodecCurrFrame;
extern sample_t RAVCodecFrameSamples[SAMPLES_PER_FRAME];
extern byte RAVCodecJPEGImage[MAX_JPEG_SIZE];
extern unsigned long RAVCodecJPEGsize;

bool RAVCodecEnter(char* path);
bool RAVCodecDecodeFrame();
void RAVCodecExit();

void _RAVCodecReadAudio();
void _RAVCodecReadVideo(unsigned long JPEGlen);

unsigned long _RAVCodecReadULong();
bool _RAVCodecAtEOF();

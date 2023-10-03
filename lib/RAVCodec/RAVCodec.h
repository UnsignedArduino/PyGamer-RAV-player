#pragma once

#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>

namespace RAVCodec {
  const uint32_t SAMPLE_RATE = 16000;
  const uint8_t SAMPLE_SIZE = 1;
  const uint8_t CHANNEL_COUNT = 1;

  const uint8_t VIDEO_FPS = 10;
  const uint32_t FRAME_LENGTH = 1000 / VIDEO_FPS;

  const uint32_t SAMPLES_PER_FRAME = SAMPLE_RATE / VIDEO_FPS;

  typedef uint8_t sample_t;

  // clang-format off
  union UInt32AsBytes {
    uint8_t as_array[sizeof(uint32_t)];
    uint32_t as_ulong;
  };

  union UInt16AsBytes {
    uint8_t as_array[sizeof(uint16_t)];
    uint16_t as_uint;
  };

  struct RAVHeader {
    uint32_t maxFrame;
    uint8_t sampleSize;
    uint32_t sampleRate;
    uint8_t fps;
    uint16_t frameWidth;
    uint16_t frameHeight;
  };
  // clang-format on

  class RAVCodec {
    public:
      RAVCodec(Adafruit_Arcada* arcada);

      bool open(char* path);
      bool isOpened();
      bool close();

    private:
      bool readHeader();
      bool allocateBuffers();
      bool deallocateBuffers();

      Adafruit_Arcada* arcada;
      File file;
      RAVHeader* header;

      uint32_t currFrame;

      sample_t* frameAudioSamples;
      uint32_t frameAudioSamplesLen;
      uint16_t* frameImageBlock;
      uint32_t frameImageBlockLen;

      uint32_t readUInt32();
      uint16_t readUInt16();
      uint8_t readUInt8();
  };
}

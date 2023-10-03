#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include <RAVCodec.h>

RAVCodec::RAVCodec::RAVCodec(Adafruit_Arcada* arcada) {
  this->arcada = arcada;
}

bool RAVCodec::RAVCodec::open(char* path) {
  Serial.printf("Opening file %s\n", path);
  this->file = arcada->open(path);
  if (!this->file) {
    Serial.println("Failed to open file!");
    return false;
  }
  if (!this->readHeader()) {
    return false;
  }
  if (!this->allocateBuffers()) {
    return false;
  }
  return true;
}

bool RAVCodec::RAVCodec::isOpened() {
  return this->file;
}

bool RAVCodec::RAVCodec::close() {
  Serial.println("Closing file");
  this->file.close();
  delete this->header;
  return this->deallocateBuffers();
}

bool RAVCodec::RAVCodec::readHeader() {
  Serial.println("Reading header");
  this->file.seek(0);
  this->header = new RAVHeader;
  if (!this->header) {
    Serial.println("Failed to allocate header struct!");
    return false;
  }
  this->header->maxFrame = this->readUInt32();
  this->header->sampleSize = this->readUInt8();
  this->header->sampleRate = this->readUInt32();
  this->header->fps = this->readUInt8();
  this->header->frameWidth = this->readUInt16();
  this->header->frameHeight = this->readUInt16();
  Serial.println("Successfully read header!");
  Serial.printf("Frame count: %d\nSample size: (bits) %d\nSample rate: %d\n", this->header->maxFrame,
                this->header->sampleSize, this->header->sampleRate);
  Serial.printf("FPS: %d\nWidth: %d\nHeight: %d\n", this->header->fps, this->header->frameWidth,
                this->header->frameHeight);
  return true;
}

bool RAVCodec::RAVCodec::allocateBuffers() {
  Serial.println("Allocating buffers");
  this->frameAudioSamplesLen = SAMPLE_RATE;
  this->frameAudioSamples = (sample_t*)malloc(this->frameAudioSamplesLen * sizeof(sample_t));
  if (!this->frameAudioSamples) {
    Serial.printf("Failed to allocate %d bytes for audio!\n", this->frameAudioSamplesLen);
    return false;
  }
  this->frameImageBlockLen = this->header->frameWidth * this->header->frameHeight * sizeof(uint16_t);
  this->frameImageBlock = (uint16_t*)malloc(this->frameImageBlockLen);
  if (!this->frameImageBlock) {
    Serial.printf("Failed to allocate %d bytes for video!\n", this->frameImageBlockLen);
    return false;
  }
  Serial.println("Successfully allocated buffers!");
  return true;
}

bool RAVCodec::RAVCodec::deallocateBuffers() {
  Serial.println("Releasing buffers");
  free(this->frameAudioSamples);
  free(this->frameImageBlock);
  Serial.println("Successfully released buffers!");
  return true;
}

uint32_t RAVCodec::RAVCodec::readUInt32() {
  UInt32AsBytes u32ab;
  this->file.read(u32ab.as_array, sizeof(uint32_t));
  return u32ab.as_ulong;
}

uint16_t RAVCodec::RAVCodec::readUInt16() {
  UInt16AsBytes u16ab;
  this->file.read(u16ab.as_array, sizeof(uint16_t));
  return u16ab.as_uint;
}

uint8_t RAVCodec::RAVCodec::readUInt8() {
  return this->file.read();
}

#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include <CircularBuffer.h>
#include <RAVCodec.h>

Adafruit_Arcada arcada;
RAVCodec::RAVCodec codec(&arcada);

CircularBuffer<RAVCodec::sample_t, RAVCodec::SAMPLES_PER_FRAME> sampleBuf;
uint8_t volume = 128;
const uint8_t MAX_VOLUME = 255;

const uint8_t MAX_PATH_SIZE = 255;

void halt() {
  Serial.println("Halting");
  digitalWrite(LED_BUILTIN, HIGH);
  while (true) {
    ;
  }
}

uint8_t battPercent() {
  float voltage = arcada.readBatterySensor();
  const uint8_t voltCounts = 21;
  // LiPo voltage is not linear
  // https://www.desmos.com/calculator/hrzzbwffct
  const uint8_t lipoPercents[voltCounts] = {100, 95, 90, 85, 80, 75, 70, 65, 60, 55, 50,
                                            45,  40, 35, 30, 25, 20, 15, 10, 5,  0};
  const float lipoVolts[voltCounts] = {4.2,  4.15, 4.11, 4.08, 4.02, 3.98, 3.95, 3.91, 3.87, 3.85, 3.84,
                                       3.82, 3.8,  3.79, 3.77, 3.75, 3.73, 3.71, 3.69, 3.61, 3.27};
  for (uint8_t i = 0; i < voltCounts; i++) {
    if (voltage > lipoVolts[i]) {
      return lipoPercents[i];
    }
  }
  return lipoPercents[voltCounts - 1];
}

void waitForPress() {
  while (arcada.readButtons() == 0) {
    ;
  }
}

void waitForRelease() {
  while (arcada.readButtons() != 0) {
    ;
  }
}

void formatFrameAsTime(uint32_t f, char* result, uint8_t resultSize) {
  uint32_t secs = f / RAVCodec::VIDEO_FPS;
  uint16_t h = secs / 3600;
  secs = secs % 3600;
  uint16_t m = secs / 60;
  uint16_t s = secs % 60;
  if (h == 0) {
    snprintf(result, resultSize, "%02d:%02d", m, s);
  } else {
    snprintf(result, resultSize, "%d:%02d:%02d", h, m, s);
  }
}

void playNextSample() {
  if (sampleBuf.isEmpty()) {
    analogWrite(A0, 0);
    analogWrite(A1, 0);
    return;
  }
  uint8_t sample = map(sampleBuf.shift(), 0, MAX_VOLUME, 0, volume);
  analogWrite(A0, sample);
  analogWrite(A1, sample);
}

void playRAV(char* path) {
  Serial.printf("Playing RAV file %s\n", path);
  if (codec.open(path)) {
    Serial.println("Decoder successfully opened!");

    const RAVCodec::RAVHeader* header = codec.getHeader();

    arcada.timerCallback(header->sampleRate, playNextSample);
    arcada.enableSpeaker(true);

    const uint32_t frameTime = 1000 / header->fps;

    const uint32_t seekFramesBy = header->fps * 5;
    const uint8_t changeVolumeBy = 16;

    bool paused = false;
    bool eof = false;
    bool rerenderFrame = false;

    while (true) {
      const uint32_t startRender = millis();

      if (codec.getCurrentFrame() == header->maxFrame - 1) {
        paused = false;
        eof = true;
      }

      if (!paused || rerenderFrame) {
        codec.readCurrentFrame();

        if (!paused) {
          uint32_t audioFrameLen;
          RAVCodec::sample_t* audioFrame = codec.getCurrentFrameAudio(audioFrameLen);
          for (uint32_t i = 0; i < RAVCodec::SAMPLES_PER_FRAME; i++) {
            sampleBuf.push(audioFrame[i]);
          }
        }

        const int16_t x = (ARCADA_TFT_WIDTH - header->frameWidth) / 2;
        const int16_t y = (ARCADA_TFT_HEIGHT - header->frameHeight) / 2;
        uint32_t videoFrameLen;
        uint16_t* videoFrame = codec.getCurrentFrameVideo(videoFrameLen);
        arcada.display->startWrite();
        arcada.display->setAddrWindow(x, y, header->frameWidth, header->frameHeight);
        arcada.display->writePixels(videoFrame, header->frameWidth * header->frameHeight, true, false);
        arcada.display->endWrite();

        rerenderFrame = false;
      }

      arcada.display->setCursor(0, ARCADA_TFT_HEIGHT - 8);
      arcada.display->setTextColor(ARCADA_WHITE, ARCADA_BLACK);
      char display[24];
      formatFrameAsTime(codec.getCurrentFrame(), display, 24);
      arcada.display->print(display);
      arcada.display->print("/");
      formatFrameAsTime(header->maxFrame, display, 24);
      arcada.display->print(display);

      arcada.readButtons();
      uint8_t justPressed = arcada.justPressedButtons();
      if (justPressed & ARCADA_BUTTONMASK_A) {
        if (!eof) {
          Serial.println("Pause");
          paused = true;
        }
      }
      if (justPressed & ARCADA_BUTTONMASK_B) {
        Serial.println("Resume");
        if (eof) {
          Serial.println("Seek to beginning");
          eof = false;
          codec.setCurrentFrame(0);
        }
        paused = false;
      }
      if (justPressed & ARCADA_BUTTONMASK_LEFT) {
        // uint32_t makes it slightly annoying as we can't max against 0
        if (codec.getCurrentFrame() < seekFramesBy) {
          codec.setCurrentFrame(0);
        } else {
          codec.setCurrentFrame(codec.getCurrentFrame() - seekFramesBy);
        }
        Serial.printf("Seek backwards by %d frames to frame %d\n", seekFramesBy, codec.getCurrentFrame());
        rerenderFrame = true;
      }
      if (justPressed & ARCADA_BUTTONMASK_RIGHT) {
        // Handles min against max frame
        codec.setCurrentFrame(codec.getCurrentFrame() + seekFramesBy);
        Serial.printf("Seek forward by %d frames to frame %d\n", seekFramesBy, codec.getCurrentFrame());
        rerenderFrame = true;
      }
      if (justPressed & ARCADA_BUTTONMASK_UP) {
        if (volume == MAX_VOLUME) {
          ;
        } else if (volume == MAX_VOLUME - changeVolumeBy + 1) {
          volume += changeVolumeBy - 1;
        } else {
          volume += changeVolumeBy;
        }
        Serial.printf("Volume increase to %d\n", volume);
      }
      if (justPressed & ARCADA_BUTTONMASK_DOWN) {
        if (volume == 0) {
          ;
        } else if (volume == MAX_VOLUME) {
          volume -= changeVolumeBy - 1;
        } else {
          volume -= changeVolumeBy;
        }
        Serial.printf("Volume decrease to %d\n", volume);
      }

      while ((millis() - startRender) < frameTime) {
        delay(1);
      }
    }

    arcada.timerStop();
    arcada.enableSpeaker(false);
  } else {
    Serial.println("Decoder failed to open!");
  }
  codec.close();
}

void setup() {
  Serial.begin(9600);
  Serial.println("RAV player");
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!arcada.arcadaBegin()) {
    Serial.println("Failed to start Arcada");
    halt();
  }

  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.display->fillScreen(ARCADA_BLACK);

  if (!arcada.filesysBegin(ARCADA_FILESYS_SD)) {
    Serial.println("Could not find SD filesystem");
    arcada.display->fillScreen(ARCADA_RED);
    arcada.haltBox("Could not find SD card! "
                   "Make sure there is a card plugged in that is formatted with FAT.");
  } else {
    Serial.println("Found SD filesystem");
  }

  analogWriteResolution(8);
}

void loop() {
  static char path[MAX_PATH_SIZE];
  if (!arcada.chooseFile("/", path, MAX_PATH_SIZE, "rav")) {
    Serial.println("Failed to choose file!");
    return;
  }
  Serial.printf("Choose %s\n", path);
  waitForRelease();
  playRAV(path);
}

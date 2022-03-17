#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include <JPEGDEC.h>
#include "pygamer_arcada.h"
#include "rav_codec.h"
#include "rav_player.h"

Adafruit_Arcada arcada;
JPEGDEC jpeg;

bool volatile playSamples = false;
unsigned long volatile sampleIdx = 0;

byte volume = 32;
const byte MAX_VOLUME = 128;

const char* playerMenu[PLAYER_MENU_LEN] = {"Cancel", "Seek", "Exit"};

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
  waitForRelease(arcada);
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

  bool paused = false;
  bool EOFed = false;

  char title[MAX_PATH_LEN + 2] = {};
  strcat(title, path);
  strcat(title, " ");

  int16_t x;
  int16_t y;
  uint16_t w;
  uint16_t h;
  arcada.display->getTextBounds(title, 0, 0, &x, &y, &w, &h);

  const byte titleChangeX = 2;
  long titleX = 0;
  long titleResetX = 0 - w;
  const unsigned long titleStayTime = 30;
  unsigned long titleStayLeft = titleStayTime;
  const bool scrollTitle = w > ARCADA_TFT_WIDTH;

  byte battMaxPercentLen = 8;
  char battMaxPercent[battMaxPercentLen] = {};
  snprintf(battMaxPercent, battMaxPercentLen, "%u%%", battPercent(arcada));
  const unsigned long battUpdateFramesTime = 50;
  unsigned long battUpdateFramesLeft = battUpdateFramesTime;
  Serial.print("Battery: ");
  Serial.println(battMaxPercent);

  byte MAX_NOTICE_LEN = 16;
  char notice[MAX_NOTICE_LEN + 1] = {};
  const unsigned long noticeStayTime = 30;
  unsigned long noticeStayLeft = 0;

  arcada.display->setTextColor(ARCADA_WHITE, ARCADA_BLACK);
  arcada.display->setTextSize(1);
  arcada.display->setTextWrap(false);
  arcada.display->cp437(true);

  arcada.timerCallback(SAMPLE_RATE, playNextSample);
  arcada.enableSpeaker(true);

  while (true) {
    unsigned long start_time = millis();
    if (!paused) {
      if (RAVCodecDecodeFrame()) {
        sampleIdx = 0;
        EOFed = false;
      } else {
        EOFed = true;
      }
    }
    drawCurrentFrame();
    // Notice
    if (noticeStayLeft > 0) {
      noticeStayLeft --;
      int16_t x;
      int16_t y;
      uint16_t w;
      uint16_t h;
      arcada.display->getTextBounds(notice, 0, 0, &x, &y, &w, &h);
      arcada.display->setCursor((ARCADA_TFT_WIDTH / 2) - (w / 2), 
                                (ARCADA_TFT_HEIGHT / 2) - (h / 2));
      arcada.display->setTextColor(ARCADA_WHITE);
      arcada.display->print(notice);
    }
    byte pressed = arcada.readButtons();
    if (!EOFed) {
      if (!paused && pressed & ARCADA_BUTTONMASK_A) {
        paused = true;
        strcpy(notice, "Pause");
        noticeStayLeft = noticeStayTime;
        Serial.println(notice);
      }
      if (paused && pressed & ARCADA_BUTTONMASK_B) {
        paused = false;
        strcpy(notice, "Resume");
        noticeStayLeft = noticeStayTime;
        Serial.println(notice);
      }
    }
    if (pressed & ARCADA_BUTTONMASK_UP) {
      volume = min(volume + 8, MAX_VOLUME);
      snprintf(notice, MAX_NOTICE_LEN, "%ld%% volume", map(volume, 0, MAX_VOLUME, 0, 100));
      noticeStayLeft = noticeStayTime;
      Serial.println(notice);
    }
    if (pressed & ARCADA_BUTTONMASK_DOWN) {
      volume = max(volume - 8, 0);
      snprintf(notice, MAX_NOTICE_LEN, "%ld%% volume", map(volume, 0, MAX_VOLUME, 0, 100));
      noticeStayLeft = noticeStayTime;
      Serial.println(notice);
    }
    if (pressed & ARCADA_BUTTONMASK_LEFT) {
      RAVCodecSeekFramesCur(-FRAMES_TO_SEEK);
      if (paused) {
        RAVCodecDecodeFrame();
      }
      snprintf(notice, MAX_NOTICE_LEN, "-%u seconds", SECS_TO_SEEK);
      noticeStayLeft = noticeStayTime;
      Serial.println(notice);
    }
    if (!EOFed && pressed & ARCADA_BUTTONMASK_RIGHT) {
      RAVCodecSeekFramesCur(FRAMES_TO_SEEK);
      if (paused) {
        RAVCodecDecodeFrame();
      }
      snprintf(notice, MAX_NOTICE_LEN, "+%u seconds", SECS_TO_SEEK);
      noticeStayLeft = noticeStayTime;
      Serial.println(notice);
    }
    if (pressed & ARCADA_BUTTONMASK_SELECT || pressed & ARCADA_BUTTONMASK_START) {
      byte selected = arcada.menu(playerMenu, PLAYER_MENU_LEN, ARCADA_WHITE, ARCADA_BLACK, true);
      if (selected == 1) {
        Serial.println("Seek");
        const unsigned long currFrame = RAVCodecCurrFrame;
        unsigned long newFrame = currFrame;
        drawCurrentFrame();
        while (true) {
          const unsigned int MAX_MESSAGE_LEN = 64;
          char message[MAX_MESSAGE_LEN] = {};
          const byte newFrameTimeLen = 16;
          char newFrameTime[newFrameTimeLen] = {};
          formatFrameAsTime(newFrame, newFrameTime, newFrameTimeLen);
          snprintf(message, MAX_MESSAGE_LEN, 
                   "New time: %s",
                   newFrameTime);
          arcada.infoBox(message, 0);
          waitForPress(arcada);
          byte pressed = arcada.readButtons();
          const unsigned long changeBy = VIDEO_FPS * 10;
          if (pressed & ARCADA_BUTTONMASK_UP) {
            newFrame = min((unsigned long)newFrame + changeBy, (unsigned long)RAVCodecMaxFrame);
          }
          if (pressed & ARCADA_BUTTONMASK_DOWN) {
            if (changeBy > newFrame) {
              newFrame = 0;
            } else {
              newFrame -= changeBy;
            }
          }
          if (pressed & ARCADA_BUTTONMASK_A) {
            long frameDiff = newFrame - currFrame;
            drawCurrentFrame();
            arcada.infoBox("Seeking...", 0);
            Serial.print("Seeking ");
            Serial.print(frameDiff);
            Serial.println(" frames");
            RAVCodecSeekFramesCur(frameDiff);
            break;
          }
          if (pressed & ARCADA_BUTTONMASK_B) {
            break;
          }
          delay(100);
        }
        if (paused) {
          RAVCodecDecodeFrame();
        }
      } else if (selected == 2) {
        Serial.println("User exit");
        break;
      }
    }
    arcada.display->setTextColor(ARCADA_WHITE, ARCADA_BLACK);
    // Title
    if (scrollTitle) {
      if (titleX > 0) {
        titleX -= titleChangeX;
      } else if (titleStayLeft > 0) {
        titleStayLeft --;
      } else if (titleX > titleResetX) {
        titleX -= titleChangeX;
      } else {
        titleX = ARCADA_TFT_WIDTH;
        titleStayLeft = titleStayTime;
      }
    }
    arcada.display->setCursor(titleX, 0);
    arcada.display->print(title);
    // Bottom row
    // Play/paused
    arcada.display->setCursor(0, ARCADA_TFT_HEIGHT - 8);
    arcada.display->print(" ");
    if (EOFed) {
      arcada.display->write(0xFE); // "■"
    } else if (paused) {
      arcada.display->write(0xBA); // "║"
    } else {
      arcada.display->write(0x10); // "►"
    }
    arcada.display->print(" ");
    // Time
    const byte timeBufSize = 12;
    char timeBuf[timeBufSize] = {};
    formatFrameAsTime(RAVCodecCurrFrame, timeBuf, timeBufSize);
    arcada.display->print(timeBuf);
    arcada.display->print("/");
    formatFrameAsTime(RAVCodecMaxFrame, timeBuf, timeBufSize);
    arcada.display->print(timeBuf);
    if (battUpdateFramesLeft == 0) {
      battUpdateFramesLeft = battUpdateFramesTime;
      snprintf(battMaxPercent, battMaxPercentLen, "  %u%%", battPercent(arcada));
      Serial.print("Battery: ");
      Serial.println(battMaxPercent);
    }
    battUpdateFramesLeft --;
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
    arcada.display->getTextBounds(battMaxPercent, 0, 0, &x, &y, &w, &h);
    const unsigned int battX = ARCADA_TFT_WIDTH - w;
    arcada.display->setCursor(battX, ARCADA_TFT_HEIGHT - 8);
    arcada.display->print(battMaxPercent);
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

void drawCurrentFrame() {
  // Draw current frame
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
    snprintf(result, resultSize, "%02d:%02d:%02d", h, m, s);
  }
}

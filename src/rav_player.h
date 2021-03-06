#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include <Adafruit_Arcada.h>
#include <JPEGDEC.h>
#include "rav_codec.h"

typedef unsigned int path_size_t;

const path_size_t MAX_PATH_LEN = 255;

extern Adafruit_Arcada arcada;
extern JPEGDEC jpeg;

extern volatile bool playSamples;
extern volatile unsigned long sampleIdx;

const byte SECS_TO_SEEK = 5;
const unsigned long FRAMES_TO_SEEK = VIDEO_FPS * SECS_TO_SEEK;

const byte PLAYER_MENU_LEN = 3;
extern const char* playerMenu[PLAYER_MENU_LEN];

bool RAVinit();
bool RAVPickFile(char* result, path_size_t resultSize);
void RAVPlayFile(char* path);

void playNextSample();
void drawCurrentFrame();
int JPEGDraw(JPEGDRAW *draw);
void formatFrameAsTime(unsigned long f, char* result, byte resultSize);

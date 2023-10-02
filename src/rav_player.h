#include <Arduino.h>
// Needed because Adafruit_Arcade needs Adafruit_ImageReader which needs Adafruit_EPD
#include <Adafruit_EPD.h>
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit Arcada Library\Boards\Adafruit_Arcada_PyGamer.h
// (or your respective board) and comment out #define ARCADA_USE_JSON because it breaks compilation
// Go to .pio\libdeps\adafruit_pygamer_m4\Adafruit GFX Library\Adafruit_SPITFT.h
// and comment out #define USE_SPI_DMA because it does not work when compiled with PlatformIO
#include "rav_codec.h"
#include <Adafruit_Arcada.h>

typedef uint16_t path_size_t;

const path_size_t MAX_PATH_LEN = 255;

extern Adafruit_Arcada arcada;

extern volatile bool playSamples;
extern volatile uint32_t sampleIdx;

const uint8_t SECS_TO_SEEK = 5;
const uint32_t FRAMES_TO_SEEK = VIDEO_FPS * SECS_TO_SEEK;

const uint8_t PLAYER_MENU_LEN = 3;
extern const char* playerMenu[PLAYER_MENU_LEN];

bool RAVinit();
bool RAVPickFile(char* result, path_size_t resultSize);
void RAVPlayFile(char* path);

void playNextSample();
void drawCurrentFrame();
void formatFrameAsTime(uint32_t f, char* result, uint8_t resultSize);

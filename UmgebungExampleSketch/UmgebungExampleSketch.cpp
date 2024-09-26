#include <iostream>
#include <stdio.h>

#ifdef KLST_VCV_DEBUG
#define KLST_VCV_LOG(...) \
    printf("+++ ");       \
    printf(__VA_ARGS__);  \
    printf("\n");
#else
#define KLST_VCV_LOG(...)
#endif

extern "C" {

void setup() { KLST_VCV_LOG("setup"); }

void loop() {  }

void beat(uint32_t beat_count) { KLST_VCV_LOG("beat: %i", beat_count); }

void audioblock(float** input, float** output, uint32_t length) { KLST_VCV_LOG("audioblock"); }

const char* name() { return "UmgebungSketch"; }

}
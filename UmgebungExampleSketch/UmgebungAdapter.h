#pragma once

#include "PApplet.h"

using namespace umgebung;

// TODO add `void settings()`
// TODO maybe add some events like mouse or key events

extern "C" {
PApplet*    create_umgebung();
void        destroy_umgebung(PApplet* application);
void        setup(PApplet* application);
void        draw(PApplet* application);
void        beat(PApplet* application, uint32_t beat_count);
void        audioblock(PApplet* application, float** input, float** output, int length);
const char* name(PApplet* application);
}

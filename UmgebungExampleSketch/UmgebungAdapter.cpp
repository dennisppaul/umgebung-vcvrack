#include <iostream>
#include "Umgebung.h"
#include "UmgebungAdapter.h"

using namespace umgebung;

extern "C" {
void        die() { std::cout << "UmgebungAdapter: application is nullptr" << std::endl; }
PApplet*    create_umgebung() { return instance(); }
void        destroy_umgebung(PApplet* application) { delete application; }
void        setup(PApplet* application) { application != nullptr ? application->setup() : die(); }
void        draw(PApplet* application) { application != nullptr ? application->draw() : die(); }
void        beat(PApplet* application, uint32_t beat_count) { application != nullptr ? application->beat(beat_count) : die(); }
void        audioblock(PApplet* application, float** input, float** output, const int length) { application != nullptr ? application->audioblock(input, output, length) : die(); }
const char* name(PApplet* application) { return application != nullptr ? application->name() : "null"; }
}
#pragma once

#include <stdint.h>

namespace umgebung {
    class PApplet;
}

class UmgebungAdapter {
public:
    umgebung::PApplet* instance;

    [[nodiscard]] umgebung::PApplet* create();

    void destroy();

    void        setup();
    void        draw();
    void        beat(uint32_t beat_count);
    void        audioblock(float** input, float** output, int length);
    const char* name();
};

extern "C" {
UmgebungAdapter* create_umgebung();
void             destroy_umgebung(UmgebungAdapter* application);
void             setup(UmgebungAdapter* application);
void             draw(UmgebungAdapter* application);
void             beat(UmgebungAdapter* application, uint32_t beat_count);
void             audioblock(UmgebungAdapter* application, float** input, float** output, int length);
const char*      name(UmgebungAdapter* application);
}
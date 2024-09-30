#include "Umgebung.h"
#include "UmgebungAdapter.h"

using namespace umgebung;

namespace umgebung {
    class PApplet;
}

PApplet* UmgebungAdapter::create() {
    instance = umgebung::instance();
    return umgebung::instance();
}

void UmgebungAdapter::destroy() {
    delete instance;
    instance = nullptr;
}

void UmgebungAdapter::setup() {
    if (instance) {
        instance->setup();
    }
}


void UmgebungAdapter::draw() {
    if (instance) {
        instance->draw();
    }
}

void UmgebungAdapter::beat(uint32_t beat_count) {
    if (instance) {
        // instance->beat(beat_count);
    }
}

void UmgebungAdapter::audioblock(float** input, float** output, int length) {
    if (instance) {
        instance->audioblock(input, output, length);
    }
}

const char* UmgebungAdapter::name() {
    if (instance) {
        return instance->name();
    }
    return "";
}

extern "C" {
UmgebungAdapter* create_umgebung() { return new UmgebungAdapter(); }
void             destroy_umgebung(UmgebungAdapter* application) {
    delete application->instance;
    application->instance = nullptr;
    delete application;
}
void        setup(UmgebungAdapter* application) { application->setup(); }
void        draw(UmgebungAdapter* application) { application->draw(); }
void        beat(UmgebungAdapter* application, uint32_t beat_count) { application->beat(beat_count); }
void        audioblock(UmgebungAdapter* application, float** input, float** output, int length) { application->audioblock(input, output, length); }
const char* name(UmgebungAdapter* application) { return application->name(); }
}
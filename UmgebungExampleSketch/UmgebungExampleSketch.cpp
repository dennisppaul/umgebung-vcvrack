#include <iostream>
#include "Umgebung.h"

using namespace umgebung;

class UmgebungApp : public PApplet {

    PFont* mFont   = nullptr;
    int    mWidth  = 1024;
    int    mHeight = 768;

    void settings() override {
        size(mWidth, mHeight);
        audio_input_device    = DEFAULT_AUDIO_DEVICE;
        audio_output_device   = DEFAULT_AUDIO_DEVICE;
        audio_input_channels  = DEFAULT_NUMBER_OF_INPUT_CHANNELS;
        audio_output_channels = DEFAULT_NUMBER_OF_OUTPUT_CHANNELS;
    }

    void setup() override {
        const std::vector search_paths = {
            get_executable_location() + "../..",
            get_executable_location() + "..",
            get_executable_location() + "."};
        const std::string mFontFile = find_file_in_paths(search_paths, "RobotoMono-Regular.ttf");
        if (!mFontFile.empty()) {
            std::cout << "found font at: " << mFontFile << std::endl;
            mFont = loadFont(mFontFile, 280);
            textFont(mFont);
        }

        println("width : ", width);
        println("height: ", height);
    }

    void draw() override {
        background(1);

        stroke(0);
        noFill();
        rect(10, 10, width / 2 - 20, height / 2 - 20);

        noStroke();
        fill(random(0, 0.2));
        rect(20, 20, width / 2 - 40, height / 2 - 40);

        fill(0, 0.5f, 1);
        text("23", 20, height - 20);

        circle(0, 0, 20);
        circle(width, 0, 20);
        circle(width, height, 20);
        circle(0, height, 20);
    }

    void audioblock(float** input, float** output, int length) override {
        for (int i = 0; i < length; i++) {
            float sample = random(-0.1, 0.1);
            for (int j = 0; j < audio_output_channels; ++j) { output[j][i] = sample; }
        }
    }

    void beat(uint32_t beat_count) override {
        // println("beat: ", beat_count);
    }

    void keyPressed() override {
        if (key == 'q') { exit(); }
        println((char) key, " pressed");
    }

    const char* name() override {
        return "Umgebung42";
    }
};

PApplet* umgebung::instance() { return new UmgebungApp(); }

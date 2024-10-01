// TODO add CV 2 inputs and 2 outputs
// TODO propagate knobs to sketch as `event(float*, uint8_t)`?

#include <dlfcn.h>
#include <iostream>
#include <osdialog.h>
#include "plugin.hpp"

#define UMGB_VCV_DEBUG
#ifdef UMGB_VCV_DEBUG
#define UMGB_VCV_LOG(...) \
    printf("\033[32m");   \
    printf("+++ ");       \
    printf(__VA_ARGS__);  \
    printf("\033[0m");    \
    printf("\n");
#else
#define UMGB_VCV_LOG(...)
#endif

uint8_t mInstanceCounter = 0;

class UmgebungApp;

struct UmgebungModule : Module {

    // float bg_color_red   = 0.0;
    // float bg_color_green = 0.0;
    // float bg_color_blue  = 0.0;

    static constexpr uint32_t SAMPLES_PER_AUDIO_BLOCK               = 512;
    float                     mLeftOutput[SAMPLES_PER_AUDIO_BLOCK]  = {0};
    float                     mRightOutput[SAMPLES_PER_AUDIO_BLOCK] = {0};
    float                     mLeftInput[SAMPLES_PER_AUDIO_BLOCK]   = {0};
    float                     mRightInput[SAMPLES_PER_AUDIO_BLOCK]  = {0};

    float    mBangReloadButtonState  = 0.0f;
    float    mBangLoadAppButtonState = 0.0f;
    float    mBeatTriggerCounter     = 0.0f;
    float    mBeatDurationSec        = 0.125f;
    uint16_t mSampleCollectorCounter = 0;
    uint32_t mBeatCounter            = 0;

    LedDisplayTextField* mTextFieldAppName   = nullptr;
    std::string          mLibraryName        = "dep/libUmgebungApp";
    bool                 mInitializeApp      = true;
    bool                 mAllocateAudioblock = true;

#if defined ARCH_WIN
    HINSTANCE mHandleUmgebungSketch = 0;
#else
    void* mHandleUmgebungSketch = nullptr;
#endif

    enum ParamId {
        PITCH_PARAM_1,
        PITCH_PARAM_2,
        RELOAD_PARAM,
        LOAD_APP,
        PARAMS_LEN
    };
    enum InputId {
        LEFT_INPUT,
        RIGHT_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        BLINK_LIGHT,
        LIGHTS_LEN
    };

    void handle_umgebung_app() {
        try {
            unload_app();
        } catch (Exception e) {
            UMGB_VCV_LOG("could not unload app");
        }
        try {
            load_app();
        } catch (Exception e) {
            UMGB_VCV_LOG("could not load app");
        }
        try {
            load_symbols();
        } catch (Exception e) {
            UMGB_VCV_LOG("could not load symbols");
        }
        create_app();
        update_app_name();
    }

    UmgebungModule() {
        mInstanceCounter++;
        if (mInstanceCounter > 1) {
            UMGB_VCV_LOG("mInstanceCounter: %i", mInstanceCounter);
            UMGB_VCV_LOG("WARNING multiple instances of plugins are currently not supported.");
            // throw Exception(string::f("multiple instances of plugins are currently not supported."));
        }
        handle_umgebung_app();

        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        configParam(PITCH_PARAM_1, 0.f, 1.f, 0.f, "");
        configParam(PITCH_PARAM_2, 0.f, 1.f, 0.f, "");
        configInput(LEFT_INPUT, "");
        configInput(RIGHT_INPUT, "");
        configOutput(LEFT_OUTPUT, "");
        configOutput(RIGHT_OUTPUT, "");
    }

    ~UmgebungModule() override {
        try {
            unload_app();
        } catch (Exception e) {
            UMGB_VCV_LOG("could not unload app");
        }
        if (mInstanceCounter > 0) {
            mInstanceCounter--;
        }
    }

    void process_audio(const int length) {
        constexpr int numChannels = 2;
        auto**        input       = new float*[numChannels];
        auto**        output      = new float*[numChannels];

        for (int i = 0; i < numChannels; ++i) {
            input[i]  = new float[length];
            output[i] = new float[length];
        }

        for (int sample = 0; sample < length; ++sample) {
            // TODO this must be read from the input buffer
            input[0][sample]  = inputs[LEFT_INPUT].getVoltage() / 5.0f;
            input[1][sample]  = inputs[RIGHT_INPUT].getVoltage() / 5.0f;
            output[0][sample] = 0.0f; // Initialize output to 0
            output[1][sample] = 0.0f;
        }

        if (fAudioblockFunction) {
            fAudioblockFunction(umgebung_app, input, output, length);
        }

        for (int sample = 0; sample < length; ++sample) {
            // TODO this must be written to the output buffer
            outputs[LEFT_OUTPUT].setVoltage(output[0][sample] * 5.0f);
            outputs[RIGHT_OUTPUT].setVoltage(output[1][sample] * 5.0f);
        }

        for (int i = 0; i < numChannels; ++i) {
            delete[] input[i];
            delete[] output[i];
        }
        delete[] input;
        delete[] output;
    }

    void process(const ProcessArgs& args) override {
        if (mInitializeApp) {
            if (umgebung_app && fSetupFunction) {
                fSetupFunction(umgebung_app);
            }
            mInitializeApp = false;
        }

        if (params[RELOAD_PARAM].getValue() > mBangReloadButtonState) {
            mBeatCounter = 0;
            reload_app();
        }
        mBangReloadButtonState = params[RELOAD_PARAM].getValue();

        if (params[LOAD_APP].getValue() > mBangLoadAppButtonState) {
            UMGB_VCV_LOG("loading app");
            // from Wavetable.hpp
            static const char WAVETABLE_FILTERS[] = "WAV (.wav):wav,WAV;Raw:f32,i8,i16,i24,i32,*";
            osdialog_filters* filters             = osdialog_filters_parse(WAVETABLE_FILTERS);
            // std::string       wavetableDir;
            char* pathC = osdialog_file(OSDIALOG_OPEN, NULL, NULL, filters);
            // char* pathC = osdialog_file(OSDIALOG_OPEN, wavetableDir.empty() ? NULL : wavetableDir.c_str(), NULL, filters);
            if (!pathC) {
                // Fail silently
                return;
            }
            std::string path = pathC;
            std::free(pathC);
            std::string wavetableDir = system::getDirectory(path);
            // load(path);
            std::string filename;
            filename = system::getFilename(path);
            UMGB_VCV_LOG("dir path: %s", wavetableDir.c_str());
            UMGB_VCV_LOG("filename: %s", filename.c_str());
        }
        mBangLoadAppButtonState = params[LOAD_APP].getValue();

        // TODO pass this on as an event
        float bg_color_green = params[PITCH_PARAM_1].getValue();
        float bg_color_blue  = params[PITCH_PARAM_2].getValue();

        // outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());  // pass through
        // outputs[RIGHT_OUTPUT].setVoltage(inputs[RIGHT_INPUT].getVoltage()); // pass through

        //         float mSampleLeft                    = mLeftOutput[mSampleCollectorCounter];
        //         float mSampleRight                   = mRightOutput[mSampleCollectorCounter];
        //         mLeftInput[mSampleCollectorCounter]  = inputs[LEFT_IN_INPUT].getVoltage() / 5.f;
        //         mRightInput[mSampleCollectorCounter] = inputs[RIGHT_IN_INPUT].getVoltage() / 5.f;
        //         mSampleCollectorCounter++;
        //         if (mSampleCollectorCounter >= SAMPLES_PER_AUDIO_BLOCK) {
        //             mSampleCollectorCounter = 0;
        //             mAudioblockCallback(mLeftOutput, mRightOutput, mLeftInput, mRightInput);
        //         }

        //         std::cout << "inputs[LEFT_INPUT] : " << inputs[LEFT_INPUT].getVoltage() << std::endl;
        //         std::cout << "inputs[RIGHT_INPUT]: " << inputs[RIGHT_INPUT].getVoltage() << std::endl;
        //         std::cout << "params[PITCH_PARAM]: " << params[PITCH_PARAM].getValue() << std::endl;
        //         std::cout << std::endl;

        // TODO implement audio processing with SAMPLES_PER_AUDIO_BLOCK greater than 1 ;)
        process_audio(1);

        mBeatTriggerCounter += args.sampleTime;
        if (mBeatTriggerCounter >= mBeatDurationSec) {
            mBeatTriggerCounter -= mBeatDurationSec;
            if (umgebung_app && fBeatFunction) {
                fBeatFunction(umgebung_app, mBeatCounter);
                mBeatCounter++;
            }
        }

        // Blink light at 1Hz
        static float blinkPhase = 0.0f;
        blinkPhase += args.sampleTime;
        if (blinkPhase >= 1.f) {
            blinkPhase -= 1.f;
        }
        lights[BLINK_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);

        // if (umgebung_app) {
        //     umgebung_app->loop();
        // }
    }

    void call_draw() const {
        if (umgebung_app && fDrawFunction) {
            fDrawFunction(umgebung_app);
        }
    }

    template<typename T>
    static void load_symbol(void* handle, const char* symbol_name, T& function_ptr, const std::string& library_name) {
#if defined ARCH_WIN
        function_ptr = (T) GetProcAddress((HINSTANCE) handle, symbol_name);
#else
        function_ptr = (T) dlsym(handle, symbol_name);
#endif
        if (!function_ptr) {
            UMGB_VCV_LOG("failed to read '%s' symbol in %s", symbol_name, library_name.c_str());
        } else {
            UMGB_VCV_LOG("successfully read '%s' symbol in %s", symbol_name, library_name.c_str());
        }
    }

    typedef UmgebungApp* (*CreateUmgebungFunctionPtr)();
    typedef void         (*DestroyFunctionPtr)(UmgebungApp*);
    typedef void         (*SetupFunctionPtr)(UmgebungApp*);
    typedef void         (*DrawFunctionPtr)(UmgebungApp*);
    typedef void         (*BeatFunctionPtr)(UmgebungApp*, uint32_t);
    typedef void         (*AudioblockFunctionPtr)(UmgebungApp*, float**, float**, int);
    typedef const char*  (*NameFunctionPtr)(UmgebungApp*);

    CreateUmgebungFunctionPtr fCreateUmgebungFunction  = nullptr;
    DestroyFunctionPtr        fDestroyUmgebungFunction = nullptr;
    SetupFunctionPtr          fSetupFunction           = nullptr;
    DrawFunctionPtr           fDrawFunction            = nullptr;
    BeatFunctionPtr           fBeatFunction            = nullptr;
    AudioblockFunctionPtr     fAudioblockFunction      = nullptr;
    NameFunctionPtr           fNameFunction            = nullptr;

    void load_symbols() {
        load_symbol(mHandleUmgebungSketch, "create_umgebung", fCreateUmgebungFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "destroy_umgebung", fDestroyUmgebungFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "setup", fSetupFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "draw", fDrawFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "beat", fBeatFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "audioblock", fAudioblockFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "name", fNameFunction, mLibraryName);
    }

    void load_app() {
        /* Load plugin library */
        std::string mAppLibraryFilename;
#if defined ARCH_LIN
        mAppLibraryFilename = pluginInstance->path + "/" + mLibraryName + ".so";
#elif defined ARCH_WIN
        mAppLibraryFilename = pluginInstance->path + "/" + mLibraryName + ".dll";
#elif ARCH_MAC
        mAppLibraryFilename = pluginInstance->path + "/" + mLibraryName + ".dylib";
#endif
        UMGB_VCV_LOG("loading app from file: %s", mAppLibraryFilename.c_str());

        /* Check file existence */
        if (!system::isFile(mAppLibraryFilename)) {
            throw Exception(string::f("Library %s does not exist", mLibraryName.c_str()));
        }

        /* Load dynamic/shared library */
#if defined ARCH_WIN
        SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
        HINSTANCE handle = LoadLibrary(mAppLibraryFilename.c_str());
        SetErrorMode(0);
        if (!handle) {
            int error = GetLastError();
            throw Exception(string::f("Failed to load library %s: code %d", mLibraryName.c_str(), error));
        }
#else
        void* handle = dlopen(mAppLibraryFilename.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            throw Exception(string::f("Failed to load library %s: %s", mLibraryName.c_str(), dlerror()));
        }
#endif
        mHandleUmgebungSketch = handle;
    }

    void unload_app() {
        if (umgebung_app) {
            /* close the library */
            UMGB_VCV_LOG("unloading app: %p ", mHandleUmgebungSketch);
            destroy_app();

            fCreateUmgebungFunction  = nullptr;
            fDestroyUmgebungFunction = nullptr;
            if (mHandleUmgebungSketch) {
#if defined ARCH_WIN
                FreeLibrary((HINSTANCE) mHandleUmgebungSketch);
#else
                dlclose(mHandleUmgebungSketch);
#endif
                mHandleUmgebungSketch = nullptr;
            }
            umgebung_app = nullptr;
        }
    }

    const char* get_name() const {
        return fNameFunction == nullptr ? DEFAULT_NAME : fNameFunction(umgebung_app);
    }

    const char* DEFAULT_NAME = "NOOP";

    void update_app_name() {
        if (umgebung_app && fNameFunction && mTextFieldAppName) {
            mTextFieldAppName->text = fNameFunction(umgebung_app);
        }
    }

    void create_app() {
        if (fCreateUmgebungFunction) {
            umgebung_app = fCreateUmgebungFunction();
        }
    }

    void destroy_app() {
        if (umgebung_app && fDestroyUmgebungFunction) {
            UMGB_VCV_LOG("destroying app: %s", get_name());
            fDestroyUmgebungFunction(umgebung_app);
            if (mTextFieldAppName) {
                mTextFieldAppName->text = DEFAULT_NAME;
            }
        }
    }

    void reload_app() {
        handle_umgebung_app();
        mInitializeApp = true;
    }

private:
    UmgebungApp* umgebung_app = nullptr;
};

struct UmgebungWidget : OpenGlWidget {

    UmgebungModule* module = nullptr;

    void step() override {
        // Render every frame
        dirty = true;
        FramebufferWidget::step();
    }

    void drawFramebuffer() override {
        math::Vec fbSize = getFramebufferSize();
        glViewport(0.0, 0.0, fbSize.x, fbSize.y);

        // glClearColor(0, 0, 0, 1.0);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // glClearColor(module->bg_color_red, module->bg_color_green, module->bg_color_blue, 1.0);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, fbSize.x, 0.0, fbSize.y, -1.0, 1.0);
        glScalef(1.0, -1.0, 1.0);
        glTranslatef(0.0, -fbSize.y, 0.0);

        //         std::cout << "fbSize: " << fbSize.x << ", " << fbSize.y << std::endl; // 317, 245

        // constexpr float padding = 2 * 15.24;
        // glColor3f(1, 1, 1);
        // glBegin(GL_TRIANGLES);
        // glVertex3f(padding, padding, 0);
        // glVertex3f(fbSize.x - padding, padding, 0);
        // glVertex3f(padding, fbSize.y - padding, 0);
        // glVertex3f(fbSize.x - padding, fbSize.y - padding, 0);
        // glVertex3f(fbSize.x - padding, padding, 0);
        // glVertex3f(padding, fbSize.y - padding, 0);
        // glEnd();

        if (module) {
            module->call_draw();
        } else {
            UMGB_VCV_LOG("module not set");
        }
    }
};

struct UmgebungModuleWidget : ModuleWidget {
    explicit UmgebungModuleWidget(UmgebungModule* module) {
        UMGB_VCV_LOG("Umgebung × VCV Rack");

        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/UmgebungModule.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        constexpr float M_GRID_SIZE  = 15.24;
        constexpr float M_GRID_ROW_1 = M_GRID_SIZE;
        constexpr float M_GRID_ROW_2 = 26.529;

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(M_GRID_ROW_1, M_GRID_SIZE * 2)), module, UmgebungModule::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(M_GRID_ROW_2, M_GRID_SIZE * 2)), module, UmgebungModule::RIGHT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(M_GRID_ROW_1, M_GRID_SIZE * 6)), module, UmgebungModule::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(M_GRID_ROW_2, M_GRID_SIZE * 6)), module, UmgebungModule::RIGHT_OUTPUT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(M_GRID_ROW_1, M_GRID_SIZE * 4)), module, UmgebungModule::PITCH_PARAM_1));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(M_GRID_ROW_2, M_GRID_SIZE * 4)), module, UmgebungModule::PITCH_PARAM_2));
        addParam(createParamCentered<CKD6>(mm2px(Vec(M_GRID_ROW_1, M_GRID_SIZE * 7.5)), module, UmgebungModule::RELOAD_PARAM));
        addParam(createParamCentered<CKD6>(mm2px(Vec(M_GRID_ROW_2, M_GRID_SIZE * 7.5)), module, UmgebungModule::LOAD_APP));

        if (module) {
            module->mTextFieldAppName            = createWidget<LedDisplayTextField>(mm2px(Vec(32.595, 107.466)));
            module->mTextFieldAppName->box.size  = mm2px(Vec(M_GRID_SIZE * 3.5f, M_GRID_SIZE * 0.66f));
            module->mTextFieldAppName->multiline = false;
            module->mTextFieldAppName->color     = color::BLACK;
            module->mTextFieldAppName->bgColor   = color::YELLOW;
            addChild(module->mTextFieldAppName);
            module->update_app_name();
        }

        auto* display   = new UmgebungWidget();
        display->module = module;

        std::cout << "RACK_GRID_WIDTH: " << RACK_GRID_WIDTH << std::endl;
        std::cout << "box.pos        : " << box.pos.x << ", " << box.pos.y << std::endl;
        std::cout << "box.size       : " << box.size.x << ", " << box.size.y << std::endl;

        display->box.pos  = Vec(M_GRID_SIZE, (RACK_GRID_HEIGHT / 8) - 10);
        display->box.size = Vec(box.size.x - 190, RACK_GRID_HEIGHT - 80);

        std::cout << "display->box.pos: " << display->box.pos.x << ", " << display->box.pos.y << std::endl;
        std::cout << "display->box.size: " << display->box.size.x << ", " << display->box.size.y << std::endl;

        /*
        box.pos: 0, 0
        box.size: 450, 380
        display->box.pos: 15.24, 37.5
        display->box.size: 260, 300
        mDisplaySize: 323.666, 220.789
        */

        Vec mDisplaySize     = mm2px(Vec(109.615, 74.774));
        Vec mDisplayPosition = mm2px(Vec(32.595, 21.684));
        std::cout << "mDisplaySize    : " << mDisplaySize.x << ", " << mDisplaySize.y << std::endl;
        std::cout << "mDisplayPosition: " << mDisplayPosition.x << ", " << mDisplayPosition.y << std::endl;

        display->setSize(mDisplaySize);
        display->setPosition(mDisplayPosition);
        // display->setSize(Vec(158 * 2, 122 * 2)); // fbSize 1580,1220
        // display->setPosition(Vec(M_GRID_SIZE * 6, M_GRID_SIZE * 3));
        addChild(display);
        std::cout << "framebuffer size: " << display->getFramebufferSize().x << ", " << display->getFramebufferSize().y << std::endl;
    }
};


Model* modelUmgebungModule = createModel<UmgebungModule, UmgebungModuleWidget>("UmgebungModule");

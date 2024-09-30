#include <dlfcn.h>
#include <iostream>
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

    float bg_color_red   = 0.0;
    float bg_color_green = 0.0;
    float bg_color_blue  = 0.0;

    static constexpr uint32_t SAMPLES_PER_AUDIO_BLOCK               = 512;
    float                     mLeftOutput[SAMPLES_PER_AUDIO_BLOCK]  = {0};
    float                     mRightOutput[SAMPLES_PER_AUDIO_BLOCK] = {0};
    float                     mLeftInput[SAMPLES_PER_AUDIO_BLOCK]   = {0};
    float                     mRightInput[SAMPLES_PER_AUDIO_BLOCK]  = {0};

    float    mBangReloadButtonState  = 0.0f;
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

    // typedef void       (*KLST_SetupCallback)(void);
    // KLST_SetupCallback mSetupCallback = nullptr;
    //
    // typedef void      (*KLST_LoopCallback)(void);
    // KLST_LoopCallback mLoopCallback = nullptr;
    //
    // typedef void      (*KLST_DrawCallback)(void);
    // KLST_DrawCallback mDrawCallback = nullptr;
    //
    // typedef void      (*KLST_BeatCallback)(uint32_t);
    // KLST_BeatCallback mBeatCallback = nullptr;
    //
    // typedef void            (*KLST_AudioblockCallback)(float**, float**, uint32_t);
    // KLST_AudioblockCallback mAudioblockCallback = nullptr;
    //
    // typedef const char* (*KLST_NameCallback)(void);
    // KLST_NameCallback   mNameCallback = nullptr;

#ifdef UMGEBUNG_ADAPTER
#endif

    enum ParamId {
        PITCH_PARAM,
        RELOAD_PARAM,
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
        configParam(PITCH_PARAM, 0.f, 1.f, 0.f, "");
        configInput(LEFT_INPUT, "");
        configInput(RIGHT_INPUT, "");
        configOutput(LEFT_OUTPUT, "");
        configOutput(RIGHT_OUTPUT, "");
    }

    ~UmgebungModule() {
        try {
            unload_app();
        } catch (Exception e) {
            UMGB_VCV_LOG("could not unload app");
        }
        if (mInstanceCounter > 0) {
            mInstanceCounter--;
        }
    }

    void process(const ProcessArgs& args) override {
        if (mInitializeApp) {
#ifdef UMGEBUNG_ADAPTER
            if (umgebung_app) {
                umgebung_app->setup();
            }
#endif
            mInitializeApp = false;
        }

        if (params[RELOAD_PARAM].getValue() > mBangReloadButtonState) {
            mBeatCounter = 0;
            reload_app();
        }
        mBangReloadButtonState = params[RELOAD_PARAM].getValue();

        bg_color_red   = inputs[LEFT_INPUT].getVoltage() / 5.0f;
        bg_color_green = inputs[RIGHT_INPUT].getVoltage() / 5.0f;
        bg_color_blue  = params[PITCH_PARAM].getValue();

        outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
        outputs[RIGHT_OUTPUT].setVoltage(inputs[RIGHT_INPUT].getVoltage());

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

        mBeatTriggerCounter += args.sampleTime;
        if (mBeatTriggerCounter >= mBeatDurationSec) {
            mBeatTriggerCounter -= mBeatDurationSec;
#ifdef UMGEBUNG_ADAPTER
            if (umgebung_app) {
                // umgebung_app->beat(mBeatCounter);
                mBeatCounter++;
            }
#endif
        }

        // Blink light at 1Hz
        static float blinkPhase = 0.0f;
        blinkPhase += args.sampleTime;
        if (blinkPhase >= 1.f) {
            blinkPhase -= 1.f;
        }
        lights[BLINK_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);

#ifdef UMGEBUNG_ADAPTER
        if (umgebung_app) {
            umgebung_app->loop();
        }
#endif
    }

    void call_draw() {
#ifdef UMGEBUNG_ADAPTER
        if (umgebung_app) {
            umgebung_app->draw();
        }
#endif
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
    typedef void         (*DestroyCallbackFunctionPtr)(UmgebungApp*);
    typedef const char*  (*NameFunctionPtr)(UmgebungApp*);

    CreateUmgebungFunctionPtr  fCreateUmgebungFunction  = nullptr;
    DestroyCallbackFunctionPtr fDestroyUmgebungFunction = nullptr;
    NameFunctionPtr            fNameFunction            = nullptr;

    void load_symbols() {
        load_symbol(mHandleUmgebungSketch, "create_umgebung", fCreateUmgebungFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "destroy_umgebung", fDestroyUmgebungFunction, mLibraryName);
        load_symbol(mHandleUmgebungSketch, "name", fNameFunction, mLibraryName);
    }

    //     typedef UmgebungAdapter*   (*CreateUmgebungFunctionPtr)();
    //     CreateUmgebungFunctionPtr  fCreateUmgebungFunction    = nullptr;
    //     const char*                fCreateUmgebungFunctionStr = "create_umgebung";
    //     typedef void               (*DestroyCallbackFunctionPtr)(UmgebungAdapter*);
    //     DestroyCallbackFunctionPtr fDestroyUmgebungFunction    = nullptr;
    //     const char*                fDestroyUmgebungFunctionStr = "destroy_umgebung";
    //     typedef const char*        (*NameFunctionPtr)(UmgebungAdapter*);
    //     NameFunctionPtr            fNameFunction    = nullptr;
    //     const char*                fNameFunctionStr = "name";
    //
    //     void load_symbols() {
    //         /* create_umgebung */
    // #if defined ARCH_WIN
    //         fCreateUmgebungCallback = (CreateUmgebungFunctionPtr) GetProcAddress(mHandleUmgebungSketch, fCreateUmgebungFunctionStr);
    // #else
    //         fCreateUmgebungFunction = (CreateUmgebungFunctionPtr) (dlsym(mHandleUmgebungSketch, fCreateUmgebungFunctionStr));
    // #endif
    //         if (!fCreateUmgebungFunction) {
    //             UMGB_VCV_LOG("failed to read '%s' symbol in %s", fCreateUmgebungFunctionStr, mLibraryName.c_str());
    //         } else {
    //             UMGB_VCV_LOG("successfully read '%s' symbol in %s", fCreateUmgebungFunctionStr, mLibraryName.c_str());
    //         }
    //         /* destroy_umgebung */
    // #if defined ARCH_WIN
    //         fDestroyUmgebungCallback = (DestroyCallbackFunctionPtr) GetProcAddress(mHandleUmgebungSketch, fDestroyUmgebungFunctionStr);
    // #else
    //         fDestroyUmgebungFunction = (DestroyCallbackFunctionPtr) (dlsym(mHandleUmgebungSketch, fDestroyUmgebungFunctionStr));
    // #endif
    //         if (!fDestroyUmgebungFunction) {
    //             UMGB_VCV_LOG("failed to read '%s' symbol in %s", fDestroyUmgebungFunctionStr, mLibraryName.c_str());
    //         } else {
    //             UMGB_VCV_LOG("successfully read '%s' symbol in %s", fDestroyUmgebungFunctionStr, mLibraryName.c_str());
    //         }
    //         /* name */
    // #if defined ARCH_WIN
    //         fNameFunction = (KLST_DestroyCallback) GetProcAddress(mHandleUmgebungSketch, fNameFunctionStr);
    // #else
    //         fNameFunction = (NameFunctionPtr) (dlsym(mHandleUmgebungSketch, fNameFunctionStr));
    // #endif
    //         if (!fNameFunction) {
    //             UMGB_VCV_LOG("failed to read '%s' symbol in %s", fNameFunctionStr, mLibraryName.c_str());
    //         } else {
    //             UMGB_VCV_LOG("successfully read '%s' symbol in %s", fNameFunctionStr, mLibraryName.c_str());
    //         }
    //     }

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
#ifdef UMGEBUNG_ADAPTER
#endif
    }

    void create_app() {
        if (fCreateUmgebungFunction) {
            umgebung_app = fCreateUmgebungFunction();
            UMGB_VCV_LOG("created app: %p", umgebung_app);
            fNameFunction = (NameFunctionPtr) dlsym(mHandleUmgebungSketch, "name");
            if (umgebung_app && fNameFunction) {
                UMGB_VCV_LOG("             %s", fNameFunction(umgebung_app));
            } else {
                UMGB_VCV_LOG("Failed to load name symbol: %s", dlerror());
            }

#ifdef UMGEBUNG_ADAPTER
            if (umgebung_app) {
                UMGB_VCV_LOG("created app: %s", umgebung_app->name());
                umgebung_app = fCreateUmgebungCallback();
                if (mTextFieldAppName) {
                    mTextFieldAppName->text = umgebung_app->name();
                }
            }
#endif
        } else {
            if (mTextFieldAppName) {
                mTextFieldAppName->text = "NOOP";
            }
        }
    }

    void destroy_app() {
#ifdef UMGEBUNG_ADAPTER
        if (fDestroyUmgebungCallback) {
            if (umgebung_app) {
                UMGB_VCV_LOG("destroying app: %s", umgebung_app->name());
                fDestroyUmgebungCallback(umgebung_app);
                mTextFieldAppName->text = "NOOP";
            }
        }
#endif
    }

    void reload_app() {
        handle_umgebung_app();
        mInitializeApp = true;
    }

#ifdef UMGEBUNG_ADAPTER
#endif
private:
    UmgebungApp* umgebung_app = nullptr;
};

struct UmgebungWidget : OpenGlWidget {

    UmgebungModule* module;

    void step() override {
        // Render every frame
        dirty = true;
        FramebufferWidget::step();
    }

    void drawFramebuffer() override {
        math::Vec fbSize = getFramebufferSize();
        glViewport(0.0, 0.0, fbSize.x, fbSize.y);
        glClearColor(module->bg_color_red, module->bg_color_green, module->bg_color_blue, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, fbSize.x, 0.0, fbSize.y, -1.0, 1.0);

        //         std::cout << "fbSize: " << fbSize.x << ", " << fbSize.y << std::endl; // 317, 245

        constexpr float padding = 2 * 15.24;
        glColor3f(1, 1, 1);
        glBegin(GL_TRIANGLES);
        glVertex3f(padding, padding, 0);
        glVertex3f(fbSize.x - padding, padding, 0);
        glVertex3f(padding, fbSize.y - padding, 0);
        glVertex3f(fbSize.x - padding, fbSize.y - padding, 0);
        glVertex3f(fbSize.x - padding, padding, 0);
        glVertex3f(padding, fbSize.y - padding, 0);
        glEnd();

        module->call_draw();
    }
};

struct UmgebungModuleWidget : ModuleWidget {
    UmgebungModuleWidget(UmgebungModule* module) {
        UMGB_VCV_LOG("Umgebung × VCV Rack");

        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/UmgebungModule.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));


        constexpr float M_GRID_SIZE = 15.24;

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 2)), module, UmgebungModule::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 3)), module, UmgebungModule::RIGHT_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 5)), module, UmgebungModule::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 6)), module, UmgebungModule::RIGHT_OUTPUT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 4)), module, UmgebungModule::PITCH_PARAM));
        addParam(createParamCentered<CKD6>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 7.5)), module, UmgebungModule::RELOAD_PARAM));

        UmgebungWidget* display = new UmgebungWidget();
        display->module         = module;

        std::cout << "box.pos: " << box.pos.x << ", " << box.pos.y << std::endl;
        std::cout << "box.size: " << box.size.x << ", " << box.size.y << std::endl;

        display->box.pos  = Vec(M_GRID_SIZE, (RACK_GRID_HEIGHT / 8) - 10);
        display->box.size = Vec(box.size.x - 190, RACK_GRID_HEIGHT - 80);

        std::cout << "display->box.pos: " << display->box.pos.x << ", " << display->box.pos.y << std::endl;
        std::cout << "display->box.size: " << display->box.size.x << ", " << display->box.size.y << std::endl;

        display->setSize(Vec(158 * 2, 122 * 2)); // fbSize 1580,1220
        display->setPosition(Vec(M_GRID_SIZE * 6, M_GRID_SIZE * 3));
        addChild(display);

        if (module) {
            module->mTextFieldAppName            = createWidget<LedDisplayTextField>(mm2px(Vec(M_GRID_SIZE * 2, M_GRID_SIZE * 7.15)));
            module->mTextFieldAppName->box.size  = mm2px(Vec(RACK_GRID_WIDTH * 3.5, RACK_GRID_WIDTH * 0.66));
            module->mTextFieldAppName->multiline = false;
            module->mTextFieldAppName->color     = color::BLACK;
            addChild(module->mTextFieldAppName);
        }
    }
};


Model* modelUmgebungModule = createModel<UmgebungModule, UmgebungModuleWidget>("UmgebungModule");

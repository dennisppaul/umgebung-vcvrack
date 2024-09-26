#include <dlfcn.h>
#include <iostream>
#include "plugin.hpp"

#define KLST_VCV_DEBUG
#ifdef KLST_VCV_DEBUG
#define KLST_VCV_LOG(...) \
    printf("+++ ");       \
    printf(__VA_ARGS__);  \
    printf("\n");
#else
#define KLST_VCV_LOG(...)
#endif

uint8_t mInstanceCounter = 0;

struct UmgebungModule : Module {

    float bg_color_red = 0.0;
    float bg_color_green = 0.0;
    float bg_color_blue = 0.0;

    static constexpr uint32_t SAMPLES_PER_AUDIO_BLOCK = 512;
    float mLeftOutput[SAMPLES_PER_AUDIO_BLOCK]  = {0};
    float mRightOutput[SAMPLES_PER_AUDIO_BLOCK] = {0};
    float mLeftInput[SAMPLES_PER_AUDIO_BLOCK]   = {0};
    float mRightInput[SAMPLES_PER_AUDIO_BLOCK]  = {0};

    float        mBangReloadButtonState  = 0.0f;
    float        mBeatTriggerCounter     = 0.0f;
    float        mBeatDurationSec        = 0.125f;
    uint16_t     mSampleCollectorCounter = 0;
    uint32_t     mBeatCounter            = 0;

    LedDisplayTextField* mTextFieldAppName   = 0;
    std::string          mLibraryName        = "dep/libUmgebungSketch";
    bool                 mInitializeApp      = true;
    bool                 mAllocateAudioblock = true;

#if defined ARCH_WIN
    HINSTANCE mHandleUmgebungSketch = 0;
#else
    void* mHandleUmgebungSketch = 0;
#endif

    typedef void (*KLST_SetupCallback)(void);
    KLST_SetupCallback mSetupCallback = 0;

    typedef void (*KLST_LoopCallback)(void);
    KLST_LoopCallback mLoopCallback = 0;

    typedef void (*KLST_BeatCallback)(uint32_t);
    KLST_BeatCallback mBeatCallback = 0;

    typedef void (*KLST_AudioblockCallback)(float**, float**, uint32_t);
    KLST_AudioblockCallback mAudioblockCallback = 0;

    typedef const char* (*KLST_NameCallback)(void);
    KLST_NameCallback mNameCallback = 0;
    
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

	UmgebungModule() {
	    try {
            load_app();
        } catch(Exception e) {
            KLST_VCV_LOG("could not load app");
        }
	    try {
            load_symbols();
        } catch(Exception e) {
            KLST_VCV_LOG("could not load symbols");
        }
        mInstanceCounter++;
        if (mInstanceCounter > 1) {
            KLST_VCV_LOG("mInstanceCounter: %i", mInstanceCounter);
            throw Exception(string::f("multiple instances of plugins are currently not supported."));
        }

		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PITCH_PARAM, 0.f, 1.f, 0.f, "");
		configInput(LEFT_INPUT, "");
		configInput(RIGHT_INPUT, "");
		configOutput(LEFT_OUTPUT, "");
		configOutput(RIGHT_OUTPUT, "");
	}
	
	~UmgebungModule() {
        unload_app();
        if (mInstanceCounter > 0) {
            mInstanceCounter--;
        }
	}

	void process(const ProcessArgs &args) override {
	    if (mInitializeApp) {
	        if (mNameCallback != nullptr) {
                mTextFieldAppName->text = mNameCallback();
            } else {
                mTextFieldAppName->text = "NOOP";
            }
		    if (mSetupCallback != nullptr) {
                mSetupCallback();
            }
            mInitializeApp = false;
        }
        
        if (params[RELOAD_PARAM].getValue() > mBangReloadButtonState) {
            mBeatCounter = 0;
            reload_app();
        }
        mBangReloadButtonState = params[RELOAD_PARAM].getValue();
	
        bg_color_red = inputs[LEFT_INPUT].getVoltage() / 5.0f;
        bg_color_green = inputs[RIGHT_INPUT].getVoltage() / 5.0f;
        bg_color_blue = params[PITCH_PARAM].getValue();
        
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
            if (mBeatCallback != nullptr) {
                mBeatCallback(mBeatCounter);
                mBeatCounter++;
            }
        }

		// Blink light at 1Hz
		static float blinkPhase = 0.0f;
		blinkPhase += args.sampleTime;
		if (blinkPhase >= 1.f)
			blinkPhase -= 1.f;
		lights[BLINK_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);
		
		if (mLoopCallback != nullptr) {
            mLoopCallback();
		}
	}
	
	void load_symbols() {
        load_symbol_setup();
        load_symbol_loop();
        load_symbol_beat();
        load_symbol_audioblock();
        load_symbol_name();
    }
    
    void load_symbol_setup() {
#if defined ARCH_WIN
        mSetupCallback = (KLST_SetupCallback)GetProcAddress(mHandleUmgebungSketch, "setup");
#else
        mSetupCallback = (KLST_SetupCallback)dlsym(mHandleUmgebungSketch, "setup");
#endif
        if (!mSetupCallback) {
            throw Exception(string::f("Failed to read `setup` symbol in %s", mLibraryName.c_str()));
        }
    }

    void load_symbol_loop() {
#if defined ARCH_WIN
        mLoopCallback = (KLST_LoopCallback)GetProcAddress(mHandleUmgebungSketch, "loop");
#else
        mLoopCallback                                = (KLST_LoopCallback)dlsym(mHandleUmgebungSketch, "loop");
#endif
        if (!mLoopCallback) {
            throw Exception(string::f("Failed to read `loop` symbol in %s", mLibraryName.c_str()));
        }
    }

    void load_symbol_beat() {
#if defined ARCH_WIN
        mBeatCallback = (KLST_BeatCallback)GetProcAddress(mHandleUmgebungSketch, "beat");
#else
        mBeatCallback                                = (KLST_BeatCallback)dlsym(mHandleUmgebungSketch, "beat");
#endif
        if (!mBeatCallback) {
            throw Exception(string::f("Failed to read `beat` symbol in %s", mLibraryName.c_str()));
        }
    }

    void load_symbol_audioblock() {
#if defined ARCH_WIN
        mAudioblockCallback = (KLST_AudioblockCallback)GetProcAddress(mHandleUmgebungSketch, "audioblock");
#else
        mAudioblockCallback                          = (KLST_AudioblockCallback)dlsym(mHandleUmgebungSketch, "audioblock");
#endif
        if (!mAudioblockCallback) {
            throw Exception(string::f("Failed to read `audioblock` symbol in %s", mLibraryName.c_str()));
        }
    }

    void load_symbol_name() {
#if defined ARCH_WIN
        mNameCallback = (KLST_NameCallback)GetProcAddress(mHandleUmgebungSketch, "name");
#else
        mNameCallback                                = (KLST_NameCallback)dlsym(mHandleUmgebungSketch, "name");
#endif
        if (!mNameCallback) {
            throw Exception(string::f("Failed to read `name` symbol in %s", mLibraryName.c_str()));
        }
    }
    
    void load_app() {
        /* Load plugin library */
        std::string mAppLibraryFilename;
#if defined ARCH_LIN
        mAppLibraryFilename = pluginInstance->path + "/" + mLibraryName + ".so";
#elif defined ARCH_WIN
        mAppLibraryFilename                          = pluginInstance->path + "/" + mLibraryName + ".dll";
#elif ARCH_MAC
        mAppLibraryFilename = pluginInstance->path + "/" + mLibraryName + ".dylib";
#endif
        KLST_VCV_LOG("loading app from file: %s", mAppLibraryFilename.c_str());

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
        void* handle                                 = dlopen(mAppLibraryFilename.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!handle) {
            throw Exception(string::f("Failed to load library %s: %s", mLibraryName.c_str(), dlerror()));
        }
#endif
        mHandleUmgebungSketch = handle;
    }

    void unload_app() {
        /* close the library */
        KLST_VCV_LOG("unloading app: %llu ", (uint64_t)(mHandleUmgebungSketch));
        if (mHandleUmgebungSketch) {
#if defined ARCH_WIN
            FreeLibrary((HINSTANCE)mHandleUmgebungSketch);
#else
            dlclose(mHandleUmgebungSketch);
#endif
            mHandleUmgebungSketch = 0;
        }
    }

    void reload_app() {
        unload_app();
        load_app();
        load_symbols();
        //         mTextFieldAppName->text = mNameCallback();
        mInitializeApp = true;
    }    
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
    }
};

struct UmgebungModuleWidget : ModuleWidget {
	UmgebungModuleWidget(UmgebungModule* module) {
    std::cout << "hello VCV Rack" << std::endl;
	
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
    addParam(createParamCentered<CKD6>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 7)), module, UmgebungModule::RELOAD_PARAM));

    UmgebungWidget *display = new UmgebungWidget();
    display->module = module;

    std::cout << "box.pos: " << box.pos.x << ", " << box.pos.y << std::endl;
    std::cout << "box.size: " << box.size.x << ", " << box.size.y << std::endl;

    display->box.pos = Vec(M_GRID_SIZE, (RACK_GRID_HEIGHT / 8) - 10);
    display->box.size = Vec(box.size.x - 190, RACK_GRID_HEIGHT - 80);

    std::cout << "display->box.pos: " << display->box.pos.x << ", " << display->box.pos.y << std::endl;
    std::cout << "display->box.size: " << display->box.size.x << ", " << display->box.size.y << std::endl;

    display->setSize(Vec(158 * 2, 122 * 2)); // fbSize 1580,1220
    display->setPosition(Vec( M_GRID_SIZE * 6 , M_GRID_SIZE * 3));
    addChild(display);
    
    if (module) {
        module->mTextFieldAppName            = createWidget<LedDisplayTextField>(mm2px(Vec(RACK_GRID_WIDTH * 0.25, RACK_GRID_WIDTH * 1.66)));
        module->mTextFieldAppName->box.size  = mm2px(Vec(RACK_GRID_WIDTH * 3.5, RACK_GRID_WIDTH * 0.66));
        module->mTextFieldAppName->multiline = false;
        module->mTextFieldAppName->color     = color::WHITE;
        addChild(module->mTextFieldAppName);
    }
  }
};


Model* modelUmgebungModule = createModel<UmgebungModule, UmgebungModuleWidget>("UmgebungModule");

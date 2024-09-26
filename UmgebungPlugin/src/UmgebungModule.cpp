#include <iostream>
#include "plugin.hpp"

struct UmgebungModule : Module {

    float bg_color_red = 0.0;
    float bg_color_green = 0.0;
    float bg_color_blue = 0.0;

	enum ParamId {
		PITCH_PARAM,
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
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PITCH_PARAM, 0.f, 1.f, 0.f, "");
		configInput(LEFT_INPUT, "");
		configInput(RIGHT_INPUT, "");
		configOutput(LEFT_OUTPUT, "");
		configOutput(RIGHT_OUTPUT, "");
	}

	void process(const ProcessArgs &args) override {
        bg_color_red = inputs[LEFT_INPUT].getVoltage() / 5.0f;
        bg_color_green = inputs[RIGHT_INPUT].getVoltage() / 5.0f;
        bg_color_blue = params[PITCH_PARAM].getValue();
        
        outputs[LEFT_OUTPUT].setVoltage(inputs[LEFT_INPUT].getVoltage());
        outputs[RIGHT_OUTPUT].setVoltage(inputs[RIGHT_INPUT].getVoltage());
        
//         std::cout << "inputs[LEFT_INPUT] : " << inputs[LEFT_INPUT].getVoltage() << std::endl;
//         std::cout << "inputs[RIGHT_INPUT]: " << inputs[RIGHT_INPUT].getVoltage() << std::endl;
//         std::cout << "params[PITCH_PARAM]: " << params[PITCH_PARAM].getValue() << std::endl;
//         std::cout << std::endl;

		// Blink light at 1Hz
		static float blinkPhase = 0.0f;
		blinkPhase += args.sampleTime;
		if (blinkPhase >= 1.f)
			blinkPhase -= 1.f;
		lights[BLINK_LIGHT].setBrightness(blinkPhase < 0.5f ? 1.f : 0.f);
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

    constexpr float M_GRID_SIZE = 15.24;

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 2)), module, UmgebungModule::LEFT_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 3)), module, UmgebungModule::RIGHT_INPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 5)), module, UmgebungModule::LEFT_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 6)), module, UmgebungModule::RIGHT_OUTPUT));
    addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(M_GRID_SIZE, M_GRID_SIZE * 4)), module, UmgebungModule::PITCH_PARAM));
    
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
  }
};


Model* modelUmgebungModule = createModel<UmgebungModule, UmgebungModuleWidget>("UmgebungModule");

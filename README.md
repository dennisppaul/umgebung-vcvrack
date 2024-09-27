# Umgebung in VCV Rack

following instructions from [Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial)

## setting up development environment

```zsh
$ brew install git wget cmake autoconf automake libtool jq python zstd pkg-config
$ git clone https://github.com/VCVRack/Rack.git
$ cd Rack
$ git submodule update --init --recursive
$ make dep
$ make
$ export RACK_DIR=~/Documents/dev/Rack
...
$ $RACK_DIR/helper.py createplugin MyPlugin
$ cd MyPlugin
$ make
( save `MyModule.svg.` to folder `res` )
$ $RACK_DIR/helper.py createmodule MyModule res/MyModule.svg src/MyModule.cpp
( modify source )
$ make
$ make dist
( copy plugin from `‌dist` to `~/Library/Application Support/Rack2/` )
```

## adobe illustrator export settings

- Export As or Artboards Export
- Styling: Inline Style
- Font: Convert to Outlines
- Images: Embed
- Object IDs: Layer Names
- Minify unchecked
- Responsive unchecked

## creating a widget

- `OpenGlWidget` might be the one we need ( see `$RACK_DIR/include/widget/OpenGlWidget.hpp` + `$RACK_DIR/src/widget/OpenGlWidget.cpp` ) 

```cpp
struct SomeWidget : ModuleWidget {
  SomeWidget(SomeModule *module){
    setModule(module);
    box.size = Vec(150, 380);
    
    {
        OpenGlWidget *w = new OpenGlWidget();

       // was trying these but no help
       // w->setSize(Vec(50, 50)); 
       // w->fbSize = Vec(50, 50);

       addChild(w);
    }
  }
}
```

(WIP)


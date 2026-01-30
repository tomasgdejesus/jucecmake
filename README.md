# Formulizer Controller

## Demonstration:

This short video demonstrates:

1. User interface panels
2. Sound effect when used with a synthesizer
3. Reprogramming and recompiling Lua code to reshape the sound

https://github.com/tomasgdejesus/jucecmake/blob/master/doc/demo1.mp4

## About

TODO: Make documentation

Run cmake to build

Find built executables in build/src/cpp/audioplugin_artefacts/...

- src/cpp = JUCE Plugin
- src/js = React Frontend for JUCE Plugin

How it works:
- JUCE Plugin contains LuaJIT environment and executes the most recently run script every audio processBlock() cycle
- React Frontend is packed and embedded in a WebView

React Frontend:
- Contains parameter controls
- Output log
- Signal Graph
- Monaco script editor
- File saving/loading

## Images:
![Formulizer Controller screenshot](doc/img4.png)

![Formulizer Controller screenshot](doc/img1.png)

![Formulizer Controller screenshot](doc/img2.png)

![Formulizer Controller screenshot](doc/img3.png)

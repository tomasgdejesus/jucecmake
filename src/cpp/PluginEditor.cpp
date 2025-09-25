#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <lua.hpp>
#include <cassert>

juce::var valueTreeToVar(const juce::ValueTree& tree)
{
    juce::DynamicObject::Ptr dict = new juce::DynamicObject();
    for (int i = 0; i < tree.getNumProperties(); i++)
        dict->setProperty(tree.getPropertyName(i), tree.getProperty(tree.getPropertyName(i)));
    return juce::var(dict.get());
}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    :   AudioProcessorEditor (&p),
        processorRef (p),
        lastOpenedFile (std::nullopt),
        webBrowser (juce::WebBrowserComponent::Options{}
                        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                        .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
                                                    .withUserDataFolder (juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory)))
                        .withNativeIntegrationEnabled()
                        /// TODO: Load saved state in JSON serialized format
                        /// so load from valuetree into xml then to json, send it to web browser
                        .withInitialisationData("savedState",
                            valueTreeToVar(p.valueTreeState.state.getChildWithName("GuiState")))
                        /// TODO: for dbg prob. Okay is works pretty well les go
                        .withNativeFunction("getSavedState",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                juce::ValueTree guiState = this->processorRef.valueTreeState.state.getChildWithName("GuiState");
                                juce::DynamicObject::Ptr dict = new juce::DynamicObject();
                                for (int i = 0; i < guiState.getNumProperties(); i++)
                                    dict->setProperty(guiState.getPropertyName(i), guiState.getProperty(guiState.getPropertyName(i)));
                                return completion(juce::var(dict.get()));
                            })
                        .withNativeFunction("setSavedState",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (args[0].isObject())
                                {
                                    auto* dict = args[0].getDynamicObject();
                                    juce::ValueTree guiState = this->processorRef.valueTreeState.state.getChildWithName("GuiState");
                                    for (const auto& prop : dict->getProperties())
                                        guiState.setProperty(prop.name, prop.value, nullptr);
                                }
                                return completion(juce::var());
                            })
                        .withNativeFunction("openFile",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                fileChooser = std::make_unique<juce::FileChooser>("Select file to open...",
                                    juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory),
                                    "*.lua");

                                auto fileChooserFlags =  juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

                                fileChooser->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser) {
                                    juce::File file = chooser.getResult();
                                    if (file.existsAsFile()) {
                                        this->webBrowser.emitEventIfBrowserIsVisible(
                                            "fileSelect",
                                            juce::var{juce::Array<juce::var>{file.getFileName(), file.loadFileAsString()}}
                                        );
                                        lastOpenedFile = file; // Store the last opened file
                                    }
                                });

                                return completion(juce::var());
                            })
                        .withNativeFunction("saveFile",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
                                /// TODO: Add error handling
                                const bool& useLastOpenedFile = args[0];
                                const juce::String& fileData = args[1];

                                if (lastOpenedFile && useLastOpenedFile) {
                                    lastOpenedFile->replaceWithText(fileData);
                                    this->webBrowser.emitEventIfBrowserIsVisible(
                                        "fileSelect",
                                        juce::var{juce::Array<juce::var>{lastOpenedFile->getFileName(), lastOpenedFile->loadFileAsString()}}
                                    );
                                }
                                else {
                                    fileChooser = std::make_unique<juce::FileChooser>("Save file as...",
                                        lastOpenedFile ? *lastOpenedFile :
                                        juce::File::getSpecialLocation(juce::File::SpecialLocationType::userHomeDirectory),
                                        "*.lua");

                                    auto fileChooserFlags = juce::FileBrowserComponent::saveMode
                                        | juce::FileBrowserComponent::canSelectFiles
                                        | juce::FileBrowserComponent::warnAboutOverwriting;

                                    fileChooser->launchAsync(fileChooserFlags, [this, fileData](const juce::FileChooser& chooser) {
                                        juce::File file = chooser.getResult();

                                        file.replaceWithText(fileData);

                                        if (file.existsAsFile()) {
                                            lastOpenedFile = file;

                                            this->webBrowser.emitEventIfBrowserIsVisible(
                                                "fileSelect",
                                                juce::var{juce::Array<juce::var>{file.getFileName(), file.loadFileAsString()}}
                                            );
                                        }
                                    });
                                }
                            }
                        )
                        .withNativeFunction("resetLastOpenedFile", // This function is called when presets are loaded
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
                                lastOpenedFile = std::nullopt;
                            }
                        )
                        .withNativeFunction("compile",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
                                if (args[0].isUndefined())
                                    return;

                                /// TODO: see if theres a better alternative to doing this?
                                const juce::String& script = args[0];
                                std::thread([this, script] {
                                    std::scoped_lock lock(this->processorRef.luaEnvCompileMutex);
                                    this->processorRef.luaEnvCompileString = script.toStdString();
                                }).detach();
                            }
                        )
                        .withNativeFunction("requestOutputLog",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
                                juce::Array<juce::var> send;

                                send.add(this->processorRef.lastCompileTime.load());
                                send.add(this->processorRef.lastProcessBlockTime.load());

                                std::vector<OutputLogMessage> messages;

                                if (!this->processorRef.luaOutputLog.clearFlag.load()) {
                                    messages = this->processorRef.luaOutputLog.read();
                                    for (const auto& message : messages) {
                                        juce::Array<juce::var> msgArray = {
                                            juce::String{message.str},
                                            juce::String{static_cast<int>(message.type)}
                                        };
                                        send.add(juce::var(msgArray));
                                    }
                                }
                                
                                this->webBrowser.emitEventIfBrowserIsVisible("outputLogUpdate", send);
                            })
                        .withNativeFunction("requestOutputMonitor",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
                                juce::Array<juce::var> send;

                                const auto& outputMonitorData = this->processorRef.outputMonitor.read();
                                for (const auto& out : outputMonitorData)
                                    send.add(out);
                                
                                this->webBrowser.emitEventIfBrowserIsVisible("outputMonitorUpdate", send);
                            })
                        .withNativeFunction("clearOutputLog",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion) {
                                this->processorRef.luaOutputLog.clearFlag.store(true);
                            })
                        .withResourceProvider ( /// TODO: Write ResourceProvider somewhere else
                            [](const juce::String& resourceName) -> std::optional<juce::WebBrowserComponent::Resource>
                            {
                                juce::String path = juce::String(SOURCE_DIR) + "/src/js/dist" + resourceName;
                                juce::File file(path);

                                if (file.existsAsFile())
                                {
                                    juce::String html = file.loadFileAsString();
                                    size_t len = html.getNumBytesAsUTF8();          // get length in bytes (no null terminator)
                                    const char* data = html.toRawUTF8();            // get UTF8 pointer

                                    // Create vector of bytes from UTF8 data
                                    std::vector<std::byte> bytes(reinterpret_cast<const std::byte*>(data),
                                                                reinterpret_cast<const std::byte*>(data) + len);

                                    juce::String mimeType = "text/plain";
                                    if (file.getFileExtension() == ".html")
                                        mimeType = "text/html";
                                    else if (file.getFileExtension() == ".js")
                                        mimeType = "application/javascript";
                                    else if (file.getFileExtension() == ".css")
                                        mimeType = "text/css";

                                    return juce::WebBrowserComponent::Resource{ std::move(bytes), mimeType };
                                }

                                return std::nullopt;
                            }))
{
    juce::ignoreUnused (processorRef);

/// TODO: Remove this
    lua_State* L = luaL_newstate(); // Test lua state working
    luaL_openlibs (L);
    assert(luaL_dostring(L, "return 'index.html'") == LUA_OK);
    assert(lua_isstring(L, -1));
    const char* result = lua_tostring(L, -1);
    juce::String resultStr(result);
    lua_close(L);

    addAndMakeVisible (webBrowser);
#ifndef VITE_DEV
    webBrowser.goToURL (juce::WebBrowserComponent::getResourceProviderRoot() + resultStr);
#else
    webBrowser.goToURL ("http://localhost:5173/");
#endif
    DBG(SOURCE_DIR);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (800, 600);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
/*void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}*/

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    webBrowser.setBounds (getLocalBounds());
}

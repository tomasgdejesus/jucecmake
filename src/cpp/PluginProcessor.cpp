#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        valueTreeState(*this, nullptr, juce::Identifier("RootValueTree"), {
            std::make_unique<juce::AudioParameterFloat>("output",
                "Output",
                -1.0f,
                1.0f,
                0.0f
            ),
            std::make_unique<juce::AudioParameterFloat>("output2",
                "Output2",
                -1.0f,
                1.0f,
                0.0f
            ),
        })
{
    paramOutput = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("output"));
    juce::ValueTree guiState("GuiState");
    guiState.setProperty("theme", "light", nullptr);
    guiState.setProperty("tab", "editor", nullptr);
    valueTreeState.state.addChild(guiState, 0, nullptr);

    /// TODO: removing
    juce::DynamicObject::Ptr dict = new juce::DynamicObject();
    for (int i = 0; i < guiState.getNumProperties(); i++) {
        dict->setProperty(guiState.getPropertyName(i), guiState.getProperty(guiState.getPropertyName(i)));
    }

    juce::String teststr = valueTreeState.state.toXmlString();
    juce::String teststr2 = juce::var(dict.get()).toString();

    luaEnv.print_callback = [this](std::string s) {
        luaOutputLog.add({s, OutputLogMessageType::Text});
    };

    /// TODO: Replace ts
    for (int i = 0; i < 400; i++)
        outputMonitor.add(0);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    /*
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());*/

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    {
        std::scoped_lock lock(luaEnvCompileMutex);
        if (luaEnvCompileString) {
            auto startTime = juce::Time::getHighResolutionTicks();
            auto err = luaEnv.compile(luaEnvCompileString->c_str());
            if (err)
                luaOutputLog.add({*err, OutputLogMessageType::Error});
            luaEnvCompileString = std::nullopt;
            auto endTime = juce::Time::getHighResolutionTicks();
            lastCompileTime.store((endTime - startTime)/double(juce::Time::getHighResolutionTicksPerSecond()));
        }
    }

    if (!luaEnv.hasInstance())
        return;

    auto startTime = juce::Time::getHighResolutionTicks();

    /// TODO: do midi output? (probably not, keep parameter output)
    /// TODO: optimization
    for (int channel = 0; channel < totalNumInputChannels; channel++) {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);
        // ..do something to the data...
        for (int i = 0; i < buffer.getNumSamples(); i++) {

            outputMonitorSampleCounter++;

            auto result = luaEnv.runInstance();
            if (result.error) {
                luaOutputLog.add({*result.error, OutputLogMessageType::Error});
                paramOutput->setValueNotifyingHost(0.0f);
                if (outputMonitorSampleCounter >= 1200) {
                    outputMonitor.add(0.0f);
                    outputMonitorSampleCounter = 0;
                }
                continue;
            }

            auto clamped = juce::jlimit(-1.0f, 1.0f, static_cast<float>(result.result));
            paramOutput->setValueNotifyingHost(clamped);
            if (outputMonitorSampleCounter >= 1200) {
                outputMonitor.add(clamped);
                outputMonitorSampleCounter = 0;
            }
        }
    }

    auto endTime = juce::Time::getHighResolutionTicks();
    lastProcessBlockTime.store((endTime - startTime)/double(juce::Time::getHighResolutionTicksPerSecond()));
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
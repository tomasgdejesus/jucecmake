#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "LuaEnv.h"
#include "CircularBuffer.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    LuaEnv luaEnv;

    constexpr static size_t OUTPUT_MONITOR_BUFFER_SIZE = 400;
    CircularBuffer<float, OUTPUT_MONITOR_BUFFER_SIZE> outputMonitor;
    size_t outputMonitorSampleCounter = 0;

    CircularBuffer<OutputLogMessage, LUAENV_OUTPUTLOG_MAX_MESSAGES> luaOutputLog;

    std::atomic<double> lastCompileTime{0};
    std::atomic<double> lastProcessBlockTime{0};

    std::mutex luaEnvCompileMutex;
    std::optional<std::string> luaEnvCompileString;

    /// TODO: Rename to paramOutputValue, or luaParamOutputValue, etc. idk yet
    juce::AudioParameterFloat* paramOutput;
 
    juce::AudioProcessorValueTreeState valueTreeState;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};

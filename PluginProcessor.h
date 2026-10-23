#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class VoidRageProcessor : public juce::AudioProcessor
{
public:
    VoidRageProcessor();
    ~VoidRageProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VOID RAGE COMP"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 3; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Meter / Visuals (vom Editor gelesen)
    std::atomic<float> inLevel { 0.f }, outLevel { 0.f }, grMeter { 0.f };
    static constexpr int scopeSize = 256;
    std::array<std::atomic<float>, scopeSize> scope;
    std::atomic<int> scopePos { 0 };

    static const char* presetNames[3];

private:
    using Filt = juce::dsp::IIR::Filter<float>;
    Filt hpf[2], scHp[2], dsBand[2], lowS[2], midP[2], airS[2];
    double sr = 44100.0;
    float lHpf = -1, lSc = -1, lDsF = -1, lBass = -99, lMid = -99, lMidF = -1, lAir = -99;

    float rmsState = 0.f, grState = 0.f, gateEnv = 1.f, dsEnv = 0.f, scopeAcc = 0.f;
    int scopeCnt = 0;
    float held[2] = { 0.f, 0.f };
    int holdCnt[2] = { 0, 0 };
    int currentPreset = 0;

    void updateFilters (float hpfF, float scF, float dsF, float bass, float mid, float midF, float air);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidRageProcessor)
};

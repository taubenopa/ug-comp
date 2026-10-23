#pragma once
#include "PluginProcessor.h"

//==============================================================================
// Neon Look: leuchtende Knobs + glühende Buttons
//==============================================================================
class RageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
};

//==============================================================================
class VoidRageEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VoidRageEditor (VoidRageProcessor&);
    ~VoidRageEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPresetButtons();

    struct Knob : public juce::Component
    {
        Knob (juce::AudioProcessorValueTreeState&, const juce::String& id,
              const juce::String& name, juce::Colour c);
        void resized() override;

        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct Section
    {
        juce::String name;
        juce::Colour col;
        int row;
        int cols;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds;
    };

    struct Particle { float x, y, vx, vy, size; juce::Colour col; };

    VoidRageProcessor& proc;
    RageLookAndFeel lnf;

    juce::OwnedArray<Knob> knobs;
    std::vector<Section> sections;

    juce::TextButton presetBtn[3];
    juce::TextButton killBtn { "KILL" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> killAtt;

    std::vector<Particle> particles;
    juce::Random rng;
    juce::Rectangle<int> vizBounds;

    float phase = 0.f, glitch = 0.f;
    float sIn = 0.f, sOut = 0.f, sGr = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidRageEditor)
};

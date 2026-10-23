#include "PluginEditor.h"

using namespace juce;

static constexpr int kW = 1180, kH = 844, kHeader = 54, kViz = 130, kM = 10;

//==============================================================================
// LOOK AND FEEL
//==============================================================================
void RageLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, Slider& s)
{
    const auto col = s.findColour (Slider::rotarySliderFillColourId);
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    const float ar = r - 5.f;

    PathStrokeType thin (5.f, PathStrokeType::curved, PathStrokeType::rounded);

    Path track;
    track.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, a1, true);
    g.setColour (col.withAlpha (0.14f));
    g.strokePath (track, thin);

    Path val;
    val.addCentredArc (c.x, c.y, ar, ar, 0.f, a0, ang, true);
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (col.withAlpha (0.07f * (float) (5 - i)));
        g.strokePath (val, PathStrokeType (5.f + (float) i * 3.f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (col);
    g.strokePath (val, thin);

    // Koerper
    const float br = ar - 9.f;
    g.setGradientFill (ColourGradient (Colour (0xff2f2f44), c.x - br * 0.4f, c.y - br * 0.5f,
                                       Colour (0xff07070d), c.x + br * 0.6f, c.y + br * 0.7f, true));
    g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (col.withAlpha (0.55f));
    g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.2f);

    // Zeiger
    const float sa = std::sin (ang), ca = std::cos (ang);
    g.setColour (col.withAlpha (0.35f));
    g.drawLine (c.x + br * 0.3f * sa, c.y - br * 0.3f * ca, c.x + br * 0.95f * sa, c.y - br * 0.95f * ca, 6.f);
    g.setColour (Colours::white);
    g.drawLine (c.x + br * 0.35f * sa, c.y - br * 0.35f * ca, c.x + br * 0.92f * sa, c.y - br * 0.92f * ca, 2.5f);
}

void RageLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hl, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.f);
    const bool on = b.getToggleState();
    const auto c = b.findColour (on ? TextButton::buttonOnColourId : TextButton::buttonColourId);

    if (on)
    {
        for (int i = 3; i > 0; --i)
        {
            g.setColour (c.withAlpha (0.12f));
            g.drawRoundedRectangle (r.expanded ((float) i * 1.5f), 6.f + (float) i, (float) i * 2.f);
        }
        g.setColour (c.withAlpha (0.38f));
    }
    else
        g.setColour (Colour (0xff0d0d18));

    g.fillRoundedRectangle (r, 5.f);
    g.setColour (c.withAlpha (hl ? 1.f : 0.8f));
    g.drawRoundedRectangle (r, 5.f, down ? 3.f : 1.6f);
}

//==============================================================================
// KNOB
//==============================================================================
VoidRageEditor::Knob::Knob (AudioProcessorValueTreeState& s, const String& id,
                            const String& name, Colour c)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 90, 16);
    slider.setColour (Slider::rotarySliderFillColourId, c);
    slider.setColour (Slider::textBoxTextColourId, Colours::white.withAlpha (0.88f));
    slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider.setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);

    label.setText (name.toUpperCase(), dontSendNotification);
    label.setJustificationType (Justification::centred);
    label.setFont (Font (11.5f, Font::bold));
    label.setColour (Label::textColourId, c);
    label.setInterceptsMouseClicks (false, false);

    addAndMakeVisible (slider);
    addAndMakeVisible (label);
    att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    setBufferedToImage (true); // Cache, damit 27 Knobs den 30fps-Hintergrund nicht ausbremsen
}

void VoidRageEditor::Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

//==============================================================================
// EDITOR
//==============================================================================
VoidRageEditor::VoidRageEditor (VoidRageProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    const Colour cyan (0xff00f5ff), pink (0xffff2a6d), acid (0xffb6ff00),
                 violet (0xffa855ff), orange (0xffff9500), red (0xffff2244);

    auto k = [this, &p] (const char* id, const char* name, Colour c)
    {
        auto* kn = new Knob (p.apvts, id, name, c);
        knobs.add (kn);
        addAndMakeVisible (kn);
        return kn;
    };

    sections.push_back ({ "INPUT / GATE", cyan, 0, 2,
        { k ("inGain", "In Gain", cyan), k ("hpf", "Low Cut", cyan), k ("gate", "Gate", cyan) }, {} });

    sections.push_back ({ "THE CRUSHER // COMPRESSOR", pink, 0, 5,
        { k ("thresh", "Threshold", pink), k ("ratio", "Ratio", pink), k ("attack", "Attack", pink),
          k ("release", "Release", pink), k ("knee", "Knee", pink), k ("makeup", "Makeup", pink),
          k ("rms", "Peak / RMS", pink), k ("scHpf", "SC Filter", pink), k ("mix", "Parallel", pink) }, {} });

    sections.push_back ({ "S-KILLER // DE-ESS", acid, 1, 3,
        { k ("dsFreq", "S-Freq", acid), k ("dsThr", "S-Thresh", acid), k ("dsAmt", "S-Kill", acid) }, {} });

    sections.push_back ({ "HEAT // BITCRUSH", red, 1, 5,
        { k ("drive", "Drive", red), k ("satMix", "Heat Mix", red), k ("bits", "Bit Crush", red),
          k ("rate", "Downsample", red), k ("crushMix", "Crush Mix", red) }, {} });

    sections.push_back ({ "TONE // AIR", violet, 2, 4,
        { k ("bass", "Bass", violet), k ("mid", "Mid", violet), k ("midFreq", "Mid Freq", violet),
          k ("air", "Air", violet) }, {} });

    sections.push_back ({ "OUTPUT // WIDTH", orange, 2, 3,
        { k ("width", "Width", orange), k ("ceiling", "Ceiling", orange), k ("outGain", "Output", orange) }, {} });

    // Presets
    const Colour pc[3] = { cyan, red, violet };
    for (int i = 0; i < 3; ++i)
    {
        presetBtn[i].setButtonText (VoidRageProcessor::presetNames[i]);
        presetBtn[i].setColour (TextButton::buttonColourId, pc[i]);
        presetBtn[i].setColour (TextButton::buttonOnColourId, pc[i]);
        presetBtn[i].setColour (TextButton::textColourOffId, Colours::white);
        presetBtn[i].setColour (TextButton::textColourOnId, Colours::white);
        presetBtn[i].onClick = [this, i] { proc.setCurrentProgram (i); refreshPresetButtons(); };
        addAndMakeVisible (presetBtn[i]);
    }

    killBtn.setClickingTogglesState (true);
    killBtn.setColour (TextButton::buttonColourId, Colour (0xff888899));
    killBtn.setColour (TextButton::buttonOnColourId, Colour (0xffff2244));
    killBtn.setColour (TextButton::textColourOffId, Colours::white);
    killBtn.setColour (TextButton::textColourOnId, Colours::white);
    addAndMakeVisible (killBtn);
    killAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "bypass", killBtn);

    // Partikel
    const Colour pcols[4] = { cyan, pink, acid, violet };
    for (int i = 0; i < 70; ++i)
        particles.push_back ({ rng.nextFloat() * kW, rng.nextFloat() * kH,
                               (rng.nextFloat() - 0.5f) * 0.6f, -0.2f - rng.nextFloat() * 0.9f,
                               1.5f + rng.nextFloat() * 3.f, pcols[rng.nextInt (4)] });

    refreshPresetButtons();
    setSize (kW, kH);
    startTimerHz (30);
}

VoidRageEditor::~VoidRageEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoidRageEditor::refreshPresetButtons()
{
    for (int i = 0; i < 3; ++i)
        presetBtn[i].setToggleState (proc.getCurrentProgram() == i, dontSendNotification);
}

void VoidRageEditor::timerCallback()
{
    phase += 1.f / 30.f;

    auto dbn = [] (float lin)
    {
        return jmap (jlimit (-60.f, 0.f, Decibels::gainToDecibels (lin, -60.f)), -60.f, 0.f, 0.f, 1.f);
    };
    sIn  = jmax (dbn (proc.inLevel.load()),  sIn  * 0.9f);
    sOut = jmax (dbn (proc.outLevel.load()), sOut * 0.9f);
    sGr  = jmax (jlimit (0.f, 1.f, proc.grMeter.load() / 24.f), sGr * 0.9f);

    const float speed = 1.f + sOut * 8.f;
    for (auto& p : particles)
    {
        p.x += p.vx * speed;
        p.y += p.vy * speed;
        if (p.y < -10.f) { p.y = (float) kH + 5.f; p.x = rng.nextFloat() * kW; }
        if (p.x < -10.f) p.x = (float) kW + 5.f;
        if (p.x > kW + 10.f) p.x = -5.f;
    }

    if (rng.nextFloat() < 0.012f || (sOut > 0.88f && rng.nextFloat() < 0.25f)) glitch = 1.f;
    glitch *= 0.8f;

    repaint();
}

//==============================================================================
void VoidRageEditor::resized()
{
    const int w = getWidth();

    int bx = w - kM - 70;
    killBtn.setBounds (bx, 12, 70, 30);
    for (int i = 2; i >= 0; --i)
    {
        bx -= 8 + 128;
        presetBtn[i].setBounds (bx, 12, 128, 30);
    }

    vizBounds = { kM, kHeader, w - 2 * kM, kViz };

    const int rowH[3] = { 300, 160, 160 };
    int y = kHeader + kViz + kM;
    for (int row = 0; row < 3; ++row)
    {
        int totalCols = 0, count = 0;
        for (auto& s : sections) if (s.row == row) { totalCols += s.cols; ++count; }

        const int availW = w - 2 * kM - (count - 1) * kM;
        int x = kM;
        for (auto& s : sections)
        {
            if (s.row != row) continue;
            const int sw = availW * s.cols / totalCols;
            s.bounds = { x, y, sw, rowH[row] };

            auto inner = s.bounds.reduced (6).withTrimmedTop (22);
            const int nk = (int) s.knobs.size();
            const int rows = (nk + s.cols - 1) / s.cols;
            const int cw = inner.getWidth() / s.cols, ch = inner.getHeight() / rows;
            for (int i = 0; i < nk; ++i)
                s.knobs[(size_t) i]->setBounds (inner.getX() + (i % s.cols) * cw,
                                                inner.getY() + (i / s.cols) * ch, cw, ch);
            x += sw + kM;
        }
        y += rowH[row] + kM;
    }
}

//==============================================================================
void VoidRageEditor::paint (Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    // Hintergrund
    g.setGradientFill (ColourGradient (Colour (0xff060410), 0, 0, Colour (0xff1a0422), 0, h, false));
    g.fillAll();

    // Beat-Puls aus der Mitte
    g.setGradientFill (ColourGradient (Colour (0xffff2a6d).withAlpha (0.10f + sOut * 0.35f), w * 0.5f, h * 0.5f,
                                       Colours::transparentBlack, w * 0.5f + w * 0.65f, h * 0.5f, true));
    g.fillAll();

    // Synthwave-Grid
    const float horizon = h * 0.30f;
    g.setColour (Colour (0xff7a2cff).withAlpha (0.22f + sOut * 0.3f));
    for (int kk = 0; kk < 18; ++kk)
    {
        const float t = ((float) kk + std::fmod (phase * 0.8f * (1.f + sOut * 3.f), 1.f)) / 18.f;
        g.drawHorizontalLine ((int) (horizon + (h - horizon) * t * t), 0.f, w);
    }
    for (int kk = -24; kk <= 24; ++kk)
        g.drawLine (w * 0.5f + (float) kk * 6.f, horizon, w * 0.5f + (float) kk * w * 0.10f, h, 1.f);

    // Partikel
    for (auto& p : particles)
    {
        g.setColour (p.col.withAlpha (0.18f));
        g.fillRect (p.x - 2.f, p.y - 2.f, p.size + 4.f, p.size + 4.f);
        g.setColour (p.col.withAlpha (0.8f));
        g.fillRect (p.x, p.y, p.size, p.size);
    }

    // Titel (mit RGB-Split Glitch)
    auto title = [&] (Colour c, float ox, float oy)
    {
        g.setColour (c);
        g.setFont (Font (32.f, Font::bold | Font::italic));
        g.drawText ("VOID//RAGE", Rectangle<float> (16.f + ox, 2.f + oy, 340.f, 36.f), Justification::centredLeft, false);
    };
    const float gl = glitch * 9.f;
    if (glitch > 0.05f)
    {
        title (Colour (0xffff2a6d).withAlpha (0.85f), -gl * rng.nextFloat(), gl * 0.4f * (rng.nextFloat() - 0.5f));
        title (Colour (0xff00f5ff).withAlpha (0.85f),  gl * rng.nextFloat(), gl * 0.4f * (rng.nextFloat() - 0.5f));
    }
    title (Colours::white, 0.f, 0.f);
    g.setColour (Colour (0xff00f5ff).withAlpha (0.8f));
    g.setFont (Font (11.f, Font::bold));
    g.drawText ("UNDERGROUND VOCAL COMPRESSOR  //  CLEAN . RAGE . NEWAGE",
                Rectangle<float> (18.f, 36.f, 520.f, 16.f), Justification::centredLeft, false);

    // ---------------- Visualizer ----------------
    auto vr = vizBounds.toFloat();
    g.setColour (Colour (0xff08080f).withAlpha (0.85f));
    g.fillRoundedRectangle (vr, 8.f);
    g.setColour (Colour (0xff00f5ff).withAlpha (0.45f + 0.4f * sOut));
    g.drawRoundedRectangle (vr, 8.f, 1.5f);

    auto scopeR = vr.reduced (10.f);
    auto metersR = scopeR.removeFromRight (scopeR.getWidth() * 0.34f);
    scopeR.removeFromRight (12.f);

    // Scope
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawHorizontalLine ((int) scopeR.getCentreY(), scopeR.getX(), scopeR.getRight());
    Path wave;
    const int N = VoidRageProcessor::scopeSize, pos = proc.scopePos.load();
    for (int i = 0; i < N; ++i)
    {
        const float v = proc.scope[(size_t) ((pos + i) % N)].load();
        const float px = scopeR.getX() + scopeR.getWidth() * (float) i / (float) (N - 1);
        const float py = scopeR.getCentreY() - jlimit (-1.f, 1.f, v * 1.6f) * scopeR.getHeight() * 0.48f;
        if (i == 0) wave.startNewSubPath (px, py); else wave.lineTo (px, py);
    }
    const auto wc = Colour::fromHSV (std::fmod (phase * 0.06f, 1.f), 0.85f, 1.f, 1.f);
    for (int kk = 4; kk >= 1; --kk)
    {
        g.setColour (wc.withAlpha (0.06f * (float) (5 - kk)));
        g.strokePath (wave, PathStrokeType ((float) kk * 3.f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (Colours::white);
    g.strokePath (wave, PathStrokeType (1.6f));

    // Meter
    auto bar = [&] (Rectangle<float> r, float v, Colour a, Colour b, bool rtl, const char* txt)
    {
        g.setColour (a);
        g.setFont (Font (11.f, Font::bold));
        g.drawText (txt, r.removeFromLeft (30.f), Justification::centredLeft, false);
        g.setColour (Colour (0xff14141f));
        g.fillRoundedRectangle (r, 3.f);
        auto f = r.withWidth (r.getWidth() * jlimit (0.f, 1.f, v));
        if (rtl) f = f.withX (r.getRight() - f.getWidth());
        g.setGradientFill (ColourGradient (a, r.getX(), 0.f, b, r.getRight(), 0.f, false));
        g.fillRoundedRectangle (f, 3.f);
    };
    const float rh = metersR.getHeight() / 3.f;
    bar (metersR.removeFromTop (rh).reduced (0.f, 5.f), sIn,  Colour (0xff00f5ff), Colour (0xffb6ff00), false, "IN");
    bar (metersR.removeFromTop (rh).reduced (0.f, 5.f), sOut, Colour (0xffb6ff00), Colour (0xffff2244), false, "OUT");
    bar (metersR.removeFromTop (rh).reduced (0.f, 5.f), sGr,  Colour (0xffff2a6d), Colour (0xffff9500), true,  "GR");

    // ---------------- Sektionen ----------------
    int idx = 0;
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        g.setColour (Colour (0xff08080f).withAlpha (0.82f));
        g.fillRoundedRectangle (r, 8.f);

        const float a = jlimit (0.f, 1.f, 0.35f + 0.22f * std::sin (phase * 2.f + (float) idx * 1.3f) + sOut * 0.4f);
        for (int i = 3; i >= 1; --i)
        {
            g.setColour (s.col.withAlpha (a * 0.12f));
            g.drawRoundedRectangle (r.expanded ((float) i), 8.f + (float) i, 2.f);
        }
        g.setColour (s.col.withAlpha (a));
        g.drawRoundedRectangle (r, 8.f, 1.5f);

        g.setColour (s.col);
        g.setFont (Font (13.f, Font::bold));
        g.drawText (s.name, Rectangle<float> (r.getX() + 14.f, r.getY() + 4.f, r.getWidth() - 28.f, 18.f),
                    Justification::centredLeft, false);
        ++idx;
    }
}

void VoidRageEditor::paintOverChildren (Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    // Scanlines
    g.setColour (Colours::black.withAlpha (0.10f));
    for (int y = 0; y < getHeight(); y += 3) g.drawHorizontalLine (y, 0.f, w);

    // wandernder Scan-Balken
    const float by = std::fmod (phase * 140.f, h + 80.f) - 40.f;
    g.setGradientFill (ColourGradient (Colours::transparentBlack, 0.f, by - 30.f,
                                       Colour (0xff00f5ff).withAlpha (0.07f), 0.f, by, false));
    g.fillRect (0.f, by - 30.f, w, 30.f);

    // Glitch-Streifen
    if (glitch > 0.3f)
    {
        g.setColour (Colour (0xffff2a6d).withAlpha (0.10f * glitch));
        g.fillRect (0.f, rng.nextFloat() * h, w, 2.f + rng.nextFloat() * 6.f);
        g.setColour (Colour (0xff00f5ff).withAlpha (0.10f * glitch));
        g.fillRect (0.f, rng.nextFloat() * h, w, 1.f + rng.nextFloat() * 4.f);
    }

    // Vignette
    g.setGradientFill (ColourGradient (Colours::transparentBlack, w * 0.5f, h * 0.5f,
                                       Colours::black.withAlpha (0.5f), 0.f, 0.f, true));
    g.fillAll();
}

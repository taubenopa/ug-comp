#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

const char* VoidRageProcessor::presetNames[3] = { "CLEAN GHOST", "RAGE DEMON", "NEWAGE SPACE" };

//==============================================================================
// PRESETS (alles was nicht genannt ist, bleibt auf Default)
//==============================================================================
namespace
{
    struct PV { const char* id; float v; };

    const std::vector<PV>& presetData (int i)
    {
        static const std::vector<PV> p[3] = {
            // 0: CLEAN GHOST - glasklar, transparent, airy
            { { "hpf", 90 }, { "gate", -62 }, { "thresh", -20 }, { "ratio", 3.5f }, { "attack", 12 },
              { "release", 90 }, { "knee", 10 }, { "makeup", 5 }, { "rms", 40 }, { "scHpf", 140 },
              { "dsFreq", 7500 }, { "dsThr", -26 }, { "dsAmt", 7 }, { "drive", 2 }, { "satMix", 20 },
              { "bass", 1.5f }, { "mid", 1.5f }, { "midFreq", 2800 }, { "air", 4 },
              { "width", 110 }, { "ceiling", -0.5f } },
            // 1: RAGE DEMON - zerquetscht, fett, dreckig, in your face
            { { "inGain", 4 }, { "hpf", 110 }, { "gate", -55 }, { "thresh", -30 }, { "ratio", 12 },
              { "attack", 0.8f }, { "release", 60 }, { "knee", 2 }, { "makeup", 10 }, { "rms", 10 },
              { "scHpf", 180 }, { "dsFreq", 6500 }, { "dsThr", -22 }, { "dsAmt", 9 },
              { "drive", 14 }, { "satMix", 70 }, { "bits", 12 }, { "rate", 3 }, { "crushMix", 15 },
              { "bass", 3 }, { "mid", 3 }, { "midFreq", 1800 }, { "air", 3 },
              { "width", 130 }, { "ceiling", -1 }, { "outGain", -2 } },
            // 2: NEWAGE SPACE - breit, schimmernd, lofi-glitzer
            { { "hpf", 120 }, { "gate", -70 }, { "thresh", -24 }, { "ratio", 6 }, { "attack", 3 },
              { "release", 250 }, { "knee", 12 }, { "makeup", 6 }, { "rms", 60 }, { "scHpf", 120 },
              { "mix", 70 }, { "dsFreq", 8500 }, { "dsThr", -25 }, { "dsAmt", 5 },
              { "drive", 6 }, { "satMix", 40 }, { "bits", 10 }, { "rate", 6 }, { "crushMix", 25 },
              { "bass", -2 }, { "mid", -1 }, { "midFreq", 400 }, { "air", 8 },
              { "width", 170 }, { "ceiling", -1 }, { "outGain", -1 } }
        };
        return p[jlimit (0, 2, i)];
    }
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout VoidRageProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto add = [&] (const String& id, const String& name, float lo, float hi, float def,
                    float skew, const String& unit, int dec = 1)
    {
        NormalisableRange<float> r (lo, hi, 0.01f, skew);
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, r, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (
                [unit, dec] (float v, int) { return String (v, dec) + " " + unit; })));
    };

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Kill", false));

    // INPUT / GATE
    add ("inGain",   "In Gain",    -24,  24,   0,    1.f,  "dB");
    add ("hpf",      "Low Cut",     20, 400,  80,    0.4f, "Hz", 0);
    add ("gate",     "Gate",       -80, -20, -80,    1.f,  "dB");
    // COMPRESSOR
    add ("thresh",   "Threshold",  -60,   0, -18,    1.f,  "dB");
    add ("ratio",    "Ratio",        1,  20,   4,    0.5f, ":1");
    add ("attack",   "Attack",     0.1f, 100, 5,    0.4f, "ms");
    add ("release",  "Release",      5, 1000, 120,   0.4f, "ms", 0);
    add ("knee",     "Knee",         0,  24,   6,    1.f,  "dB");
    add ("makeup",   "Makeup",       0,  24,   3,    1.f,  "dB");
    add ("rms",      "Peak/RMS",     0, 100,  30,    1.f,  "%", 0);
    add ("scHpf",    "SC Filter",   20, 500, 100,    0.5f, "Hz", 0);
    add ("mix",      "Parallel",     0, 100, 100,    1.f,  "%", 0);
    // DE-ESSER
    add ("dsFreq",   "S-Freq",    3000, 12000, 7000, 0.6f, "Hz", 0);
    add ("dsThr",    "S-Thresh",   -50,   0, -24,    1.f,  "dB");
    add ("dsAmt",    "S-Kill",       0,  15,   0,    1.f,  "dB");
    // SATURATION / CRUSH
    add ("drive",    "Drive",        0,  24,   0,    1.f,  "dB");
    add ("satMix",   "Heat Mix",     0, 100,   0,    1.f,  "%", 0);
    add ("bits",     "Bit Crush",    4,  16,  16,    1.f,  "bit");
    add ("rate",     "Downsample",   1,  32,   1,    0.5f, "x", 0);
    add ("crushMix", "Crush Mix",    0, 100,   0,    1.f,  "%", 0);
    // TONE
    add ("bass",     "Bass",       -12,  12,   0,    1.f,  "dB");
    add ("mid",      "Mid",        -12,  12,   0,    1.f,  "dB");
    add ("midFreq",  "Mid Freq",   200, 5000, 1000,  0.4f, "Hz", 0);
    add ("air",      "Air",        -12,  12,   0,    1.f,  "dB");
    // OUTPUT
    add ("width",    "Width",        0, 200, 100,    1.f,  "%", 0);
    add ("ceiling",  "Ceiling",    -12,   0, -0.3f,  1.f,  "dB");
    add ("outGain",  "Output",     -24,  12,   0,    1.f,  "dB");

    return { p.begin(), p.end() };
}

//==============================================================================
VoidRageProcessor::VoidRageProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (auto& s : scope) s.store (0.f);
}

bool VoidRageProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto o = l.getMainOutputChannelSet();
    if (o != AudioChannelSet::mono() && o != AudioChannelSet::stereo()) return false;
    return o == l.getMainInputChannelSet();
}

void VoidRageProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    rmsState = grState = dsEnv = scopeAcc = 0.f;
    gateEnv = 1.f;
    holdCnt[0] = holdCnt[1] = 0;
    lHpf = lSc = lDsF = lMidF = -1; lBass = lMid = lAir = -99;

    updateFilters (80.f, 100.f, 7000.f, 0.f, 0.f, 1000.f, 0.f);
    for (int c = 0; c < 2; ++c)
    {
        hpf[c].reset(); scHp[c].reset(); dsBand[c].reset();
        lowS[c].reset(); midP[c].reset(); airS[c].reset();
    }
}

void VoidRageProcessor::updateFilters (float hpfF, float scF, float dsF, float bass,
                                       float mid, float midF, float air)
{
    using C = dsp::IIR::Coefficients<float>;
    const float nyq = 0.45f * (float) sr;
    hpfF = jmin (hpfF, nyq); scF = jmin (scF, nyq); dsF = jmin (dsF, nyq); midF = jmin (midF, nyq);

    if (hpfF != lHpf) { lHpf = hpfF; auto c = C::makeHighPass (sr, hpfF, 0.707f); for (auto& f : hpf) f.coefficients = c; }
    if (scF  != lSc)  { lSc = scF;   auto c = C::makeHighPass (sr, scF, 0.707f);  for (auto& f : scHp) f.coefficients = c; }
    if (dsF  != lDsF) { lDsF = dsF;  auto c = C::makeBandPass (sr, dsF, 1.2f);    for (auto& f : dsBand) f.coefficients = c; }
    if (bass != lBass) { lBass = bass; auto c = C::makeLowShelf (sr, 120.f, 0.707f, Decibels::decibelsToGain (bass)); for (auto& f : lowS) f.coefficients = c; }
    if (mid != lMid || midF != lMidF) { lMid = mid; lMidF = midF; auto c = C::makePeakFilter (sr, midF, 0.9f, Decibels::decibelsToGain (mid)); for (auto& f : midP) f.coefficients = c; }
    if (air != lAir) { lAir = air; auto c = C::makeHighShelf (sr, 9000.f, 0.707f, Decibels::decibelsToGain (air)); for (auto& f : airS) f.coefficients = c; }
}

//==============================================================================
void VoidRageProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = jmin (2, buffer.getNumChannels());
    for (int c = nch; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (nch < 1) return;

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    float* d[2] = { buffer.getWritePointer (0), nch > 1 ? buffer.getWritePointer (1) : nullptr };
    const float blockDecay = std::exp (-(float) n / (0.25f * (float) sr));

    // KILL = Bypass
    if (P ("bypass") > 0.5f)
    {
        float pk = 0.f;
        for (int c = 0; c < nch; ++c) pk = jmax (pk, buffer.getMagnitude (c, 0, n));
        inLevel.store (jmax (pk, inLevel.load() * blockDecay));
        outLevel.store (jmax (pk, outLevel.load() * blockDecay));
        grMeter.store (grMeter.load() * blockDecay);
        return;
    }

    // --- Parameter holen
    const float inG      = Decibels::decibelsToGain (P ("inGain"));
    const float gateThr  = P ("gate");
    const bool  gateOn   = gateThr > -79.f;
    const float thresh   = P ("thresh");
    const float ratio    = P ("ratio");
    const float knee     = P ("knee");
    const float makeupG  = Decibels::decibelsToGain (P ("makeup"));
    const float rmsAmt   = P ("rms") * 0.01f;
    const float mixAmt   = P ("mix") * 0.01f;
    const float dsThr    = P ("dsThr");
    const float dsAmt    = P ("dsAmt");
    const float driveDb  = P ("drive");
    const float dg       = Decibels::decibelsToGain (driveDb);
    const float satMix   = P ("satMix") * 0.01f;
    const float bits     = P ("bits");
    const int   rateN    = jmax (1, (int) std::round (P ("rate")));
    const float crushMix = P ("crushMix") * 0.01f;
    const float width    = P ("width") * 0.01f;
    const float outG     = Decibels::decibelsToGain (P ("outGain"));
    const float ceil     = Decibels::decibelsToGain (P ("ceiling"));

    updateFilters (P ("hpf"), P ("scHpf"), P ("dsFreq"), P ("bass"), P ("mid"), P ("midFreq"), P ("air"));

    auto tc = [this] (float ms) { return std::exp (-1.0f / (0.001f * ms * (float) sr)); };
    const float atk = tc (P ("attack")), rel = tc (P ("release"));
    const float rmsCoef = tc (10.f);
    const float gAtt = tc (1.f), gRel = tc (120.f);
    const float dsA = tc (0.5f), dsR = tc (25.f);

    const bool crushOn = crushMix > 0.001f && (bits < 15.9f || rateN > 1);
    const float q = std::pow (2.f, bits - 1.f);
    const float th = 0.8f * ceil;

    float inPk = 0.f, outPk = 0.f, grMax = 0.f;

    for (int i = 0; i < n; ++i)
    {
        float x[2] = { 0.f, 0.f };
        for (int c = 0; c < nch; ++c)
        {
            inPk = jmax (inPk, std::abs (d[c][i]));
            x[c] = hpf[c].processSample (d[c][i] * inG);
        }

        // ---- De-Esser (dynamisches Band-Absenken)
        float s[2] = { 0.f, 0.f };
        float dsDet = 0.f;
        for (int c = 0; c < nch; ++c)
        {
            s[c] = dsBand[c].processSample (x[c]);
            dsDet = jmax (dsDet, std::abs (s[c]));
        }
        dsEnv = dsDet + (dsDet > dsEnv ? dsA : dsR) * (dsEnv - dsDet);
        if (dsAmt > 0.01f)
        {
            const float over = Decibels::gainToDecibels (dsEnv, -120.f) - dsThr;
            const float red = jlimit (0.f, dsAmt, over * 0.8f);
            const float dsGain = Decibels::decibelsToGain (-red);
            for (int c = 0; c < nch; ++c) x[c] -= s[c] * (1.f - dsGain);
        }

        // ---- Detektor (Sidechain-Filter, linked, Peak/RMS Blend)
        float peak = 0.f, sq = 0.f;
        for (int c = 0; c < nch; ++c)
        {
            const float sc = scHp[c].processSample (x[c]);
            peak = jmax (peak, std::abs (sc));
            sq += sc * sc;
        }
        sq /= (float) nch;
        rmsState = rmsCoef * rmsState + (1.f - rmsCoef) * sq;
        const float det = peak * (1.f - rmsAmt) + std::sqrt (rmsState) * 1.4142f * rmsAmt;
        const float lvlDb = Decibels::gainToDecibels (det, -120.f);

        // ---- Gate
        const float gTarget = (!gateOn || lvlDb > gateThr) ? 1.f : 0.f;
        gateEnv = gTarget + (gTarget > gateEnv ? gAtt : gRel) * (gateEnv - gTarget);

        // ---- Kompressor (Soft Knee)
        const float over = lvlDb - thresh;
        float outDb;
        if (2.f * over < -knee)              outDb = lvlDb;
        else if (2.f * std::abs (over) <= knee)
            outDb = lvlDb + (1.f / ratio - 1.f) * square (over + knee * 0.5f) / (2.f * jmax (knee, 0.001f));
        else                                  outDb = thresh + over / ratio;
        const float target = jmax (0.f, lvlDb - outDb);
        grState = target + (target > grState ? atk : rel) * (grState - target);
        grMax = jmax (grMax, grState);
        const float compGain = Decibels::decibelsToGain (-grState);

        for (int c = 0; c < nch; ++c)
        {
            float dry = x[c] * gateEnv;
            float y = dry * (1.f - mixAmt) + dry * compGain * makeupG * mixAmt;

            // ---- Saturation
            if (satMix > 0.001f && driveDb > 0.01f)
            {
                const float sat = std::tanh (y * dg) / std::sqrt (dg);
                y += (sat - y) * satMix;
            }

            // ---- Bit/Sample Crush
            if (crushOn)
            {
                float yc = std::round (y * q) / q;
                if (holdCnt[c] <= 0) { held[c] = yc; holdCnt[c] = rateN; }
                holdCnt[c]--;
                y += (held[c] - y) * crushMix;
            }

            // ---- EQ
            y = lowS[c].processSample (y);
            y = midP[c].processSample (y);
            y = airS[c].processSample (y);
            x[c] = y;
        }

        // ---- Width (M/S)
        if (nch == 2)
        {
            const float m = (x[0] + x[1]) * 0.5f, sd = (x[0] - x[1]) * 0.5f * width;
            x[0] = m + sd; x[1] = m - sd;
        }

        // ---- Output + Soft-Ceiling
        for (int c = 0; c < nch; ++c)
        {
            float y = x[c] * outG;
            const float a = std::abs (y);
            if (a > th)
            {
                const float sgn = y < 0.f ? -1.f : 1.f;
                y = sgn * (th + (ceil - th) * std::tanh ((a - th) / jmax (ceil - th, 1e-4f)));
            }
            d[c][i] = y;
            outPk = jmax (outPk, std::abs (y));
        }

        // ---- Scope
        const float mono = d[0][i];
        if (std::abs (mono) > std::abs (scopeAcc)) scopeAcc = mono;
        if (++scopeCnt >= 24)
        {
            const int pos = scopePos.load();
            scope[(size_t) pos].store (scopeAcc);
            scopePos.store ((pos + 1) % scopeSize);
            scopeAcc = 0.f; scopeCnt = 0;
        }
    }

    inLevel.store  (jmax (inPk,  inLevel.load()  * blockDecay));
    outLevel.store (jmax (outPk, outLevel.load() * blockDecay));
    grMeter.store  (jmax (grMax, grMeter.load()  * blockDecay));
}

//==============================================================================
void VoidRageProcessor::setCurrentProgram (int index)
{
    currentPreset = jlimit (0, 2, index);

    for (auto* prm : apvts.processor.getParameters())
        if (auto* r = dynamic_cast<RangedAudioParameter*> (prm))
            r->setValueNotifyingHost (r->getDefaultValue());

    for (auto& pv : presetData (currentPreset))
        if (auto* r = apvts.getParameter (pv.id))
            r->setValueNotifyingHost (r->convertTo0to1 (pv.v));
}

const String VoidRageProcessor::getProgramName (int index)
{
    return presetNames[jlimit (0, 2, index)];
}

void VoidRageProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void VoidRageProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* VoidRageProcessor::createEditor() { return new VoidRageEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoidRageProcessor(); }

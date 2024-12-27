#pragma once
#include <JuceHeader.h>

class KronosLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KronosLookAndFeel();
    ~KronosLookAndFeel() override;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;

private:
    juce::Typeface::Ptr asteraTypeface;
};

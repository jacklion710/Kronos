#pragma once
#include <JuceHeader.h>

class KronosLookAndFeel : public juce::LookAndFeel_V4, public juce::ChangeBroadcaster
{
public:
    class GlowingLabelLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawLabel(juce::Graphics& g, juce::Label& label) override;
    };

    enum ColourIds
    {
        outlineColourId = 0x2000000,  // Custom color ID starting at a safe value
    };

    KronosLookAndFeel();
    ~KronosLookAndFeel() override;

    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;
    
    // Add color scheme methods
    void setDarkMode(bool isDark);
    bool isDarkMode() const { return darkModeEnabled; }

    GlowingLabelLookAndFeel glowingLabelLookAndFeel;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

private:
    void setupDarkColorScheme();
    void setupLightColorScheme();
    
    juce::Typeface::Ptr asteraTypeface;
    bool darkModeEnabled = true;  // Default to dark mode
};

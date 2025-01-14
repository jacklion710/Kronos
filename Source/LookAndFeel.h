#pragma once
#include <JuceHeader.h>

class KronosLookAndFeel : public juce::LookAndFeel_V4, public juce::ChangeBroadcaster
{
public:
    class GlowingLabelLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawLabel(juce::Graphics& g, juce::Label& label) override;
        void setDarkMode(bool isDark) { darkModeEnabled = isDark; }
    private:
        bool darkModeEnabled = true;
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

    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;

    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted,
                          bool isTicked, bool hasSubMenu, const juce::String& text,
                          const juce::String& shortcutKeyText,
                          const juce::Drawable* icon, const juce::Colour* textColour) override;

    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

    void drawAlertBox(juce::Graphics&, juce::AlertWindow&, 
                     const juce::Rectangle<int>& textArea, 
                     juce::TextLayout&) override;

    juce::Font getAlertWindowMessageFont() override
    {
        return juce::Font(14.0f);
    }

    // Add color getters for theme-aware colors
    juce::Colour getStrokeColor() const
    {
        return darkModeEnabled ? 
            juce::Colour(30, 50, 150) :     // Dark blue-grey for dark mode
            juce::Colour(120, 100, 80);     // Warm metallic grey for light mode
    }

    juce::Colour getGlowColor() const
    {
        return darkModeEnabled ? 
            juce::Colour(64, 64, 255) :     // Blue for dark mode
            juce::Colour(160, 140, 120);    // Soft warm grey for light mode
    }

    juce::Colour getTextColor() const
    {
        return darkModeEnabled ? 
            juce::Colour(0xE6, 0xE6, 0xFF) :    // Light blue-white for dark mode
            juce::Colour(0xE6, 0xD5, 0xBF);     // Beige for light mode
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted, 
                       bool shouldDrawButtonAsDown) override;

private:
    void setupDarkColorScheme();
    void setupLightColorScheme();
    
    juce::Typeface::Ptr asteraTypeface;
    bool darkModeEnabled = true;  // Default to dark mode
};

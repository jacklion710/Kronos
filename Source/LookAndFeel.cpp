#include "LookAndFeel.h"
#include "BinaryData.h"

KronosLookAndFeel::KronosLookAndFeel()
{
    // Load the ASTERA font from binary data
    auto typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::ASTERA_ttf, BinaryData::ASTERA_ttfSize);
    
    // Check to ensure font loaded
    if (typeface != nullptr)
    {
        asteraTypeface = typeface;
    }

    // Set initial color scheme (dark mode by default)
    setupDarkColorScheme();
}

KronosLookAndFeel::~KronosLookAndFeel()
{
}

juce::Typeface::Ptr KronosLookAndFeel::getTypefaceForFont(const juce::Font& font)
{
    if (asteraTypeface != nullptr)
        return asteraTypeface;
    
    return juce::LookAndFeel_V4::getTypefaceForFont(font);
}

void KronosLookAndFeel::setupDarkColorScheme()
{
    // Background colors
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF1E1E1E));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2D2D2D));
    
    // Text colors
    setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE0E0E0));
    
    // Border and accent colors
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF007AFF));
    setColour(KronosLookAndFeel::outlineColourId, juce::Colour(0xFFE0E0E0));  // For borders
    
    // Date label colors (slightly dimmer than main text)
    setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
}

void KronosLookAndFeel::setupLightColorScheme()
{
    // Background colors
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFFF5F5F5));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFE8E8E8));
    
    // Text colors
    setColour(juce::Label::textColourId, juce::Colour(0xFF2D2D2D));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xFF2D2D2D));
    
    // Border and accent colors
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF0A84FF));
    setColour(KronosLookAndFeel::outlineColourId, juce::Colour(0xFF2D2D2D));  // For borders
    
    // Date label colors (slightly dimmer than main text)
    setColour(juce::Label::textColourId, juce::Colour(0xFF2D2D2D));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentWhite);
}

void KronosLookAndFeel::setDarkMode(bool isDark)
{
    darkModeEnabled = isDark;
    if (isDark)
        setupDarkColorScheme();
    else
        setupLightColorScheme();
        
    // Notify any components using this LookAndFeel to repaint
    sendChangeMessage();
}


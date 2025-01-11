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

void KronosLookAndFeel::GlowingLabelLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    auto bounds = label.getLocalBounds().toFloat();
    auto text = label.getText();
    auto font = label.getFont();

    // Draw glow (multiple passes with decreasing alpha)
    auto glowColor = juce::Colour(64, 64, 255).withAlpha(0.2f);
    for (float i = 6; i > 0; --i)
    {
        g.setColour(glowColor.withAlpha(glowColor.getFloatAlpha() / i));
        g.setFont(font);
        auto glowBounds = bounds.expanded(i * 0.3f);
        g.drawText(text, glowBounds, label.getJustificationType(), true);
    }

    // Draw multiple colored strokes for a neon effect
    float strokeWidth = 0.8f;
    
    // Inner blue stroke with reduced alpha
    g.setColour(juce::Colour(64, 64, 255).withAlpha(0.6f));
    g.setFont(font);
    for (float x = -strokeWidth; x <= strokeWidth; x += strokeWidth)
        for (float y = -strokeWidth; y <= strokeWidth; y += strokeWidth)
            if (x != 0 || y != 0)
                g.drawText(text, bounds.translated(x, y), label.getJustificationType(), true);

    // Outer white stroke with reduced alpha
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(font);
    g.drawText(text, bounds, label.getJustificationType(), true);

    // Draw main text
    g.setColour(label.findColour(juce::Label::textColourId));
    g.drawText(text, bounds, label.getJustificationType(), true);
}

void KronosLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    // Check if this is a time label (hours, minutes, seconds)
    if (label.getName().contains("TimeLabel"))
    {
        auto bounds = label.getLocalBounds().toFloat();
        auto text = label.getText();
        auto font = label.getFont();
        
        // Draw multiple layers for stroke effect
        auto textArea = bounds.withSizeKeepingCentre(bounds.getWidth(), bounds.getHeight());
        
        // Draw stroke layers with a dark blue-grey color
        g.setColour(juce::Colour(30, 50, 150));  // Dark blue-grey
        g.setFont(font);
        
        // Multiple offset positions for stroke effect
        float strokeSize = 1.25f;
        float positions[][2] = {
            {-strokeSize, -strokeSize},
            {-strokeSize, strokeSize},
            {strokeSize, -strokeSize},
            {strokeSize, strokeSize},
            {0, strokeSize},
            {0, -strokeSize},
            {strokeSize, 0},
            {-strokeSize, 0}
        };
        
        // Draw stroke positions
        for (auto& pos : positions)
        {
            g.drawText(text, 
                      textArea.translated(pos[0], pos[1]).toNearestInt(), 
                      label.getJustificationType(), 
                      true);
        }
        
        // Draw main text
        g.setColour(label.findColour(juce::Label::textColourId));
        g.drawText(text, textArea.toNearestInt(), label.getJustificationType(), true);
    }
    else
    {
        // Default label drawing for other labels
        LookAndFeel_V4::drawLabel(g, label);
    }
}


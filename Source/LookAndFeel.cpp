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
    // Check if this is a time label or date label
    if (label.getName().contains("TimeLabel") || label.getName().contains("DateLabel"))
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
        float strokeSize = label.getName().contains("TimeLabel") ? 1.25f : 0.75f; // Smaller stroke for date labels
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

juce::Font KronosLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(18.0f, juce::Font::bold);
}

void KronosLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto background = juce::Colour(30, 30, 30);  // Dark background
    auto highlight = juce::Colour(40, 40, 40);   // Slightly lighter for gradient
    
    g.setGradientFill(juce::ColourGradient(background, 0.0f, 0.0f,
                                          highlight, 0.0f, (float)height,
                                          false));
    g.fillAll();
    
    // Add subtle border
    g.setColour(juce::Colour(60, 60, 60));
    g.drawRect(0, 0, width, height, 1);
}

void KronosLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                        bool isSeparator, bool isActive, bool isHighlighted,
                                        bool isTicked, bool hasSubMenu, const juce::String& text,
                                        const juce::String& shortcutKeyText,
                                        const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isHighlighted && isActive)
    {
        // Create a gradient for the highlight
        auto bounds = area.toFloat().reduced(2);  // Reduce slightly for padding
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(64, 64, 255).withAlpha(0.2f),  // Lighter blue
            bounds.getTopLeft(),
            juce::Colour(64, 64, 255).withAlpha(0.3f),  // Slightly darker blue
            bounds.getBottomRight(),
            false));
        g.fillRoundedRectangle(bounds, 4.0f);  // Rounded corners
        
        // Add subtle glow border
        g.setColour(juce::Colour(64, 64, 255).withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    // Use ASTERA font for menu items
    auto font = juce::Font("ASTERA", 16.0f, juce::Font::plain);
    auto textColor = juce::Colour(0xE6, 0xE6, 0xFF);  // Matching existing text color
    
    if (!isSeparator)
    {
        // Draw text with subtle glow effect
        auto textArea = area.reduced(15, 0);
        
        // Draw glow
        g.setColour(textColor.withAlpha(0.3f));
        for (float i = 2; i > 0; --i)
        {
            g.setFont(font);
            auto glowBounds = textArea.toFloat().translated(0, i * 0.5f);
            g.drawText(text, glowBounds, juce::Justification::centredLeft, true);
        }
        
        // Draw main text
        g.setColour(isActive ? textColor : textColor.withAlpha(0.5f));
        g.setFont(font);
        g.drawText(text, textArea, juce::Justification::centredLeft, true);
    }
}

void KronosLookAndFeel::drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert,
                                    const juce::Rectangle<int>& textArea, 
                                    juce::TextLayout& textLayout)
{
    auto bounds = alert.getLocalBounds().toFloat();
    float borderThickness = 4.0f;
    
    // Draw background
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(30, 30, 30),  // Dark background
        bounds.getTopLeft(),
        juce::Colour(40, 40, 40),  // Slightly lighter
        bounds.getBottomRight(),
        false));
    g.fillRect(bounds.reduced(borderThickness));
    
    // Create gradient for border with multiple points for sine-wave like effect
    juce::ColourGradient borderGradient(
        juce::Colour(130, 130, 130),  // Light grey (bottom left)
        bounds.getBottomLeft(),
        juce::Colour(130, 130, 130),  // Light grey (top right)
        bounds.getTopRight(),
        false
    );
    
    // Add intermediate points for the sine-wave like effect
    borderGradient.addColour(0.25, juce::Colour(40, 40, 40));    // Dark (first quarter)
    borderGradient.addColour(0.5, juce::Colour(40, 40, 40));     // Dark (middle)
    borderGradient.addColour(0.75, juce::Colour(130, 130, 130)); // Light (third quarter)
    
    // Draw border with gradient
    g.setGradientFill(borderGradient);
    g.drawRect(bounds, borderThickness);
    
    // Set text color
    alert.setColour(juce::AlertWindow::textColourId, juce::Colour(0xE6, 0xE6, 0xFF));
    
    // Draw the text layout
    textLayout.draw(g, textArea.toFloat());
}


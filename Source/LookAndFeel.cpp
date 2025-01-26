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
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentWhite);
}

void KronosLookAndFeel::setDarkMode(bool isDark)
{
    darkModeEnabled = isDark;
    glowingLabelLookAndFeel.setDarkMode(isDark);
    if (isDark)
        setupDarkColorScheme();
    else
        setupLightColorScheme();
    sendChangeMessage();
}

void KronosLookAndFeel::GlowingLabelLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    auto bounds = label.getLocalBounds().toFloat();
    auto text = label.getText();
    
    // Set Astera font with dynamic sizing
    g.setFont(juce::Font("ASTERA", bounds.getHeight() * 0.5f, juce::Font::plain));
    
    // Enhanced glow color with higher alpha
    auto glowColor = darkModeEnabled ? 
        juce::Colour(64, 64, 255).withAlpha(0.3f) :     // Brighter blue glow for dark mode
        juce::Colour(160, 140, 120).withAlpha(0.3f);    // Brighter warm grey for light mode

    // Number of glow passes for more pronounced effect
    for (float i = 8; i > 0; --i) 
    {
        float alpha = glowColor.getFloatAlpha() / (i * 0.7f);  // Adjusted alpha falloff
        g.setColour(glowColor.withAlpha(alpha));
        g.drawText(text, bounds, label.getJustificationType(), true);
    }

    // Enhanced stroke effect
    auto strokeColor = darkModeEnabled ?
        juce::Colour(30, 50, 150).brighter(0.2f) :      // Stroke for dark mode
        juce::Colour(120, 100, 80).brighter(0.2f);      // Stroke for light mode

    // Thick stroke
    float strokeWidth = 2.3f;
    g.setColour(strokeColor);
    
    for (float x = -strokeWidth; x <= strokeWidth; x += strokeWidth)
        for (float y = -strokeWidth; y <= strokeWidth; y += strokeWidth)
            if (x != 0 || y != 0)
                g.drawText(text, bounds.translated(x, y), label.getJustificationType(), true);

    // Main text in pure white for better contrast
    g.setColour(juce::Colours::white);
    g.drawText(text, bounds, label.getJustificationType(), true);
}

void KronosLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    if (label.getName() == "TitleLabel") 
    {
        // Let the glowing label look handle all title drawing
        glowingLabelLookAndFeel.drawLabel(g, label);
        return; 
    }
    else if (label.getName().contains("PreviousSessions"))
    {
        // Use same very muted grey for both modes
        g.setColour(juce::Colour(0x30, 0x30, 0x35));    // Very muted grey with slight blue tint
        g.setFont(label.getFont());
        g.drawText(label.getText(), label.getLocalBounds(), 
                  label.getJustificationType(), true);
        return;
    }
    
    // Check if this is a time label or date label
    if (label.getName().contains("TimeLabel") || label.getName().contains("DateLabel"))
    {
        auto bounds = label.getLocalBounds().toFloat();
        auto text = label.getText();
        auto font = label.getFont();
        
        auto textArea = bounds.withSizeKeepingCentre(bounds.getWidth(), bounds.getHeight());
        
        // Get theme-appropriate stroke color
        g.setColour(getStrokeColor());
        g.setFont(font);
        
        // Multiple offset positions for stroke effect
        float strokeSize = label.getName().contains("TimeLabel") ? 1.25f : 0.75f;
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
        
        // Draw main text in pure white
        g.setColour(juce::Colours::white);
        g.drawText(text, textArea.toNearestInt(), label.getJustificationType(), true);
    }
    else if (label.getName().contains("BarTimeLabel"))  // Time labels over bars
    {
        auto bounds = label.getLocalBounds().toFloat();
        auto text = label.getText();
        
        // First draw black stroke
        g.setColour(juce::Colours::black);
        float blackStrokeSize = 1.5f; 
        for (float x = -blackStrokeSize; x <= blackStrokeSize; x += blackStrokeSize)
            for (float y = -blackStrokeSize; y <= blackStrokeSize; y += blackStrokeSize)
                if (x != 0 || y != 0)
                    g.drawText(text, bounds.translated(x, y), 
                             label.getJustificationType(), true);
        
        // Draw main text in white
        g.setColour(juce::Colours::white);
        g.drawText(text, bounds, label.getJustificationType(), true);
    }
    else
    {
        LookAndFeel_V4::drawLabel(g, label);
    }
}

juce::Font KronosLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    // Special case for close button
    if (button.getButtonText() == "x")
        return juce::Font("ASTERA", 16.0f, juce::Font::plain);  // Smaller font size
        
    return juce::Font(18.0f, juce::Font::bold);
}

void KronosLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto background = darkModeEnabled ? 
        juce::Colour(30, 30, 30) :       // Dark background for dark mode
        juce::Colour(200, 195, 190);     // Darker warm grey for light mode
    auto highlight = darkModeEnabled ? 
        juce::Colour(40, 40, 40) :       // Slightly lighter for dark mode
        juce::Colour(210, 205, 200);     // Slightly lighter warm grey for light mode
    
    g.setGradientFill(juce::ColourGradient(background, 0.0f, 0.0f,
                                          highlight, 0.0f, (float)height,
                                          false));
    g.fillAll();
    
    // Add subtle border
    g.setColour(darkModeEnabled ? 
        juce::Colour(60, 60, 60) :       // Dark mode border
        juce::Colour(120, 100, 80));     // Light mode border - warm metallic
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
        // Create a gradient for the highlight that matches our theme
        auto bounds = area.toFloat().reduced(2);
        g.setGradientFill(juce::ColourGradient(
            darkModeEnabled ?
                juce::Colour(64, 64, 255).withAlpha(0.2f) :     // Blue for dark mode
                juce::Colour(160, 140, 120).withAlpha(0.2f),    // Warm metallic for light mode
            bounds.getTopLeft(),
            darkModeEnabled ?
                juce::Colour(64, 64, 255).withAlpha(0.3f) :     // Darker blue for dark mode
                juce::Colour(120, 100, 80).withAlpha(0.3f),     // Darker metallic for light mode
            bounds.getBottomRight(),
            false));
        g.fillRoundedRectangle(bounds, 4.0f);
        
        // Add subtle glow border
        g.setColour(darkModeEnabled ?
            juce::Colour(64, 64, 255).withAlpha(0.4f) :         // Blue glow for dark mode
            juce::Colour(160, 140, 120).withAlpha(0.4f));       // Warm metallic glow for light mode
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    auto font = juce::Font("ASTERA", 16.0f, juce::Font::plain);
    auto textColor = darkModeEnabled ? 
        juce::Colour(0xE6, 0xE6, 0xFF) :      // Light blue-white for dark mode
        juce::Colour(50, 40, 35);             // Darker warm grey for light mode
    
    if (!isSeparator)
    {
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
    auto bounds = alert.getLocalBounds().toFloat().reduced(1.0f);
    
    // Fill background with a muted, warm grey
    g.setColour(darkModeEnabled ? 
        juce::Colour(0x1E, 0x1E, 0x1E) :      // Dark grey for dark mode
        juce::Colour(100, 95, 90));            // Muted warm grey for light mode
    g.fillRoundedRectangle(bounds, 10.0f);
    
    // Draw border with gradient
    juce::ColourGradient borderGradient(
        darkModeEnabled ?
            juce::Colour(130, 130, 130) :           // Dark mode highlight
            juce::Colour(160, 140, 120),            // Light mode warm highlight
        bounds.getBottomLeft(),
        darkModeEnabled ?
            juce::Colour(40, 40, 40) :             // Dark mode shadow
            juce::Colour(100, 85, 70),             // Light mode warm shadow
        bounds.getTopRight(),
        false
    );
    
    g.setGradientFill(borderGradient);
    g.drawRoundedRectangle(bounds, 10.0f, 2.0f);
    
    // Draw the provided TextLayout
    textLayout.draw(g, textArea.toFloat());
}

// Add ASTERA font to alert window buttons
void KronosLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                     bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto font = juce::Font("ASTERA", 16.0f, juce::Font::plain);
    g.setFont(font);
    
    g.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                         : juce::TextButton::textColourOffId)
                       .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

    auto bounds = button.getLocalBounds();
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred, true);
}

void KronosLookAndFeel::drawHyperlinkButton(juce::Graphics& g, juce::HyperlinkButton& button,
                                           const juce::Colour& textColour)
{
    auto buttonText = button.getButtonText();
    auto font = juce::Font("ASTERA", 16.0f, juce::Font::plain);
    
    auto textBounds = button.getLocalBounds();
    
    // Set color based on mouse state and theme
    juce::Colour linkColour = darkModeEnabled ?
        (button.isMouseOver() ? juce::Colour(150, 150, 255) : juce::Colour(100, 100, 255)) :  // Blue shades for dark mode
        (button.isMouseOver() ? juce::Colour(255, 140, 0) : juce::Colour(200, 110, 0));       // Orange shades for light mode
    
    g.setColour(linkColour);
    g.setFont(font);
    g.drawText(buttonText, textBounds, juce::Justification::centred, true);
    
    // Draw underline
    auto textWidth = font.getStringWidth(buttonText);
    auto centreX = textBounds.getCentreX();
    auto baseY = textBounds.getCentreY() + 5;  // Adjust this value to position the underline
    
    g.drawLine(centreX - textWidth/2, baseY,
               centreX + textWidth/2, baseY,
               1.0f);  // Line thickness
}


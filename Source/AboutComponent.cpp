#include "AboutComponent.h"

AboutComponent::AboutComponent(KronosAudioProcessor& processor)
    : audioProcessor(processor)
{
    // No need to set a fixed size anymore - we'll use parent bounds
    
    // Close button setup
    closeButton.setButtonText("x");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(closeButton);
    
    // Initialize hyperlinks
    gumroadLink.setButtonText("JACKLION.GUMROAD.COM");
    gumroadLink.setURL(juce::URL("https://jacklion.gumroad.com"));
    
    soundcloudLink.setButtonText("SOUNDCLOUD.COM/JACK0LION");
    soundcloudLink.setURL(juce::URL("https://soundcloud.com/jack0lion"));
    
    jlWebLink.setButtonText("JACKLION.XYZ");
    jlWebLink.setURL(juce::URL("https://jacklion.xyz"));
    
    jlInstaLink.setButtonText("INSTAGRAM.COM/JACK.LION");
    jlInstaLink.setURL(juce::URL("https://www.instagram.com/jack.lion"));
    
    discordLink.setButtonText("DISCORD.GG/EF0Q7BX");
    discordLink.setURL(juce::URL("https://discord.gg/EFQq7BX"));
    
    aznadelLink.setButtonText("LINKTR.EE/AZNADEL");
    aznadelLink.setURL(juce::URL("https://linktr.ee/aznadel"));

    // Make all links visible
    for (auto* link : {&gumroadLink, &soundcloudLink, &jlInstaLink, 
                      &jlWebLink, &discordLink, &aznadelLink})
    {
        addAndMakeVisible(link);
    }

    positionLinks();
}

void AboutComponent::resized()
{
    // Calculate scale factor based on default size of 600x450
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    float scale = juce::jmin(widthScale, heightScale);

    // Position close button relative to parent bounds with scaling
    closeButton.setBounds(getWidth() - (25 * scale), 5 * scale, 20 * scale, 20 * scale);
    
    // Update other component positions with scaling
    positionLinks();
}

void AboutComponent::paint(juce::Graphics& g)
{
    // Calculate scale factor
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    float scale = juce::jmin(widthScale, heightScale);

    bool isDarkMode = audioProcessor.isDarkMode();
    
    // Background color based on theme
    g.fillAll(isDarkMode ? 
        juce::Colour(0xFF1A1A1A) :           // Dark charcoal for dark mode
        juce::Colour(0xFF8A8A8A));           // Concrete gray for light mode
    
    // Text color - white for both modes for now
    g.setColour(juce::Colours::white);
    
    auto asteraFont = juce::Font("ASTERA", 24.0f * scale, juce::Font::plain);
    auto contentBounds = getLocalBounds().reduced(20 * scale);
    
    auto topSectionBounds = contentBounds.removeFromTop(170 * scale); 
    
    // Title
    g.setFont(asteraFont);
    g.drawText("KRONOS", topSectionBounds.removeFromTop(30 * scale), juce::Justification::centred, true);
    
    // Version
    g.setFont(asteraFont.withHeight(20.0f * scale));
    g.drawText("V1.0.1", topSectionBounds.removeFromTop(30 * scale), juce::Justification::centred, true);
    
    topSectionBounds.removeFromTop(20 * scale); // Space before description
    
    // Description
    g.setFont(asteraFont.withHeight(16.0f * scale));
    auto descriptionBounds = topSectionBounds.removeFromTop(30 * scale);
    g.drawFittedText("A SIMPLE TIME TRACKING PLUGIN FOR YOUR DAW", 
                     descriptionBounds,
                     juce::Justification::centred, 
                     2);
    
    // Creator credit
    auto creditBounds = topSectionBounds.removeFromTop(30 * scale);
    g.drawFittedText("CREATED BY JACOB LEONE AKA JACK.LION", 
                     creditBounds,
                     juce::Justification::centred, 
                     2);

    // Draw separator line
    int lineWidth = getWidth() * 0.8f;
    int lineX = (getWidth() - lineWidth) / 2;
    int lineY = 190 * scale;
    g.setColour(juce::Colours::white);
    g.drawLine(lineX, lineY, lineX + lineWidth, lineY, 1.0f * scale);
    
    contentBounds.removeFromTop(40 * scale); // Space after separator

    // Split remaining content into two columns
    auto leftColumn = contentBounds.removeFromLeft(contentBounds.getWidth() / 2);
    auto rightColumn = contentBounds;

    // Left Column
    g.setColour(juce::Colours::white); // Reset color after line
    g.drawText("PLUGINS and MORE AT:", 
               leftColumn.removeFromTop(30 * scale),
               juce::Justification::centred, true);
               
    leftColumn.removeFromTop(45 * scale); // Space between sections
    g.drawText("MUSIC", 
               leftColumn.removeFromTop(30 * scale), 
               juce::Justification::centred, true);
               
    leftColumn.removeFromTop(45 * scale); // Space between sections
    g.drawText("INSTAGRAM", 
               leftColumn.removeFromTop(30 * scale), 
               juce::Justification::centred, true);

    // Right Column
    g.drawText("WEBSITE", 
               rightColumn.removeFromTop(30 * scale), 
               juce::Justification::centred, true);
               
    rightColumn.removeFromTop(45 * scale); // Space between sections
    g.drawText("BUG REPORTS", 
               rightColumn.removeFromTop(30 * scale), 
               juce::Justification::centred, true);
               
    rightColumn.removeFromTop(45 * scale); // Space between sections
    g.drawText("GRAPHICS BY AZNADEL", 
               rightColumn.removeFromTop(30 * scale), 
               juce::Justification::centred, true);

    // Draw metallic border
    float borderThickness = 2.0f * scale;
    
    // Create gradient for border
    juce::ColourGradient borderGradient(
        isDarkMode ?
            juce::Colour(180, 180, 180) :     // Lighter silver for dark mode start
            juce::Colour(200, 180, 140),      // Warmer gold for light mode start
        getLocalBounds().getTopLeft().toFloat(),
        isDarkMode ?
            juce::Colour(100, 100, 100) :     // Darker silver for dark mode end
            juce::Colour(120, 100, 80),       // Darker gold for light mode end
        getLocalBounds().getBottomRight().toFloat(),
        false);
    
    auto borderBounds = getLocalBounds().toFloat();
    g.setGradientFill(borderGradient);
    g.drawRect(borderBounds, borderThickness);
}

void AboutComponent::positionLinks()
{
    // Calculate scale factor
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    float scale = juce::jmin(widthScale, heightScale);

    // Calculate center position and consistent width for all links with scaling
    int linkWidth = 300 * scale;
    int linkHeight = 30 * scale;
    
    // Get bounds for layout
    auto bounds = getLocalBounds().reduced(20 * scale);
    
    // Skip past the fixed content at the top (including separator line)
    bounds.removeFromTop(190 * scale); 
    
    // Split into two columns
    auto leftColumn = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto rightColumn = bounds;

    // Position links
    int leftY = leftColumn.getY();
    gumroadLink.setBounds(leftColumn.getX() + (leftColumn.getWidth() - linkWidth) / 2, 
                         leftY + 45 * scale, linkWidth, linkHeight);
    soundcloudLink.setBounds(leftColumn.getX() + (leftColumn.getWidth() - linkWidth) / 2, 
                            leftY + 120 * scale, linkWidth, linkHeight);
    jlInstaLink.setBounds(leftColumn.getX() + (leftColumn.getWidth() - linkWidth) / 2, 
                         leftY + 195 * scale, linkWidth, linkHeight);

    // Right column links
    int rightY = rightColumn.getY();
    jlWebLink.setBounds(rightColumn.getX() + (rightColumn.getWidth() - linkWidth) / 2, 
                       rightY + 45 * scale, linkWidth, linkHeight);
    discordLink.setBounds(rightColumn.getX() + (rightColumn.getWidth() - linkWidth) / 2, 
                         rightY + 120 * scale, linkWidth, linkHeight);
    aznadelLink.setBounds(rightColumn.getX() + (rightColumn.getWidth() - linkWidth) / 2, 
                         rightY + 195 * scale, linkWidth, linkHeight);

    // Update link appearance
    bool isDarkMode = audioProcessor.isDarkMode();
    juce::Colour linkColor = isDarkMode ? 
        juce::Colour(64, 64, 255) :      // Blue for dark mode
        juce::Colour(255, 150, 50);      // Orange for light mode
    
    auto asteraFont = juce::Font("ASTERA", 16.0f * scale, juce::Font::plain);
    
    for (auto* link : {&gumroadLink, &soundcloudLink, &jlInstaLink, 
                      &jlWebLink, &discordLink, &aznadelLink})
    {
        link->setFont(asteraFont, false);
        link->setColour(juce::HyperlinkButton::textColourId, linkColor);
        link->setJustificationType(juce::Justification::centred);
    }
}

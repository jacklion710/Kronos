#include "AboutComponent.h"

AboutComponent::AboutComponent(KronosAudioProcessor& processor)
    : audioProcessor(processor)
{
    // Window size
    setSize(400, 550);

    // Close button setup
    closeButton.setButtonText("x");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    closeButton.setBounds(370, 5, 20, 20);
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

void AboutComponent::paint(juce::Graphics& g)
{
    bool isDarkMode = audioProcessor.isDarkMode();
    
    // Background color based on theme
    g.fillAll(isDarkMode ? 
        juce::Colour(0xFF1A1A1A) :           // Dark charcoal for dark mode
        juce::Colour(0xFF8A8A8A));           // Concrete gray for light mode
    
    // Text color - white for dark mode, dark gray for light mode
    g.setColour(isDarkMode ? juce::Colours::white : juce::Colours::white);
    
    auto asteraFont = juce::Font("ASTERA", 24.0f, juce::Font::plain);
    auto contentBounds = getLocalBounds().reduced(20);
    
    // Title - stick to top with minimal padding
    g.setFont(asteraFont);
    g.drawText("KRONOS", contentBounds.removeFromTop(30), juce::Justification::centred, true);
    
    // Version - immediately below title
    g.setFont(asteraFont.withHeight(20.0f));
    g.drawText("V1.0.0-beta.4", contentBounds.removeFromTop(30), juce::Justification::centred, true);
    
    contentBounds.removeFromTop(20); // Space before description
    
    // Description - using taller bounds for wrapping
    g.setFont(asteraFont.withHeight(16.0f));
    auto descriptionBounds = contentBounds.removeFromTop(30);  
    g.drawFittedText("A SIMPLE TIME TRACKING PLUGIN FOR YOUR DAW", 
                     descriptionBounds,
                     juce::Justification::centred, 
                     2);  // Allow up to 2 lines of text
    
    // Creator credit - also with wrapping
    auto creditBounds = contentBounds.removeFromTop(30);  
    g.drawFittedText("CREATED BY JACOB LEONE AKA JACK.LION", 
                     creditBounds,
                     juce::Justification::centred, 
                     2);  // Allow up to 2 lines of text
    
    contentBounds.removeFromTop(40); // Space after description
    
    // Draw all labels first
    g.drawText("PLUGINS and MORE AT:", 
               contentBounds.removeFromTop(20), 
               juce::Justification::centred, true);
    
    contentBounds.removeFromTop(30);
    g.drawText("MUSIC", 
               contentBounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    contentBounds.removeFromTop(30);
    g.drawText("INSTAGRAM", 
               contentBounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    contentBounds.removeFromTop(30);
    g.drawText("WEBSITE", 
               contentBounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    contentBounds.removeFromTop(30);
    g.drawText("BUG REPORTS", 
               contentBounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    contentBounds.removeFromTop(30);
    g.drawText("GRAPHICS BY AZNADEL", 
               contentBounds.removeFromTop(30), 
               juce::Justification::centred, true);

    // Draw custom metallic border
    float borderThickness = 2.0f;
    
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

void AboutComponent::resized()
{
    positionLinks();
}

void AboutComponent::positionLinks()
{
    // Calculate center position and consistent width for all links
    int linkWidth = 300;
    int linkHeight = 30;
    int centerX = getWidth() / 2 - linkWidth / 2;
    
    // Starting Y position after the initial text
    int currentY = 215;
    
    // Position each link with reduced spacing
    gumroadLink.setBounds(centerX, currentY, linkWidth, linkHeight);
    currentY += 55; // Space to next link
    
    soundcloudLink.setBounds(centerX, currentY, linkWidth, linkHeight);
    currentY += 60;
    
    jlInstaLink.setBounds(centerX, currentY, linkWidth, linkHeight);
    currentY += 60;
    
    jlWebLink.setBounds(centerX, currentY, linkWidth, linkHeight);
    currentY += 60;
    
    discordLink.setBounds(centerX, currentY, linkWidth, linkHeight);
    currentY += 60;
    
    aznadelLink.setBounds(centerX, currentY, linkWidth, linkHeight);

    // Update link appearance based on theme
    bool isDarkMode = audioProcessor.isDarkMode();
    juce::Colour linkColor = isDarkMode ? 
        juce::Colour(64, 64, 255) :      // Blue for dark mode
        juce::Colour(255, 150, 50);      // Orange for light mode
    
    auto asteraFont = juce::Font("ASTERA", 16.0f, juce::Font::plain);
    
    for (auto* link : {&gumroadLink, &soundcloudLink, &jlInstaLink, 
                      &jlWebLink, &discordLink, &aznadelLink})
    {
        link->setFont(asteraFont, false);
        link->setColour(juce::HyperlinkButton::textColourId, linkColor);
        link->setJustificationType(juce::Justification::centred);
    }
}

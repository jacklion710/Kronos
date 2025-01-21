#include "AboutComponent.h"

AboutComponent::AboutComponent(KronosAudioProcessor& processor)
    : audioProcessor(processor)
{
    // Increase window size to fit all content
    setSize(400, 650); // Increased height to prevent cutoff

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
    // Background color - dark charcoal gray
    g.fillAll(juce::Colour(0xFF1A1A1A));  // Adjust this value if needed to match exactly
    
    // Text color - white for all text
    g.setColour(juce::Colours::white);
    
    auto asteraFont = juce::Font("ASTERA", 24.0f, juce::Font::plain);
    auto bounds = getLocalBounds().reduced(20);
    
    // Title - stick to top with minimal padding
    g.setFont(asteraFont);
    g.drawText("KRONOS", bounds.removeFromTop(30), juce::Justification::centred, true);
    
    // Version - immediately below title
    g.setFont(asteraFont.withHeight(20.0f));
    g.drawText("V1.0.0", bounds.removeFromTop(30), juce::Justification::centred, true);
    
    bounds.removeFromTop(20); // Reduced space before description (was 30)
    
    // Description - using taller bounds for wrapping
    g.setFont(asteraFont.withHeight(16.0f));
    auto descriptionBounds = bounds.removeFromTop(30);  // Increased height for two lines
    g.drawFittedText("A SIMPLE TIME TRACKING PLUGIN FOR YOUR DAW", 
                     descriptionBounds,
                     juce::Justification::centred, 
                     2);  // Allow up to 2 lines of text
    
    // Creator credit - also with wrapping
    auto creditBounds = bounds.removeFromTop(30);  // Increased height for two lines
    g.drawFittedText("CREATED BY JACOB LEONE AKA JACK.LION", 
                     creditBounds,
                     juce::Justification::centred, 
                     2);  // Allow up to 2 lines of text
    
    bounds.removeFromTop(40); // Reduced space after description
    
    // Draw all labels first, with less spacing between them and links
    g.drawText("PLUGINS and MORE AT:", 
               bounds.removeFromTop(20), 
               juce::Justification::centred, true);
    
    bounds.removeFromTop(30);
    g.drawText("MUSIC", 
               bounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    bounds.removeFromTop(30);
    g.drawText("INSTAGRAM", 
               bounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    bounds.removeFromTop(30);
    g.drawText("WEBSITE", 
               bounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    bounds.removeFromTop(30);
    g.drawText("BUG REPORTS and FEATURE REQUESTS", 
               bounds.removeFromTop(30), 
               juce::Justification::centred, true);
               
    bounds.removeFromTop(30);
    g.drawText("GRAPHICS BY AZNADEL", 
               bounds.removeFromTop(30), 
               juce::Justification::centred, true);

    // Border
    float borderThickness = 2.0f;
    auto borderBounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white.withAlpha(0.3f));
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
    
    // Position each link with consistent spacing
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

    // Update link appearance
    juce::Colour linkColor(0x64, 0x64, 0xFF); // Electric blue color
    auto asteraFont = juce::Font("ASTERA", 16.0f, juce::Font::plain);
    
    for (auto* link : {&gumroadLink, &soundcloudLink, &jlInstaLink, 
                      &jlWebLink, &discordLink, &aznadelLink})
    {
        link->setFont(asteraFont, false);
        link->setColour(juce::HyperlinkButton::textColourId, linkColor);
        link->setJustificationType(juce::Justification::centred);
    }
}

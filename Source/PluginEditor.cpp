/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>

//==============================================================================
KronosAudioProcessorEditor::KronosAudioProcessorEditor (KronosAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Apply the custom look and feel and set initial theme
    setLookAndFeel(&customLookAndFeel);
    customLookAndFeel.setDarkMode(audioProcessor.isDarkMode());
    
    // Create fonts
    auto asteraFontLarge = juce::Font(36.0f);
    auto asteraFontSmall = juce::Font(16.0f);
    asteraFontLarge.setTypefaceName("ASTERA");
    asteraFontSmall.setTypefaceName("ASTERA");
    
    // Add the time label
    addAndMakeVisible(hoursLabel);
    addAndMakeVisible(minutesLabel);
    addAndMakeVisible(secondsLabel);

    hoursLabel.setFont(asteraFontLarge);
    minutesLabel.setFont(asteraFontLarge);
    secondsLabel.setFont(asteraFontLarge);

    hoursLabel.setJustificationType(juce::Justification::centred);
    minutesLabel.setJustificationType(juce::Justification::centred);
    secondsLabel.setJustificationType(juce::Justification::centred);
    
    // Setup date labels
    for (int i = 0; i < 3; ++i)
    {
        addAndMakeVisible(dateLabels[i]);
        dateLabels[i].setFont(asteraFontSmall);
        dateLabels[i].setJustificationType(juce::Justification::centred);
    }
    
    // Initialize play/pause button
    addAndMakeVisible(playPauseButton);
    playPauseButton.setButtonText(""); // Clear text as we'll use images

    // Load SVG assets
    std::unique_ptr<juce::Drawable> playSvg = juce::Drawable::createFromImageData(BinaryData::Play_Button_svg, 
                                                                                 BinaryData::Play_Button_svgSize);
    std::unique_ptr<juce::Drawable> playPressedSvg = juce::Drawable::createFromImageData(BinaryData::Play_Button_Pressed_svg, 
                                                                                        BinaryData::Play_Button_Pressed_svgSize);
    std::unique_ptr<juce::Drawable> pauseSvg = juce::Drawable::createFromImageData(BinaryData::Pause_Button_svg, 
                                                                                  BinaryData::Pause_Button_svgSize);
    std::unique_ptr<juce::Drawable> pausePressedSvg = juce::Drawable::createFromImageData(BinaryData::Pause_Button_Pressed_svg, 
                                                                                         BinaryData::Pause_Button_Pressed_svgSize);

    // Make button background transparent
    playPauseButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    playPauseButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Store the drawables in the button with their pressed states
    playPauseButton.setImages(playSvg.get(),          // normal
                             nullptr,                  // over (use normal)
                             playPressedSvg.get(),     // down
                             nullptr,                  // disabled (use normal)
                             pauseSvg.get(),          // normal (on)
                             nullptr,                  // over (on) (use normal)
                             pausePressedSvg.get(),    // down (on)
                             nullptr);                 // disabled (on) (use normal)

    // Set initial state
    playPauseButton.setToggleState(audioProcessor.isTracking, juce::dontSendNotification);

    playPauseButton.onClick = [this]() {
        if (audioProcessor.isTracking) {
            audioProcessor.stopTracking();
            playPauseButton.setToggleState(false, juce::dontSendNotification);
        } else {
            audioProcessor.startTracking();
            playPauseButton.setToggleState(true, juce::dontSendNotification);
        }
    };
    
    // Initialize theme toggle button
    addAndMakeVisible(themeToggleButton);
    themeToggleButton.setButtonText(audioProcessor.isDarkMode() ? "L" : "D");
    themeToggleButton.onClick = [this]() {
        bool isDark = customLookAndFeel.isDarkMode();
        customLookAndFeel.setDarkMode(!isDark);
        audioProcessor.setDarkMode(!isDark);  // Save the state
        themeToggleButton.setButtonText(!isDark ? "L" : "D");
        repaint();
    };
    
    // Set a fixed size for our editor
    setSize(600, 450);
    
    // Start the timer for updates
    startTimerHz(1);
}

void KronosAudioProcessorEditor::timerCallback()
{
    auto seconds = audioProcessor.getTotalTimeInSeconds();
    auto hours = seconds / 3600;
    auto minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    
    hoursLabel.setText(juce::String::formatted("%02d", (int)hours), 
                      juce::dontSendNotification);
    minutesLabel.setText(juce::String::formatted("%02d", (int)minutes), 
                        juce::dontSendNotification);
    secondsLabel.setText(juce::String::formatted("%02d", (int)seconds), 
                        juce::dontSendNotification);
    
    // Update date labels with MM-DD-YYYY and time
    auto& dates = audioProcessor.getSessionDates();
    for (int i = 0; i < 3; ++i)
    {
        if (i < dates.size())
        {
            auto dateSeconds = audioProcessor.getTimeForDate(dates[i]);
            auto dateHours = dateSeconds / 3600;
            auto dateMinutes = (dateSeconds % 3600) / 60;
            dateSeconds = dateSeconds % 60;
            
            juce::String timeStr = juce::String::formatted("%02d:%02d:%02d", 
                                 (int)dateHours, (int)dateMinutes, (int)dateSeconds);
            
            dateLabels[i].setText(dates[i].formatted("%m-%d-%Y") + " - " + timeStr,
                                juce::dontSendNotification);
        }
        else
        {
            dateLabels[i].setText("", juce::dontSendNotification);
        }
    }
}

void KronosAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto buttonHeight = 60;
    auto margin = 10;
    auto dateHeight = 20;
    auto titleHeight = 80;
    auto playPauseWidth = 60;

    // Title area
    bounds.removeFromTop(titleHeight);
    bounds.removeFromTop(margin * 3);

    // Time display area - store the bounds
    timeDisplayBounds = bounds.removeFromTop(buttonHeight * 2.5);  // Store in member variable

    // Calculate center positions
    auto centerX = getWidth() / 2;
    auto timeWidth = 400.0f;  // Increased time display width
    
    // Position play button halfway between window edge and time display
    auto timeDisplayLeft = centerX - (timeWidth / 2);
    auto playButtonX = timeDisplayLeft / 2 - (playPauseWidth / 2);  // Center in left space
    auto playButtonY = timeDisplayBounds.getCentreY() - (buttonHeight / 3.3);
    playPauseButton.setBounds(playButtonX, playButtonY, playPauseWidth, buttonHeight);

    // Center time label
    auto timeLabelWidth = timeWidth - margin * 2;
    auto timeLabelHeight = buttonHeight * 2;     // Taller for bigger display
    auto timeDisplayArea = timeDisplayBounds.toFloat();
    auto labelWidth = timeLabelWidth / 3;  // Split into thirds
    auto labelHeight = timeLabelHeight;

    // Position the labels within the time display
    hoursLabel.setBounds(centerX - timeLabelWidth/2,
                        timeDisplayBounds.getCentreY() - (labelHeight / 2),
                        labelWidth,
                        labelHeight);

    minutesLabel.setBounds(centerX - labelWidth/2,
                          timeDisplayBounds.getCentreY() - (labelHeight / 2),
                          labelWidth,
                          labelHeight);

    secondsLabel.setBounds(centerX + timeLabelWidth/2 - labelWidth,
                          timeDisplayBounds.getCentreY() - (labelHeight / 2),
                          labelWidth,
                          labelHeight);

    // Move dates even further down
    auto bottomSection = bounds.removeFromBottom(140);  // Keep this the same
    bottomSection.removeFromTop(margin * 6);  // Increased from 4 to 6 margins for more space
    bottomSection.removeFromBottom(margin);
    
    // Position date labels in the bottom section
    for (int i = 0; i < 3; ++i)
    {
        dateLabels[i].setBounds(bottomSection.removeFromTop(dateHeight));
    }

    // Position theme toggle button
    auto buttonSize = 50;
    themeToggleButton.setBounds(getWidth() - buttonSize - margin,
                               getHeight() - buttonSize - margin,
                               buttonSize, buttonSize);
}

KronosAudioProcessorEditor::~KronosAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
    playPauseButton.onClick = nullptr;
}

void KronosAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Draw custom metallic border
    auto bounds = getLocalBounds().toFloat();
    float borderThickness = 4.0f;
    
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

    // Load and draw background SVG slightly inset
    auto backgroundSvg = juce::Drawable::createFromImageData(BinaryData::Background_svg, 
                                                           BinaryData::Background_svgSize);
    if (backgroundSvg != nullptr)
    {
        float padding = borderThickness + 1.0f;
        auto paddedBounds = bounds.reduced(padding);
        
        backgroundSvg->drawWithin(g, paddedBounds, 
                                juce::RectanglePlacement::centred | 
                                juce::RectanglePlacement::stretchToFit, 
                                1.0f);
    }

    // Draw grit texture overlay
    auto gritImage = juce::ImageCache::getFromMemory(BinaryData::Grit_jpg, 
                                                    BinaryData::Grit_jpgSize);
    if (gritImage.isValid())
    {
        // Create a copy of the image that we can modify
        juce::Image gritCopy = gritImage.createCopy();
        
        // Adjust the alpha of the entire image
        gritCopy.multiplyAllAlphas(0.035f);
        
        // Create a slightly reduced bounds to fit inside border
        auto gritBounds = bounds.reduced(borderThickness + 1.0f);
        
        g.drawImage(gritCopy, gritBounds,
                   juce::RectanglePlacement::stretchToFit);
    }

    // Time Display SVG - position independently from labels
    auto timeDisplaySvg = juce::Drawable::createFromImageData(BinaryData::Time_Display_svg, 
                                                            BinaryData::Time_Display_svgSize);
    if (timeDisplaySvg != nullptr)
    {
        float desiredWidth = 400.0f;
        float desiredHeight = 170.0f;
        
        // Position SVG relative to window rather than labels
        float x = getWidth()/2 - desiredWidth/2;
        float y = timeDisplayBounds.getCentreY() - desiredHeight/2;  // You can adjust this Y value
        
        juce::Rectangle<float> scaledArea(x, y, desiredWidth, desiredHeight);
        
        timeDisplaySvg->drawWithin(g, scaledArea,
                                 juce::RectanglePlacement::centred | 
                                 juce::RectanglePlacement::stretchToFit,
                                 1.0f);
    }

    // Load and draw Previous Sessions SVG
    auto previousSessionsSvg = juce::Drawable::createFromImageData(BinaryData::Previous_Sessions_svg, 
                                                                 BinaryData::Previous_Sessions_svgSize);
    if (previousSessionsSvg != nullptr)
    {
        // Create area for previous sessions at bottom
        float desiredWidth = 300.0f;  // Adjust as needed
        float desiredHeight = 100.0f;  // Adjust as needed
        
        float x = getWidth() / 2.0f - (desiredWidth / 2.0f);
        float y = getHeight() - desiredHeight - 10.0f;  // 10px from bottom
        
        juce::Rectangle<float> sessionsArea(x, y, desiredWidth, desiredHeight);
        
        previousSessionsSvg->drawWithin(g, sessionsArea,
                                      juce::RectanglePlacement::centred | 
                                      juce::RectanglePlacement::stretchToFit,
                                      1.0f);
    }

    // Load and draw header SVG
    auto headerSvg = juce::Drawable::createFromImageData(BinaryData::Header_svg,
                                                       BinaryData::Header_svgSize);
    if (headerSvg != nullptr)
    {
        auto headerArea = bounds.removeFromTop(80);
        float originalWidth = 400.0f;
        float originalHeight = 60.0f;
        
        float x = (getWidth() - originalWidth) / 2.0f;
        float y = 10.0f;
        
        juce::Rectangle<float> headerBounds(x, y, originalWidth, originalHeight);
        headerSvg->drawWithin(g, headerBounds, 
                            juce::RectanglePlacement::centred, 
                            1.0f);
    }

    // Draw title text - adjusted position and size
    auto asteraFont = juce::Font(24.0f);  // Increased from 20.0f
    asteraFont.setTypefaceName("ASTERA");
    g.setFont(asteraFont);
    g.setColour(getLookAndFeel().findColour(juce::Label::textColourId));
    g.drawText("KRONOS", juce::Rectangle<int>(0, 17, getWidth(), 50),  // Moved from 15 to 17
               juce::Justification::centred, true);
}

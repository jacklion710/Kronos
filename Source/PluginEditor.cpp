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

    addAndMakeVisible(sortModeButton);
    sortModeButton.setButtonText("Sort by Time");
    sortModeButton.onClick = [this]() {
        audioProcessor.toggleDateSortMode();
        updateSortButtonText();
        updateDateLabels();
    };

    // Initialize visual mode toggle button
    addAndMakeVisible(visualModeButton);
    visualModeButton.setButtonText("Show Bars");
    visualModeButton.onClick = [this]() {
        toggleVisualMode();
    };

    // Add mouse listener
    addMouseListener(this, true);

    // Initialize scroll buttons with the smallest triangles
    addAndMakeVisible(scrollUpButton);
    addAndMakeVisible(scrollDownButton);
    scrollUpButton.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xb4"));    // SINGLE UP POINTING ANGLE QUOTATION MARK
    scrollDownButton.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xb7")); // SINGLE DOWN POINTING ANGLE QUOTATION MARK
    
    scrollUpButton.addListener(this);
    scrollDownButton.addListener(this);

    // Initialize visual states from processor
    showBars = audioProcessor.isShowBarsEnabled();
    visualModeButton.setButtonText(showBars ? "Show Times" : "Show Bars");
    
    updateSortButtonText(); // This will reflect the loaded sort mode

    // Add the unit labels
    addAndMakeVisible(hourUnitLabel);
    addAndMakeVisible(minuteUnitLabel);
    addAndMakeVisible(secondUnitLabel);

    hourUnitLabel.setFont(asteraFontSmall.withHeight(18.0f));
    minuteUnitLabel.setFont(asteraFontSmall.withHeight(18.0f));
    secondUnitLabel.setFont(asteraFontSmall.withHeight(18.0f));

    juce::Colour labelColor(0xE6, 0xE6, 0xFF);  // #E6E6FF
    hourUnitLabel.setColour(juce::Label::textColourId, labelColor);
    minuteUnitLabel.setColour(juce::Label::textColourId, labelColor);
    secondUnitLabel.setColour(juce::Label::textColourId, labelColor);

    hourUnitLabel.setJustificationType(juce::Justification::centred);
    minuteUnitLabel.setJustificationType(juce::Justification::centred);
    secondUnitLabel.setJustificationType(juce::Justification::centred);

    hourUnitLabel.setText("HOURS", juce::dontSendNotification);
    minuteUnitLabel.setText("MINUTES", juce::dontSendNotification);
    secondUnitLabel.setText("SECONDS", juce::dontSendNotification);

    juce::Colour textColor(0xE6, 0xE6, 0xFF);  // Slightly blue-tinted white (#E6E6FF)
    
    hoursLabel.setColour(juce::Label::textColourId, textColor);
    minutesLabel.setColour(juce::Label::textColourId, textColor);
    secondsLabel.setColour(juce::Label::textColourId, textColor);

    // Apply glowing effect to unit labels
    hourUnitLabel.setLookAndFeel(&customLookAndFeel.glowingLabelLookAndFeel);
    minuteUnitLabel.setLookAndFeel(&customLookAndFeel.glowingLabelLookAndFeel);
    secondUnitLabel.setLookAndFeel(&customLookAndFeel.glowingLabelLookAndFeel);

    hoursLabel.setName("TimeLabel");
    minutesLabel.setName("TimeLabel");
    secondsLabel.setName("TimeLabel");

    // Set names for date labels
    for (auto& label : dateLabels)
    {
        label.setName("DateLabel");
    }
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
    
    // Get the bounds of the Previous Sessions panel
    auto bounds = getLocalBounds();
    auto bottomSection = bounds.removeFromBottom(160);
    bottomSection.removeFromTop(margin * 5);  // Match new top margin
    bottomSection.removeFromBottom(margin);
    
    // Update date labels with scrolling
    auto dates = audioProcessor.getSortedDates();
    for (int i = 0; i < 3; ++i)
    {
        int dateIndex = i + (int)(scrollOffset / dateHeight);
        
        if (dateIndex < dates.size())
        {
            auto date = dates[dateIndex];
            auto timeSpent = audioProcessor.getTimeForDate(date);
            
            // Calculate position within the Previous Sessions panel
            float y = bottomSection.getY() + (i * dateHeight);
            
            if (showBars)
            {
                float barWidth = 70.0f;
                float startX = (getWidth() - barWidth) / 2.0f;
                dateLabels[i].setBounds(startX - 100, y, 100, dateHeight);
                dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - ", 
                                    juce::dontSendNotification);
            }
            else
            {
                auto hours = timeSpent / 3600;
                auto minutes = (timeSpent % 3600) / 60;
                auto seconds = timeSpent % 60;
                juce::String timeStr = juce::String::formatted("%02d:%02d:%02d", 
                                     (int)hours, (int)minutes, (int)seconds);
                
                float totalWidth = 250;
                float startX = (getWidth() - totalWidth) / 2.0f;
                dateLabels[i].setBounds(startX, y, totalWidth, dateHeight);
                dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - " + timeStr, 
                                    juce::dontSendNotification);
            }
            
            // Only show label if it's within the Previous Sessions panel
            dateLabels[i].setVisible(y >= bottomSection.getY() && 
                                   y + dateHeight <= bottomSection.getBottom());
        }
        else
        {
            dateLabels[i].setVisible(false);
        }
    }

    if (showBars)
    {
        repaint();
    }
}

void KronosAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto buttonHeight = 60;
    auto margin = 10;
    auto dateHeight = 25;  // Increased height remains
    auto titleHeight = 80;
    auto playPauseWidth = 60;
    auto previousSessionsHeight = 160;

    // Title area
    bounds.removeFromTop(titleHeight);
    bounds.removeFromTop(margin * 3);

    // Time display area - store the bounds
    timeDisplayBounds = bounds.removeFromTop(buttonHeight * 2.5);

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

    // Position the unit labels above the time labels
    float unitLabelHeight = 20;
    float unitLabelOffset = 0;

    hourUnitLabel.setBounds(hoursLabel.getX() - 5,  // Nudged 5 pixels to the left
                           hoursLabel.getY() - unitLabelHeight - unitLabelOffset,
                           hoursLabel.getWidth(),
                           unitLabelHeight);

    minuteUnitLabel.setBounds(minutesLabel.getX(),
                             minutesLabel.getY() - unitLabelHeight - unitLabelOffset,
                             minutesLabel.getWidth(),
                             unitLabelHeight);

    secondUnitLabel.setBounds(secondsLabel.getX() + 2,
                             secondsLabel.getY() - unitLabelHeight - unitLabelOffset,
                             secondsLabel.getWidth(),
                             unitLabelHeight);

    // Move dates section
    auto bottomSection = bounds.removeFromBottom(previousSessionsHeight);
    bottomSection.removeFromTop(margin * 5);  // Increased from 4 to move labels down
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

    // Stack the buttons in the bottom left
    auto stackedButtonWidth = 100;
    auto stackedButtonHeight = 30;
    auto stackedMargin = 10;
    
    // Sort button on top
    sortModeButton.setBounds(stackedMargin, 
                           getHeight() - (stackedButtonHeight * 2) - (stackedMargin * 2),
                           stackedButtonWidth, 
                           stackedButtonHeight);

    // Visual mode button below
    visualModeButton.setBounds(stackedMargin, 
                             getHeight() - stackedButtonHeight - stackedMargin,
                             stackedButtonWidth, 
                             stackedButtonHeight);

    // Adjust scroll button positions to match new label positions
    int scrollButtonSize = 25;
    int buttonX = getWidth() - scrollButtonSize - (margin * 11);
    int buttonsY = getHeight() - previousSessionsHeight + (margin * 6);  // Keep at 11
    
    scrollUpButton.setBounds(buttonX, buttonsY, scrollButtonSize, scrollButtonSize);
    scrollDownButton.setBounds(buttonX, buttonsY + scrollButtonSize + 5, scrollButtonSize, scrollButtonSize);
}

KronosAudioProcessorEditor::~KronosAudioProcessorEditor()
{
    removeMouseListener(this);
    setLookAndFeel(nullptr);
    stopTimer();
    playPauseButton.onClick = nullptr;
    hoursLabel.setLookAndFeel(nullptr);
    minutesLabel.setLookAndFeel(nullptr);
    secondsLabel.setLookAndFeel(nullptr);
    hourUnitLabel.setLookAndFeel(nullptr);
    minuteUnitLabel.setLookAndFeel(nullptr);
    secondUnitLabel.setLookAndFeel(nullptr);
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
        float desiredHeight = 200.0f;  // Increased from 170.0f to make it taller
        
        float x = getWidth()/2 - desiredWidth/2;
        float y = timeDisplayBounds.getCentreY() - desiredHeight/2;
        
        timeDisplaySvg->drawWithin(g, 
                                 juce::Rectangle<float>(x, y, desiredWidth, desiredHeight),
                                 juce::RectanglePlacement::centred | 
                                 juce::RectanglePlacement::stretchToFit,
                                 1.0f);
    }

    // Load and draw Previous Sessions SVG
    auto previousSessionsSvg = juce::Drawable::createFromImageData(BinaryData::Previous_Sessions_svg, 
                                                                 BinaryData::Previous_Sessions_svgSize);
    if (previousSessionsSvg != nullptr)
    {
        // Create taller area for previous sessions
        float desiredWidth = 300.0f;
        float desiredHeight = 140.0f;  // Increased from 100.0f to match new panel height
        
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
        float originalWidth = 400.0f;
        float originalHeight = 60.0f;
        
        float x = (getWidth() - originalWidth) / 2.0f;
        float y = 10.0f;
        
        headerSvg->drawWithin(g, 
                            juce::Rectangle<float>(x, y, originalWidth, originalHeight),
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

    if (showBars)
    {
        drawTimeBars(g);
    }
}

void KronosAudioProcessorEditor::updateSortButtonText()
{
    if (audioProcessor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostRecent)
        sortModeButton.setButtonText("Sort by Time");
    else
        sortModeButton.setButtonText("Sort by Date");
}

void KronosAudioProcessorEditor::updateDateLabels()
{
    auto dates = audioProcessor.getSortedDates();
    
    for (int i = 0; i < 3; ++i)
    {
        if (i < dates.size())
        {
            auto date = dates[i];
            auto timeSpent = audioProcessor.getTimeForDate(date);
            
            if (showBars)
            {
                dateLabels[i].setVisible(false);
            }
            else
            {
                dateLabels[i].setVisible(true);
                auto hours = timeSpent / 3600;
                auto minutes = (timeSpent % 3600) / 60;
                auto seconds = timeSpent % 60;
                
                juce::String timeStr = juce::String::formatted("%02d:%02d:%02d", 
                                                              (int)hours, 
                                                              (int)minutes, 
                                                              (int)seconds);
                
                dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - " + timeStr, 
                                     juce::dontSendNotification);
            }
        }
        else
        {
            dateLabels[i].setVisible(false);
        }
    }
    repaint();
}

void KronosAudioProcessorEditor::toggleVisualMode()
{
    showBars = !showBars;
    audioProcessor.setShowBarsEnabled(showBars);  // Save state to processor
    visualModeButton.setButtonText(showBars ? "Show Times" : "Show Bars");
    
    // Calculate total width for centering
    float dateWidth = 120.0f;  // Width for date text
    float dashWidth = 20.0f;   // Width for " - "
    float timeWidth = 110.0f;  // Width for time text (or bar)
    float totalWidth = dateWidth + dashWidth + timeWidth;
    float startX = (getWidth() - totalWidth) / 2.0f;

    // Recalculate label bounds based on mode
    auto dates = audioProcessor.getSortedDates();
    for (int i = 0; i < 3; ++i)
    {
        if (i < dates.size())
        {
            auto labelY = dateLabels[i].getBounds().getY();
            auto labelHeight = dateLabels[i].getBounds().getHeight();
            
            if (showBars)
            {
                // Shorter width for date + dash only
                dateLabels[i].setBounds(startX, labelY, 
                                      dateWidth + dashWidth, labelHeight);
            }
            else
            {
                // Full width for date + dash + time, centered
                dateLabels[i].setBounds(startX, labelY, 
                                      totalWidth, labelHeight);
            }
        }
    }
    
    timerCallback();
}

float KronosAudioProcessorEditor::getTimeRatio(juce::int64 time, juce::int64 maxTime) const
{
    if (maxTime == 0) return 0.0f;
    return static_cast<float>(time) / static_cast<float>(maxTime);
}

void KronosAudioProcessorEditor::drawTimeBars(juce::Graphics& g)
{
    if (!showBars) return;  // Don't draw bars if we're showing times
    
    auto dates = audioProcessor.getSortedDates();
    if (dates.isEmpty()) return;

    // Find maximum time
    juce::int64 maxTime = 0;
    for (const auto& date : dates)
    {
        auto timeSpent = audioProcessor.getTimeForDate(date);
        maxTime = juce::jmax(maxTime, timeSpent);
    }

    // Get the bounds of the Previous Sessions panel
    auto bounds = getLocalBounds();
    auto bottomSection = bounds.removeFromBottom(160);
    bottomSection.removeFromTop(margin * 5);  // Match new top margin
    bottomSection.removeFromBottom(margin);

    // Draw bars for visible dates
    for (int i = 0; i < 3; ++i)
    {
        int dateIndex = i + (int)(scrollOffset / dateHeight);
        
        if (dateIndex < dates.size())
        {
            auto date = dates[dateIndex];
            auto timeSpent = audioProcessor.getTimeForDate(date);
            float ratio = getTimeRatio(timeSpent, maxTime);
            
            // Calculate bar position
            float barHeight = 16.0f;
            float maxBarWidth = 70.0f;
            float dateWidth = 120.0f;
            float dashWidth = 20.0f;
            float totalWidth = dateWidth + dashWidth + maxBarWidth;
            float startX = (getWidth() - totalWidth) / 2.0f;
            
            // Calculate Y position with scroll offset
            float y = bottomSection.getY() + (i * dateHeight);
            
            // Only draw if within bounds
            if (y >= bottomSection.getY() && y + barHeight <= bottomSection.getBottom())
            {
                // Position date label
                dateLabels[i].setBounds(startX, y, dateWidth + dashWidth, dateHeight);
                dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - ", 
                                    juce::dontSendNotification);
                dateLabels[i].setVisible(true);
                
                // Draw bar background (darker charcoal)
                float barX = startX + dateWidth + dashWidth;
                g.setColour(juce::Colour(40, 40, 40));
                g.fillRoundedRectangle(barX, y + (dateHeight - barHeight) / 2, 
                                     maxBarWidth, barHeight, 3.0f);
                
                // Draw actual bar (matching UI blue)
                float barWidth = juce::jmax(2.0f, ratio * maxBarWidth);
                g.setColour(juce::Colour(64, 64, 255));
                g.fillRoundedRectangle(barX, y + (dateHeight - barHeight) / 2, 
                                     barWidth, barHeight, 3.0f);
            }
            else
            {
                dateLabels[i].setVisible(false);
            }
        }
        else
        {
            dateLabels[i].setVisible(false);
        }
    }
}

void KronosAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event, 
                                               const juce::MouseWheelDetails& wheel)
{
    DBG("Mouse wheel moved: deltaY = " << wheel.deltaY);  // Debug output
    
    // Get the bounds of the Previous Sessions panel
    auto bounds = getLocalBounds();
    auto bottomSection = bounds.removeFromBottom(140);
    bottomSection.removeFromTop(margin * 6);
    bottomSection.removeFromBottom(margin);
    
    // Only handle scrolling if mouse is over the Previous Sessions area
    if (bottomSection.contains(event.position.toInt()))
    {
        DBG("Mouse is in Previous Sessions area");  // Debug output
        scrollOffset += wheel.deltaY * 20.0f;
        constrainScrollOffset();
        timerCallback();
    }
}

void KronosAudioProcessorEditor::constrainScrollOffset()
{
    auto dates = audioProcessor.getSortedDates();
    float maxScroll = juce::jmax(0.0f, (dates.size() - visibleDates) * dateHeight);
    scrollOffset = juce::jlimit(0.0f, maxScroll, scrollOffset);
}

void KronosAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &scrollUpButton)
    {
        scrollOffset -= dateHeight;
        constrainScrollOffset();
        timerCallback();
    }
    else if (button == &scrollDownButton)
    {
        scrollOffset += dateHeight;
        constrainScrollOffset();
        timerCallback();
    }
}

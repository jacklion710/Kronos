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
    
    // Cache ALL SVGs first
    backgroundSvgCache = juce::Drawable::createFromImageData(BinaryData::Background_Dark_svg, 
                                                           BinaryData::Background_Dark_svgSize);
    timeDisplaySvgCache = juce::Drawable::createFromImageData(BinaryData::Time_Display_Dark_svg, 
                                                            BinaryData::Time_Display_Dark_svgSize);
    previousSessionsSvgCache = juce::Drawable::createFromImageData(BinaryData::Previous_Sessions_Dark_svg, 
                                                                 BinaryData::Previous_Sessions_Dark_svgSize);
    headerSvgCache = juce::Drawable::createFromImageData(BinaryData::Header_Dark_svg,
                                                       BinaryData::Header_Dark_svgSize);
    
    // Cache play/pause button SVGs
    playSvgCache = juce::Drawable::createFromImageData(BinaryData::Play_Button_Dark_svg, 
                                                      BinaryData::Play_Button_Dark_svgSize);
    playPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Play_Button_Pressed_Dark_svg, 
                                                            BinaryData::Play_Button_Pressed_Dark_svgSize);
    pauseSvgCache = juce::Drawable::createFromImageData(BinaryData::Pause_Button_Dark_svg, 
                                                      BinaryData::Pause_Button_Dark_svgSize);
    pausePressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Pause_Button_Pressed_Dark_svg, 
                                                             BinaryData::Pause_Button_Pressed_Dark_svgSize);

    // THEN initialize play/pause button
    addAndMakeVisible(playPauseButton);
    playPauseButton.setButtonText("");
    playPauseButton.addListener(this);

    // Make button background transparent
    playPauseButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    playPauseButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Use cached SVGs for play/pause button AFTER they're initialized
    playPauseButton.setImages(playSvgCache.get(),          // normal
                             nullptr,                       // over (use normal)
                             playPressedSvgCache.get(),     // down
                             nullptr,                       // disabled (use normal)
                             pauseSvgCache.get(),          // normal (on)
                             nullptr,                       // over (on) (use normal)
                             pausePressedSvgCache.get(),    // down (on)
                             nullptr);                      // disabled (on) (use normal)

    // Set initial toggle state based on processor
    playPauseButton.setToggleState(audioProcessor.isTracking, juce::dontSendNotification);

    // Replace the onClick handler with mouseDown and mouseUp handlers
    playPauseButton.onStateChange = [this]()
    {
        // This ensures the button's visual state matches its toggle state
        repaint();
    };

    playPauseButton.onClick = nullptr; // Remove old handler
    
    playPauseButton.addMouseListener(this, false);
    
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

    // Add mouse listener
    addMouseListener(this, true);

    // Initialize scroll buttons with the smallest triangles
    addAndMakeVisible(scrollUpButton);
    addAndMakeVisible(scrollDownButton);
    scrollUpButton.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xb4"));    // SINGLE UP POINTING ANGLE QUOTATION MARK
    scrollDownButton.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xb7")); // SINGLE DOWN POINTING ANGLE QUOTATION MARK
    
    scrollUpButton.addListener(this);
    scrollDownButton.addListener(this);

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

    menuButton.setButtonText("...");
    addAndMakeVisible(menuButton);
    menuButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    menuButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xE6, 0xE6, 0xFF));  // Match other text
    menuButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xE6, 0xE6, 0xFF));   // Match when pressed
    menuButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);    // Remove border
    menuButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);  // Remove press color
    menuButton.onClick = [this]() {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&customLookAndFeel);
        menu.addItem(1, "About");
        
        menu.showMenuAsync(juce::PopupMenu::Options()
            .withTargetComponent(&menuButton),
            [this](int result) {
                if (result == 1) // About was selected
                {
                    auto* window = new juce::AlertWindow(
                        "About Kronos",
                        "Kronos v1.0.0\n"
                        "Created by Jack Lion\n"
                        "https://jacklion.com\n\n"
                        "https://jacklion.gumroad.com\n\n"
                        ""
                        "A simple time tracking plugin for your DAW.",
                        juce::MessageBoxIconType::InfoIcon);
                    
                    window->addButton("Close", 0);
                    window->setLookAndFeel(&customLookAndFeel);
                    
                    // Make the window larger with more padding
                    window->setSize(400, 250);
                    
                    // Center on screen and make it modal
                    window->centreAroundComponent(this, window->getWidth(), window->getHeight());
                    window->setAlwaysOnTop(true);
                    
                    window->enterModalState(true, juce::ModalCallbackFunction::create(
                        [window](int) {
                            window->setLookAndFeel(nullptr);
                            delete window;
                        }), true);
                }
            });
    };
}

void KronosAudioProcessorEditor::timerCallback()
{
    if (isTransitioningButton)
    {
        DBG("Timer fired - Completing transition");
        DBG("Current tracking state before action: " + juce::String(audioProcessor.isTracking ? "true" : "false"));
        
        // Now actually toggle the tracking state and immediately show opposite button
        if (audioProcessor.isTracking)
        {
            DBG("Stopping tracking and switching to play button");
            audioProcessor.stopTracking();
            playPauseButton.setToggleState(false, juce::dontSendNotification);
            playPauseButton.setImages(playSvgCache.get(),          // normal
                                    nullptr,                        // over
                                    playPressedSvgCache.get(),      // down
                                    nullptr,                        // disabled
                                    playSvgCache.get(),            // normal (on)
                                    nullptr,                        // over (on)
                                    playPressedSvgCache.get(),      // down (on)
                                    nullptr);                       // disabled (on)
        }
        else
        {
            DBG("Starting tracking and switching to pause button");
            audioProcessor.startTracking();
            playPauseButton.setToggleState(true, juce::dontSendNotification);
            playPauseButton.setImages(pauseSvgCache.get(),          // normal
                                    nullptr,                         // over
                                    pausePressedSvgCache.get(),     // down
                                    nullptr,                         // disabled
                                    pauseSvgCache.get(),            // normal (on)
                                    nullptr,                         // over (on)
                                    pausePressedSvgCache.get(),     // down (on)
                                    nullptr);                        // disabled (on)
        }
            
        DBG("Restarting regular timer");
        DBG("Final tracking state: " + juce::String(audioProcessor.isTracking ? "true" : "false"));
        isTransitioningButton = false;  // Reset flag after transition is complete
        startTimerHz(1);
        return;
    }
    
    // Handle regular timer updates
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
            
            // Calculate position within the Previous Sessions panel
            float y = bottomSection.getY() + (i * dateHeight);
            
            float barWidth = 70.0f;
            float startX = (getWidth() - barWidth) / 2.0f;
            dateLabels[i].setBounds(startX - 100, y, 100, dateHeight);
            dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - ", 
                                juce::dontSendNotification);
            
            // Only show label if it's within the Previous Sessions panel
            dateLabels[i].setVisible(y >= bottomSection.getY() && 
                                   y + dateHeight <= bottomSection.getBottom());
        }
        else
        {
            dateLabels[i].setVisible(false);
        }
    }

    // Always draw the bars
    repaint();
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

    // Adjust scroll button positions to match new label positions
    int scrollButtonSize = 25;
    int buttonX = getWidth() - scrollButtonSize - (margin * 11);
    int buttonsY = getHeight() - previousSessionsHeight + (margin * 6);  // Keep at 11
    
    scrollUpButton.setBounds(buttonX, buttonsY, scrollButtonSize, scrollButtonSize);
    scrollDownButton.setBounds(buttonX, buttonsY + scrollButtonSize + 5, scrollButtonSize, scrollButtonSize);

    // Position menu button in top-right corner
    auto menuButtonSize = 30;
    menuButton.setBounds(getWidth() - menuButtonSize - 10,  // X position (10px from right)
                        0.5,                                   // Y position (changed from 10 to 5)
                        menuButtonSize, 
                        menuButtonSize);
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
    menuButton.setLookAndFeel(nullptr);
}

void KronosAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Draw custom metallic border
    auto bounds = getLocalBounds().toFloat();
    float borderThickness = 4.0f;
    
    // Create gradient for border with multiple points for sine-wave like effect
    juce::ColourGradient borderGradient(
        juce::Colour(130, 130, 130),
        bounds.getBottomLeft(),
        juce::Colour(130, 130, 130),
        bounds.getTopRight(),
        false
    );
    
    borderGradient.addColour(0.25, juce::Colour(40, 40, 40));
    borderGradient.addColour(0.5, juce::Colour(40, 40, 40));
    borderGradient.addColour(0.75, juce::Colour(130, 130, 130));
    
    g.setGradientFill(borderGradient);
    g.drawRect(bounds, borderThickness);

    // Use cached background SVG
    if (backgroundSvgCache != nullptr)
    {
        float padding = borderThickness + 1.0f;
        auto paddedBounds = bounds.reduced(padding);
        backgroundSvgCache->drawWithin(g, paddedBounds, 
                                     juce::RectanglePlacement::centred | 
                                     juce::RectanglePlacement::stretchToFit, 
                                     1.0f);
    }

    // Draw grit texture overlay
    auto gritImage = juce::ImageCache::getFromMemory(BinaryData::Grit_jpg, 
                                                    BinaryData::Grit_jpgSize);
    if (gritImage.isValid())
    {
        juce::Image gritCopy = gritImage.createCopy();
        gritCopy.multiplyAllAlphas(0.035f);
        auto gritBounds = bounds.reduced(borderThickness + 1.0f);
        g.drawImage(gritCopy, gritBounds,
                   juce::RectanglePlacement::stretchToFit);
    }

    // Use cached time display SVG
    if (timeDisplaySvgCache != nullptr)
    {
        float desiredWidth = 400.0f;
        float desiredHeight = 200.0f;
        float x = getWidth()/2 - desiredWidth/2;
        float y = timeDisplayBounds.getCentreY() - desiredHeight/2;
        
        timeDisplaySvgCache->drawWithin(g, 
                                      juce::Rectangle<float>(x, y, desiredWidth, desiredHeight),
                                      juce::RectanglePlacement::centred | 
                                      juce::RectanglePlacement::stretchToFit,
                                      1.0f);
    }

    // Use cached Previous Sessions SVG
    if (previousSessionsSvgCache != nullptr)
    {
        float desiredWidth = 300.0f;
        float desiredHeight = 140.0f;
        float x = getWidth() / 2.0f - (desiredWidth / 2.0f);
        float y = getHeight() - desiredHeight - 10.0f;
        
        previousSessionsSvgCache->drawWithin(g, 
                                           juce::Rectangle<float>(x, y, desiredWidth, desiredHeight),
                                           juce::RectanglePlacement::centred | 
                                           juce::RectanglePlacement::stretchToFit,
                                           1.0f);
    }

    // Use cached header SVG
    if (headerSvgCache != nullptr)
    {
        float originalWidth = 400.0f;
        float originalHeight = 60.0f;
        float x = (getWidth() - originalWidth) / 2.0f;
        float y = 10.0f;
        
        headerSvgCache->drawWithin(g, 
                                 juce::Rectangle<float>(x, y, originalWidth, originalHeight),
                                 juce::RectanglePlacement::centred, 
                                 1.0f);
    }

    // Draw title text with glow effect
    auto asteraFont = juce::Font(24.0f);
    asteraFont.setTypefaceName("ASTERA");
    g.setFont(asteraFont);
    
    auto titleBounds = juce::Rectangle<int>(0, 17, getWidth(), 50);
    auto text = "KRONOS";
    
    // Draw stroke layers with a dark blue-grey color
    g.setColour(juce::Colour(30, 50, 150));  // Dark blue-grey
    
    // Increased stroke size specifically for title
    float strokeSize = 2.5f;  // Increased from 1.25f
    float positions[][2] = {
        {-strokeSize, -strokeSize},
        {-strokeSize, strokeSize},
        {strokeSize, -strokeSize},
        {strokeSize, strokeSize},
        {0, strokeSize},
        {0, -strokeSize},
        {strokeSize, 0},
        {-strokeSize, 0},
        // Add diagonal positions for thicker appearance
        {-strokeSize * 0.7f, -strokeSize * 0.7f},
        {-strokeSize * 0.7f, strokeSize * 0.7f},
        {strokeSize * 0.7f, -strokeSize * 0.7f},
        {strokeSize * 0.7f, strokeSize * 0.7f}
    };
    
    // Draw stroke positions
    for (auto& pos : positions)
    {
        auto offsetBounds = titleBounds.translated(pos[0], pos[1]);
        g.drawText(text, offsetBounds, juce::Justification::centred, true);
    }
    
    // Draw main text
    g.setColour(juce::Colour(0xE6, 0xE6, 0xFF));  // Light blue-white color
    g.drawText(text, titleBounds, juce::Justification::centred, true);

    // Draw time bars unconditionally
    drawTimeBars(g);
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
        else
        {
            dateLabels[i].setVisible(false);
        }
    }
    repaint();
}

float KronosAudioProcessorEditor::getTimeRatio(juce::int64 time, juce::int64 maxTime) const
{
    if (maxTime == 0) return 0.0f;
    return static_cast<float>(time) / static_cast<float>(maxTime);
}

void KronosAudioProcessorEditor::drawTimeBars(juce::Graphics& g)
{
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
    bottomSection.removeFromTop(margin * 5);
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
            
            // Calculate positions
            float barHeight = 16.0f;
            float maxBarWidth = 100.0f;  // Increased from 70.0f
            float dateWidth = 120.0f;
            float dashWidth = 20.0f;
            float totalWidth = dateWidth + dashWidth + maxBarWidth;
            float startX = (getWidth() - totalWidth) / 2.0f;
            
            float y = bottomSection.getY() + (i * dateHeight);
            
            if (y >= bottomSection.getY() && y + barHeight <= bottomSection.getBottom())
            {
                // Position date label
                dateLabels[i].setBounds(startX, y, dateWidth + dashWidth, dateHeight);
                dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - ", 
                                    juce::dontSendNotification);
                dateLabels[i].setVisible(true);
                
                // Draw bar background
                float barX = startX + dateWidth + dashWidth;
                g.setColour(juce::Colour(40, 40, 40));
                g.fillRoundedRectangle(barX, y + (dateHeight - barHeight) / 2, 
                                     maxBarWidth, barHeight, 3.0f);
                
                // Draw actual bar
                float barWidth = juce::jmax(2.0f, ratio * maxBarWidth);
                g.setColour(juce::Colour(64, 64, 255));
                g.fillRoundedRectangle(barX, y + (dateHeight - barHeight) / 2, 
                                     barWidth, barHeight, 3.0f);

                // Format and draw time text
                auto hours = timeSpent / 3600;
                auto minutes = (timeSpent % 3600) / 60;
                auto seconds = timeSpent % 60;
                juce::String timeStr = juce::String::formatted("%02d:%02d:%02d", 
                                                             (int)hours, (int)minutes, (int)seconds);
                
                // Draw time text centered over the bar with slight vertical offset
                g.setColour(juce::Colour(0xE6, 0xE6, 0xFF));  // Light blue-white color
                g.setFont(juce::Font("ASTERA", 14.0f, juce::Font::plain));
                float textY = y + (dateHeight - barHeight) / 2 + 2.0f; // Added 2.0f for slight downward adjustment
                g.drawText(timeStr, barX, textY, 
                          maxBarWidth, barHeight,
                          juce::Justification::centred, true);
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
    // Completely ignore the button clicked event
    if (button == &playPauseButton)
    {
        DBG("Button clicked - ignoring in favor of mouse events");
    }
}

void KronosAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.eventComponent == &playPauseButton && !isTransitioningButton)
    {
        DBG("Mouse down on button - Starting transition sequence");
        DBG("Current tracking state before transition: " + juce::String(audioProcessor.isTracking ? "true" : "false"));
        
        // Set transitioning flag first to prevent double triggers
        isTransitioningButton = true;
        
        // Force the pressed state to be visible
        if (!audioProcessor.isTracking)  // Changed condition to match the state we're going TO
        {
            DBG("Setting play pressed state (transitioning to pause)");
            playPauseButton.setImages(playPressedSvgCache.get(),    // normal
                                    nullptr,                         // over
                                    playPressedSvgCache.get(),      // down
                                    nullptr,                         // disabled
                                    playPressedSvgCache.get(),      // normal (on)
                                    nullptr,                         // over (on)
                                    playPressedSvgCache.get(),      // down (on)
                                    nullptr);                        // disabled (on)
        }
        else
        {
            DBG("Setting pause pressed state (transitioning to play)");
            playPauseButton.setImages(pausePressedSvgCache.get(),   // normal
                                    nullptr,                         // over
                                    pausePressedSvgCache.get(),     // down
                                    nullptr,                         // disabled
                                    pausePressedSvgCache.get(),     // normal (on)
                                    nullptr,                         // over (on)
                                    pausePressedSvgCache.get(),     // down (on)
                                    nullptr);                        // disabled (on)
        }
        
        startTimer(100);  // Start the transition timer
    }
}

void KronosAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    if (event.eventComponent == &playPauseButton)
    {
        // Now handle the action and state change
        if (audioProcessor.isTracking)
        {
            audioProcessor.stopTracking();
            playPauseButton.setToggleState(false, juce::dontSendNotification);
        }
        else
        {
            audioProcessor.startTracking();
            playPauseButton.setToggleState(true, juce::dontSendNotification);
        }
    }
}

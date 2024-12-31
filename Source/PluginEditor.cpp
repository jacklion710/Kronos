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
    addAndMakeVisible(timeLabel);
    timeLabel.setFont(asteraFontLarge);
    timeLabel.setJustificationType(juce::Justification::centred);
    
    // Setup date labels
    for (int i = 0; i < 3; ++i)
    {
        addAndMakeVisible(dateLabels[i]);
        dateLabels[i].setFont(asteraFontSmall);
        dateLabels[i].setJustificationType(juce::Justification::centred);
    }
    
    // Initialize play/pause button
    addAndMakeVisible(playPauseButton);
    playPauseButton.setButtonText(audioProcessor.isTracking ? "||" : ">");
    playPauseButton.onClick = [this]() {
        if (audioProcessor.isTracking) {
            audioProcessor.stopTracking();
            playPauseButton.setButtonText(">");
        } else {
            audioProcessor.startTracking();
            playPauseButton.setButtonText("||");
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
    setSize(400, 300);
    
    // Start the timer for updates
    startTimerHz(1);
}

void KronosAudioProcessorEditor::timerCallback()
{
    auto seconds = audioProcessor.getTotalTimeInSeconds();
    auto hours = seconds / 3600;
    auto minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    
    timeLabel.setText(juce::String::formatted("%02d:%02d:%02d", 
                     (int)hours, (int)minutes, (int)seconds), 
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
    auto buttonHeight = 40;
    auto margin = 10;
    auto dateHeight = 20;
    auto titleHeight = 50;  // Space for title
    auto playPauseWidth = 40;  // Width for the play/pause button

    // Title area
    bounds.removeFromTop(titleHeight);  // Reserve space for title
    bounds.removeFromTop(margin * 2);   // Extra space after title

    // Time display area
    auto timeDisplayBounds = bounds.removeFromTop(buttonHeight);
    
    // Position play/pause button to the left of the time display
    playPauseButton.setBounds(timeDisplayBounds.removeFromLeft(playPauseWidth));
    timeDisplayBounds.removeFromLeft(margin); // Add some space between button and time
    timeLabel.setBounds(timeDisplayBounds);
    
    bounds.removeFromTop(margin);
    for (int i = 0; i < 3; ++i)
    {
        dateLabels[i].setBounds(bounds.removeFromTop(dateHeight));
    }

    // Position theme toggle button in bottom right
    auto buttonSize = 30;
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
    // Fill background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Add a border
    g.setColour(getLookAndFeel().findColour(KronosLookAndFeel::outlineColourId));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f), 10.0f, 2.0f);

    // Add a title with ASTERA font
    auto asteraFont = juce::Font(20.0f);
    asteraFont.setTypefaceName("ASTERA");
    g.setFont(asteraFont);
    g.setColour(getLookAndFeel().findColour(juce::Label::textColourId));
    g.drawText("KRONOS", getLocalBounds().removeFromTop(50),
               juce::Justification::centred, true);
}

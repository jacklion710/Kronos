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
    // Apply the custom look and feel
    setLookAndFeel(&customLookAndFeel);
    
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
    
    // Update date labels with MM-DD-YYYY format
    auto& dates = audioProcessor.getSessionDates();
    for (int i = 0; i < 3; ++i)
    {
        if (i < dates.size())
        {
            dateLabels[i].setText(dates[i].formatted("%m-%d-%Y"), 
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

    // Title area
    bounds.removeFromTop(titleHeight);  // Reserve space for title
    bounds.removeFromTop(margin * 2);   // Extra space after title

    // Time display
    timeLabel.setBounds(bounds.removeFromTop(buttonHeight));
    
    bounds.removeFromTop(margin);
    for (int i = 0; i < 3; ++i)
    {
        dateLabels[i].setBounds(bounds.removeFromTop(dateHeight));
    }
}

KronosAudioProcessorEditor::~KronosAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void KronosAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Add a border
    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f), 10.0f, 2.0f);

    // Add a title with ASTERA font
    auto asteraFont = juce::Font(20.0f);
    asteraFont.setTypefaceName("ASTERA");
    g.setFont(asteraFont);
    g.drawText("KRONOS", getLocalBounds().removeFromTop(50),
               juce::Justification::centred, true);
}

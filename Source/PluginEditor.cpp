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
    
    // Create a font object using ASTERA
    auto asteraFont = juce::Font(36.0f);
    asteraFont.setTypefaceName("ASTERA"); // Set the typeface name explicitly
    
    // Add the time label
    addAndMakeVisible(timeLabel);
    timeLabel.setFont(asteraFont);
    timeLabel.setJustificationType(juce::Justification::centred);
    
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
}

void KronosAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto buttonHeight = 40;
    auto margin = 10;

    // Title area
    bounds.removeFromTop(margin);

    // Buttons area
    auto buttonArea = bounds.removeFromTop(buttonHeight);
    auto halfWidth = buttonArea.getWidth() / 2;
    
    // Time display
    bounds.removeFromTop(margin);
    timeLabel.setBounds(bounds.removeFromTop(buttonHeight));
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
    g.drawText("KRONOS", getLocalBounds().removeFromTop(30),
               juce::Justification::centred, true);
}

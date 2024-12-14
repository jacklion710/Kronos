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
    addAndMakeVisible(&startStopButton);
    addAndMakeVisible(&timeLabel);
    
    startStopButton.onClick = [this]() {
        if (!audioProcessor.isTracking)
        {
            audioProcessor.startTracking();
            startStopButton.setButtonText("Stop Tracking");
        }
        else
        {
            audioProcessor.stopTracking();
            startStopButton.setButtonText("Start Tracking");
        }
    };

    startTimer(1000); // Update every second
    
    setSize(400, 300);
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
    startStopButton.setBounds(bounds.removeFromTop(50));
    timeLabel.setBounds(bounds.removeFromTop(50));
}

KronosAudioProcessorEditor::~KronosAudioProcessorEditor()
{
    stopTimer(); // Make sure to stop the timer when the editor is destroyed
}

void KronosAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // Add a border
    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(5.0f), 10.0f, 2.0f);

    // Add a title
    g.setFont(20.0f);
    g.drawText("Kronos Time Tracker", getLocalBounds().removeFromTop(30),
               juce::Justification::centred, true);
}

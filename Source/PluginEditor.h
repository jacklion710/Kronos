/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"

//==============================================================================
/**
*/
class KronosAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    KronosAudioProcessorEditor (KronosAudioProcessor&);
    ~KronosAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    KronosAudioProcessor& audioProcessor;

    KronosLookAndFeel customLookAndFeel;

    juce::Label hoursLabel;
    juce::Label minutesLabel;
    juce::Label secondsLabel;

    juce::Label dateLabels[3];  // Labels for recent dates

    juce::DrawableButton playPauseButton {"PlayPauseButton", juce::DrawableButton::ButtonStyle::ImageFitted};
    juce::TextButton themeToggleButton;

    juce::Rectangle<int> timeDisplayBounds;

    juce::TextButton sortModeButton;

    void updateSortButtonText();
    void updateDateLabels();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KronosAudioProcessorEditor)
};

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
class KronosAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer, public juce::Button::Listener
{
public:
    KronosAudioProcessorEditor (KronosAudioProcessor&);
    ~KronosAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseWheelMove(const juce::MouseEvent& event, 
                       const juce::MouseWheelDetails& wheel) override;
    void buttonClicked(juce::Button* button) override;

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

    bool showBars = false;
    juce::TextButton visualModeButton;
    void drawTimeBars(juce::Graphics& g);
    void toggleVisualMode();
    float getTimeRatio(juce::int64 time, juce::int64 maxTime) const;

    void updateSortButtonText();
    void updateDateLabels();

    float scrollOffset = 0.0f;
    const float dateHeight = 30.0f;  // Height of each date entry
    const int visibleDates = 3;      // Number of dates visible at once
    const float margin = 10.0f;        // Standard margin size
    
    void constrainScrollOffset();     // Helper to keep scrolling in bounds

    juce::TextButton scrollUpButton;
    juce::TextButton scrollDownButton;

    juce::Label hourUnitLabel;
    juce::Label minuteUnitLabel;
    juce::Label secondUnitLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KronosAudioProcessorEditor)
};

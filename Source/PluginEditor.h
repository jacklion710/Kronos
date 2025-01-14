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

    void drawTimeBars(juce::Graphics& g);

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

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

    juce::TextButton menuButton;

    std::unique_ptr<juce::Drawable> backgroundSvgCache;
    std::unique_ptr<juce::Drawable> timeDisplaySvgCache;
    std::unique_ptr<juce::Drawable> previousSessionsSvgCache;
    std::unique_ptr<juce::Drawable> headerSvgCache;
    std::unique_ptr<juce::Drawable> playSvgCache;
    std::unique_ptr<juce::Drawable> playPressedSvgCache;
    std::unique_ptr<juce::Drawable> pauseSvgCache;
    std::unique_ptr<juce::Drawable> pausePressedSvgCache;

    juce::Image gritTextureCache;

    juce::ColourGradient borderGradientCache;

    bool needsRepaint = true;

    // Add this to track button state transition
    bool isTransitioningButton = false;
    const int buttonTransitionDelay = 50; // milliseconds

    std::unique_ptr<juce::Drawable> backgroundLightSvgCache;
    std::unique_ptr<juce::Drawable> timeDisplayLightSvgCache;
    std::unique_ptr<juce::Drawable> previousSessionsLightSvgCache;
    std::unique_ptr<juce::Drawable> headerLightSvgCache;
    std::unique_ptr<juce::Drawable> playLightSvgCache;
    std::unique_ptr<juce::Drawable> playPressedLightSvgCache;
    std::unique_ptr<juce::Drawable> pauseLightSvgCache;
    std::unique_ptr<juce::Drawable> pausePressedLightSvgCache;

    void updateButtonImages();

    std::unique_ptr<juce::Drawable> createNormalizedDrawable(juce::Drawable* source, float targetSize);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KronosAudioProcessorEditor)
};

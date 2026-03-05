/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "AboutComponent.h"
#include "BackupComponent.h"
#include "RestoreComponent.h"

//==============================================================================
/**
*/
class KronosAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer, public juce::Button::Listener, private juce::AudioProcessorValueTreeState::Listener
{
public:
    KronosAudioProcessorEditor (KronosAudioProcessor&);
    ~KronosAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void buttonClicked(juce::Button* button) override;

    void drawTimeBars(juce::Graphics& g);

    void mouseUp(const juce::MouseEvent& event) override;

    // Add parameter callback
    void parameterChanged(const juce::String& parameterID, float newValue) override;

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
    juce::DrawableButton themeToggleButton {"ThemeToggleButton", juce::DrawableButton::ButtonStyle::ImageFitted};

    juce::Rectangle<int> timeDisplayBounds;

    juce::DrawableButton sortModeButton{"SortButton", juce::DrawableButton::ButtonStyle::ImageFitted};

    float getTimeRatio(juce::int64 time, juce::int64 maxTime) const;

    void updateSortButtonText();
    void updateDateLabels();

    float scrollOffset = 0.0f;
    const float dateHeight = 30.0f;  // Height of each date entry
    const int visibleDates = 3;      // Number of dates visible at once
    const float margin = 10.0f;        // Standard margin size
    
    void constrainScrollOffset();     // Helper to keep scrolling in bounds

    juce::DrawableButton scrollUpButton{"Up", juce::DrawableButton::ImageFitted};
    juce::DrawableButton scrollDownButton{"Down", juce::DrawableButton::ImageFitted};

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

    juce::Image gritTexture; // Pre-processed texture
    juce::Rectangle<float> previousSessionsBounds;
    juce::Rectangle<float> headerBounds;

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

    std::unique_ptr<juce::Drawable> darkModeSvgCache;
    std::unique_ptr<juce::Drawable> darkModePressedSvgCache;
    std::unique_ptr<juce::Drawable> lightModeSvgCache;
    std::unique_ptr<juce::Drawable> lightModePressedSvgCache;

    void updateThemeButtonImages();

    const float targetButtonSize = 25.0f;

    std::unique_ptr<juce::Drawable> sortTimeDarkSvgCache;
    std::unique_ptr<juce::Drawable> sortTimeDarkPressedSvgCache;
    std::unique_ptr<juce::Drawable> sortTimeLightSvgCache;
    std::unique_ptr<juce::Drawable> sortTimeLightPressedSvgCache;
    std::unique_ptr<juce::Drawable> sortRecencyDarkSvgCache;
    std::unique_ptr<juce::Drawable> sortRecencyDarkPressedSvgCache;
    std::unique_ptr<juce::Drawable> sortRecencyLightSvgCache;
    std::unique_ptr<juce::Drawable> sortRecencyLightPressedSvgCache;

    void updateSortButtonImages();

    std::unique_ptr<juce::Drawable> upArrowDarkSvgCache;
    std::unique_ptr<juce::Drawable> upArrowDarkPressedSvgCache;
    std::unique_ptr<juce::Drawable> upArrowLightSvgCache;
    std::unique_ptr<juce::Drawable> upArrowLightPressedSvgCache;
    std::unique_ptr<juce::Drawable> downArrowDarkSvgCache;
    std::unique_ptr<juce::Drawable> downArrowDarkPressedSvgCache;
    std::unique_ptr<juce::Drawable> downArrowLightSvgCache;
    std::unique_ptr<juce::Drawable> downArrowLightPressedSvgCache;

    bool isAtTop = true;  // Start at top by default
    bool isAtBottom = false;
    
    void updateScrollButtonImages();
    void updateScrollButtonStates();
    void applyParameterChange(const juce::String& parameterID, float newValue);

    // Add scale factor
    float scale = 1.0f;

    juce::Label titleLabel;  // Add title label declaration

    std::unique_ptr<AboutComponent> aboutComponent;
    std::unique_ptr<BackupComponent> backupComponent;
    std::unique_ptr<RestoreComponent> restoreComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KronosAudioProcessorEditor)
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "KronosViews.h"

//==============================================================================
/**
*/
class KronosAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                    private juce::Timer,
                                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit KronosAudioProcessorEditor (KronosAudioProcessor&);
    ~KronosAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    enum class Sheet { backup, restore, about };

    void openSheet(Sheet sheet, bool animate = true);
    void setMenuOpen(bool shouldBeOpen);
    void scrollSessionsToRow(int row) { sessions.scrollToRow(row); }

    // Stills need every animation at rest.
    void setMotionEnabled(bool shouldAnimate) { motionEnabled = shouldAnimate; }
    KronosSheet* getOpenSheet() const { return sheetHost.getSheet(); }

    enum class Control { play, theme, menu, sortRecent, sortLongest, modeManual, modeAuto, backupItem, restoreItem, aboutItem };
    juce::Button& getControl(Control control);
    bool isShowingIdle() const { return header.isIdle(); }

    // Brings the display up to date immediately rather than on the next tick.
    void refreshNow();

private:
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void applyParameterChange(const juce::String& parameterID);

    void applyTheme(bool isDark);
    void updateFrameRate();
    void updateReadouts();
    bool shouldAnimate() const;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    KronosAudioProcessor& audioProcessor;

    KronosLookAndFeel customLookAndFeel;

    // Laid out at 600x450 and scaled as one, so nothing else knows the window size.
    juce::Component content;
    WeaveBackground background;
    HeaderBar header;
    ReadoutPanel readout;
    SessionsPanel sessions;
    MenuDismissArea menuDismiss;
    MenuPanel menu;
    KronosSheetHost sheetHost;

    juce::Image themeFadeImage;
    Tween themeFade;

    double lastTickTime = 0.0;
    int currentFrameRate = 0;
    int ticksSinceRefresh = 0;
    bool wasTracking = false;
    bool wasCounting = false;
    bool motionEnabled = true;
    bool menuOpenedByKeyboard = false;
    juce::int64 lastSheetTotal = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KronosAudioProcessorEditor)
};

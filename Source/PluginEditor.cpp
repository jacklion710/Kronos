/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "AboutComponent.h"
#include "BackupComponent.h"
#include "RestoreComponent.h"

namespace
{
constexpr int baseWidth = 600, baseHeight = 450;
constexpr int ambientFrameRate = 30, restingFrameRate = 1;
}

//==============================================================================
KronosAudioProcessorEditor::KronosAudioProcessorEditor (KronosAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), sessions (p)
{
    customLookAndFeel.setDarkMode(audioProcessor.isDarkMode());
    setLookAndFeel(&customLookAndFeel);

    setOpaque(true);
    setResizable(true, true); // First true enables resizing, second true enables fixed aspect ratio
    setResizeLimits(400, 300, 1200, 900); // Min and max sizes
    getConstrainer()->setFixedAspectRatio(static_cast<double>(baseWidth) / baseHeight);

    setFocusContainerType(FocusContainerType::keyboardFocusContainer);
    addAndMakeVisible(content);

    content.addAndMakeVisible(background);
    content.addAndMakeVisible(header);
    content.addAndMakeVisible(readout);
    content.addAndMakeVisible(sessions);
    content.addChildComponent(menuDismiss);
    content.addChildComponent(menu);
    content.addChildComponent(sheetHost);

    content.setBounds(0, 0, baseWidth, baseHeight);
    background.setBounds(content.getLocalBounds());
    header.setBounds(16, 10, 568, 36);
    readout.setBounds(16, 56, 568, 188);
    sessions.setBounds(16, 256, 568, 178);
    menuDismiss.setBounds(content.getLocalBounds());
    menu.setBounds(baseWidth - 16 - 176, 48, 176, 125);
    sheetHost.setBounds(content.getLocalBounds());

    readout.dial.playButton.onActivated = [this]
    {
        if (audioProcessor.isTracking())
            audioProcessor.stopTracking();
        else
            audioProcessor.startTracking();

        refreshNow();
    };

    header.themeButton.onActivated = [this] { audioProcessor.setDarkMode(!audioProcessor.isDarkMode()); };
    header.menuButton.onActivated = [this] { setMenuOpen(!menu.isVisible()); };

    menu.backupItem.onActivated = [this] { openSheet(Sheet::backup); };
    menu.restoreItem.onActivated = [this] { openSheet(Sheet::restore); };
    menu.aboutItem.onActivated = [this] { openSheet(Sheet::about); };
    menu.onDismiss = [this]
    {
        setMenuOpen(false);

        if (menuOpenedByKeyboard)
            header.menuButton.grabKeyboardFocus();
    };
    menuDismiss.onDismiss = [this] { setMenuOpen(false); };

    sheetHost.onClosed = [this]
    {
        refreshNow();

        // A mouse user keeps their host's keyboard focus, so Space stays with the transport
        if (menuOpenedByKeyboard)
            header.menuButton.grabKeyboardFocus();
    };

    using SortMode = KronosAudioProcessor::DateSortMode;
    sessions.recentButton.onActivated = [this] { audioProcessor.setDateSortMode(SortMode::MostRecent); };
    sessions.longestButton.onActivated = [this] { audioProcessor.setDateSortMode(SortMode::MostTime); };

    themeFade.jumpTo(0.0f);
    themeFade.onChange = [this]
    {
        if (themeFade.get() <= 0.0f)
            themeFadeImage = {};

        repaint();
    };

    header.setDark(audioProcessor.isDarkMode());
    wasTracking = audioProcessor.isTracking();
    header.setTracking(wasTracking, false);
    sessions.showSortMode(false);
    refreshNow();

    // Add parameter listeners
    audioProcessor.parameters->addParameterListener("tracking", this);
    audioProcessor.parameters->addParameterListener("darkMode", this);
    audioProcessor.parameters->addParameterListener("dateSortMode", this);

    setSize(baseWidth, baseHeight);
    lastTickTime = juce::Time::getMillisecondCounterHiRes();
    updateFrameRate();
}

KronosAudioProcessorEditor::~KronosAudioProcessorEditor()
{
    stopTimer();

    // Remove parameter listeners
    audioProcessor.parameters->removeParameterListener("tracking", this);
    audioProcessor.parameters->removeParameterListener("darkMode", this);
    audioProcessor.parameters->removeParameterListener("dateSortMode", this);

    setLookAndFeel(nullptr);
}

void KronosAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(findColour(KronosLookAndFeel::groundColourId));
}

void KronosAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    if (themeFadeImage.isValid())
    {
        g.setOpacity(themeFade.get());
        g.drawImage(themeFadeImage, getLocalBounds().toFloat());
    }
}

void KronosAudioProcessorEditor::resized()
{
    content.setTransform(juce::AffineTransform::scale(static_cast<float>(getWidth()) / baseWidth));
}

void KronosAudioProcessorEditor::visibilityChanged()
{
    updateFrameRate();
}

void KronosAudioProcessorEditor::parentHierarchyChanged()
{
    updateFrameRate();
}

bool KronosAudioProcessorEditor::shouldAnimate() const
{
    return audioProcessor.isTracking() && isShowing();
}

void KronosAudioProcessorEditor::updateFrameRate()
{
    const auto rate = shouldAnimate() ? ambientFrameRate : restingFrameRate;

    if (rate != currentFrameRate)
    {
        currentFrameRate = rate;
        lastTickTime = juce::Time::getMillisecondCounterHiRes();
        startTimerHz(rate);
    }
}

void KronosAudioProcessorEditor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto elapsed = juce::jmin(0.25, (now - lastTickTime) / 1000.0);
    lastTickTime = now;

    updateFrameRate();

    if (shouldAnimate())
    {
        readout.dial.advance(elapsed);
        header.advance(elapsed);
    }

    // The numbers only change once a second, whatever the frame rate is
    if (currentFrameRate == restingFrameRate || ++ticksSinceRefresh >= ambientFrameRate / 3)
    {
        ticksSinceRefresh = 0;
        updateReadouts();
    }
}

void KronosAudioProcessorEditor::updateReadouts()
{
    ReadoutPanel::Values values;
    values.total = audioProcessor.getTotalTimeInSeconds();
    values.run = audioProcessor.getCurrentRunSeconds();
    values.today = audioProcessor.getTodaySeconds();
    values.average = audioProcessor.getDailyAverageSeconds();
    values.longest = audioProcessor.getLongestDaySeconds();
    values.tracking = audioProcessor.isTracking();

    readout.setValues(values);
    sessions.refresh(false);

    // Open sheets show the live total
    if (values.total != lastSheetTotal)
    {
        lastSheetTotal = values.total;

        if (auto* sheet = sheetHost.getSheet())
            sheet->repaint();
    }
}

void KronosAudioProcessorEditor::refreshNow()
{
    const auto tracking = audioProcessor.isTracking();

    header.setTracking(tracking, motionEnabled && tracking && !wasTracking);
    wasTracking = tracking;

    updateReadouts();
    sessions.refresh(true);
    updateFrameRate();
}

juce::Button& KronosAudioProcessorEditor::getControl(Control control)
{
    switch (control)
    {
        case Control::play:        return readout.dial.playButton;
        case Control::theme:       return header.themeButton;
        case Control::menu:        return header.menuButton;
        case Control::sortRecent:  return sessions.recentButton;
        case Control::sortLongest: return sessions.longestButton;
        case Control::backupItem:  return menu.backupItem;
        case Control::restoreItem: return menu.restoreItem;
        case Control::aboutItem:   return menu.aboutItem;
    }

    return header.menuButton;
}

void KronosAudioProcessorEditor::setMenuOpen(bool shouldBeOpen)
{
    menuDismiss.setVisible(shouldBeOpen);
    menu.setVisible(shouldBeOpen);
    header.setMenuOpen(shouldBeOpen);

    if (shouldBeOpen)
    {
        menuDismiss.toFront(false);
        menu.toFront(false);

        menuOpenedByKeyboard = header.menuButton.hasKeyboardFocus(false);

        if (menuOpenedByKeyboard)
            menu.focusFirstItem();
        else
            menu.grabKeyboardFocus(); // so Escape still closes it
    }
}

void KronosAudioProcessorEditor::openSheet(Sheet sheet, bool animate)
{
    setMenuOpen(false);

    switch (sheet)
    {
        case Sheet::backup:  sheetHost.open(std::make_unique<BackupComponent>(audioProcessor), animate && motionEnabled); break;
        case Sheet::restore: sheetHost.open(std::make_unique<RestoreComponent>(audioProcessor), animate && motionEnabled); break;
        case Sheet::about:   sheetHost.open(std::make_unique<AboutComponent>(), animate && motionEnabled); break;
    }
}

void KronosAudioProcessorEditor::applyTheme(bool isDark)
{
    // Cross-fade from a snapshot of the old theme
    if (isShowing() && motionEnabled)
    {
        themeFadeImage = content.createComponentSnapshot(content.getLocalBounds(), true,
                                                         static_cast<float>(getWidth()) / baseWidth);
        themeFade.jumpTo(1.0f);
        themeFade.animateTo(0.0f, 340);
    }

    customLookAndFeel.setDarkMode(isDark);
    sendLookAndFeelChange();
    header.setDark(isDark);
    repaint();
}

void KronosAudioProcessorEditor::applyParameterChange(const juce::String& parameterID)
{
    if (parameterID == "tracking")
        refreshNow();
    else if (parameterID == "darkMode")
        applyTheme(audioProcessor.isDarkMode());
    else if (parameterID == "dateSortMode")
        sessions.showSortMode(motionEnabled && isShowing());
}

void KronosAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float)
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        applyParameterChange(parameterID);
        return;
    }

    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<KronosAudioProcessorEditor>(this),
                                     parameterID]() {
        if (safeThis != nullptr)
            safeThis->applyParameterChange(parameterID);
    });
}

#pragma once

#include <JuceHeader.h>
#include "KronosWidgets.h"
#include "PluginProcessor.h"

// Woven weft behind everything. It never changes between theme swaps, so it
// is cached rather than redrawn.
class WeaveBackground : public juce::Component
{
public:
    WeaveBackground();
    void paint(juce::Graphics&) override;
    void lookAndFeelChanged() override { repaint(); }
};

class HeaderBar : public juce::Component
{
public:
    HeaderBar();

    void paint(juce::Graphics&) override;
    void resized() override;

    void setTracking(bool isTracking, bool playStartGlitch);
    void setDark(bool isDark);
    void setMenuOpen(bool isOpen);
    void advance(double seconds);

    KronosButton themeButton { "Switch theme", KronosButton::Style::icon };
    KronosButton menuButton { "Menu", KronosButton::Style::icon };

private:
    void drawWordmark(juce::Graphics&);

    bool tracking = false;
    double glintPhase = 0.0, ledPhase = 0.0;
    Tween glitch;
    juce::Rectangle<float> ruleArea, chipArea, wordmarkArea;
};

class DialComponent : public juce::Component
{
public:
    DialComponent();

    void paint(juce::Graphics&) override;
    void resized() override;

    void setSeconds(juce::int64 totalSeconds);
    void setTracking(bool isTracking);
    void advance(double seconds);

    KronosButton playButton { "Start tracking", KronosButton::Style::play };

private:
    juce::Path buildInscription() const;

    juce::int64 total = 0;
    double inscriptionTurn = 0.0, hexagramTurn = 0.0;
    juce::Path inscription;
};

class ReadoutPanel : public ChamferPanel
{
public:
    ReadoutPanel();

    void paint(juce::Graphics&) override;
    void resized() override;

    struct Values
    {
        juce::int64 total = 0, run = 0, today = 0, average = 0, longest = 0;
        bool tracking = false;

        bool operator!=(const Values& other) const
        {
            return total != other.total || run != other.run || today != other.today
                || average != other.average || longest != other.longest || tracking != other.tracking;
        }
    };

    void setValues(const Values&);

    DialComponent dial;

private:
    Values values;
};

class SessionsPanel : public ChamferPanel, private juce::ListBoxModel
{
public:
    explicit SessionsPanel(KronosAudioProcessor&);

    void paint(juce::Graphics&) override;
    void resized() override;

    void refresh(bool sortOrderMayHaveChanged);
    void showSortMode(bool animate);
    void scrollToRow(int row) { list.scrollToEnsureRowIsOnscreen(row); }
    float getRowReveal(int row) const;

    KronosButton recentButton { "Sort by most recent", KronosButton::Style::segment };
    KronosButton longestButton { "Sort by longest day", KronosButton::Style::segment };

private:
    class Row;

    int getNumRows() override { return dates.size(); }
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}
    juce::Component* refreshComponentForRow(int row, bool isSelected, juce::Component* existing) override;

    KronosAudioProcessor& processor;
    KronosListBox list;
    juce::Array<juce::Time> dates;
    juce::int64 longestDay = 0;
    int daysLogged = 0;
    juce::String todayKey;
    juce::uint32 lastStateRevision = 0;
    Tween reorder;
};

class MenuPanel : public juce::Component
{
public:
    MenuPanel();

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void focusFirstItem();

    KronosButton backupItem { "Back up", KronosButton::Style::menuItem };
    KronosButton restoreItem { "Restore", KronosButton::Style::menuItem };
    KronosButton aboutItem { "About Kronos", KronosButton::Style::menuItem };
    std::function<void()> onDismiss;
};

// Catches a click outside the open menu and closes it.
class MenuDismissArea : public juce::Component
{
public:
    std::function<void()> onDismiss;
    void mouseDown(const juce::MouseEvent&) override { if (onDismiss) onDismiss(); }
};

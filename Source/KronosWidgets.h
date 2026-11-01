#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"

namespace KronosArt
{
// A rectangle with 45 degree cuts on two opposing corners. The default cuts the
// top left and bottom right, as the readout panel and buttons do.
juce::Path chamfered(juce::Rectangle<float> area, float cut, bool cutsTopRight = false);

// Lays out a path drawn in a viewBox of viewBoxSize into area.
juce::AffineTransform fit(float viewBoxSize, juce::Rectangle<float> area);

void drawSigil(juce::Graphics&, juce::Rectangle<float> area, juce::Colour edge, juce::Colour sickle, juce::Colour stem);
void drawOuroboros(juce::Graphics&, juce::Rectangle<float> area, juce::Colour body, juce::Colour head,
                   juce::Colour scales, juce::Colour stem);

// Weekday as juce::Time::getDayOfWeek() numbers it, 0 for Sunday. 14 unit viewBox, stroked.
juce::Path planetGlyph(int dayOfWeek);

juce::Path playIcon();
juce::Path pauseIcon();
juce::Path sunIcon();
juce::Path moonIcon();
juce::Path menuIcon();
juce::Path closeIcon();
juce::Path checkIcon();
juce::Path backupIcon();
juce::Path restoreIcon();
juce::Path aboutIcon();

juce::String formatDuration(juce::int64 seconds);
juce::String formatDays(int days);
juce::String formatDate(const juce::Time& date);
}

// Eases a value towards a target. It only runs while it is moving, so resting
// controls add no timers to the message thread.
class Tween : private juce::Timer
{
public:
    std::function<void()> onChange;

    void animateTo(float target, int durationMs);
    void jumpTo(float target);
    float get() const { return value; }

private:
    void timerCallback() override;

    float value = 0.0f, startValue = 0.0f, targetValue = 0.0f;
    double startedAt = 0.0;
    int duration = 1;
};

class KronosButton : public juce::Button
{
public:
    enum class Style { primary, secondary, emphasised, icon, segment, play, menuItem, link };

    KronosButton(const juce::String& accessibleName, Style style);

    void setIcon(juce::Path path, float viewBoxSize, float iconSize, float strokeWidth);
    void setDetail(const juce::String& decorativeDetail) { detail = decorativeDetail; repaint(); }
    void setActive(bool shouldBeActive);
    void setIgnoresMotion(bool shouldIgnore);
    void playLabelDecode();

    void paintButton(juce::Graphics&, bool isHighlighted, bool isDown) override;
    void buttonStateChanged() override;
    bool keyPressed(const juce::KeyPress&) override;
    bool hitTest(int x, int y) override;

    std::function<void()> onActivated;

private:
    juce::Rectangle<float> getVisualBounds() const;
    juce::String getDisplayText() const;

    Style style;
    bool active = false;
    bool ignoresMotion = false;
    juce::Path icon;
    float iconViewBox = 16.0f, iconSize = 16.0f, iconStroke = 0.0f;
    juce::String detail;
    Tween hover, press, decode;
    juce::Random decodeNoise { 11 };
};

// A list that shows the accent outline while it holds keyboard focus.
class KronosListBox : public juce::ListBox
{
public:
    void paintOverChildren(juce::Graphics&) override;
    void focusGained(FocusChangeType) override { repaint(); }
    void focusLost(FocusChangeType) override { repaint(); }
    void focusOfChildComponentChanged(FocusChangeType) override { repaint(); }
};

class ChamferPanel : public juce::Component
{
public:
    explicit ChamferPanel(bool cutsTopRight);

    void paint(juce::Graphics&) override;

private:
    bool cutsTopRight;
};

// Body text that wraps, drawn with the prototype's 1.45 line height.
void drawParagraph(juce::Graphics&, const juce::String& text, juce::Font font, juce::Colour colour,
                   juce::Rectangle<float> area);

// One sheet's content. KronosSheetHost supplies the frame, kicker and close button.
class KronosSheet : public juce::Component
{
public:
    virtual juce::String getKicker() const = 0;
    virtual juce::String getSheetTitle() const = 0;
    virtual int getPreferredHeight() const = 0;
    virtual void focusDefaultControl() {}
    virtual void cancelOrClose() { if (onClose) onClose(); }

    std::function<void()> onClose;
    std::function<void()> onLayoutChanged;
};

class KronosSheetHost : public juce::Component
{
public:
    KronosSheetHost();

    void open(std::unique_ptr<KronosSheet> sheet, bool animate);
    void close();
    bool isOpen() const { return currentSheet != nullptr; }
    KronosSheet* getSheet() const { return currentSheet.get(); }

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void mouseDown(const juce::MouseEvent&) override;

    std::function<void()> onClosed;

private:
    class Frame : public juce::Component
    {
    public:
        explicit Frame(KronosSheetHost& owner);
        void paint(juce::Graphics&) override;
        void resized() override;

        KronosSheetHost& host;
        KronosButton closeButton { "Close", KronosButton::Style::icon };
    };

    void layoutFrame();
    void applyMotion();

    Frame frame { *this };
    std::unique_ptr<KronosSheet> currentSheet;
    Tween arrival, ruleDraw;
    bool closing = false;
};

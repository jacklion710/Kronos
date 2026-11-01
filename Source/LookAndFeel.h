#pragma once
#include <JuceHeader.h>

class KronosLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // Tokens from the design system. Every colour in the UI comes from here.
    enum ColourIds
    {
        groundColourId = 0x2000000,
        surfaceColourId,
        raisedColourId,
        lineColourId,
        textColourId,
        mutedColourId,
        faintColourId,
        accentColourId,
        accentTextColourId,
        ctaColourId,
        trackColourId,
        weaveColourId,
        scrimColourId
    };

    enum class Face
    {
        sansRegular,
        sansSemibold,
        sansBold,
        monoLight,
        monoRegular,
        monoMedium
    };

    KronosLookAndFeel();

    void setDarkMode(bool isDark);
    bool isDarkMode() const { return darkModeEnabled; }

    // Sizes are CSS pixels and tracking is in em, matching the design spec.
    static juce::Font font(Face face, float size, float tracking = 0.0f);

    int getDefaultScrollbarWidth() override { return 6; }
    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;

    void drawCornerResizer(juce::Graphics&, int width, int height, bool isMouseOver, bool isMouseDragging) override;
    void fillTextEditorBackground(juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline(juce::Graphics&, int width, int height, juce::TextEditor&) override;

private:
    bool darkModeEnabled = true;
};

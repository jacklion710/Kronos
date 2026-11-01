#include "LookAndFeel.h"
#include "BinaryData.h"

namespace
{
struct Theme
{
    juce::uint32 ground, surface, raised, line, text, muted, faint, accent, accentText, cta, track, weave, scrim;
};

constexpr Theme darkTheme  { 0xff0b0c0c, 0xff101212, 0xff161919, 0xff343737, 0xffeeece9, 0xffb2b3b1, 0xff8f918f,
                             0xffef593d, 0xffef593d, 0xffbc321e, 0xff1f2323, 0x0deeece9, 0xbd050606 };

constexpr Theme lightTheme { 0xffe9e7e3, 0xfff5f4f1, 0xfffbfaf8, 0xffcbc8c2, 0xff151616, 0xff4c4e50, 0xff5f6265,
                             0xffd2452b, 0xffa82c19, 0xffa82c19, 0xffe0ddd7, 0x12151616, 0xb8e9e7e3 };

// Fonts are registered once per process and shared by every open editor.
struct Typefaces
{
    juce::Typeface::Ptr sansRegular  = make(BinaryData::SourceSans3Regular_ttf,  BinaryData::SourceSans3Regular_ttfSize);
    juce::Typeface::Ptr sansSemibold = make(BinaryData::SourceSans3Semibold_ttf, BinaryData::SourceSans3Semibold_ttfSize);
    juce::Typeface::Ptr sansBold     = make(BinaryData::SourceSans3Bold_ttf,     BinaryData::SourceSans3Bold_ttfSize);
    juce::Typeface::Ptr monoLight    = make(BinaryData::JetBrainsMonoLight_ttf,  BinaryData::JetBrainsMonoLight_ttfSize);
    juce::Typeface::Ptr monoRegular  = make(BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize);
    juce::Typeface::Ptr monoMedium   = make(BinaryData::JetBrainsMonoMedium_ttf, BinaryData::JetBrainsMonoMedium_ttfSize);

    static juce::Typeface::Ptr make(const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor(data, static_cast<size_t>(size));
    }
};

juce::Typeface::Ptr typefaceFor(KronosLookAndFeel::Face face)
{
    static const Typefaces typefaces;

    switch (face)
    {
        case KronosLookAndFeel::Face::sansRegular:  return typefaces.sansRegular;
        case KronosLookAndFeel::Face::sansSemibold: return typefaces.sansSemibold;
        case KronosLookAndFeel::Face::sansBold:     return typefaces.sansBold;
        case KronosLookAndFeel::Face::monoLight:    return typefaces.monoLight;
        case KronosLookAndFeel::Face::monoRegular:  return typefaces.monoRegular;
        case KronosLookAndFeel::Face::monoMedium:   return typefaces.monoMedium;
    }

    return typefaces.sansRegular;
}
}

KronosLookAndFeel::KronosLookAndFeel()
{
    setDarkMode(true);
}

void KronosLookAndFeel::setDarkMode(bool isDark)
{
    darkModeEnabled = isDark;
    const auto& theme = isDark ? darkTheme : lightTheme;

    setColour(groundColourId,     juce::Colour(theme.ground));
    setColour(surfaceColourId,    juce::Colour(theme.surface));
    setColour(raisedColourId,     juce::Colour(theme.raised));
    setColour(lineColourId,       juce::Colour(theme.line));
    setColour(textColourId,       juce::Colour(theme.text));
    setColour(mutedColourId,      juce::Colour(theme.muted));
    setColour(faintColourId,      juce::Colour(theme.faint));
    setColour(accentColourId,     juce::Colour(theme.accent));
    setColour(accentTextColourId, juce::Colour(theme.accentText));
    setColour(ctaColourId,        juce::Colour(theme.cta));
    setColour(trackColourId,      juce::Colour(theme.track));
    setColour(weaveColourId,      juce::Colour(theme.weave));
    setColour(scrimColourId,      juce::Colour(theme.scrim));

    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(theme.ground));
    setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(theme.surface));
    setColour(juce::TextEditor::textColourId, juce::Colour(theme.text));
    setColour(juce::TextEditor::highlightColourId, juce::Colour(theme.accent).withAlpha(0.3f));
    setColour(juce::TextEditor::highlightedTextColourId, juce::Colour(theme.text));
    setColour(juce::CaretComponent::caretColourId, juce::Colour(theme.accent));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(theme.raised));
    setColour(juce::PopupMenu::textColourId, juce::Colour(theme.text));
    setColour(juce::PopupMenu::headerTextColourId, juce::Colour(theme.muted));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(theme.accent).withAlpha(0.2f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(theme.text));
}

juce::Font KronosLookAndFeel::font(Face face, float size, float tracking)
{
    const auto font = juce::Font(typefaceFor(face)).withPointHeight(size);

    // JUCE measures extra kerning against the font height, the spec against the em
    return font.withExtraKerningFactor(tracking * size / font.getHeight());
}

void KronosLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height,
                                      bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                                      bool isMouseOver, bool isMouseDown)
{
    const auto thumb = isScrollbarVertical ? juce::Rectangle<int>(x, thumbStartPosition, width, thumbSize)
                                           : juce::Rectangle<int>(thumbStartPosition, y, thumbSize, height);

    g.setColour(findColour(mutedColourId).withAlpha(isMouseOver || isMouseDown ? 0.44f : 0.28f));
    g.fillRect(thumb);
}

void KronosLookAndFeel::drawCornerResizer(juce::Graphics& g, int width, int height, bool isMouseOver, bool isMouseDragging)
{
    const auto size = static_cast<float>(juce::jmin(width, height));

    g.setColour(findColour(isMouseOver || isMouseDragging ? mutedColourId : lineColourId));

    for (float inset = 0.35f; inset < 0.9f; inset += 0.25f)
        g.drawLine(static_cast<float>(width) - size * (1.0f - inset), static_cast<float>(height),
                   static_cast<float>(width), static_cast<float>(height) - size * (1.0f - inset), 1.0f);
}

void KronosLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int, int, juce::TextEditor&)
{
    g.fillAll(findColour(surfaceColourId));
}

void KronosLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    const auto bounds = juce::Rectangle<int>(width, height).toFloat();

    g.setColour(findColour(lineColourId));
    g.drawRect(bounds, 1.0f);

    g.setColour(findColour(accentColourId));
    if (editor.hasKeyboardFocus(true))
        g.drawRect(bounds, 2.0f);
    else
        g.fillRect(bounds.withTop(bounds.getBottom() - 2.0f));
}

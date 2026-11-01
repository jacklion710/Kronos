#include "KronosViews.h"

using KLF = KronosLookAndFeel;

namespace
{
constexpr auto twoPi = juce::MathConstants<double>::twoPi;

// Each ambient layer keeps its own period so nothing pulses in step
constexpr double inscriptionPeriod = 120.0, hexagramPeriod = 72.0, glintPeriod = 11.0, ledPeriod = 2.4;

juce::String greek(const char* utf8)
{
    return juce::String(juce::CharPointer_UTF8(utf8));
}

const auto headerInscription = "\xce\xa7\xce\xa1\xce\x9f\xce\x9d\xce\x9f\xce\xa3 \xc2\xb7 \xce\x9a\xce\x91\xce\x99\xce\xa1\xce\x9f\xce\xa3 \xc2\xb7 \xce\x91\xce\x99\xce\xa9\xce\x9d";
const auto dialInscription = "\xce\xa7\xce\xa1\xce\x9f\xce\x9d\xce\x9f\xce\xa3 \xc2\xb7 \xce\x9a\xce\x91\xce\x99\xce\xa1\xce\x9f\xce\xa3 \xc2\xb7 \xce\x91\xce\x99\xce\xa9\xce\x9d \xc2\xb7 \xce\x95\xce\x9d \xce\xa4\xce\x9f \xce\xa0\xce\x91\xce\x9d \xc2\xb7 ";
const auto chronos = "\xce\xa7\xce\xa1\xce\x9f\xce\x9d\xce\x9f\xce\xa3";
const auto kairos = "\xce\x9a\xce\x91\xce\x99\xce\xa1\xce\x9f\xce\xa3";

float widthOf(const juce::Font& font, const juce::String& text)
{
    return font.getStringWidthFloat(text);
}

juce::Path arc(float radius, double fraction)
{
    juce::Path path;
    path.addCentredArc(80.0f, 80.0f, radius, radius, 0.0f, 0.0f,
                       static_cast<float>(fraction * twoPi), true);
    return path;
}

juce::Path circle(float radius)
{
    juce::Path path;
    path.addEllipse(80.0f - radius, 80.0f - radius, radius * 2.0f, radius * 2.0f);
    return path;
}
}

//==============================================================================
WeaveBackground::WeaveBackground()
{
    setOpaque(true);
    setInterceptsMouseClicks(false, false);
    setBufferedToImage(true);
}

void WeaveBackground::paint(juce::Graphics& g)
{
    g.fillAll(findColour(KLF::groundColourId));

    static const auto tile = [] {
        return juce::Drawable::parseSVGPath("M0 8 L7 2 L14 8 L21 2 L28 8 M0 16 L7 10 L14 16 L21 10 L28 16");
    }();
    static const auto ties = juce::Drawable::parseSVGPath("M7 2 L7 10 M21 2 L21 10");

    g.setColour(findColour(KLF::weaveColourId));

    for (int y = 0; y < getHeight(); y += 16)
    {
        for (int x = 0; x < getWidth(); x += 28)
        {
            const auto offset = juce::AffineTransform::translation(static_cast<float>(x), static_cast<float>(y));
            g.strokePath(tile, juce::PathStrokeType(0.8f), offset);
            g.strokePath(ties, juce::PathStrokeType(0.5f), offset);
        }
    }
}

//==============================================================================
HeaderBar::HeaderBar()
{
    setInterceptsMouseClicks(false, true);
    setOpaque(false);

    themeButton.setIcon(KronosArt::sunIcon(), 16.0f, 16.0f, 1.2f);
    menuButton.setIcon(KronosArt::menuIcon(), 16.0f, 16.0f, 1.3f);
    addAndMakeVisible(themeButton);
    addAndMakeVisible(menuButton);

    glitch.jumpTo(1.0f);
    glitch.onChange = [this] { repaint(wordmarkArea.getSmallestIntegerContainer().expanded(6, 0)); };
}

void HeaderBar::setDark(bool isDark)
{
    themeButton.setIcon(isDark ? KronosArt::sunIcon() : KronosArt::moonIcon(), 16.0f, 16.0f, isDark ? 1.2f : 1.3f);
    themeButton.setTitle(isDark ? "Switch to light theme" : "Switch to dark theme");
}

void HeaderBar::setMenuOpen(bool isOpen)
{
    menuButton.setActive(isOpen);
}

void HeaderBar::setTracking(bool isTracking, bool playStartGlitch)
{
    tracking = isTracking;
    resized();
    repaint();

    if (playStartGlitch)
    {
        glitch.jumpTo(0.0f);
        glitch.animateTo(1.0f, 340);
    }
}

void HeaderBar::advance(double seconds)
{
    glintPhase = std::fmod(glintPhase + seconds / glintPeriod, 1.0);
    ledPhase = std::fmod(ledPhase + seconds / ledPeriod, 1.0);

    repaint(ruleArea.getSmallestIntegerContainer());
    repaint(chipArea.getSmallestIntegerContainer());
}

void HeaderBar::resized()
{
    menuButton.setBounds(getWidth() - 32, 2, 32, 32);
    themeButton.setBounds(menuButton.getX() - 44, 2, 32, 32);

    const auto chipFont = KLF::font(KLF::Face::monoRegular, 10.0f, 0.16f);
    const auto chipWidth = 10.0f + 6.0f + 8.0f + widthOf(chipFont, tracking ? "TRACKING" : "PAUSED") + 10.0f;
    chipArea = { static_cast<float>(themeButton.getX()) - 12.0f - chipWidth, 6.0f, chipWidth, 24.0f };

    const auto wordmarkFont = KLF::font(KLF::Face::sansBold, 15.0f, 0.34f);
    wordmarkArea = { 36.0f, 8.0f, widthOf(wordmarkFont, "KRONOS"), 20.0f };

    const auto inscriptionFont = KLF::font(KLF::Face::sansRegular, 9.0f, 0.42f);
    const auto inscriptionWidth = widthOf(inscriptionFont, greek(headerInscription));
    const auto inscriptionX = chipArea.getX() - 12.0f - 18.0f - 12.0f - inscriptionWidth;
    const auto ruleX = wordmarkArea.getRight() + 12.0f;

    ruleArea = { ruleX, 17.0f, juce::jmax(0.0f, inscriptionX - 12.0f - ruleX), 2.0f };
}

void HeaderBar::drawWordmark(juce::Graphics& g)
{
    const auto text = findColour(KLF::textColourId);
    const auto font = KLF::font(KLF::Face::sansBold, 15.0f, 0.34f);
    const auto progress = glitch.get();

    if (progress >= 1.0f)
    {
        g.setColour(text);
        g.setFont(font);
        g.drawText("KRONOS", wordmarkArea, juce::Justification::centredLeft, false);
        return;
    }

    // Sliced horizontal bands that settle back into place
    constexpr int bands = 7;
    const auto bandHeight = wordmarkArea.getHeight() / bands;
    juce::Random noise(juce::roundToInt(progress * 9.0f) + 3);

    for (int band = 0; band < bands; ++band)
    {
        const auto area = wordmarkArea.expanded(6.0f, 0.0f)
                                      .withY(wordmarkArea.getY() + band * bandHeight)
                                      .withHeight(bandHeight);
        const auto shift = (noise.nextFloat() - 0.5f) * 6.0f * (1.0f - progress);

        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(area.getSmallestIntegerContainer());
        g.setColour(noise.nextFloat() > 0.75f ? findColour(KLF::accentColourId) : text);
        g.setFont(font);
        g.drawText("KRONOS", wordmarkArea.translated(shift, 0.0f), juce::Justification::centredLeft, false);
    }
}

void HeaderBar::paint(juce::Graphics& g)
{
    const auto text = findColour(KLF::textColourId);
    const auto muted = findColour(KLF::mutedColourId);
    const auto faint = findColour(KLF::faintColourId);
    const auto line = findColour(KLF::lineColourId);
    const auto accent = findColour(KLF::accentColourId);

    KronosArt::drawSigil(g, { 0.0f, 6.0f, 24.0f, 24.0f }, muted, accent, text);
    drawWordmark(g);

    // A single light on the rule: no ticks, and dark for most of the cycle
    g.setColour(line);
    g.fillRect(ruleArea.withHeight(1.0f));

    if (tracking && glintPhase < 0.55)
    {
        const auto travel = static_cast<float>(glintPhase / 0.55);
        const auto opacity = juce::jmin(1.0f, travel / 0.22f) * juce::jmin(1.0f, (1.0f - travel) / 0.27f);
        const auto x = ruleArea.getX() - 40.0f + (ruleArea.getWidth() + 40.0f) * travel;
        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(ruleArea.getSmallestIntegerContainer());
        juce::ColourGradient gradient(accent.withAlpha(0.0f), x, 0.0f, accent.withAlpha(0.0f), x + 40.0f, 0.0f, false);
        gradient.addColour(0.5, accent.withAlpha(opacity));
        g.setGradientFill(gradient);
        g.fillRect(x, ruleArea.getY(), 40.0f, 1.0f);
    }

    const auto inscriptionFont = KLF::font(KLF::Face::sansRegular, 9.0f, 0.42f);
    const auto inscription = greek(headerInscription);
    const auto inscriptionWidth = widthOf(inscriptionFont, inscription);
    const auto inscriptionX = chipArea.getX() - 12.0f - 18.0f - 12.0f - inscriptionWidth;

    g.setColour(faint);
    g.setFont(inscriptionFont);
    g.drawText(inscription, juce::Rectangle<float>(inscriptionX, 8.0f, inscriptionWidth + 4.0f, 20.0f),
               juce::Justification::centredLeft, false);

    g.setColour(line);
    g.fillRect(chipArea.getX() - 12.0f - 18.0f, 17.0f, 18.0f, 1.0f);

    g.drawRect(chipArea, 1.0f);

    const auto led = juce::Rectangle<float>(6.0f, 6.0f).withCentre({ chipArea.getX() + 13.0f, chipArea.getCentreY() });

    if (tracking)
    {
        const auto breath = static_cast<float>(0.5 - 0.5 * std::cos(ledPhase * twoPi));
        const auto glow = led.expanded(1.0f + 3.0f * breath);
        g.setGradientFill(juce::ColourGradient(accent.withAlpha(0.55f * breath), glow.getCentre(),
                                               accent.withAlpha(0.0f), glow.getTopLeft().translated(0.0f, glow.getHeight() * 0.5f), true));
        g.fillEllipse(glow);
        g.setColour(accent);
        g.fillEllipse(led);
    }
    else
    {
        g.setColour(faint);
        g.drawEllipse(led.reduced(0.5f), 1.0f);
    }

    g.setColour(muted);
    g.setFont(KLF::font(KLF::Face::monoRegular, 10.0f, 0.16f));
    g.drawText(tracking ? "TRACKING" : "PAUSED", chipArea.withTrimmedLeft(24.0f), juce::Justification::centredLeft, false);
}

//==============================================================================
DialComponent::DialComponent()
{
    setOpaque(true);
    addAndMakeVisible(playButton);
    setTracking(false);
}

void DialComponent::resized()
{
    playButton.setBounds(44, 44, 72, 72);
    inscription = buildInscription();
}

juce::Path DialComponent::buildInscription() const
{
    // The prototype sets this on a path; JUCE has no text-on-a-path, so each
    // glyph is placed and turned individually, then cached.
    const auto font = KLF::font(KLF::Face::sansSemibold, 6.4f);
    const auto text = greek(dialInscription) + greek(dialInscription);
    constexpr float radius = 65.5f;

    juce::Array<int> glyphs;
    juce::Array<float> offsets;
    font.getGlyphPositions(text, glyphs, offsets);

    if (offsets.size() < 2)
        return {};

    const auto naturalWidth = offsets.getLast();
    const auto spacing = (static_cast<float>(twoPi) * radius - naturalWidth) / static_cast<float>(glyphs.size());

    juce::Path result;
    auto* typeface = font.getTypefacePtr().get();

    for (int i = 0; i < glyphs.size(); ++i)
    {
        juce::Path glyph;
        typeface->getOutlineForGlyph(glyphs[i], glyph);

        const auto position = offsets[i] + spacing * static_cast<float>(i);
        const auto angle = position / radius;
        const auto scale = font.getHeight() * font.getHorizontalScale();

        result.addPath(glyph, juce::AffineTransform::scale(scale, font.getHeight())
                                  .translated(0.0f, -radius)
                                  .rotated(angle)
                                  .translated(80.0f, 80.0f));
    }

    return result;
}

void DialComponent::setSeconds(juce::int64 totalSeconds)
{
    if (total != totalSeconds)
    {
        total = totalSeconds;
        repaint();
    }
}

void DialComponent::setTracking(bool isTracking)
{
    playButton.setActive(isTracking);
    playButton.setIcon(isTracking ? KronosArt::pauseIcon() : KronosArt::playIcon(), 22.0f, 22.0f, 0.0f);
    playButton.setTitle(isTracking ? "Pause tracking" : "Start tracking");
    repaint();
}

void DialComponent::advance(double seconds)
{
    inscriptionTurn = std::fmod(inscriptionTurn + seconds / inscriptionPeriod, 1.0);
    hexagramTurn = std::fmod(hexagramTurn - seconds / hexagramPeriod, 1.0);
    repaint();
}

void DialComponent::paint(juce::Graphics& g)
{
    const auto text = findColour(KLF::textColourId);
    const auto muted = findColour(KLF::mutedColourId);
    const auto faint = findColour(KLF::faintColourId);
    const auto accent = findColour(KLF::accentColourId);
    const auto track = findColour(KLF::trackColourId);

    g.fillAll(findColour(KLF::surfaceColourId));

    // Real values: position in the current minute and the current hour
    const auto secondFraction = static_cast<double>(total % 60) / 60.0;
    const auto minuteFraction = static_cast<double>(total % 3600) / 3600.0;

    g.setColour(track);
    g.strokePath(circle(74.0f), juce::PathStrokeType(3.0f));
    g.strokePath(circle(57.0f), juce::PathStrokeType(1.0f));

    g.setColour(accent);
    if (secondFraction > 0.0)
        g.strokePath(arc(74.0f, secondFraction), juce::PathStrokeType(3.0f));

    const auto tipAngle = secondFraction * twoPi - juce::MathConstants<double>::halfPi;
    const auto tipX = 80.0f + 74.0f * static_cast<float>(std::cos(tipAngle));
    const auto tipY = 80.0f + 74.0f * static_cast<float>(std::sin(tipAngle));
    juce::Path tip;
    tip.addRectangle(-2.6f, -2.6f, 5.2f, 5.2f);
    g.fillPath(tip, juce::AffineTransform::rotation(juce::MathConstants<float>::pi / 4.0f).translated(tipX, tipY));

    g.setColour(text.withAlpha(0.7f));
    if (minuteFraction > 0.0)
        g.strokePath(arc(57.0f, minuteFraction), juce::PathStrokeType(1.4f));

    g.setColour(faint.withAlpha(0.5f));
    g.fillPath(inscription, juce::AffineTransform::rotation(static_cast<float>(inscriptionTurn * twoPi), 80.0f, 80.0f));

    static const auto triangleUp = juce::Drawable::parseSVGPath("M80 30 L123.3 105 L36.7 105 Z");
    static const auto triangleDown = juce::Drawable::parseSVGPath("M80 130 L36.7 55 L123.3 55 Z");
    static const auto hexagon = juce::Drawable::parseSVGPath("M80 30 L123.3 55 L123.3 105 L80 130 L36.7 105 L36.7 55 Z");
    const auto turn = juce::AffineTransform::rotation(static_cast<float>(hexagramTurn * twoPi), 80.0f, 80.0f);

    g.setColour(muted.withAlpha(0.45f));
    g.strokePath(triangleUp, juce::PathStrokeType(0.7f), turn);
    g.strokePath(triangleDown, juce::PathStrokeType(0.7f), turn);

    g.setColour(accent.withAlpha(0.5f));
    g.strokePath(hexagon, juce::PathStrokeType(0.8f), turn);

    const auto node = [&](float x, float y, float radius, juce::Colour colour)
    {
        auto centre = juce::Point<float>(x, y).transformedBy(turn);
        g.setColour(colour);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    };

    node(80.0f, 30.0f, 1.6f, accent);
    node(123.3f, 105.0f, 1.2f, muted);
    node(36.7f, 105.0f, 1.2f, muted);
}

//==============================================================================
ReadoutPanel::ReadoutPanel() : ChamferPanel(false)
{
    setTitle("Project time");
    addAndMakeVisible(dial);
}

void ReadoutPanel::resized()
{
    dial.setBounds(19, 14, 160, 160);
}

void ReadoutPanel::setValues(const Values& newValues)
{
    if (values != newValues)
    {
        values = newValues;
        dial.setSeconds(values.total);
        dial.setTracking(values.tracking);
        setDescription("Project time " + KronosArt::formatDuration(values.total));
        repaint();
    }
}

void ReadoutPanel::paint(juce::Graphics& g)
{
    ChamferPanel::paint(g);

    const auto text = findColour(KLF::textColourId);
    const auto muted = findColour(KLF::mutedColourId);
    const auto faint = findColour(KLF::faintColourId);
    const auto accentText = findColour(KLF::accentTextColourId);

    constexpr float left = 203.0f, right = 545.0f, top = 19.0f;

    // Label row: Greek sits beside the English and never replaces it
    const auto greekFont = KLF::font(KLF::Face::sansSemibold, 10.0f, 0.2f);
    const auto labelFont = KLF::font(KLF::Face::monoRegular, 10.0f, 0.18f);
    const juce::Rectangle<float> labels(left, top, right - left, 13.0f);

    g.setFont(greekFont);
    g.setColour(accentText);
    g.drawText(greek(chronos), labels, juce::Justification::centredLeft, false);

    g.setFont(labelFont);
    g.setColour(muted);
    g.drawText("PROJECT TIME", labels.withTrimmedLeft(widthOf(greekFont, greek(chronos)) + 8.0f),
               juce::Justification::centredLeft, false);

    const auto runText = KronosArt::formatDuration(values.run);
    const auto runWidth = widthOf(labelFont, runText);
    const auto runLabelWidth = widthOf(labelFont, "THIS RUN ");
    const auto kairosWidth = widthOf(greekFont, greek(kairos));

    g.setColour(text);
    g.drawText(runText, labels, juce::Justification::centredRight, false);
    g.setColour(muted);
    g.drawText("THIS RUN", labels.withRight(right - runWidth - 2.0f).withLeft(right - runWidth - runLabelWidth - 2.0f),
               juce::Justification::centredLeft, false);
    g.setFont(greekFont);
    g.setColour(faint);
    g.drawText(greek(kairos), labels.withWidth(kairosWidth + 4.0f).withX(right - runWidth - runLabelWidth - kairosWidth - 10.0f),
               juce::Justification::centredLeft, false);

    // H:MM:SS. Very long totals shrink rather than clip.
    const auto hours = juce::String(values.total / 3600);
    const auto minutes = juce::String((values.total % 3600) / 60).paddedLeft('0', 2);
    const auto seconds = juce::String(values.total % 60).paddedLeft('0', 2);

    auto digitSize = 54.0f;
    const auto digitFontAt = [](float size) { return KLF::font(KLF::Face::monoLight, size, -0.02f); };
    const auto totalWidth = [&](float size)
    {
        const auto font = digitFontAt(size);
        const auto colon = KLF::font(KLF::Face::monoLight, size);
        return widthOf(font, hours) + widthOf(font, minutes) + widthOf(font, seconds) + 2.0f * widthOf(colon, ":") + 16.0f;
    };

    while (digitSize > 30.0f && totalWidth(digitSize) > right - left)
        digitSize -= 2.0f;

    const auto digitFont = digitFontAt(digitSize);
    const auto colonFont = KLF::font(KLF::Face::monoLight, digitSize);
    const auto unitFont = KLF::font(KLF::Face::monoRegular, 9.0f, 0.2f);
    const auto digitsTop = top + 13.0f + 8.0f + (54.0f - digitSize) * 0.5f;
    const auto unitsTop = top + 13.0f + 8.0f + 56.0f;

    auto x = left;

    const auto drawGroup = [&](const juce::String& digits, const juce::String& unit, juce::Colour colour)
    {
        const auto width = widthOf(digitFont, digits);
        g.setFont(digitFont);
        g.setColour(colour);
        g.drawText(digits, juce::Rectangle<float>(x, digitsTop, width + 4.0f, digitSize), juce::Justification::centredLeft, false);
        g.setFont(unitFont);
        g.setColour(faint);
        g.drawText(unit, juce::Rectangle<float>(x, unitsTop, juce::jmax(width, 40.0f), 11.0f), juce::Justification::centredLeft, false);
        x += width + 4.0f;
    };

    const auto drawColon = [&]
    {
        const auto width = widthOf(colonFont, ":");
        g.setFont(colonFont);
        g.setColour(faint);
        g.drawText(":", juce::Rectangle<float>(x, digitsTop, width + 2.0f, digitSize), juce::Justification::centredLeft, false);
        x += width + 4.0f;
    };

    drawGroup(hours, "HRS", text);
    drawColon();
    drawGroup(minutes, "MIN", text);
    drawColon();
    drawGroup(seconds, "SEC", values.tracking ? accentText : text);

    // Stats row
    const auto ruleY = unitsTop + 11.0f + 16.0f;
    g.setColour(findColour(KLF::lineColourId));
    g.fillRect(left, ruleY, right - left, 1.0f);

    const auto columnWidth = (right - left - 24.0f) / 3.0f;
    const auto statTop = ruleY + 13.0f;

    const auto drawStat = [&](int column, const char* label, juce::int64 value, juce::Colour colour)
    {
        const auto statX = left + static_cast<float>(column) * (columnWidth + 12.0f);
        g.setFont(KLF::font(KLF::Face::sansSemibold, 11.0f, 0.12f));
        g.setColour(muted);
        g.drawText(label, juce::Rectangle<float>(statX, statTop, columnWidth, 14.0f), juce::Justification::centredLeft, false);
        g.setFont(KLF::font(KLF::Face::monoRegular, 15.0f));
        g.setColour(colour);
        g.drawText(KronosArt::formatDuration(value), juce::Rectangle<float>(statX, statTop + 18.0f, columnWidth, 18.0f),
                   juce::Justification::centredLeft, false);
    };

    drawStat(0, "TODAY", values.today, accentText);
    drawStat(1, "DAILY AVG", values.average, text);
    drawStat(2, "LONGEST DAY", values.longest, text);
}

//==============================================================================
class SessionsPanel::Row : public juce::Component
{
public:
    explicit Row(SessionsPanel& ownerPanel) : owner(ownerPanel)
    {
        setInterceptsMouseClicks(true, false);
        hover.onChange = [this] { repaint(); };
    }

    void update(int newIndex)
    {
        index = newIndex;
        repaint();
    }

    void mouseEnter(const juce::MouseEvent&) override { hover.animateTo(1.0f, 180); }
    void mouseExit(const juce::MouseEvent&) override { hover.animateTo(0.0f, 180); }

    void paint(juce::Graphics& g) override
    {
        if (!juce::isPositiveAndBelow(index, owner.dates.size()))
            return;

        const auto date = owner.dates[index];
        const auto today = owner.processor.isToday(date);
        const auto seconds = today ? owner.processor.getTodaySeconds() : owner.processor.getTimeForDate(date);
        const auto longest = juce::jmax(owner.longestDay, seconds);
        const auto isBest = seconds > 0 && seconds == longest;
        const auto tracking = owner.processor.isTracking();

        const auto text = findColour(KLF::textColourId);
        const auto muted = findColour(KLF::mutedColourId);
        const auto faint = findColour(KLF::faintColourId);
        const auto accent = findColour(KLF::accentColourId);
        const auto accentText = findColour(KLF::accentTextColourId);

        // Rows re-order with a 40ms stagger after a sort change
        const auto reveal = juce::jlimit(0.0f, 1.0f, (owner.reorder.get() * 340.0f - static_cast<float>(index) * 40.0f) / 180.0f + 1.0f);
        g.beginTransparencyLayer(reveal);

        g.setColour(accent.withAlpha(0.06f * hover.get()));
        g.fillAll();

        const auto height = static_cast<float>(getHeight());
        const auto width = static_cast<float>(getWidth());
        constexpr float padding = 10.0f, gap = 10.0f, dayWidth = 52.0f, dateWidth = 88.0f, timeWidth = 86.0f;

        const auto glyphArea = juce::Rectangle<float>(14.0f, 14.0f).withPosition(padding, (height - 14.0f) * 0.5f);
        g.setColour(today ? accent : muted);
        g.strokePath(KronosArt::planetGlyph(date.getDayOfWeek()), juce::PathStrokeType(1.05f), KronosArt::fit(14.0f, glyphArea));

        static const char* const dayNames[] { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
        g.setColour(faint);
        g.setFont(KLF::font(KLF::Face::monoRegular, 10.0f, 0.1f));
        g.drawText(today ? "TDY" : dayNames[date.getDayOfWeek()],
                   juce::Rectangle<float>(padding + 21.0f, 0.0f, dayWidth - 21.0f, height),
                   juce::Justification::centredLeft, false);

        const auto dateX = padding + dayWidth + gap;
        g.setColour(today ? accentText : text);
        g.setFont(KLF::font(KLF::Face::sansSemibold, 13.0f));
        g.drawText(KronosArt::formatDate(date), juce::Rectangle<float>(dateX, 0.0f, dateWidth, height),
                   juce::Justification::centredLeft, false);

        const auto barX = dateX + dateWidth + gap;
        const auto barWidth = width - padding - timeWidth - gap - barX;
        const juce::Rectangle<float> bar(barX, (height - 6.0f) * 0.5f, barWidth, 6.0f);

        g.setColour(findColour(KLF::trackColourId));
        g.fillRect(bar);

        const auto ratio = longest > 0 ? static_cast<float>(seconds) / static_cast<float>(longest) : 0.0f;
        g.setColour(today ? accent : (isBest ? text : muted));
        g.fillRect(bar.withWidth(barWidth * juce::jmax(0.02f, ratio)));

        g.setColour(today && tracking ? accentText : text);
        g.setFont(KLF::font(KLF::Face::monoRegular, 12.0f));
        g.drawText(KronosArt::formatDuration(seconds),
                   juce::Rectangle<float>(width - padding - timeWidth, 0.0f, timeWidth, height),
                   juce::Justification::centredRight, false);

        g.endTransparencyLayer();
    }

private:
    SessionsPanel& owner;
    int index = -1;
    Tween hover;
};

SessionsPanel::SessionsPanel(KronosAudioProcessor& processorToUse)
    : ChamferPanel(true), processor(processorToUse)
{
    setTitle("Sessions");
    list.setModel(this);
    list.setRowHeight(27);
    list.setMultipleSelectionEnabled(false);
    list.setWantsKeyboardFocus(true);
    list.setTitle("Session history");
    list.getViewport()->setScrollBarThickness(6);
    addAndMakeVisible(list);

    recentButton.setButtonText("RECENT");
    longestButton.setButtonText("LONGEST");
    addAndMakeVisible(recentButton);
    addAndMakeVisible(longestButton);

    reorder.jumpTo(1.0f);
    reorder.onChange = [this] { list.repaint(); };
}

void SessionsPanel::resized()
{
    const auto font = KLF::font(KLF::Face::monoRegular, 10.0f, 0.14f);
    const auto recentWidth = juce::roundToInt(font.getStringWidthFloat("RECENT")) + 22;
    const auto longestWidth = juce::roundToInt(font.getStringWidthFloat("LONGEST")) + 22;

    const auto right = getWidth() - 15;
    longestButton.setBounds(right - longestWidth, 3, longestWidth, 32);
    recentButton.setBounds(longestButton.getX() - recentWidth, 3, recentWidth, 32);

    list.setBounds(9, 44, getWidth() - 18, getHeight() - 51);
}

void SessionsPanel::paint(juce::Graphics& g)
{
    ChamferPanel::paint(g);

    const auto line = findColour(KLF::lineColourId);

    g.setColour(line);
    g.fillRect(1.0f, 39.0f, static_cast<float>(getWidth()) - 2.0f, 1.0f);

    g.setColour(findColour(KLF::textColourId));
    const auto titleFont = KLF::font(KLF::Face::sansBold, 12.0f, 0.2f);
    g.setFont(titleFont);
    g.drawText("SESSIONS", juce::Rectangle<float>(19.0f, 1.0f, 100.0f, 38.0f), juce::Justification::centredLeft, false);

    g.setColour(findColour(KLF::faintColourId));
    g.setFont(KLF::font(KLF::Face::monoRegular, 10.0f, 0.12f));
    g.drawText(KronosArt::formatDays(daysLogged),
               juce::Rectangle<float>(19.0f + titleFont.getStringWidthFloat("SESSIONS") + 10.0f, 1.0f, 120.0f, 38.0f),
               juce::Justification::centredLeft, false);

    // The two segments share one outline
    const auto segments = recentButton.getBounds().getUnion(longestButton.getBounds()).reduced(0, 3).toFloat().expanded(1.0f, 0.0f);
    g.setColour(line);
    g.drawRect(segments, 1.0f);
    g.fillRect(static_cast<float>(longestButton.getX()), segments.getY(), 1.0f, segments.getHeight());
}

void SessionsPanel::refresh(bool sortOrderMayHaveChanged)
{
    const auto newLongest = processor.getLongestDaySeconds();
    const auto newDays = processor.getDaysLogged();
    const auto scaleChanged = newLongest != longestDay;

    longestDay = newLongest;

    if (newDays != daysLogged)
    {
        daysLogged = newDays;
        repaint(0, 0, 300, 39);
    }

    const auto newDates = processor.getSortedDates();

    if (newDates != dates)
    {
        const auto countChanged = newDates.size() != dates.size();
        dates = newDates;

        if (countChanged)
            list.updateContent();

        list.repaint();
    }
    else if (sortOrderMayHaveChanged || scaleChanged)
    {
        list.repaint();
    }
    else
    {
        // Only today's row moves while tracking
        for (int i = list.getRowContainingPosition(4, 4), end = i + 9; i >= 0 && i < end && i < dates.size(); ++i)
            if (processor.isToday(dates[i]))
                list.repaintRow(i);
    }
}

void SessionsPanel::showSortMode(bool animate)
{
    const auto byTime = processor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostTime;

    recentButton.setActive(!byTime);
    longestButton.setActive(byTime);

    if (animate)
    {
        (byTime ? longestButton : recentButton).playLabelDecode();
        reorder.jumpTo(0.0f);
        reorder.animateTo(1.0f, 340);
    }

    refresh(true);
}

juce::Component* SessionsPanel::refreshComponentForRow(int row, bool, juce::Component* existing)
{
    auto* rowComponent = dynamic_cast<Row*>(existing);

    if (rowComponent == nullptr)
    {
        delete existing;
        rowComponent = new Row(*this);
    }

    rowComponent->update(row);
    return rowComponent;
}

//==============================================================================
MenuPanel::MenuPanel()
{
    setFocusContainerType(FocusContainerType::keyboardFocusContainer);

    backupItem.setButtonText(juce::String(juce::CharPointer_UTF8("Back up\xe2\x80\xa6")));
    backupItem.setIcon(KronosArt::backupIcon(), 14.0f, 14.0f, 1.2f);
    backupItem.setDetail(greek("\xce\x9c\xce\x9d\xce\x97\xce\x9c\xce\x97"));

    restoreItem.setButtonText(juce::String(juce::CharPointer_UTF8("Restore\xe2\x80\xa6")));
    restoreItem.setIcon(KronosArt::restoreIcon(), 14.0f, 14.0f, 1.2f);
    restoreItem.setDetail(greek("\xce\x91\xce\x9d\xce\x91\xce\x9c\xce\x9d\xce\x97\xce\xa3\xce\x99\xce\xa3"));

    aboutItem.setButtonText("About Kronos");
    aboutItem.setIcon(KronosArt::aboutIcon(), 14.0f, 14.0f, 1.2f);
    aboutItem.setDetail(greek("\xce\xa0\xce\x95\xce\xa1\xce\x99"));

    addAndMakeVisible(backupItem);
    addAndMakeVisible(restoreItem);
    addAndMakeVisible(aboutItem);
}

void MenuPanel::resized()
{
    auto area = getLocalBounds().reduced(5);
    backupItem.setBounds(area.removeFromTop(34));
    restoreItem.setBounds(area.removeFromTop(34));
    area.removeFromTop(9);
    aboutItem.setBounds(area.removeFromTop(34));
}

void MenuPanel::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour(findColour(KLF::raisedColourId));
    g.fillRect(bounds);
    g.setColour(findColour(KLF::lineColourId));
    g.drawRect(bounds, 1.0f);
    g.fillRect(11.0f, static_cast<float>(restoreItem.getBottom()) + 4.0f, bounds.getWidth() - 22.0f, 1.0f);
}

bool MenuPanel::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        if (onDismiss)
            onDismiss();

        return true;
    }

    return false;
}

void MenuPanel::focusFirstItem()
{
    backupItem.grabKeyboardFocus();
}

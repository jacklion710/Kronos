#include "KronosWidgets.h"

using KLF = KronosLookAndFeel;

namespace
{
juce::Path svg(const char* data)
{
    return juce::Drawable::parseSVGPath(data);
}

void strokePath(juce::Graphics& g, const juce::Path& path, const juce::AffineTransform& transform, float width, float scale)
{
    g.strokePath(path, juce::PathStrokeType(width * scale), transform);
}

constexpr int sheetWidth = 440;
constexpr int sheetChromeHeight = 86; // padding, kicker row and the gap beneath it
}

//==============================================================================
juce::Path KronosArt::chamfered(juce::Rectangle<float> area, float cut, bool cutsTopRight)
{
    const auto l = area.getX(), t = area.getY(), r = area.getRight(), b = area.getBottom();
    juce::Path path;

    if (cutsTopRight)
    {
        path.startNewSubPath(l, t);
        path.lineTo(r - cut, t);
        path.lineTo(r, t + cut);
        path.lineTo(r, b);
        path.lineTo(l + cut, b);
        path.lineTo(l, b - cut);
    }
    else
    {
        path.startNewSubPath(l + cut, t);
        path.lineTo(r, t);
        path.lineTo(r, b - cut);
        path.lineTo(r - cut, b);
        path.lineTo(l, b);
        path.lineTo(l, t + cut);
    }

    path.closeSubPath();
    return path;
}

juce::AffineTransform KronosArt::fit(float viewBoxSize, juce::Rectangle<float> area)
{
    const auto scale = area.getWidth() / viewBoxSize;
    return juce::AffineTransform::scale(scale).translated(area.getX(), area.getY());
}

void KronosArt::drawSigil(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour edge, juce::Colour sickle, juce::Colour stem)
{
    static const auto hexagon = svg("M11 1.5 L19.2 6.25 L19.2 15.75 L11 20.5 L2.8 15.75 L2.8 6.25 Z");
    static const auto harpe = svg("M6.2 8.4 A6 6 0 0 1 15.8 9.4");
    static const auto saturnStem = svg("M11 6.6 L11 16.4 M8.6 12.2 L13.4 12.2");

    const auto scale = area.getWidth() / 22.0f;
    const auto transform = fit(22.0f, area);

    g.setColour(edge);
    strokePath(g, hexagon, transform, 0.9f, scale);
    g.setColour(sickle);
    strokePath(g, harpe, transform, 1.5f, scale);
    g.setColour(stem);
    strokePath(g, saturnStem, transform, 1.1f, scale);

    juce::Path foot;
    foot.addCentredArc(11.0f, 16.4f, 1.1f, 1.1f, 0.0f, 0.0f, juce::MathConstants<float>::twoPi, true);
    g.fillPath(foot, transform);
}

void KronosArt::drawOuroboros(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour body, juce::Colour head,
                              juce::Colour scales, juce::Colour stem)
{
    static const auto ring = svg("M32 4 A28 28 0 1 1 18.5 7.5");
    static const auto serpentHead = svg("M18.5 7.5 L23.5 2.8 L25.2 9.6 Z");

    const auto scale = area.getWidth() / 64.0f;
    const auto transform = fit(64.0f, area);

    g.setColour(body);
    g.strokePath(ring, juce::PathStrokeType(2.2f * scale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), transform);

    g.setColour(head);
    g.fillPath(serpentHead, transform);

    const float dashes[] { 1.5f * scale, 3.0f * scale };
    juce::Path scaled(ring), dashed;
    scaled.applyTransform(transform);
    juce::PathStrokeType(0.6f * scale).createDashedStroke(dashed, scaled, dashes, 2);
    g.setColour(scales);
    g.fillPath(dashed);

    drawSigil(g, juce::Rectangle<float>(21.0f, 21.0f, 22.0f, 22.0f).transformedBy(transform), body, head, stem);
}

juce::Path KronosArt::planetGlyph(int dayOfWeek)
{
    static const juce::Path glyphs[]
    {
        [] { juce::Path p; p.addEllipse(2.0f, 2.0f, 10.0f, 10.0f); p.addEllipse(6.1f, 6.1f, 1.8f, 1.8f); return p; }(),
        svg("M10.2 1.8 A5.6 5.6 0 1 0 10.2 12.2 A6.8 6.8 0 0 1 10.2 1.8 Z"),
        [] { auto p = svg("M8.2 5.8 L12.4 1.6 M8.8 1.6 L12.4 1.6 L12.4 5.2"); p.addEllipse(1.8f, 4.6f, 7.4f, 7.4f); return p; }(),
        [] { auto p = svg("M3.9 0.8 A3.2 3.2 0 0 0 10.1 0.8 M7 9.4 L7 13.4 M5 11.4 L9 11.4"); p.addEllipse(4.1f, 3.6f, 5.8f, 5.8f); return p; }(),
        svg("M2.6 3.6 C4.2 1.4 7.6 2 7.4 5 C7.2 7.4 4.6 8.8 2.4 9.6 L12 9.6 M9.6 1.4 L9.6 13"),
        [] { auto p = svg("M7 8.4 L7 13.4 M4.8 11 L9.2 11"); p.addEllipse(3.6f, 1.6f, 6.8f, 6.8f); return p; }(),
        svg("M3 3.4 L8.2 3.4 M5 1 L5 8.4 C5.6 6.2 7.2 5.2 8.8 5.8 C10.8 6.6 10.4 8.8 9 10 C8 10.9 7.8 12.2 9.4 13")
    };

    return glyphs[juce::jlimit(0, 6, dayOfWeek)];
}

juce::Path KronosArt::playIcon()   { return svg("M7 4 L18 11 L7 18 Z"); }
juce::Path KronosArt::pauseIcon()  { juce::Path p; p.addRectangle(5.0f, 4.0f, 4.0f, 14.0f); p.addRectangle(13.0f, 4.0f, 4.0f, 14.0f); return p; }
juce::Path KronosArt::sunIcon()    { juce::Path p; p.addEllipse(2.5f, 2.5f, 11.0f, 11.0f); p.addEllipse(6.7f, 6.7f, 2.6f, 2.6f); return p; }
juce::Path KronosArt::moonIcon()   { return svg("M13 9.5 A5.5 5.5 0 0 1 6.5 3 A5.5 5.5 0 1 0 13 9.5 Z"); }
juce::Path KronosArt::menuIcon()   { return svg("M3 4.5 L13 4.5 M3 8 L13 8 M3 11.5 L9 11.5"); }
juce::Path KronosArt::closeIcon()  { return svg("M2 2 L10 10 M10 2 L2 10"); }
juce::Path KronosArt::checkIcon()  { auto p = svg("M8.5 14.5 L12 18 L19.5 10"); p.addEllipse(2.0f, 2.0f, 24.0f, 24.0f); return p; }
juce::Path KronosArt::backupIcon() { return svg("M7 2 L7 9 M4 6 L7 9 L10 6 M2 11.5 L12 11.5"); }
juce::Path KronosArt::restoreIcon(){ return svg("M7 11 L7 4 M4 7 L7 4 L10 7 M2 2.5 L12 2.5"); }
juce::Path KronosArt::aboutIcon()  { auto p = svg("M7 6.5 L7 10 M7 4.2 L7 4.3"); p.addEllipse(1.5f, 1.5f, 11.0f, 11.0f); return p; }

juce::String KronosArt::formatDuration(juce::int64 seconds)
{
    return juce::String(seconds / 3600).paddedLeft('0', 2) + ":"
         + juce::String((seconds % 3600) / 60).paddedLeft('0', 2) + ":"
         + juce::String(seconds % 60).paddedLeft('0', 2);
}

juce::String KronosArt::formatDays(int days)
{
    return juce::String(days) + (days == 1 ? " DAY" : " DAYS");
}

juce::String KronosArt::formatDate(const juce::Time& date)
{
    return date.getMonthName(true) + " " + juce::String(date.getDayOfMonth()) + ", " + juce::String(date.getYear());
}

//==============================================================================
void Tween::animateTo(float target, int durationMs)
{
    if (juce::approximatelyEqual(target, targetValue) && (isTimerRunning() || juce::approximatelyEqual(target, value)))
        return;

    startValue = value;
    targetValue = target;
    duration = juce::jmax(1, durationMs);
    startedAt = juce::Time::getMillisecondCounterHiRes();
    startTimerHz(60);
}

void Tween::jumpTo(float target)
{
    stopTimer();
    value = startValue = targetValue = target;

    if (onChange)
        onChange();
}

void Tween::timerCallback()
{
    const auto progress = juce::jlimit(0.0, 1.0, (juce::Time::getMillisecondCounterHiRes() - startedAt) / duration);
    const auto eased = 1.0 - std::pow(1.0 - progress, 3.0);

    value = startValue + (targetValue - startValue) * static_cast<float>(eased);

    if (progress >= 1.0)
    {
        value = targetValue;
        stopTimer();
    }

    if (onChange)
        onChange();
}

//==============================================================================
KronosButton::KronosButton(const juce::String& accessibleName, Style buttonStyle)
    : juce::Button(accessibleName), style(buttonStyle)
{
    setTitle(accessibleName);
    setWantsKeyboardFocus(true);
    setMouseClickGrabsKeyboardFocus(false);

    hover.onChange = press.onChange = decode.onChange = [this] { repaint(); };
    decode.jumpTo(1.0f);
    onClick = [this] { if (onActivated) onActivated(); };
}

void KronosButton::setIcon(juce::Path path, float viewBoxSize, float size, float strokeWidth)
{
    icon = std::move(path);
    iconViewBox = viewBoxSize;
    iconSize = size;
    iconStroke = strokeWidth;
    repaint();
}

void KronosButton::setActive(bool shouldBeActive)
{
    if (active != shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }
}

void KronosButton::setIgnoresMotion(bool shouldIgnore)
{
    ignoresMotion = shouldIgnore;
}

void KronosButton::playLabelDecode()
{
    decode.jumpTo(0.0f);
    decode.animateTo(1.0f, 180);
}

void KronosButton::buttonStateChanged()
{
    const auto state = getState();
    hover.animateTo(state == buttonNormal ? 0.0f : 1.0f, 180);
    press.animateTo(state == buttonDown ? 1.0f : 0.0f, 80);
}

bool KronosButton::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        triggerClick();
        return true;
    }

    return juce::Button::keyPressed(key);
}

juce::Rectangle<float> KronosButton::getVisualBounds() const
{
    auto bounds = getLocalBounds().toFloat();

    // Controls drawn smaller than 32px keep a 32px hit target around them
    if (style == Style::segment)
        return bounds.reduced(0.0f, 4.0f);

    if (style == Style::icon && getWidth() == 32 && iconViewBox < 14.0f)
        return bounds.reduced(2.0f);

    return bounds;
}

bool KronosButton::hitTest(int x, int y)
{
    if (style == Style::play)
        return KronosArt::chamfered(getLocalBounds().toFloat(), 10.0f).contains(static_cast<float>(x), static_cast<float>(y));

    return juce::Button::hitTest(x, y);
}

juce::String KronosButton::getDisplayText() const
{
    const auto text = getButtonText();
    const auto progress = decode.get();

    if (progress >= 1.0f)
        return text;

    const juce::String glyphs("#/:+=<>*");

    const auto resolved = juce::roundToInt(progress * static_cast<float>(text.length()));
    juce::String result;

    for (int i = 0; i < text.length(); ++i)
        result += i < resolved ? text.substring(i, i + 1)
                               : glyphs.substring((i * 7 + juce::roundToInt(progress * 40.0f)) % glyphs.length()).substring(0, 1);

    return result;
}

void KronosButton::paintButton(juce::Graphics& g, bool, bool)
{
    const auto bounds = getVisualBounds();
    const auto hoverAmount = hover.get();
    const auto pressAmount = press.get();
    const auto text = findColour(KLF::textColourId);
    const auto muted = findColour(KLF::mutedColourId);
    const auto line = findColour(KLF::lineColourId);
    const auto accent = findColour(KLF::accentColourId);
    const auto accentText = findColour(KLF::accentTextColourId);
    const auto raised = findColour(KLF::raisedColourId);
    const auto surface = findColour(KLF::surfaceColourId);
    const auto cta = findColour(KLF::ctaColourId);
    const auto dimming = isEnabled() ? 1.0f : 0.45f;

    if (style == Style::play || style == Style::primary)
        g.addTransform(juce::AffineTransform::scale(1.0f - 0.04f * pressAmount, 1.0f - 0.04f * pressAmount,
                                                    bounds.getCentreX(), bounds.getCentreY()));

    const auto drawIcon = [&](juce::Colour colour, juce::Rectangle<float> area)
    {
        g.setColour(colour);
        const auto transform = KronosArt::fit(iconViewBox, area);

        if (iconStroke > 0.0f)
            g.strokePath(icon, juce::PathStrokeType(iconStroke * area.getWidth() / iconViewBox), transform);
        else
            g.fillPath(icon, transform);
    };

    switch (style)
    {
        case Style::primary:
            g.setColour(cta.interpolatedWith(accent, 0.3f * hoverAmount).withMultipliedAlpha(dimming));
            g.fillRect(bounds);
            g.setColour(juce::Colours::white.withMultipliedAlpha(dimming));
            g.setFont(KLF::font(KLF::Face::sansSemibold, 13.0f, 0.04f));
            g.drawText(getButtonText(), bounds, juce::Justification::centred, false);
            break;

        case Style::secondary:
        case Style::emphasised:
            g.setColour(text.withAlpha(0.06f * hoverAmount));
            g.fillRect(bounds);
            g.setColour((style == Style::emphasised ? text : line).withMultipliedAlpha(dimming));
            g.drawRect(bounds, 1.0f);
            g.setColour(text.withMultipliedAlpha(dimming));
            g.setFont(KLF::font(KLF::Face::sansSemibold, 13.0f));
            g.drawText(getButtonText(), bounds, juce::Justification::centred, false);
            break;

        case Style::icon:
            g.setColour(active ? raised : text.withAlpha(0.06f * hoverAmount));
            g.fillRect(bounds);
            g.setColour(line);
            g.drawRect(bounds, 1.0f);
            drawIcon(muted.interpolatedWith(text, hoverAmount),
                     juce::Rectangle<float>(iconSize, iconSize).withCentre(bounds.getCentre()));
            break;

        case Style::segment:
            if (active)
            {
                g.setColour(raised);
                g.fillRect(bounds);
                g.setColour(accent);
                g.fillRect(bounds.withTop(bounds.getBottom() - 2.0f));
            }
            g.setColour(active ? text : muted.interpolatedWith(text, hoverAmount));
            g.setFont(KLF::font(KLF::Face::monoRegular, 10.0f, 0.14f));
            g.drawText(getDisplayText(), bounds, juce::Justification::centred, false);
            break;

        case Style::play:
        {
            const auto shape = KronosArt::chamfered(bounds, 10.0f);
            g.setColour(active ? raised : cta.interpolatedWith(accent, 0.3f * hoverAmount));
            g.fillPath(shape);
            g.setColour(active ? accent : cta);
            g.strokePath(KronosArt::chamfered(bounds.reduced(0.5f), 9.5f), juce::PathStrokeType(1.0f));
            drawIcon(active ? accent : juce::Colours::white,
                     juce::Rectangle<float>(22.0f, 22.0f).withCentre(bounds.getCentre()));
            break;
        }

        case Style::menuItem:
        {
            g.setColour(accent.withAlpha(0.08f * hoverAmount));
            g.fillRect(bounds);
            drawIcon(muted, juce::Rectangle<float>(10.0f, bounds.getCentreY() - 7.0f, 14.0f, 14.0f));
            g.setColour(text);
            g.setFont(KLF::font(KLF::Face::sansRegular, 13.0f));
            g.drawText(getButtonText(), bounds.withLeft(34.0f), juce::Justification::centredLeft, false);
            break;
        }

        case Style::link:
            g.setColour(surface.interpolatedWith(accent, 0.08f * hoverAmount));
            g.fillRect(bounds);
            g.setColour(text);
            g.setFont(KLF::font(KLF::Face::sansSemibold, 13.0f));
            g.drawText(getButtonText(), bounds.withTrimmedLeft(12.0f), juce::Justification::centredLeft, false);
            g.setColour(accentText);
            g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x97")), bounds.withTrimmedRight(12.0f),
                       juce::Justification::centredRight, false);
            break;
    }

    if (hasKeyboardFocus(false))
    {
        g.setColour(accent);
        g.drawRect(getLocalBounds().toFloat(), 2.0f);
    }
}

//==============================================================================
void KronosListBox::paintOverChildren(juce::Graphics& g)
{
    juce::ListBox::paintOverChildren(g);

    if (hasKeyboardFocus(true))
    {
        g.setColour(findColour(KLF::accentColourId));
        g.drawRect(getLocalBounds().toFloat(), 2.0f);
    }
}

ChamferPanel::ChamferPanel(bool cutsTopRightCorner) : cutsTopRight(cutsTopRightCorner)
{
}

void ChamferPanel::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour(findColour(KLF::lineColourId));
    g.fillPath(KronosArt::chamfered(bounds, 12.0f, cutsTopRight));

    g.setColour(findColour(KLF::surfaceColourId));
    g.fillPath(KronosArt::chamfered(bounds.reduced(1.0f), 11.5f, cutsTopRight));
}

void drawParagraph(juce::Graphics& g, const juce::String& text, juce::Font font, juce::Colour colour,
                   juce::Rectangle<float> area)
{
    g.setColour(colour);
    g.setFont(font);
    g.drawMultiLineText(text, juce::roundToInt(area.getX()), juce::roundToInt(area.getY() + font.getAscent()),
                        juce::roundToInt(area.getWidth()), juce::Justification::left, 2.0f);
}

//==============================================================================
KronosSheetHost::KronosSheetHost()
{
    setVisible(false);
    setWantsKeyboardFocus(false);
    setFocusContainerType(FocusContainerType::keyboardFocusContainer);
    addAndMakeVisible(frame);

    arrival.onChange = [this] { applyMotion(); };
    ruleDraw.onChange = [this] { frame.repaint(); };
}

void KronosSheetHost::open(std::unique_ptr<KronosSheet> sheet, bool animate)
{
    if (currentSheet != nullptr)
        frame.removeChildComponent(currentSheet.get());

    currentSheet = std::move(sheet);
    closing = false;
    frame.setEnabled(true);

    currentSheet->onClose = [this] { close(); };
    currentSheet->onLayoutChanged = [this] { layoutFrame(); frame.repaint(); };
    frame.addAndMakeVisible(*currentSheet);
    layoutFrame();

    setVisible(true);
    toFront(false);

    if (animate)
    {
        arrival.jumpTo(0.0f);
        ruleDraw.jumpTo(0.0f);
        arrival.animateTo(1.0f, 340);
        ruleDraw.animateTo(1.0f, 760);
    }
    else
    {
        arrival.jumpTo(1.0f);
        ruleDraw.jumpTo(1.0f);
    }

    currentSheet->focusDefaultControl();
}

void KronosSheetHost::close()
{
    if (currentSheet == nullptr || closing)
        return;

    // Nothing in a departing sheet may act: Save after Cancel would still write
    closing = true;
    frame.setEnabled(false);
    arrival.onChange = [this]
    {
        applyMotion();

        if (closing && arrival.get() <= 0.0f)
        {
            juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<KronosSheetHost>(this)]
            {
                if (safeThis == nullptr || !safeThis->closing)
                    return;

                safeThis->closing = false;
                safeThis->frame.setEnabled(true);
                safeThis->frame.removeChildComponent(safeThis->currentSheet.get());
                safeThis->currentSheet.reset();
                safeThis->setVisible(false);
                safeThis->arrival.onChange = [host = safeThis.getComponent()] { host->applyMotion(); };

                if (safeThis->onClosed)
                    safeThis->onClosed();
            });
        }
    };

    arrival.animateTo(0.0f, 340);
}

void KronosSheetHost::layoutFrame()
{
    if (currentSheet == nullptr)
        return;

    const auto height = sheetChromeHeight + currentSheet->getPreferredHeight();
    frame.setBounds(juce::Rectangle<int>(sheetWidth, height).withCentre(getLocalBounds().getCentre()));
    currentSheet->setBounds(25, 65, sheetWidth - 50, currentSheet->getPreferredHeight());
}

void KronosSheetHost::applyMotion()
{
    const auto amount = arrival.get();
    frame.setAlpha(amount);
    frame.setTransform(juce::AffineTransform::translation(0.0f, 12.0f * (1.0f - amount)));
    repaint();
}

void KronosSheetHost::paint(juce::Graphics& g)
{
    g.fillAll(findColour(KLF::scrimColourId).withMultipliedAlpha(arrival.get()));
}

void KronosSheetHost::resized()
{
    layoutFrame();
}

bool KronosSheetHost::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && currentSheet != nullptr)
    {
        if (!closing)
            currentSheet->cancelOrClose();

        return true;
    }

    return false;
}

void KronosSheetHost::mouseDown(const juce::MouseEvent&)
{
    // The scrim blocks the window beneath the sheet but does not dismiss it,
    // so a stray click cannot throw away a half-typed backup name.
}

KronosSheetHost::Frame::Frame(KronosSheetHost& owner) : host(owner)
{
    closeButton.setIcon(KronosArt::closeIcon(), 12.0f, 12.0f, 1.3f);
    closeButton.onActivated = [this] { if (auto* sheet = host.getSheet()) sheet->cancelOrClose(); };
    addAndMakeVisible(closeButton);
}

void KronosSheetHost::Frame::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.setColour(findColour(KLF::lineColourId));
    g.fillPath(KronosArt::chamfered(bounds, 14.0f));

    g.setColour(findColour(KLF::raisedColourId));
    g.fillPath(KronosArt::chamfered(bounds.reduced(1.0f), 13.5f));

    if (auto* sheet = host.getSheet())
    {
        g.setColour(findColour(KLF::accentColourId));
        g.fillRect(25.0f, 36.0f, 18.0f * host.ruleDraw.get(), 2.0f);

        g.setColour(findColour(KLF::mutedColourId));
        g.setFont(KLF::font(KLF::Face::monoRegular, 10.0f, 0.2f));
        g.drawText(sheet->getKicker(), juce::Rectangle<float>(53.0f, 23.0f, 300.0f, 28.0f),
                   juce::Justification::centredLeft, false);
    }
}

void KronosSheetHost::Frame::resized()
{
    closeButton.setBounds(getWidth() - 57, 21, 32, 32);
}

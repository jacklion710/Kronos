#include "AboutComponent.h"
#include "JucePluginDefines.h"

using KLF = KronosLookAndFeel;

AboutComponent::AboutComponent()
{
    const auto addLink = [this](KronosButton& link, const char* url)
    {
        link.setButtonText(link.getName());
        link.onActivated = [url] { juce::URL(url).launchInDefaultBrowser(); };
        addAndMakeVisible(link);
    };

    addLink(websiteLink, "https://jacklion.xyz");
    addLink(pluginsLink, "https://jacklion.gumroad.com");
    addLink(discordLink, "https://discord.gg/EFQq7BX");
    addLink(instagramLink, "https://www.instagram.com/jack.lion");
}

juce::String AboutComponent::getKicker() const
{
    return "ABOUT";
}

void AboutComponent::focusDefaultControl()
{
    websiteLink.grabKeyboardFocus();
}

void AboutComponent::resized()
{
    constexpr int linksTop = 138, rowHeight = 36;
    const auto columnWidth = (getWidth() - 3) / 2;
    const auto rightX = columnWidth + 2;
    const auto rightWidth = getWidth() - 1 - rightX;

    websiteLink.setBounds(1, linksTop + 1, columnWidth, rowHeight);
    pluginsLink.setBounds(rightX, linksTop + 1, rightWidth, rowHeight);
    discordLink.setBounds(1, linksTop + rowHeight + 2, columnWidth, rowHeight);
    instagramLink.setBounds(rightX, linksTop + rowHeight + 2, rightWidth, rowHeight);
}

void AboutComponent::paint(juce::Graphics& g)
{
    const auto text = findColour(KLF::textColourId);
    const auto muted = findColour(KLF::mutedColourId);
    const auto faint = findColour(KLF::faintColourId);
    const auto line = findColour(KLF::lineColourId);
    const auto width = static_cast<float>(getWidth());

    KronosArt::drawOuroboros(g, { 0.0f, 0.0f, 64.0f, 64.0f }, muted, findColour(KLF::accentColourId),
                             findColour(KLF::raisedColourId), text);

    g.setColour(text);
    g.setFont(KLF::font(KLF::Face::sansBold, 26.0f, 0.3f));
    g.drawText("KRONOS", juce::Rectangle<float>(78.0f, 8.0f, 300.0f, 30.0f), juce::Justification::centredLeft, false);

    g.setColour(faint);
    g.setFont(KLF::font(KLF::Face::monoRegular, 11.0f, 0.12f));
    g.drawText(juce::String(juce::CharPointer_UTF8("v")) + JucePlugin_VersionString
                   + juce::String(juce::CharPointer_UTF8(" \xc2\xb7 VST3 \xc2\xb7 AU")),
               juce::Rectangle<float>(78.0f, 40.0f, 300.0f, 14.0f), juce::Justification::centredLeft, false);

    g.setColour(line);
    g.fillRect(0.0f, 78.0f, width, 1.0f);

    drawParagraph(g, "Time tracking for your DAW sessions. Created by Jacob Leone (Jack.Lion).",
                  KLF::font(KLF::Face::sansRegular, 13.0f), muted, { 0.0f, 92.0f, width, 40.0f });

    // One pixel gaps between the link cells draw as a grid
    g.setColour(line);
    g.fillRect(0.0f, 138.0f, width, 75.0f);

    g.setColour(faint);
    g.setFont(KLF::font(KLF::Face::monoRegular, 10.0f, 0.12f));
    g.drawText(juce::String(juce::CharPointer_UTF8("GPL v3 \xc2\xb7 JACK.LION")),
               juce::Rectangle<float>(0.0f, 228.0f, width, 14.0f), juce::Justification::centredLeft, false);
}

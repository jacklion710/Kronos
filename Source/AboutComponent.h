#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AboutComponent : public juce::Component
{
public:
    AboutComponent(KronosAudioProcessor& processor);
    ~AboutComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::TextButton closeButton;

private:
    void positionLinks();

    KronosAudioProcessor& audioProcessor;
    juce::HyperlinkButton gumroadLink, soundcloudLink, jlWebLink, 
                         jlInstaLink, discordLink, aznadelLink;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutComponent)
};
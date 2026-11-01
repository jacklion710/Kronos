#pragma once

#include <JuceHeader.h>
#include "KronosWidgets.h"

class AboutComponent : public KronosSheet
{
public:
    AboutComponent();

    juce::String getKicker() const override;
    juce::String getSheetTitle() const override { return "About Kronos"; }
    int getPreferredHeight() const override { return 224; }
    void focusDefaultControl() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    KronosButton websiteLink { "Website", KronosButton::Style::link };
    KronosButton pluginsLink { "More plugins", KronosButton::Style::link };
    KronosButton instagramLink { "Instagram", KronosButton::Style::link };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutComponent)
};

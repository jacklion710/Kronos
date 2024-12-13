/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
KronosAudioProcessorEditor::KronosAudioProcessorEditor (KronosAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
}

KronosAudioProcessorEditor::~KronosAudioProcessorEditor()
{
}

//==============================================================================
void KronosAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::blue);
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("KRONOS", getLocalBounds(), juce::Justification::centred, 1);
}

void KronosAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

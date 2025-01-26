/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>

// Add near the top of the file, before the class declaration
#define USE_DUMMY_DATES 0  // Set to 1 to use dummy dates, 0 for real dates

//==============================================================================
/**
*/
class KronosAudioProcessor  : public juce::AudioProcessor, public juce::Timer
{
public:
    //==============================================================================
    KronosAudioProcessor();
    ~KronosAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    void startTracking();
    void stopTracking();

    juce::int64 getTotalTimeInSeconds() const;
    bool isTracking() const;

    void suspendProcessing(bool shouldSuspend);
    void timerCallback() override;

    const juce::Array<juce::Time>& getSessionDates() const { return sessionDates; }

    bool isDarkMode() const;
    void setDarkMode(bool shouldBeDark);

    // Add the callback
    std::function<void()> onStateLoaded;

    // Fix isSuspended by removing the implementation from header
    bool isSuspended() const;

    // Map to store time spent per date
    juce::HashMap<juce::String, juce::int64> timePerDate;

    juce::int64 getTimeForDate(const juce::Time& date) const;

    enum class DateSortMode {
        MostRecent,
        MostTime
    };
    
    DateSortMode getDateSortMode() const;
    void setDateSortMode(DateSortMode mode);

    bool isShowBarsEnabled() const { return showBarsEnabled; }
    void setShowBarsEnabled(bool enabled) { showBarsEnabled = enabled; }

    // Add parameter state
    std::unique_ptr<juce::AudioProcessorValueTreeState> parameters;

    void setTracking(bool shouldTrack);
    juce::Array<juce::Time> getSortedDates() const;

    // Add this declaration
    void addDummyDates();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KronosAudioProcessor)
    juce::Time startTime;
    juce::int64 totalTimeInSeconds = 0;
    
    // Array to store session dates
    juce::Array<juce::Time> sessionDates;
    void addSessionDate();  // Helper function to add today's date

    juce::Time lastSaveTime;
    const int minimumSaveIntervalMs = 100; // Minimum time between saves

    bool showBarsEnabled = false;

    // Add or update these member variables
    bool trackingState = false;  
    DateSortMode currentSortMode = DateSortMode::MostRecent;
};

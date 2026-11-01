/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

#define USE_DUMMY_DATES 0  // Set to 1 to use dummy dates, 0 for real dates

//==============================================================================
/**
*/
class KronosAudioProcessor  : public juce::AudioProcessor,
                             public juce::AudioProcessorValueTreeState::Listener,
                             public juce::Timer
{
public:
    //==============================================================================
    KronosAudioProcessor(std::function<juce::Time()> nowProvider = {});
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
    bool restoreStateFromData(const void* data, int sizeInBytes);

    //==============================================================================
    void startTracking();
    void stopTracking();

    juce::int64 getTotalTimeInSeconds() const;
    bool isTracking() const;

    void timerCallback() override;

    const juce::Array<juce::Time>& getSessionDates() const { return sessionDates; }

    bool isDarkMode() const;
    void setDarkMode(bool shouldBeDark);

    std::function<void()> onStateLoaded;

    bool isSuspended() const;

    // Map to store time spent per date
    juce::HashMap<juce::String, juce::int64> timePerDate;

    juce::int64 getTimeForDate(const juce::Time& date) const;
    juce::int64 getLongestDaySeconds() const;

    enum class DateSortMode {
        MostRecent,
        MostTime
    };
    
    DateSortMode getDateSortMode() const;
    void setDateSortMode(DateSortMode mode);

    bool isShowBarsEnabled() const { return showBarsEnabled; }
    void setShowBarsEnabled(bool enabled) { showBarsEnabled = enabled; }

    // Parameter state
    std::unique_ptr<juce::AudioProcessorValueTreeState> parameters;

    void setTracking(bool shouldTrack);
    juce::Array<juce::Time> getSortedDates() const;
    void setNowProviderForTests(std::function<juce::Time()> nowProvider);
    static int runEmbeddedTests();

    void addDummyDates();

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KronosAudioProcessor)
    juce::Time startTime;
    juce::int64 totalTimeInSeconds = 0;

    // A gap longer than this between credits means the machine slept or the
    // clock changed, so it is treated as an interruption rather than work.
    static constexpr juce::int64 maximumCreditableGapMs = 5 * 60 * 1000;

    // Guards the tracking data below. Hosts may save from any thread. Never
    // hold this across a parameter change, which can call back into the host.
    mutable juce::CriticalSection trackingLock;
    bool isAccumulating = false;
    juce::int64 carriedMilliseconds = 0;
    
    // Array to store session dates
    juce::Array<juce::Time> sessionDates;
    void addSessionDate(const juce::Time& dateToAdd);
    juce::String makeDateKey(const juce::Time& time) const;
    void ensureDateEntryExists(const juce::Time& dateTime, bool addSessionIfMissing);
    void addTrackedSeconds(juce::int64 seconds, const juce::Time& dateTime, const juce::String& dateKey, bool addSessionIfMissing);
    void addElapsedSecondsAcrossDates(const juce::Time& start, juce::int64 seconds, bool addSessionIfMissing);
    bool applyState(const void* data, int sizeInBytes, bool requireTrackingData);
    void beginAccumulating(const juce::Time& now);
    void creditElapsedTime(const juce::Time& now);
    void synchroniseWithTrackingParameter(const juce::Time& now);
    void markTrackingDataDirty();
    void invalidateAllSortCaches();

    juce::Time lastSaveTime;
    const int minimumSaveIntervalMs = 100; // Minimum time between saves
    juce::MemoryBlock lastSerializedState;
    bool hasSerializedState = false;
    std::atomic<bool> serializedStateDirty { true };

    bool showBarsEnabled = false;
    std::function<juce::Time()> nowProviderForTests;

    mutable juce::Array<juce::Time> cachedMostRecentDates;
    mutable juce::Array<juce::Time> cachedMostTimeDates;
    mutable std::atomic<bool> mostRecentCacheDirty { true };
    mutable std::atomic<bool> mostTimeCacheDirty { true };
    juce::Time getCurrentTime() const;

    void parameterChanged(const juce::String& parameterID, float newValue) override;
};

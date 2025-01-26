/*
  ==============================================================================

    My main plugin processor implementation.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "JucePluginDefines.h"

//==============================================================================
KronosAudioProcessor::KronosAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Create the parameter layout first
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    // Add parameters with version hints for AU compatibility
    auto trackingParam = std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("tracking", 1),  // ID and version hint
        "Tracking",
        false
    );
    
    auto darkModeParam = std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("darkMode", 1),  // ID and version hint
        "Dark Mode",
        true
    );
    
    auto dateSortModeParam = std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("dateSortMode", 1),  // ID and version hint
        "Date Sort Mode",
        juce::StringArray {"Most Recent", "Most Time"},
        0
    );

    layout.add(std::move(trackingParam));
    layout.add(std::move(darkModeParam));
    layout.add(std::move(dateSortModeParam));

    // Then create the parameters with the layout
    parameters = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "Parameters", std::move(layout));

    startTime = juce::Time::getCurrentTime();
    totalTimeInSeconds = 0;
    startTimer(1000);

    // Start tracking automatically upon instantiation
    startTracking();

#if USE_DUMMY_DATES
    addDummyDates();
#else
    addSessionDate();
#endif
}

KronosAudioProcessor::~KronosAudioProcessor()
{
    stopTimer();
}

void KronosAudioProcessor::startTracking()
{
    if (!isTracking())  // Use the accessor method
    {
        // Get current date
        auto today = juce::Time::getCurrentTime();
        juce::String dateKey = today.formatted("%Y-%m-%d");
        
        // Check if we already have time logged for today
        if (timePerDate.contains(dateKey))
        {
            // Start from the existing time for today
            totalTimeInSeconds = timePerDate[dateKey];
        }
        else
        {
            timePerDate.set(dateKey, 0);
        }
        
        startTime = juce::Time::getCurrentTime();
        setTracking(true);  // Use the parameter system
        startTimer(1000);        
    }
}

void KronosAudioProcessor::stopTracking()
{
    if (isTracking())  // Use the accessor method
    {
        // Get final time including any partial seconds
        auto currentTime = juce::Time::getCurrentTime();
        auto elapsedSeconds = (currentTime - startTime).inSeconds();
        totalTimeInSeconds += elapsedSeconds;
        
        // Store the current time for today in timePerDate
        auto today = juce::Time::getCurrentTime();
        juce::String dateKey = today.formatted("%Y-%m-%d");
        
        // Get existing time for today (if any) and add elapsed time
        auto existingTime = timePerDate[dateKey];
        timePerDate.set(dateKey, existingTime + elapsedSeconds);
        
        stopTimer();
        setTracking(false);  // Use the parameter system
                
        startTime = juce::Time::getCurrentTime();
    }
}

juce::int64 KronosAudioProcessor::getTotalTimeInSeconds() const
{
    if (isTracking())
    {
        // Return real-time value including partial seconds
        auto currentTime = juce::Time::getCurrentTime();
        auto elapsedSeconds = (currentTime - startTime).inSeconds();
        return totalTimeInSeconds + elapsedSeconds;
    }
    return totalTimeInSeconds;
}

//==============================================================================
const juce::String KronosAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool KronosAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool KronosAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool KronosAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double KronosAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int KronosAudioProcessor::getNumPrograms()
{
    return 1;   // I need at least 1 program or some hosts might complain
}

int KronosAudioProcessor::getCurrentProgram()
{
    return 0;
}

void KronosAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String KronosAudioProcessor::getProgramName (int index)
{
    return {};
}

void KronosAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void KronosAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Nothing to implement here since my plugin doesn't use any audio processing. 
    // It's just a timer and a bunch of UI elements.
    // Since JUCE wants to see this method, I'm just going to leave it empty.
}

void KronosAudioProcessor::releaseResources()
{
    // When the plugin is released (project closed/plugin removed)
    if (isTracking())
    {
        juce::MemoryBlock state;
        getStateInformation(state);
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool KronosAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // I'm only supporting mono and stereo layouts here.
    // Some hosts (like certain GarageBand versions) will only
    // load plugins that support stereo bus layouts.
    // Not that it matters because this plugin doesn't use any audio processing.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void KronosAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Process the audio...
}

//==============================================================================
bool KronosAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* KronosAudioProcessor::createEditor()
{
    return new KronosAudioProcessorEditor (*this);
}

//==============================================================================
void KronosAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Add debouncing
    auto now = juce::Time::getCurrentTime();
    if ((now - lastSaveTime).inMilliseconds() < minimumSaveIntervalMs)
    {
        return;
    }
    lastSaveTime = now;

    auto state = parameters->copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void KronosAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        parameters->replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KronosAudioProcessor();
}

void KronosAudioProcessor::suspendProcessing(bool shouldSuspend)
{
    if (shouldSuspend)
    {
        juce::MemoryBlock state;
        getStateInformation(state);
    }
}

void KronosAudioProcessor::timerCallback()
{
    if (isTracking())  // Use the accessor method
    {
        // Increment main display time
        totalTimeInSeconds++;
        
        // Update per-date tracking separately
        auto currentTime = juce::Time::getCurrentTime();
        juce::String dateKey = currentTime.formatted("%Y-%m-%d");
        
        // Initialize new date if needed
        if (!timePerDate.contains(dateKey))
        {
            timePerDate.set(dateKey, 0);
            DBG("New date initialized: " + dateKey);
        }
        
        // Get existing time and increment
        auto existingTime = timePerDate[dateKey];
        timePerDate.set(dateKey, existingTime + 1);
        
        // Update start time to maintain accuracy
        startTime = currentTime;
    }
}

void KronosAudioProcessor::addSessionDate()
{
    auto today = juce::Time::getCurrentTime();
    juce::String dateKey = today.formatted("%Y-%m-%d");
    
    // Check if we already have today's date
    bool dateExists = false;
    for (auto& date : sessionDates)
    {
        if (date.getDayOfMonth() == today.getDayOfMonth() &&
            date.getMonth() == today.getMonth() &&
            date.getYear() == today.getYear())
        {
            dateExists = true;
            break;
        }
    }
    
    if (!dateExists)
    {
        sessionDates.insert(0, today);  // Add to front of array
    }
}

void KronosAudioProcessor::setDarkMode(bool isDark)
{
    auto* param = parameters->getParameter("darkMode");
    if (param != nullptr)
        param->setValueNotifyingHost(isDark ? 1.0f : 0.0f);
}

bool KronosAudioProcessor::isSuspended() const
{
    return AudioProcessor::isSuspended();
}

juce::int64 KronosAudioProcessor::getTimeForDate(const juce::Time& date) const
{
    juce::String dateKey = date.formatted("%Y-%m-%d");
    return timePerDate[dateKey];
}

void KronosAudioProcessor::setDateSortMode(KronosAudioProcessor::DateSortMode mode)
{
    auto* param = parameters->getParameter("dateSortMode");
    if (param != nullptr)
        param->setValueNotifyingHost(static_cast<float>(mode));
}

void KronosAudioProcessor::addDummyDates()
{
    // Clear existing dates for testing
    sessionDates.clear();
    timePerDate.clear();

    // Add 10 dummy dates with different times
    juce::Time baseDate = juce::Time::getCurrentTime();
    
    // First add a date with a very large time value (over 100 hours)
    // juce::Time longDate = baseDate - juce::RelativeTime::days(10);
    // sessionDates.add(longDate);
    // juce::String longDateKey = longDate.formatted("%Y-%m-%d");
    // timePerDate.set(longDateKey, 400000);  // About 111 hours
    
    // Then add the regular test dates
    for (int i = 0; i < 9; ++i)  // Reduced to 9 to keep total at 10 with long session
    {
        // Create dates going backwards from today
        juce::Time date = baseDate - juce::RelativeTime::days(i);
        sessionDates.add(date);
        
        // Add varying times (increasing pattern for testing)
        juce::String dateKey = date.formatted("%Y-%m-%d");
        timePerDate.set(dateKey, (i + 1) * 300);  // Varying seconds (5 min increments)
    }
}

// Implement the accessor methods
void KronosAudioProcessor::setTracking(bool shouldTrack)
{
    auto* param = parameters->getParameter("tracking");
    if (param != nullptr)
        param->setValueNotifyingHost(shouldTrack ? 1.0f : 0.0f);
}

bool KronosAudioProcessor::isTracking() const
{
    return parameters->getRawParameterValue("tracking")->load() >= 0.5f;
}

bool KronosAudioProcessor::isDarkMode() const
{
    return parameters->getRawParameterValue("darkMode")->load() >= 0.5f;
}

KronosAudioProcessor::DateSortMode KronosAudioProcessor::getDateSortMode() const
{
    return static_cast<KronosAudioProcessor::DateSortMode>(
        parameters->getRawParameterValue("dateSortMode")->load());
}

juce::Array<juce::Time> KronosAudioProcessor::getSortedDates() const
{
    juce::Array<juce::Time> sortedDates = sessionDates;
    
    if (getDateSortMode() == DateSortMode::MostTime)
    {
        struct TimeComparator
        {
            const KronosAudioProcessor* processor;
            TimeComparator(const KronosAudioProcessor* p) : processor(p) {}
            
            int compareElements(juce::Time first, juce::Time second)
            {
                auto timeA = processor->getTimeForDate(first);
                auto timeB = processor->getTimeForDate(second);
                return timeB < timeA ? -1 : (timeB > timeA ? 1 : 0);
            }
        };
        
        TimeComparator comparator(this);
        sortedDates.sort(comparator);
    }
    else // DateSortMode::MostRecent
    {
        struct DateComparator
        {
            static int compareElements(juce::Time first, juce::Time second)
            {
                return second < first ? -1 : (second > first ? 1 : 0);
            }
        };
        
        DateComparator comparator;
        sortedDates.sort(comparator);
    }
    
    return sortedDates;
}

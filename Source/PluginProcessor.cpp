/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

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
                       ),
       parameters(*this, nullptr, "Parameters", {})
#endif
{
    startTime = juce::Time::getCurrentTime();
    isTracking = true;
    totalTimeInSeconds = 0; // Initialize total time to 0 when plugin is loaded
    // totalTimeInSeconds = UNCOMMENT FOR TESTING LARGE TIME ONLY
    // 359970;  // This is 99:59:45 in seconds (99 * 3600 + 59 * 60 + 45)
    darkModeEnabled = true;  // Set default dark mode
    startTimer(1000);

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
    if (!isTracking)
    {
        // Get current date
        auto today = juce::Time::getCurrentTime();
        juce::String dateKey = today.formatted("%Y-%m-%d");
        
        // Check if we already have time logged for today
        if (timePerDate.contains(dateKey))
        {
            // Start from the existing time for today
            totalTimeInSeconds = timePerDate[dateKey];
            DBG("Resuming tracking from existing time: " + juce::String(totalTimeInSeconds) + " seconds");
        }
        else
        {
            // Start fresh for a new day in timePerDate only
            timePerDate.set(dateKey, 0);  // Initialize new date at 0
            // Keep existing totalTimeInSeconds
            DBG("Starting fresh tracking for new date: " + dateKey + " with 0 seconds");
        }
        
        startTime = juce::Time::getCurrentTime();
        isTracking = true;
        startTimer(1000);  // Start the timer for updates
        
        DBG("Tracking started with initial time: " + juce::String(totalTimeInSeconds));
    }
}

void KronosAudioProcessor::stopTracking()
{
    if (isTracking)
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
        
        stopTimer();  // Stop the timer
        
        DBG("Tracking stopped. Total time for " + dateKey + ": " + 
            juce::String(timePerDate[dateKey]) + " seconds");
        
        isTracking = false;
        startTime = juce::Time::getCurrentTime();  // Reset start time
    }
}

juce::int64 KronosAudioProcessor::getTotalTimeInSeconds() const
{
    if (isTracking)
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
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
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void KronosAudioProcessor::releaseResources()
{
    // When the plugin is released (project closed/plugin removed)
    if (isTracking)
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
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

    juce::ValueTree state("KronosState");
    
    state.setProperty("isTracking", isTracking, nullptr);
    state.setProperty("totalTimeInSeconds", totalTimeInSeconds, nullptr);
    state.setProperty("darkMode", darkModeEnabled, nullptr);
    
    // Add new state properties
    state.setProperty("sortMode", (int)currentSortMode, nullptr);
    state.setProperty("showBars", showBarsEnabled, nullptr);
    
    // Save session dates
    juce::StringArray dateStrings;
    for (auto& date : sessionDates)
    {
        dateStrings.add(date.toISO8601(true));
    }
    state.setProperty("sessionDates", dateStrings.joinIntoString(";"), nullptr);
    
    // Save time per date
    juce::String timePerDateString;
    for (juce::HashMap<juce::String, juce::int64>::Iterator i(timePerDate); i.next();)
    {
        timePerDateString += i.getKey() + "=" + juce::String(i.getValue()) + ";";
    }
    state.setProperty("timePerDate", timePerDateString, nullptr);
    
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void KronosAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    
    if (xml.get() != nullptr)
    {
        juce::ValueTree state = juce::ValueTree::fromXml(*xml);
        
        // Stop any current timing operations
        stopTimer();
        
        // Load time per date first
        if (state.hasProperty("timePerDate"))
        {
            juce::String timePerDateString = state.getProperty("timePerDate");
            juce::StringArray pairs;
            pairs.addTokens(timePerDateString, ";", "");
            
            for (const auto& pair : pairs)
            {
                if (pair.isNotEmpty())
                {
                    auto parts = juce::StringArray::fromTokens(pair, "=", "");
                    if (parts.size() == 2)
                    {
                        timePerDate.set(parts[0], parts[1].getLargeIntValue());
                    }
                }
            }
        }
        
        // Get current date's time if it exists
        auto today = juce::Time::getCurrentTime();
        juce::String dateKey = today.formatted("%Y-%m-%d");
        if (timePerDate.contains(dateKey))
        {
            totalTimeInSeconds = timePerDate[dateKey];
            DBG("Loaded existing time for today: " + juce::String(totalTimeInSeconds));
        }
        
        // Load play/pause state
        bool newTrackingState = state.getProperty("isTracking", false);
        isTracking = newTrackingState;
        
        if (isTracking && !isSuspended())
        {
            startTime = juce::Time::getCurrentTime();
            startTimer(1000);
        }
        
        // Load session dates
        sessionDates.clear();
        if (state.hasProperty("sessionDates"))
        {
            juce::String datesString = state.getProperty("sessionDates");
            juce::StringArray dateStrings;
            dateStrings.addTokens(datesString, ";", "");
            
            for (const auto& dateStr : dateStrings)
            {
                juce::Time date = juce::Time::fromISO8601(dateStr);
                if (date != juce::Time())  // Valid date
                {
                    sessionDates.add(date);
                }
            }
        }
        
        // Load dark mode state
        darkModeEnabled = state.getProperty("darkMode", true);
        
        // Always add today's date when loading
        addSessionDate();
        
        // Load sort mode and bars mode
        if (state.hasProperty("sortMode"))
            currentSortMode = (DateSortMode)(int)state.getProperty("sortMode");
        
        if (state.hasProperty("showBars"))
            showBarsEnabled = state.getProperty("showBars");
        
        if (onStateLoaded)
        {
            onStateLoaded();
        }
    }
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
    if (isTracking)
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
        if (sessionDates.size() > 10)   // Keep only last 10 sessions
            sessionDates.removeLast();
    }
}

void KronosAudioProcessor::setDarkMode(bool isDark)
{
    darkModeEnabled = isDark;
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

void KronosAudioProcessor::toggleDateSortMode()
{
    currentSortMode = (currentSortMode == DateSortMode::MostRecent) ? 
                      DateSortMode::MostTime : DateSortMode::MostRecent;
}

juce::Array<juce::Time> KronosAudioProcessor::getSortedDates() const
{
    juce::Array<juce::Time> sortedDates = sessionDates;
    
    if (currentSortMode == DateSortMode::MostTime)
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

void KronosAudioProcessor::addDummyDates()
{
    // Clear existing dates for testing
    sessionDates.clear();
    timePerDate.clear();

    // Add 10 dummy dates with different times
    juce::Time baseDate = juce::Time::getCurrentTime();
    
    // First add a date with a very large time value (over 100 hours)
    juce::Time longDate = baseDate - juce::RelativeTime::days(10);
    sessionDates.add(longDate);
    juce::String longDateKey = longDate.formatted("%Y-%m-%d");
    timePerDate.set(longDateKey, 400000);  // About 111 hours
    
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

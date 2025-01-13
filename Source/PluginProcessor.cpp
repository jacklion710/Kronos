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
    currentDateKey = juce::Time::getCurrentTime().formatted("%Y-%m-%d");
    startTime = juce::Time::getCurrentTime();
    isTracking = true;
    totalTimeInSeconds = 0;
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
            // Start fresh for a new day
            totalTimeInSeconds = 0;
            DBG("Starting fresh tracking for new date: " + dateKey);
        }
        
        startTime = juce::Time::getCurrentTime();
        isTracking = true;
        startTimer(1000);  // Start the timer for updates
        
        // Immediately update the time to ensure sync
        timePerDate.set(dateKey, totalTimeInSeconds);
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
        
        // Store the current total time for today
        auto today = juce::Time::getCurrentTime();
        juce::String dateKey = today.formatted("%Y-%m-%d");
        timePerDate.set(dateKey, totalTimeInSeconds);
        
        stopTimer();  // Stop the timer
        
        DBG("Tracking stopped. Total time for " + dateKey + ": " + 
            juce::String(totalTimeInSeconds) + " seconds");
        
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
        
        // Load total time first
        totalTimeInSeconds = state.getProperty("totalTimeInSeconds", (juce::int64)0);
        
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

void KronosAudioProcessor::checkAndHandleDateChange()
{
    auto now = juce::Time::getCurrentTime();
    juce::String newDateKey = now.formatted("%Y-%m-%d");
    
    // If the date has changed
    if (currentDateKey != newDateKey)
    {
        if (isTracking)
        {
            // Save the final time for the previous date
            if (currentDateKey.isNotEmpty())
            {
                auto finalTime = timePerDate[currentDateKey];
                timePerDate.set(currentDateKey, finalTime);
            }
            
            // Initialize the new date with 0 time
            currentDateKey = newDateKey;
            timePerDate.set(currentDateKey, 0);
            addSessionDate(); // Add the new date to our sessions
        }
        else
        {
            currentDateKey = newDateKey;
        }
    }
}

void KronosAudioProcessor::timerCallback()
{
    if (isTracking)
    {
        // Check for date change first
        checkAndHandleDateChange();
        
        // Increment total time
        totalTimeInSeconds++;
        
        // Update current date's time independently
        auto currentTime = timePerDate[currentDateKey];
        timePerDate.set(currentDateKey, currentTime + 1);
        
        // Update start time to maintain accuracy
        startTime = juce::Time::getCurrentTime();
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
        auto* rawData = sortedDates.getRawDataPointer();
        std::sort(rawData, rawData + sortedDates.size(), 
            [this](const juce::Time& first, const juce::Time& second)
            {
                juce::String dateKeyA = first.formatted("%Y-%m-%d");
                juce::String dateKeyB = second.formatted("%Y-%m-%d");
                
                juce::int64 timeA = timePerDate[dateKeyA];
                juce::int64 timeB = timePerDate[dateKeyB];
                
                return timeA > timeB;
            });
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
    
    for (int i = 0; i < 10; ++i)
    {
        // Create dates going backwards from today
        juce::Time date = baseDate - juce::RelativeTime::days(i);
        sessionDates.add(date);
        
        // Add varying times (increasing pattern for testing)
        juce::String dateKey = date.formatted("%Y-%m-%d");
        timePerDate.set(dateKey, (i + 1) * 300);  // Varying seconds (5 min increments)
    }
}

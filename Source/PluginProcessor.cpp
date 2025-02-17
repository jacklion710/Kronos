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

    parameters->addParameterListener("dateSortMode", dynamic_cast<juce::AudioProcessorValueTreeState::Listener*>(this));
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

    // Create the base state from parameters
    auto state = parameters->copyState();
    
    // Add tracking data as child elements
    auto trackingData = state.getOrCreateChildWithName("TrackingData", nullptr);
    
    // Save total time
    trackingData.setProperty("totalTimeInSeconds", totalTimeInSeconds, nullptr);
    
    // Save dates and their times
    auto datesElement = trackingData.getOrCreateChildWithName("DatesData", nullptr);
    datesElement.removeAllChildren(nullptr);
    
    // Store session dates
    for (const auto& date : sessionDates)
    {
        auto dateElement = juce::ValueTree("Date");
        dateElement.setProperty("timestamp", date.toMilliseconds(), nullptr);
        datesElement.appendChild(dateElement, nullptr);
    }
    
    // Store time per date
    auto timePerDateElement = trackingData.getOrCreateChildWithName("TimePerDate", nullptr);
    timePerDateElement.removeAllChildren(nullptr);
    
    for (auto it = timePerDate.begin(); it != timePerDate.end(); ++it)
    {
        auto entry = juce::ValueTree("DateEntry");
        entry.setProperty("key", it.getKey(), nullptr);
        entry.setProperty("time", it.getValue(), nullptr);
        timePerDateElement.appendChild(entry, nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void KronosAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    
    if (xmlState != nullptr)
    {
        auto state = juce::ValueTree::fromXml(*xmlState);
        parameters->replaceState(state);
        
        // Restore tracking data
        auto trackingData = state.getChildWithName("TrackingData");
        if (trackingData.isValid()) 
        {
            // Restore total time
            totalTimeInSeconds = trackingData.getProperty("totalTimeInSeconds", 0);
            
            // Restore dates
            sessionDates.clear();
            auto datesElement = trackingData.getChildWithName("DatesData");
            if (datesElement.isValid()) 
            {
                for (auto dateElement : datesElement)
                {
                    auto timestamp = dateElement.getProperty("timestamp").toString().getLargeIntValue();
                    sessionDates.add(juce::Time(timestamp));
                }
            }
            
            // Restore time per date
            timePerDate.clear();
            auto timePerDateElement = trackingData.getChildWithName("TimePerDate");
            if (timePerDateElement.isValid())  
            {
                for (auto entry : timePerDateElement)
                {
                    auto key = entry.getProperty("key").toString();
                    auto time = entry.getProperty("time");
                    timePerDate.set(key, static_cast<juce::int64>(time));
                }
            }
            
            // Check if we need to add today's date
            auto today = juce::Time::getCurrentTime();
            juce::String todayKey = today.formatted("%Y-%m-%d");
            
            if (!timePerDate.contains(todayKey))
            {
                timePerDate.set(todayKey, 0);
                addSessionDate();
            }
            
            sortedDatesNeedsRefresh = true;
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
    if (isTracking())  // Use the accessor method
    {
        // Timer callback duration enter
        auto timerCallbackStart = juce::Time::getMillisecondCounterHiRes();

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

        // Check for midnight crossing
        static juce::String lastDateKey = juce::Time::getCurrentTime().formatted("%Y-%m-%d");
        juce::String currentDateKey = juce::Time::getCurrentTime().formatted("%Y-%m-%d");
        
        if (lastDateKey != currentDateKey)
        {
            addSessionDate();
            sortedDatesNeedsRefresh = true;
            lastDateKey = currentDateKey;
        }

        // Timer callback duration exit
        DBG("Timer callback duration: " << (juce::Time::getMillisecondCounterHiRes() - timerCallbackStart) << " ms");
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
    // Clear existing dates and times
    sessionDates.clear();
    timePerDate.clear();
    
    // Set a realistic total accumulated time (around 180 hours)
    totalTimeInSeconds = 648000;  // 180 hours in seconds
    
    // Create base date as current time
    juce::Time baseDate = juce::Time::getCurrentTime();
    
    // Recent dates (last 5 days) with higher activity
    for (int i = 0; i < 5; ++i)
    {
        juce::Time date = baseDate - juce::RelativeTime::days(i);
        sessionDates.add(date);
        
        // Vary times between 2-6 hours for recent days
        juce::String dateKey = date.formatted("%Y-%m-%d");
        int hoursWorked = 2 + (rand() % 4);  // Random between 2-6 hours
        timePerDate.set(dateKey, hoursWorked * 3600);  // Convert to seconds
    }
    
    // Add some medium-length sessions from last week
    for (int i = 6; i < 12; ++i)
    {
        juce::Time date = baseDate - juce::RelativeTime::days(i);
        sessionDates.add(date);
        
        // Medium sessions 4-7 hours
        juce::String dateKey = date.formatted("%Y-%m-%d");
        int hoursWorked = 4 + (rand() % 3);  // Random between 4-7 hours
        timePerDate.set(dateKey, hoursWorked * 3600);
    }
    
    // Add sparse older dates with varying times
    int olderDates[] = {14, 16, 19, 22, 25, 28};
    for (int daysAgo : olderDates)
    {
        juce::Time date = baseDate - juce::RelativeTime::days(daysAgo);
        sessionDates.add(date);
        
        // Varying times 3-8 hours
        juce::String dateKey = date.formatted("%Y-%m-%d");
        int hoursWorked = 3 + (rand() % 5);  // Random between 3-8 hours
        timePerDate.set(dateKey, hoursWorked * 3600);
    }
    
    // Add a few very productive days scattered throughout
    int longDates[] = {8, 17, 24};
    for (int daysAgo : longDates)
    {
        juce::Time date = baseDate - juce::RelativeTime::days(daysAgo);
        sessionDates.add(date);
        
        // Long sessions 9-12 hours
        juce::String dateKey = date.formatted("%Y-%m-%d");
        int hoursWorked = 9 + (rand() % 3);  // Random between 9-12 hours
        timePerDate.set(dateKey, hoursWorked * 3600);
    }
    
    sortedDatesNeedsRefresh = true;
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
    // Only sort if something has changed
    if (!sortedDatesNeedsRefresh)
        return cachedSortedDates;

    // Sorting duration enter
    auto sortStart = juce::Time::getMillisecondCounterHiRes();

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
    
    // Update cache and reset flag
    cachedSortedDates = sortedDates;
    sortedDatesNeedsRefresh = false;
    
    // Sorting duration exit
    DBG("Date sorting took: " << (juce::Time::getMillisecondCounterHiRes() - sortStart) << " ms");
    
    return cachedSortedDates;
}

void KronosAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "dateSortMode")
    {
        sortedDatesNeedsRefresh = true;
    }
}

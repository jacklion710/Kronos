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
    totalTimeInSeconds = 0;
    darkModeEnabled = true;  // Set default dark mode
    startTimer(1000);
    addSessionDate();
}

KronosAudioProcessor::~KronosAudioProcessor()
{
    stopTimer();
}

void KronosAudioProcessor::startTracking()
{
    if (!isTracking && !isSuspended())
    {
        isTracking = true;
        startTime = juce::Time::getCurrentTime();
        startTimer(1000);
        
        // Save state immediately after changing it
        juce::MemoryBlock state;
        getStateInformation(state);
    }
}

void KronosAudioProcessor::stopTracking()
{
    if (isTracking)
    {
        isTracking = false;
        auto currentTime = juce::Time::getCurrentTime();
        totalTimeInSeconds += (currentTime - startTime).inSeconds();
        stopTimer();
        
        // Save state immediately after changing it
        juce::MemoryBlock state;
        getStateInformation(state);
    }
}

juce::int64 KronosAudioProcessor::getTotalTimeInSeconds() const
{
    if (isTracking)
    {
        auto currentTime = juce::Time::getCurrentTime();
        return totalTimeInSeconds + (currentTime - startTime).inSeconds();
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
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
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
        totalTimeInSeconds++;
        
        // Update today's time
        auto today = juce::Time::getCurrentTime();
        juce::String dateKey = today.formatted("%Y-%m-%d");
        timePerDate.set(dateKey, timePerDate[dateKey] + 1);
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

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
                       )
#endif
{
    isTracking = false;
    totalTimeInSeconds = 0;
}

KronosAudioProcessor::~KronosAudioProcessor()
{
    isTracking = false;
    totalTimeInSeconds = 0;
}

void KronosAudioProcessor::startTracking()
{
    if (!isTracking)
    {
        startTime = juce::Time::getCurrentTime();
        isTracking = true;
    }
}

void KronosAudioProcessor::stopTracking()
{
    if (isTracking)
    {
        auto currentTime = juce::Time::getCurrentTime();
        totalTimeInSeconds += (currentTime - startTime).inSeconds();
        isTracking = false;
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

void KronosAudioProcessor::saveToJson(const juce::File& file)
{
    juce::DynamicObject* dataObject = new juce::DynamicObject();
    
    // Data to save
    dataObject->setProperty("totalTimeInSeconds", getTotalTimeInSeconds());
    dataObject->setProperty("isTracking", isTracking);
    dataObject->setProperty("startTime", startTime.toISO8601(true));

    // Convert to JSON string
    juce::var jsonVar = juce::JSON::toString(dataObject);
    juce::String jsonString = juce::JSON::toString(jsonVar);

    // Write to file
    file.replaceWithText(jsonString);
}

juce::File KronosAudioProcessor::getDefaultSaveDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("KronosPlugin");
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
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
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

    static juce::String lastProjectName;
    juce::String currentProjectName = getHostProjectName();
    
    // Check if project was just saved (name changed from *_Unsaved to something else)
    if (lastProjectName.endsWith("_Unsaved") && !currentProjectName.endsWith("_Unsaved"))
    {
        // Load data from unsaved file
        juce::File unsavedFile = getPluginStateDirectory()
            .getChildFile("KronosProjectsTimeData")
            .getChildFile(lastProjectName + ".json");
            
        if (unsavedFile.exists())
        {
            // Save to new location first
            autoSaveState();
            
            // Then delete the unsaved file
            unsavedFile.deleteFile();
        }
    }
    
    lastProjectName = currentProjectName;

    // Regular autosave check
    static juce::int64 lastSaveTime = 0;
    juce::int64 currentTime = juce::Time::currentTimeMillis();
    
    // Auto-save every minute
    if (currentTime - lastSaveTime > 60000) // 60000ms = 1 minute
    {
        autoSaveState();
        lastSaveTime = currentTime;
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
void KronosAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void KronosAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KronosAudioProcessor();
}

juce::File KronosAudioProcessor::getPluginStateDirectory()
{
    // Get the user's application data directory
    #if JUCE_MAC
        // On macOS: ~/Library/Application Support/Kronos
        juce::File dataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                .getChildFile("Application Support")
                                .getChildFile("Kronos");
    #else
        // On Windows: %APPDATA%\Kronos
        // On Linux: ~/.config/Kronos
        juce::File dataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                                .getChildFile("Kronos");
    #endif
    
    // Ensure the directory exists
    if (!dataDir.exists())
        dataDir.createDirectory();
        
    return dataDir;
}

void KronosAudioProcessor::autoSaveState()
{
    juce::File dataDir = getPluginStateDirectory();
    juce::File projectsDir = dataDir.getChildFile("KronosProjectsTimeData");
    
    if (!projectsDir.exists())
        projectsDir.createDirectory();
    
    juce::String filename = getHostProjectName() + ".json";
    juce::File stateFile = projectsDir.getChildFile(filename);
    
    DBG("Saving state to: " + stateFile.getFullPathName());
    
    juce::DynamicObject* dataObject = new juce::DynamicObject();
    
    // Add your data to save
    dataObject->setProperty("totalTimeInSeconds", getTotalTimeInSeconds());
    dataObject->setProperty("isTracking", isTracking);
    dataObject->setProperty("lastSaveTime", juce::Time::getCurrentTime().toISO8601(true));
    
    // Add host information
    juce::PluginHostType hostType;
    dataObject->setProperty("hostName", hostType.getHostDescription());
    dataObject->setProperty("hostPath", juce::PluginHostType::getHostPath());
    
    juce::var jsonVar(dataObject);
    juce::String jsonString = juce::JSON::toString(jsonVar);
    
    stateFile.replaceWithText(jsonString);
}

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "JucePluginDefines.h"

// [...previous code remains unchanged until getHostProjectName()]

juce::String KronosAudioProcessor::getHostProjectName()
{
    juce::PluginHostType hostType;
    juce::String projectName;
    
    // Try to get the actual project name from the host
    auto* wrapper = getActiveEditor();
    if (wrapper != nullptr)
    {
        projectName = wrapper->getName();
        
        // If we got a meaningful project name, use it
        if (!projectName.isEmpty() && projectName != "Unknown" && 
            projectName != hostType.getHostDescription())
        {
            projectName = juce::File::createLegalFileName(projectName);
            return projectName;
        }
    }
    
    // Fallback to host name if no project name available
    if (hostType.isAbletonLive())
        projectName = "Ableton_Unsaved";
    else if (hostType.isLogic())
        projectName = "Logic_Unsaved";
    else if (hostType.isCubase())
        projectName = "Cubase_Unsaved";
    else if (hostType.isReaper())
        projectName = "Reaper_Unsaved";
    else if (hostType.isStudioOne())
        projectName = "StudioOne_Unsaved";
    else if (hostType.isProTools())
        projectName = "ProTools_Unsaved";
    else if (hostType.isFruityLoops())
        projectName = "FL_Studio_Unsaved";
    else
        projectName = juce::String(hostType.getHostDescription()) + juce::String("_Unsaved");    
    
    // Remove any illegal characters
    projectName = juce::File::createLegalFileName(projectName);
    
    return projectName;
}

/*
  ==============================================================================

    My main plugin processor implementation.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "JucePluginDefines.h"
#include <atomic>

namespace
{
std::atomic<bool> gEmbeddedTestsRunning { false };

bool isEnvEnabled(const char* key)
{
    return juce::SystemStats::getEnvironmentVariable(key, "0") == "1";
}

void emitEmbeddedTestLog(const juce::String& message)
{
    juce::Logger::writeToLog(message);

    const auto logPath = juce::SystemStats::getEnvironmentVariable("KRONOS_TEST_LOG_FILE", {});
    if (logPath.isNotEmpty())
        juce::File(logPath).appendText(message + "\n");
}

class StreamingUnitTestRunner : public juce::UnitTestRunner
{
public:
    explicit StreamingUnitTestRunner(const juce::File& logFileToUse)
        : logFile(logFileToUse)
    {
    }

    void logMessage(const juce::String& message) override
    {
        juce::UnitTestRunner::logMessage(message);

        if (logFile != juce::File() && !message.isEmpty())
            logFile.appendText(message + "\n");
    }

private:
    juce::File logFile;
};
}

//==============================================================================
KronosAudioProcessor::KronosAudioProcessor(std::function<juce::Time()> nowProvider)
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
    nowProviderForTests = std::move(nowProvider);
    const bool isTestMode = isEnvEnabled("KRONOS_TEST_MODE") || isEnvEnabled("KRONOS_RUN_TESTS");

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

    startTime = getCurrentTime();
    totalTimeInSeconds = 0;
    lastTimerDateKey = makeDateKey(startTime);

    if (!isTestMode)
    {
        startTimer(1000);
        // Start tracking automatically upon instantiation
        startTracking();
    }
    else
    {
        setTracking(false);
    }

#if USE_DUMMY_DATES
    addDummyDates();
#else
    addSessionDate(getCurrentTime());
#endif

    parameters->addParameterListener("tracking", this);
    parameters->addParameterListener("darkMode", this);
    parameters->addParameterListener("dateSortMode", this);

    if (isEnvEnabled("KRONOS_RUN_TESTS")
        && juce::JUCEApplicationBase::isStandaloneApp()
        && !gEmbeddedTestsRunning.load())
    {
        const auto failures = KronosAudioProcessor::runEmbeddedTests();
        const auto resultPath = juce::SystemStats::getEnvironmentVariable("KRONOS_TEST_RESULTS_FILE", {});
        if (resultPath.isNotEmpty())
        {
            juce::File(resultPath).replaceWithText(juce::String(failures));
        }
        juce::MessageManager::callAsync([] { juce::JUCEApplicationBase::quit(); });
    }
}

KronosAudioProcessor::~KronosAudioProcessor()
{
    stopTimer();
    if (parameters != nullptr)
    {
        parameters->removeParameterListener("tracking", this);
        parameters->removeParameterListener("darkMode", this);
        parameters->removeParameterListener("dateSortMode", this);
    }
}

void KronosAudioProcessor::startTracking()
{
    if (!isTracking())  // Use the accessor method
    {
        const auto now = getCurrentTime();
        ensureDateEntryExists(now, true);

        startTime = now;
        lastTimerDateKey = makeDateKey(now);
        setTracking(true);  // Use the parameter system
        startTimer(1000);        
    }
}

void KronosAudioProcessor::stopTracking()
{
    if (isTracking())  // Use the accessor method
    {
        // Get final time including any partial seconds.
        const auto now = getCurrentTime();
        addElapsedSecondsAcrossDates(startTime, now, true);

        stopTimer();
        setTracking(false);  // Use the parameter system

        startTime = now;
        lastTimerDateKey = makeDateKey(now);
    }
}

juce::int64 KronosAudioProcessor::getTotalTimeInSeconds() const
{
    if (isTracking())
    {
        // Return real-time value including partial seconds
        auto currentTime = getCurrentTime();
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
    // Debounce serialization work, but always return valid state data.
    auto now = getCurrentTime();
    if ((now - lastSaveTime).inMilliseconds() < minimumSaveIntervalMs
        && hasSerializedState
        && !serializedStateDirty)
    {
        destData = lastSerializedState;
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
    lastSerializedState = destData;
    hasSerializedState = true;
    serializedStateDirty = false;
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
            auto today = getCurrentTime();
            juce::String todayKey = makeDateKey(today);
            
            if (!timePerDate.contains(todayKey))
            {
                timePerDate.set(todayKey, 0);
                addSessionDate(today);
            }

            invalidateAllSortCaches();
            markTrackingDataDirty();
            lastTimerDateKey = makeDateKey(today);
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
        const auto now = getCurrentTime();
        addElapsedSecondsAcrossDates(startTime, now, true);
        startTime = now;
        lastTimerDateKey = makeDateKey(now);
    }
}

void KronosAudioProcessor::addSessionDate(const juce::Time& dateToAdd)
{
    juce::String dateKey = makeDateKey(dateToAdd);
    
    // Check if we already have today's date
    bool dateExists = false;
    for (auto& date : sessionDates)
    {
        if (makeDateKey(date) == dateKey)
        {
            dateExists = true;
            break;
        }
    }
    
    if (!dateExists)
    {
        sessionDates.insert(0, dateToAdd);  // Add to front of array
        mostRecentCacheDirty = true;
        mostTimeCacheDirty = true;
        markTrackingDataDirty();
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
    juce::String dateKey = makeDateKey(date);
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
    juce::Time baseDate = getCurrentTime();
    
    // Recent dates (last 5 days) with higher activity
    for (int i = 0; i < 5; ++i)
    {
        juce::Time date = baseDate - juce::RelativeTime::days(i);
        sessionDates.add(date);
        
        // Vary times between 2-6 hours for recent days
        juce::String dateKey = makeDateKey(date);
        int hoursWorked = 2 + (rand() % 4);  // Random between 2-6 hours
        timePerDate.set(dateKey, hoursWorked * 3600);  // Convert to seconds
    }
    
    // Add some medium-length sessions from last week
    for (int i = 6; i < 12; ++i)
    {
        juce::Time date = baseDate - juce::RelativeTime::days(i);
        sessionDates.add(date);
        
        // Medium sessions 4-7 hours
        juce::String dateKey = makeDateKey(date);
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
        juce::String dateKey = makeDateKey(date);
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
        juce::String dateKey = makeDateKey(date);
        int hoursWorked = 9 + (rand() % 3);  // Random between 9-12 hours
        timePerDate.set(dateKey, hoursWorked * 3600);
    }

    invalidateAllSortCaches();
    markTrackingDataDirty();
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
    const auto mode = getDateSortMode();
    if (mode == DateSortMode::MostRecent && !mostRecentCacheDirty)
        return cachedMostRecentDates;
    if (mode == DateSortMode::MostTime && !mostTimeCacheDirty)
        return cachedMostTimeDates;

    juce::Array<juce::Time> sortedDates = sessionDates;
    
    if (mode == DateSortMode::MostTime)
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
        cachedMostTimeDates = sortedDates;
        mostTimeCacheDirty = false;
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
        cachedMostRecentDates = sortedDates;
        mostRecentCacheDirty = false;
    }

    return sortedDates;
}

void KronosAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused(newValue);
    serializedStateDirty = true;

    if (parameterID == "dateSortMode")
    {
        mostRecentCacheDirty = true;
        mostTimeCacheDirty = true;
    }
}

juce::String KronosAudioProcessor::makeDateKey(const juce::Time& time) const
{
    return time.formatted("%Y-%m-%d");
}

void KronosAudioProcessor::markTrackingDataDirty()
{
    serializedStateDirty = true;
}

void KronosAudioProcessor::invalidateAllSortCaches()
{
    mostRecentCacheDirty = true;
    mostTimeCacheDirty = true;
}

void KronosAudioProcessor::ensureDateEntryExists(const juce::Time& dateTime, bool addSessionIfMissing)
{
    const auto dateKey = makeDateKey(dateTime);
    if (timePerDate.contains(dateKey))
        return;

    timePerDate.set(dateKey, 0);
    if (addSessionIfMissing)
        addSessionDate(dateTime);
    else
        markTrackingDataDirty();

    mostTimeCacheDirty = true;
}

void KronosAudioProcessor::addTrackedSeconds(juce::int64 seconds, const juce::Time& dateTime, bool addSessionIfMissing)
{
    if (seconds <= 0)
        return;

    ensureDateEntryExists(dateTime, addSessionIfMissing);
    const auto dateKey = makeDateKey(dateTime);
    timePerDate.set(dateKey, timePerDate[dateKey] + seconds);
    totalTimeInSeconds += seconds;
    mostTimeCacheDirty = true;
    markTrackingDataDirty();
}

void KronosAudioProcessor::addElapsedSecondsAcrossDates(const juce::Time& start,
                                                        const juce::Time& end,
                                                        bool addSessionIfMissing)
{
    const auto totalElapsedSeconds = (end - start).inSeconds();
    if (totalElapsedSeconds <= 0)
        return;

    auto remainingSeconds = totalElapsedSeconds;
    auto cursor = start;

    while (remainingSeconds > 0)
    {
        const auto cursorDayStart = juce::Time(cursor.getYear(),
                                               cursor.getMonth(),
                                               cursor.getDayOfMonth(),
                                               0, 0, 0, 0, true);
        const auto nextDayStart = cursorDayStart + juce::RelativeTime::days(1);
        auto secondsUntilNextDay = (nextDayStart - cursor).inSeconds();
        if (secondsUntilNextDay <= 0)
            secondsUntilNextDay = remainingSeconds;

        const auto chunkSeconds = juce::jmin<juce::int64>(remainingSeconds, secondsUntilNextDay);
        addTrackedSeconds(chunkSeconds, cursor, addSessionIfMissing);

        remainingSeconds -= chunkSeconds;
        cursor += juce::RelativeTime::seconds(static_cast<double>(chunkSeconds));
    }
}

juce::Time KronosAudioProcessor::getCurrentTime() const
{
    return nowProviderForTests ? nowProviderForTests() : juce::Time::getCurrentTime();
}

void KronosAudioProcessor::setNowProviderForTests(std::function<juce::Time()> nowProvider)
{
    nowProviderForTests = std::move(nowProvider);
}

#if JUCE_UNIT_TESTS
namespace
{
class FakeClock
{
public:
    explicit FakeClock(const juce::Time& start) : current(start) {}

    juce::Time now() const { return current; }

    void advanceSeconds(int secondsToAdvance)
    {
        current += juce::RelativeTime::seconds(secondsToAdvance);
    }

private:
    juce::Time current;
};

juce::MemoryBlock buildStateWithTrackingData(KronosAudioProcessor& processor,
                                             juce::int64 totalSeconds,
                                             const juce::Array<juce::Time>& sessionDates,
                                             const juce::Array<std::pair<juce::String, juce::int64>>& timeEntries)
{
    auto state = processor.parameters->copyState();
    auto trackingData = state.getOrCreateChildWithName("TrackingData", nullptr);
    trackingData.setProperty("totalTimeInSeconds", totalSeconds, nullptr);

    auto datesElement = trackingData.getOrCreateChildWithName("DatesData", nullptr);
    datesElement.removeAllChildren(nullptr);
    for (const auto& date : sessionDates)
    {
        auto dateElement = juce::ValueTree("Date");
        dateElement.setProperty("timestamp", date.toMilliseconds(), nullptr);
        datesElement.appendChild(dateElement, nullptr);
    }

    auto timePerDateElement = trackingData.getOrCreateChildWithName("TimePerDate", nullptr);
    timePerDateElement.removeAllChildren(nullptr);
    for (const auto& entryPair : timeEntries)
    {
        auto entry = juce::ValueTree("DateEntry");
        entry.setProperty("key", entryPair.first, nullptr);
        entry.setProperty("time", entryPair.second, nullptr);
        timePerDateElement.appendChild(entry, nullptr);
    }

    juce::MemoryBlock data;
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    KronosAudioProcessor::copyXmlToBinary(*xml, data);
    return data;
}

bool hasDateKey(const juce::Array<juce::Time>& dates, const juce::String& expectedDateKey)
{
    for (const auto& date : dates)
    {
        if (date.formatted("%Y-%m-%d") == expectedDateKey)
            return true;
    }

    return false;
}

bool areMemoryBlocksEqual(const juce::MemoryBlock& a, const juce::MemoryBlock& b)
{
    if (a.getSize() != b.getSize())
        return false;

    if (a.getSize() == 0)
        return true;

    return std::memcmp(a.getData(), b.getData(), a.getSize()) == 0;
}

int countDateOccurrences(const juce::Array<juce::Time>& dates, const juce::String& dateKey)
{
    int count = 0;
    for (const auto& date : dates)
    {
        if (date.formatted("%Y-%m-%d") == dateKey)
            ++count;
    }

    return count;
}

void emitTestIO(const juce::String& message)
{
    emitEmbeddedTestLog("[TEST][IO] " + message);
}

class KronosProcessorTests : public juce::UnitTest
{
public:
    KronosProcessorTests() : juce::UnitTest("Kronos Processor Tests", "Kronos") {}

    void runTest() override
    {
        emitEmbeddedTestLog("[TEST] State serialization returns data even when called rapidly");
        beginTest("State serialization returns data even when called rapidly");
        {
            FakeClock clock(juce::Time(2025, 0, 1, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            juce::MemoryBlock first;
            juce::MemoryBlock second;

            emitTestIO("Input: now=" + clock.now().formatted("%Y-%m-%d %H:%M:%S"));
            processor.getStateInformation(first);
            processor.getStateInformation(second);
            emitTestIO("Output: firstBytes=" + juce::String(static_cast<int>(first.getSize()))
                     + ", secondBytes=" + juce::String(static_cast<int>(second.getSize())));

            expect(first.getSize() > 0, "First serialized state should not be empty.");
            expect(second.getSize() > 0, "Debounced serialized state should not be empty.");
        }

        emitEmbeddedTestLog("[TEST] New day is added to session dates when tracking starts");
        beginTest("New day is added to session dates when tracking starts");
        {
            FakeClock clock(juce::Time(2025, 0, 1, 23, 50, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            const auto beforeCount = processor.getSessionDates().size();
            clock.advanceSeconds(20 * 60); // move to next day

            emitTestIO("Input: beforeDateCount=" + juce::String(beforeCount)
                     + ", startDate=" + clock.now().formatted("%Y-%m-%d"));
            processor.startTracking();
            const auto afterCount = processor.getSessionDates().size();
            emitTestIO("Output: afterDateCount=" + juce::String(afterCount));

            expect(afterCount >= beforeCount + 1, "Expected a new session date to be added.");
        }

        emitEmbeddedTestLog("[TEST] Midnight rollover uses per-instance state");
        beginTest("Midnight rollover uses per-instance state");
        {
            FakeClock clockA(juce::Time(2025, 0, 1, 23, 59, 59, 0, true));
            FakeClock clockB(juce::Time(2025, 0, 1, 12, 0, 0, 0, true));

            KronosAudioProcessor processorA([&clockA]() { return clockA.now(); });
            KronosAudioProcessor processorB([&clockB]() { return clockB.now(); });

            processorA.startTracking();
            processorB.startTracking();

            const auto bBefore = processorB.getSessionDates().size();

            // A crosses midnight first.
            clockA.advanceSeconds(1);
            processorA.timerCallback();

            // B crosses midnight afterwards.
            clockB.advanceSeconds(12 * 60 * 60 + 1);
            processorB.timerCallback();

            const auto bAfter = processorB.getSessionDates().size();
            emitTestIO("Input: bBefore=" + juce::String(bBefore) + ", clockA="
                     + clockA.now().formatted("%Y-%m-%d %H:%M:%S") + ", clockB="
                     + clockB.now().formatted("%Y-%m-%d %H:%M:%S"));
            emitTestIO("Output: bAfter=" + juce::String(bAfter));
            expect(bAfter >= bBefore + 1, "Instance B should independently detect midnight rollover.");
        }

        emitEmbeddedTestLog("[TEST] Debounced serialization reuses previous state bytes");
        beginTest("Debounced serialization reuses previous state bytes");
        {
            FakeClock clock(juce::Time(2025, 0, 1, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            juce::MemoryBlock first;
            juce::MemoryBlock second;
            emitTestIO("Input: two immediate serialization calls in debounce window");
            processor.getStateInformation(first);
            processor.getStateInformation(second);
            emitTestIO("Output: firstBytes=" + juce::String(static_cast<int>(first.getSize()))
                     + ", secondBytes=" + juce::String(static_cast<int>(second.getSize()))
                     + ", bytewiseEqual=" + juce::String(areMemoryBlocksEqual(first, second) ? "true" : "false"));

            expect(first.getSize() > 0, "First state should not be empty.");
            expect(second.getSize() > 0, "Second state should not be empty.");
            expect(areMemoryBlocksEqual(first, second), "Expected same serialized bytes inside debounce window.");
        }

        emitEmbeddedTestLog("[TEST] Stop tracking accumulates elapsed time");
        beginTest("Stop tracking accumulates elapsed time");
        {
            FakeClock clock(juce::Time(2025, 0, 1, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            const auto todayKey = clock.now().formatted("%Y-%m-%d");

            processor.startTracking();
            clock.advanceSeconds(65);
            processor.stopTracking();
            emitTestIO("Input: startDate=" + todayKey + ", elapsedSeconds=65");
            emitTestIO("Output: totalSeconds=" + juce::String(static_cast<int>(processor.getTotalTimeInSeconds()))
                     + ", daySeconds=" + juce::String(static_cast<int>(processor.timePerDate[todayKey])));

            expectEquals(static_cast<int>(processor.getTotalTimeInSeconds()), 65,
                         "Stopping tracking should add elapsed seconds.");
            expectEquals(static_cast<int>(processor.timePerDate[todayKey]), 65,
                         "Today's bucket should receive elapsed seconds.");
        }

        emitEmbeddedTestLog("[TEST] Timer callback increments total and per-day counters");
        beginTest("Timer callback increments total and per-day counters");
        {
            FakeClock clock(juce::Time(2025, 0, 1, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            const auto todayKey = clock.now().formatted("%Y-%m-%d");
            const auto initialDateSeconds = processor.timePerDate[todayKey];

            clock.advanceSeconds(1);
            processor.timerCallback();
            emitTestIO("Input: date=" + todayKey + ", initialDaySeconds="
                     + juce::String(static_cast<int>(initialDateSeconds)) + ", elapsedSeconds=1");
            emitTestIO("Output: totalSeconds=" + juce::String(static_cast<int>(processor.getTotalTimeInSeconds()))
                     + ", daySeconds=" + juce::String(static_cast<int>(processor.timePerDate[todayKey])));

            expectEquals(static_cast<int>(processor.getTotalTimeInSeconds()), 1,
                         "Timer callback should increment total time by 1 second.");
            expectEquals(static_cast<int>(processor.timePerDate[todayKey]), static_cast<int>(initialDateSeconds + 1),
                         "Timer callback should increment today's tracked time by 1 second.");
        }

        emitEmbeddedTestLog("[TEST] Delayed timer callback catches up elapsed seconds");
        beginTest("Delayed timer callback catches up elapsed seconds");
        {
            FakeClock clock(juce::Time(2025, 0, 1, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            const auto todayKey = clock.now().formatted("%Y-%m-%d");

            clock.advanceSeconds(5);
            processor.timerCallback();
            emitTestIO("Input: date=" + todayKey + ", delayedSeconds=5");
            emitTestIO("Output: totalSeconds=" + juce::String(static_cast<int>(processor.getTotalTimeInSeconds()))
                     + ", daySeconds=" + juce::String(static_cast<int>(processor.timePerDate[todayKey])));

            expectEquals(static_cast<int>(processor.getTotalTimeInSeconds()), 5,
                         "Delayed callback should accumulate full elapsed seconds.");
            expectEquals(static_cast<int>(processor.timePerDate[todayKey]), 5,
                         "Today's bucket should include all elapsed seconds.");
        }

        emitEmbeddedTestLog("[TEST] Stop tracking splits elapsed time across midnight");
        beginTest("Stop tracking splits elapsed time across midnight");
        {
            FakeClock clock(juce::Time(2025, 0, 1, 23, 59, 58, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            const juce::String jan1Key = clock.now().formatted("%Y-%m-%d");

            processor.startTracking();
            clock.advanceSeconds(5);
            const juce::String jan2Key = clock.now().formatted("%Y-%m-%d");
            processor.stopTracking();
            emitTestIO("Input: startDate=" + jan1Key + ", elapsedSeconds=5, crossedInto=" + jan2Key);
            emitTestIO("Output: totalSeconds=" + juce::String(static_cast<int>(processor.getTotalTimeInSeconds()))
                     + ", jan1Seconds=" + juce::String(static_cast<int>(processor.timePerDate[jan1Key]))
                     + ", jan2Seconds=" + juce::String(static_cast<int>(processor.timePerDate[jan2Key])));

            expectEquals(static_cast<int>(processor.getTotalTimeInSeconds()), 5);
            expectEquals(static_cast<int>(processor.timePerDate[jan1Key]), 2,
                         "Expected two seconds to remain on the previous day.");
            expectEquals(static_cast<int>(processor.timePerDate[jan2Key]), 3,
                         "Expected three seconds to roll into the next day.");
        }

        emitEmbeddedTestLog("[TEST] Timer logic handles month rollover");
        beginTest("Timer logic handles month rollover");
        {
            FakeClock clock(juce::Time(2025, 0, 31, 23, 59, 58, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            const juce::String jan31Key = clock.now().formatted("%Y-%m-%d");

            processor.startTracking();
            clock.advanceSeconds(5);
            const juce::String feb1Key = clock.now().formatted("%Y-%m-%d");
            processor.stopTracking();
            emitTestIO("Input: startDate=" + jan31Key + ", elapsedSeconds=5, crossedInto=" + feb1Key);
            emitTestIO("Output: totalSeconds=" + juce::String(static_cast<int>(processor.getTotalTimeInSeconds()))
                     + ", jan31Seconds=" + juce::String(static_cast<int>(processor.timePerDate[jan31Key]))
                     + ", feb1Seconds=" + juce::String(static_cast<int>(processor.timePerDate[feb1Key])));

            expectEquals(static_cast<int>(processor.getTotalTimeInSeconds()), 5);
            expectEquals(static_cast<int>(processor.timePerDate[jan31Key]), 2);
            expectEquals(static_cast<int>(processor.timePerDate[feb1Key]), 3);
        }

        emitEmbeddedTestLog("[TEST] Timer logic handles year rollover");
        beginTest("Timer logic handles year rollover");
        {
            FakeClock clock(juce::Time(2024, 11, 31, 23, 59, 58, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            const juce::String dec31Key = clock.now().formatted("%Y-%m-%d");

            processor.startTracking();
            clock.advanceSeconds(5);
            const juce::String jan1Key = clock.now().formatted("%Y-%m-%d");
            processor.stopTracking();
            emitTestIO("Input: startDate=" + dec31Key + ", elapsedSeconds=5, crossedInto=" + jan1Key);
            emitTestIO("Output: totalSeconds=" + juce::String(static_cast<int>(processor.getTotalTimeInSeconds()))
                     + ", dec31Seconds=" + juce::String(static_cast<int>(processor.timePerDate[dec31Key]))
                     + ", jan1Seconds=" + juce::String(static_cast<int>(processor.timePerDate[jan1Key])));

            expectEquals(static_cast<int>(processor.getTotalTimeInSeconds()), 5);
            expectEquals(static_cast<int>(processor.timePerDate[dec31Key]), 2);
            expectEquals(static_cast<int>(processor.timePerDate[jan1Key]), 3);
        }

        emitEmbeddedTestLog("[TEST] Restoring old state inserts current day");
        beginTest("Restoring old state inserts current day");
        {
            FakeClock sourceClock(juce::Time(2025, 0, 1, 10, 0, 0, 0, true));
            KronosAudioProcessor sourceProcessor([&sourceClock]() { return sourceClock.now(); });

            juce::MemoryBlock oldState;
            sourceProcessor.getStateInformation(oldState);

            FakeClock targetClock(juce::Time(2025, 0, 2, 10, 0, 0, 0, true));
            KronosAudioProcessor targetProcessor([&targetClock]() { return targetClock.now(); });
            targetProcessor.setStateInformation(oldState.getData(), static_cast<int>(oldState.getSize()));

            const auto todayKey = targetClock.now().formatted("%Y-%m-%d");
            emitTestIO("Input: stateSavedAt=2025-01-01, restoredAt=" + todayKey);
            emitTestIO("Output: containsTodayKey="
                     + juce::String(targetProcessor.timePerDate.contains(todayKey) ? "true" : "false")
                     + ", hasTodaySessionDate="
                     + juce::String(hasDateKey(targetProcessor.getSessionDates(), todayKey) ? "true" : "false"));
            expect(targetProcessor.timePerDate.contains(todayKey),
                   "Current day should be inserted when restoring old state.");
            expect(hasDateKey(targetProcessor.getSessionDates(), todayKey),
                   "Current day should exist in visible session dates after restore.");
        }

        emitEmbeddedTestLog("[TEST] Most-time sorting orders days by descending tracked time");
        beginTest("Most-time sorting orders days by descending tracked time");
        {
            FakeClock clock(juce::Time(2025, 0, 5, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            const juce::Time jan2(2025, 0, 2, 10, 0, 0, 0, true);
            const juce::Time jan3(2025, 0, 3, 10, 0, 0, 0, true);
            const juce::Time jan4(2025, 0, 4, 10, 0, 0, 0, true);
            juce::Array<juce::Time> dates { jan2, jan3, jan4 };
            juce::Array<std::pair<juce::String, juce::int64>> times;
            times.add({ jan2.formatted("%Y-%m-%d"), 100 });
            times.add({ jan3.formatted("%Y-%m-%d"), 350 });
            times.add({ jan4.formatted("%Y-%m-%d"), 200 });

            auto state = buildStateWithTrackingData(processor, 650, dates, times);
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            processor.setDateSortMode(KronosAudioProcessor::DateSortMode::MostTime);
            const auto sorted = processor.getSortedDates();
            emitTestIO("Input: times={"
                     + jan2.formatted("%Y-%m-%d") + "=100,"
                     + jan3.formatted("%Y-%m-%d") + "=350,"
                     + jan4.formatted("%Y-%m-%d") + "=200}");
            if (sorted.size() >= 3)
            {
                emitTestIO("Output: sortedTop3={"
                         + sorted[0].formatted("%Y-%m-%d") + ","
                         + sorted[1].formatted("%Y-%m-%d") + ","
                         + sorted[2].formatted("%Y-%m-%d") + "}");
            }
            else
            {
                emitTestIO("Output: sortedSize=" + juce::String(sorted.size()));
            }

            expect(sorted.size() >= 3, "Expected at least three sorted dates.");
            expectEquals(sorted[0].formatted("%Y-%m-%d"), jan3.formatted("%Y-%m-%d"));
            expectEquals(sorted[1].formatted("%Y-%m-%d"), jan4.formatted("%Y-%m-%d"));
            expectEquals(sorted[2].formatted("%Y-%m-%d"), jan2.formatted("%Y-%m-%d"));
        }

        emitEmbeddedTestLog("[TEST] State round-trip preserves parameters and tracking data");
        beginTest("State round-trip preserves parameters and tracking data");
        {
            FakeClock sourceClock(juce::Time(2025, 0, 10, 12, 0, 0, 0, true));
            KronosAudioProcessor sourceProcessor([&sourceClock]() { return sourceClock.now(); });

            const juce::Time jan7(2025, 0, 7, 12, 0, 0, 0, true);
            const juce::Time jan8(2025, 0, 8, 12, 0, 0, 0, true);
            juce::Array<juce::Time> sourceDates { jan7, jan8 };
            juce::Array<std::pair<juce::String, juce::int64>> sourceTimes;
            sourceTimes.add({ jan7.formatted("%Y-%m-%d"), 120 });
            sourceTimes.add({ jan8.formatted("%Y-%m-%d"), 240 });

            sourceProcessor.setDarkMode(false);
            sourceProcessor.setDateSortMode(KronosAudioProcessor::DateSortMode::MostTime);

            auto seededState = buildStateWithTrackingData(sourceProcessor, 360, sourceDates, sourceTimes);
            sourceProcessor.setStateInformation(seededState.getData(), static_cast<int>(seededState.getSize()));

            juce::MemoryBlock snapshot;
            sourceProcessor.getStateInformation(snapshot);

            FakeClock targetClock(juce::Time(2025, 0, 11, 12, 0, 0, 0, true));
            KronosAudioProcessor targetProcessor([&targetClock]() { return targetClock.now(); });
            targetProcessor.setStateInformation(snapshot.getData(), static_cast<int>(snapshot.getSize()));
            emitTestIO("Input: sourceDates={"
                     + jan7.formatted("%Y-%m-%d") + "=120,"
                     + jan8.formatted("%Y-%m-%d") + "=240}, total=360, darkMode=false, sort=MostTime");
            emitTestIO("Output: total=" + juce::String(static_cast<int>(targetProcessor.getTotalTimeInSeconds()))
                     + ", jan7=" + juce::String(static_cast<int>(targetProcessor.timePerDate[jan7.formatted("%Y-%m-%d")]))
                     + ", jan8=" + juce::String(static_cast<int>(targetProcessor.timePerDate[jan8.formatted("%Y-%m-%d")]))
                     + ", darkMode=" + juce::String(targetProcessor.isDarkMode() ? "true" : "false")
                     + ", sortMode="
                     + juce::String(targetProcessor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostTime ? "MostTime" : "MostRecent"));

            expect(!targetProcessor.isDarkMode(), "Dark mode parameter should persist across round-trip.");
            expect(targetProcessor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostTime,
                   "Date sort mode should persist across round-trip.");
            expectEquals(static_cast<int>(targetProcessor.getTotalTimeInSeconds()), 360);
            expectEquals(static_cast<int>(targetProcessor.timePerDate[jan7.formatted("%Y-%m-%d")]), 120);
            expectEquals(static_cast<int>(targetProcessor.timePerDate[jan8.formatted("%Y-%m-%d")]), 240);
            expect(hasDateKey(targetProcessor.getSessionDates(), jan7.formatted("%Y-%m-%d")),
                   "Expected preserved session date for Jan 7.");
            expect(hasDateKey(targetProcessor.getSessionDates(), jan8.formatted("%Y-%m-%d")),
                   "Expected preserved session date for Jan 8.");
        }

        emitEmbeddedTestLog("[TEST] Invalid state input is ignored safely");
        beginTest("Invalid state input is ignored safely");
        {
            FakeClock clock(juce::Time(2025, 0, 12, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            processor.setDarkMode(false);

            const auto beforeDates = processor.getSessionDates();
            const auto beforeDarkMode = processor.isDarkMode();

            const char invalidData[] = "not-an-xml-state";
            processor.setStateInformation(invalidData, static_cast<int>(sizeof(invalidData)));
            emitTestIO("Input: invalidPayload=\"not-an-xml-state\""
                     + juce::String(", beforeDarkMode=") + (beforeDarkMode ? "true" : "false")
                     + ", beforeDateCount=" + juce::String(beforeDates.size()));
            emitTestIO("Output: afterDarkMode=" + juce::String(processor.isDarkMode() ? "true" : "false")
                     + ", afterDateCount=" + juce::String(processor.getSessionDates().size()));

            expect(processor.isDarkMode() == beforeDarkMode,
                   "Invalid state payload should not change dark mode.");
            expectEquals(processor.getSessionDates().size(), beforeDates.size());
        }

        emitEmbeddedTestLog("[TEST] Repeated tracking on same day does not duplicate session dates");
        beginTest("Repeated tracking on same day does not duplicate session dates");
        {
            FakeClock clock(juce::Time(2025, 0, 13, 9, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            const auto dayKey = clock.now().formatted("%Y-%m-%d");

            processor.startTracking();
            processor.timerCallback();
            processor.stopTracking();
            processor.startTracking();
            processor.timerCallback();
            processor.stopTracking();

            const auto occurrences = countDateOccurrences(processor.getSessionDates(), dayKey);
            emitTestIO("Input: sameDay=" + dayKey + ", start/stop cycles=2");
            emitTestIO("Output: dateOccurrences=" + juce::String(occurrences));
            expectEquals(occurrences, 1, "Same calendar day should appear only once in session dates.");
        }

        emitEmbeddedTestLog("[TEST] Parameter values persist through serialization");
        beginTest("Parameter values persist through serialization");
        {
            FakeClock sourceClock(juce::Time(2025, 0, 14, 10, 0, 0, 0, true));
            KronosAudioProcessor sourceProcessor([&sourceClock]() { return sourceClock.now(); });
            sourceProcessor.setDarkMode(false);
            sourceProcessor.setDateSortMode(KronosAudioProcessor::DateSortMode::MostTime);
            sourceProcessor.setTracking(true);

            juce::MemoryBlock state;
            sourceProcessor.getStateInformation(state);

            FakeClock targetClock(juce::Time(2025, 0, 14, 11, 0, 0, 0, true));
            KronosAudioProcessor targetProcessor([&targetClock]() { return targetClock.now(); });
            targetProcessor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            emitTestIO("Input: darkMode=false, sort=MostTime, tracking=true");
            emitTestIO("Output: darkMode=" + juce::String(targetProcessor.isDarkMode() ? "true" : "false")
                     + ", sortMode="
                     + juce::String(targetProcessor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostTime ? "MostTime" : "MostRecent")
                     + ", tracking=" + juce::String(targetProcessor.isTracking() ? "true" : "false"));

            expect(!targetProcessor.isDarkMode(), "Dark mode should restore to false.");
            expect(targetProcessor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostTime,
                   "Sort mode should restore to MostTime.");
            expect(targetProcessor.isTracking(), "Tracking parameter value should restore as true.");
        }
    }
};

KronosProcessorTests kronosProcessorTests;
} // namespace
#endif

int KronosAudioProcessor::runEmbeddedTests()
{
#if JUCE_UNIT_TESTS
    gEmbeddedTestsRunning = true;

    const auto logPath = juce::SystemStats::getEnvironmentVariable("KRONOS_TEST_LOG_FILE", {});
    const juce::File logFile(logPath);

    if (logPath.isNotEmpty())
        logFile.replaceWithText({});

    StreamingUnitTestRunner runner(logFile);
    runner.setAssertOnFailure(false);
    runner.setPassesAreLogged(true);
    runner.runTestsInCategory("Kronos");

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (const auto* result = runner.getResult(i))
            failures += result->failures;
    }

    gEmbeddedTestsRunning = false;
    return failures;
#else
    emitEmbeddedTestLog("[TEST] JUCE_UNIT_TESTS is disabled; no embedded tests were executed.");
    return -1;
#endif
}



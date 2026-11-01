/*
  ==============================================================================

    My main plugin processor implementation.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "JucePluginDefines.h"
#include "BackupComponent.h"
#include "RestoreComponent.h"
#include "KronosViews.h"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <map>
#include <thread>
#include <vector>

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

#if JUCE_UNIT_TESTS
void startRealClockSoakIfRequested(KronosAudioProcessor&);
#endif

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
        false,
        juce::AudioParameterBoolAttributes().withAutomatable(false)
    );
    
    auto darkModeParam = std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("darkMode", 1),  // ID and version hint
        "Dark Mode",
        true,
        juce::AudioParameterBoolAttributes().withAutomatable(false)
    );
    
    auto dateSortModeParam = std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("dateSortMode", 1),  // ID and version hint
        "Date Sort Mode",
        juce::StringArray {"Most Recent", "Most Time"},
        0,
        juce::AudioParameterChoiceAttributes().withAutomatable(false)
    );

    layout.add(std::move(trackingParam));
    layout.add(std::move(darkModeParam));
    layout.add(std::move(dateSortModeParam));

    // Then create the parameters with the layout
    parameters = std::make_unique<juce::AudioProcessorValueTreeState>(*this, nullptr, "Parameters", std::move(layout));

    startTime = getCurrentTime();
    totalTimeInSeconds = 0;

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
    ensureDateEntryExists(getCurrentTime(), true);
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

#if JUCE_UNIT_TESTS
    startRealClockSoakIfRequested(*this);
#endif
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
        {
            const juce::ScopedLock lock(trackingLock);
            const auto now = getCurrentTime();
            ensureDateEntryExists(now, true);
            beginAccumulating(now);
        }

        setTracking(true);  // Use the parameter system
        startTimer(1000);
    }
}

void KronosAudioProcessor::stopTracking()
{
    if (isTracking())  // Use the accessor method
    {
        {
            const juce::ScopedLock lock(trackingLock);
            const auto now = getCurrentTime();

            if (isAccumulating)
            {
                creditElapsedTime(now);
                carriedMilliseconds = juce::jmax<juce::int64>(0, now.toMilliseconds() - startTime.toMilliseconds());
                isAccumulating = false;
            }
        }

        setTracking(false);  // Use the parameter system
    }
}

juce::int64 KronosAudioProcessor::getTotalTimeInSeconds() const
{
    const juce::ScopedLock lock(trackingLock);
    return totalTimeInSeconds + getPendingSeconds(getCurrentTime());
}

juce::int64 KronosAudioProcessor::getPendingSeconds(const juce::Time& now) const
{
    if (isAccumulating)
    {
        const auto pendingMs = now.toMilliseconds() - startTime.toMilliseconds();
        if (pendingMs > 0 && pendingMs <= maximumCreditableGapMs)
            return pendingMs / 1000;
    }

    return 0;
}

juce::int64 KronosAudioProcessor::getTodaySeconds() const
{
    const juce::ScopedLock lock(trackingLock);
    const auto now = getCurrentTime();
    return timePerDate[makeDateKey(now)] + getPendingSeconds(now);
}

bool KronosAudioProcessor::isToday(const juce::Time& date) const
{
    return makeDateKey(date) == makeDateKey(getCurrentTime());
}

juce::int64 KronosAudioProcessor::getCurrentRunSeconds() const
{
    const juce::ScopedLock lock(trackingLock);
    return getTotalTimeInSeconds() - runStartTotalSeconds;
}

int KronosAudioProcessor::getDaysLogged() const
{
    const juce::ScopedLock lock(trackingLock);

    int days = 0;
    for (auto it = timePerDate.begin(); it != timePerDate.end(); ++it)
        if (it.getValue() > 0)
            ++days;

    return days;
}

juce::int64 KronosAudioProcessor::getDailyAverageSeconds() const
{
    const juce::ScopedLock lock(trackingLock);
    const auto days = getDaysLogged();
    return days > 0 ? getTotalTimeInSeconds() / days : 0;
}

KronosAudioProcessor::BackupSummary KronosAudioProcessor::summariseBackup(const void* data, int sizeInBytes)
{
    BackupSummary summary;
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr)
        return summary;

    const auto* trackingData = xmlState->getChildByName("TrackingData");
    if (trackingData == nullptr)
        return summary;

    juce::int64 sumOfDays = 0;

    if (const auto* days = trackingData->getChildByName("TimePerDate"))
    {
        for (const auto* entry : days->getChildIterator())
        {
            const auto seconds = entry->getStringAttribute("time").getLargeIntValue();
            if (seconds > 0)
            {
                sumOfDays += seconds;
                ++summary.daysLogged;
            }
        }
    }

    summary.totalSeconds = juce::jmax(trackingData->getStringAttribute("totalTimeInSeconds").getLargeIntValue(), sumOfDays);
    summary.isValid = true;
    return summary;
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
    juce::Time now;

    {
        // Read the clock under the lock: a value older than startTime would look
        // like the clock moving backwards and rewind the accounting.
        const juce::ScopedLock lock(trackingLock);
        now = getCurrentTime();

        if (isAccumulating)
            creditElapsedTime(now);
    }

    // Clear the flag before copying so a parameter change made during the copy
    // marks the next save as dirty. The copy stays outside trackingLock because
    // restoring state takes the same two locks in the opposite order.
    const auto wasDirty = serializedStateDirty.exchange(false);

    // Create the base state from parameters
    auto state = parameters->copyState();

    const juce::ScopedLock lock(trackingLock);

    // Debounce serialization work, but always return valid state data.
    if ((now - lastSaveTime).inMilliseconds() < minimumSaveIntervalMs
        && hasSerializedState
        && !wasDirty)
    {
        destData = lastSerializedState;
        return;
    }
    lastSaveTime = now;

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
}

void KronosAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    applyState(data, sizeInBytes, false);
}

bool KronosAudioProcessor::restoreStateFromData(const void* data, int sizeInBytes)
{
    return applyState(data, sizeInBytes, true);
}

bool KronosAudioProcessor::applyState(const void* data, int sizeInBytes, bool requireTrackingData)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr)
        return false;

    const auto hasKronosRoot = xmlState->hasTagName(parameters->state.getType());
    const auto hasTrackingData = xmlState->getChildByName("TrackingData") != nullptr;

    if ((!hasKronosRoot && !hasTrackingData) || (requireTrackingData && !hasTrackingData))
        return false;

    // Builds before February 2025 saved under a different root name, and
    // re-saving those projects kept it, so only the contents are compared.
    juce::ValueTree state(parameters->state.getType());
    state.copyPropertiesAndChildrenFrom(juce::ValueTree::fromXml(*xmlState), nullptr);
    parameters->replaceState(state);

    // Restore tracking data
    auto trackingData = state.getChildWithName("TrackingData");
    if (!trackingData.isValid())
        return true;

    const juce::ScopedLock lock(trackingLock);

    // Restore time per date. The date keys are the source of truth: stored
    // timestamps would land on a different day after a time zone change.
    timePerDate.clear();
    sessionDates.clear();
    juce::int64 sumOfDays = 0;

    auto timePerDateElement = trackingData.getChildWithName("TimePerDate");
    if (timePerDateElement.isValid())
    {
        for (auto entry : timePerDateElement)
        {
            const auto key = entry.getProperty("key").toString();
            const auto seconds = static_cast<juce::int64>(entry.getProperty("time"));
            const auto tokens = juce::StringArray::fromTokens(key, "-", {});
            const juce::Time date(tokens[0].getIntValue(), tokens[1].getIntValue() - 1,
                                  tokens[2].getIntValue(), 12, 0, 0, 0, true);

            // A key that does not survive a round trip is not a real calendar day
            if (tokens.size() != 3 || seconds < 0 || makeDateKey(date) != key || timePerDate.contains(key))
                continue;

            timePerDate.set(key, seconds);
            sumOfDays += seconds;
            sessionDates.add(date);
        }
    }

    // State saved before per-day history existed only has timestamps.
    auto datesElement = trackingData.getChildWithName("DatesData");
    if (timePerDate.size() == 0 && datesElement.isValid())
    {
        for (auto dateElement : datesElement)
        {
            const auto timestamp = dateElement.getProperty("timestamp").toString().getLargeIntValue();
            ensureDateEntryExists(juce::Time(timestamp), true);
        }
    }

    // A total can only fall behind its own history through old corruption.
    const auto savedTotal = static_cast<juce::int64>(trackingData.getProperty("totalTimeInSeconds", 0));
    totalTimeInSeconds = juce::jmax(savedTotal, sumOfDays);

    const auto now = getCurrentTime();
    ensureDateEntryExists(now, true);

    startTime = now;
    carriedMilliseconds = 0;
    runStartTotalSeconds = totalTimeInSeconds;
    isAccumulating = isTracking();

    invalidateAllSortCaches();
    markTrackingDataDirty();
    ++stateRevision;
    return true;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KronosAudioProcessor();
}

void KronosAudioProcessor::timerCallback()
{
    const juce::ScopedLock lock(trackingLock);
    synchroniseWithTrackingParameter(getCurrentTime());
}

void KronosAudioProcessor::synchroniseWithTrackingParameter(const juce::Time& now)
{
    // The parameter can change without startTracking()/stopTracking() running,
    // for example from a host control surface or a restored state.
    const auto shouldAccumulate = isTracking();

    if (isAccumulating)
    {
        creditElapsedTime(now);
        isAccumulating = shouldAccumulate;
    }
    else if (shouldAccumulate)
    {
        ensureDateEntryExists(now, true);
        beginAccumulating(now);
    }
}

void KronosAudioProcessor::beginAccumulating(const juce::Time& now)
{
    startTime = juce::Time(now.toMilliseconds() - carriedMilliseconds);
    carriedMilliseconds = 0;
    runStartTotalSeconds = totalTimeInSeconds;
    isAccumulating = true;
}

void KronosAudioProcessor::creditElapsedTime(const juce::Time& now)
{
    const auto elapsedMs = now.toMilliseconds() - startTime.toMilliseconds();

    if (elapsedMs < 0 || elapsedMs > maximumCreditableGapMs)
    {
        ensureDateEntryExists(now, true);
        startTime = now;
        return;
    }

    // Only whole seconds are credited. The remainder stays in startTime so
    // that it counts towards the next credit instead of being lost.
    const auto wholeSeconds = elapsedMs / 1000;
    addElapsedSecondsAcrossDates(startTime, wholeSeconds, true);
    startTime = juce::Time(startTime.toMilliseconds() + wholeSeconds * 1000);
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
    const juce::ScopedLock lock(trackingLock);
    juce::String dateKey = makeDateKey(date);
    return timePerDate[dateKey];
}

juce::int64 KronosAudioProcessor::getLongestDaySeconds() const
{
    const juce::ScopedLock lock(trackingLock);

    juce::int64 longest = 0;
    for (auto it = timePerDate.begin(); it != timePerDate.end(); ++it)
        longest = juce::jmax(longest, it.getValue());

    return longest;
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
    const juce::ScopedLock lock(trackingLock);

    const auto mode = getDateSortMode();
    if (mode == DateSortMode::MostRecent && !mostRecentCacheDirty)
        return cachedMostRecentDates;
    if (mode == DateSortMode::MostTime && !mostTimeCacheDirty)
        return cachedMostTimeDates;

    // Clear the flag first so a change made while sorting re-dirties the cache.
    (mode == DateSortMode::MostTime ? mostTimeCacheDirty : mostRecentCacheDirty) = false;

    struct SortEntry
    {
        juce::Time date;
        juce::int64 seconds;
    };

    // Look each day up once rather than formatting its key on every comparison.
    std::vector<SortEntry> entries;
    entries.reserve(static_cast<size_t>(sessionDates.size()));

    for (const auto& date : sessionDates)
        entries.push_back({ date, mode == DateSortMode::MostTime ? timePerDate[makeDateKey(date)] : 0 });

    std::sort(entries.begin(), entries.end(), [](const SortEntry& a, const SortEntry& b)
    {
        return a.seconds != b.seconds ? a.seconds > b.seconds : a.date > b.date;
    });

    juce::Array<juce::Time> sortedDates;
    sortedDates.ensureStorageAllocated(sessionDates.size());

    for (const auto& entry : entries)
        sortedDates.add(entry.date);

    (mode == DateSortMode::MostTime ? cachedMostTimeDates : cachedMostRecentDates) = sortedDates;
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

void KronosAudioProcessor::addTrackedSeconds(juce::int64 seconds,
                                             const juce::Time& dateTime,
                                             const juce::String& dateKey,
                                             bool addSessionIfMissing)
{
    if (seconds <= 0)
        return;

    ensureDateEntryExists(dateTime, addSessionIfMissing);
    timePerDate.set(dateKey, timePerDate[dateKey] + seconds);
    totalTimeInSeconds += seconds;
    mostTimeCacheDirty = true;
    markTrackingDataDirty();
}

void KronosAudioProcessor::addElapsedSecondsAcrossDates(const juce::Time& start,
                                                        juce::int64 seconds,
                                                        bool addSessionIfMissing)
{
    auto cursorMs = start.toMilliseconds();
    auto remainingSeconds = seconds;

    while (remainingSeconds > 0)
    {
        const juce::Time cursor(cursorMs);
        const auto dateKey = makeDateKey(cursor);

        auto keyAfter = [this, cursorMs](juce::int64 secondsAhead)
        {
            return makeDateKey(juce::Time(cursorMs + secondsAhead * 1000));
        };

        // Search for the date change rather than computing it: daylight saving
        // and zone changes mean days are not always 24 hours long, and local
        // midnight does not always exist.
        auto chunkSeconds = remainingSeconds;

        if (keyAfter(remainingSeconds - 1) != dateKey)
        {
            juce::int64 sameDay = 0;
            auto otherDay = remainingSeconds - 1;

            while (otherDay - sameDay > 1)
            {
                const auto middle = sameDay + (otherDay - sameDay) / 2;

                if (keyAfter(middle) == dateKey)
                    sameDay = middle;
                else
                    otherDay = middle;
            }

            chunkSeconds = otherDay;
        }

        addTrackedSeconds(chunkSeconds, cursor, dateKey, addSessionIfMissing);

        remainingSeconds -= chunkSeconds;
        cursorMs += chunkSeconds * 1000;
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

    void advanceMilliseconds(juce::int64 millisecondsToAdvance)
    {
        current = juce::Time(current.toMilliseconds() + millisecondsToAdvance);
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
            FakeClock clockB(juce::Time(2025, 0, 1, 23, 57, 0, 0, true));

            KronosAudioProcessor processorA([&clockA]() { return clockA.now(); });
            KronosAudioProcessor processorB([&clockB]() { return clockB.now(); });

            processorA.startTracking();
            processorB.startTracking();

            const auto bBefore = processorB.getSessionDates().size();

            // A crosses midnight first.
            clockA.advanceSeconds(1);
            processorA.timerCallback();

            // B crosses midnight afterwards.
            clockB.advanceSeconds(3 * 60 + 1);
            processorB.timerCallback();

            const auto bAfter = processorB.getSessionDates().size();
            emitTestIO("Input: bBefore=" + juce::String(bBefore) + ", clockA="
                     + clockA.now().formatted("%Y-%m-%d %H:%M:%S") + ", clockB="
                     + clockB.now().formatted("%Y-%m-%d %H:%M:%S"));
            emitTestIO("Output: bAfter=" + juce::String(bAfter));
            expect(bAfter >= bBefore + 1, "Instance B should independently detect midnight rollover.");
            expectEquals(processorB.getTimeForDate(clockB.now()), static_cast<juce::int64>(1));
            expectEquals(processorB.getTotalTimeInSeconds(), static_cast<juce::int64>(181));
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

#if JUCE_MAC || JUCE_LINUX
 #define KRONOS_CAN_SIMULATE_TIME_ZONES 1
#else
 #define KRONOS_CAN_SIMULATE_TIME_ZONES 0
#endif

// Runs the real JUCE Timer against the real clock for KRONOS_SOAK_SECONDS, then
// compares what was tracked with what the wall clock says and quits.
class RealClockSoak : private juce::Timer
{
public:
    RealClockSoak(KronosAudioProcessor& processorToWatch, int secondsToRun)
        : processor(processorToWatch), seconds(secondsToRun)
    {
        startTimer(1500);
    }

private:
    void timerCallback() override
    {
        if (!started)
        {
            started = true;
            processor.startTracking();
            trackedAtStart = processor.getTotalTimeInSeconds();
            wallAtStart = juce::Time::currentTimeMillis();
            return;
        }

        const auto wall = (juce::Time::currentTimeMillis() - wallAtStart) / 1000;
        if (wall < seconds)
            return;

        stopTimer();
        const auto tracked = processor.getTotalTimeInSeconds() - trackedAtStart;
        juce::File(juce::SystemStats::getEnvironmentVariable("KRONOS_TEST_RESULTS_FILE", {}))
            .replaceWithText("wall=" + juce::String(wall) + " tracked=" + juce::String(tracked) + "\n");
        juce::JUCEApplicationBase::quit();
    }

    KronosAudioProcessor& processor;
    int seconds = 0;
    bool started = false;
    juce::int64 trackedAtStart = 0;
    juce::int64 wallAtStart = 0;
};

void startRealClockSoakIfRequested(KronosAudioProcessor& processor)
{
    static std::unique_ptr<RealClockSoak> soak;
    const auto seconds = juce::SystemStats::getEnvironmentVariable("KRONOS_SOAK_SECONDS", "0").getIntValue();

    if (seconds > 0 && juce::JUCEApplicationBase::isStandaloneApp())
        soak = std::make_unique<RealClockSoak>(processor, seconds);
}

#if KRONOS_CAN_SIMULATE_TIME_ZONES
// Switches the zone used by Time::formatted()/mktime() for one scenario.
class ScopedTimeZone
{
public:
    explicit ScopedTimeZone(const char* zone)
    {
        if (const auto* existing = std::getenv("TZ"))
        {
            hadZone = true;
            previousZone = existing;
        }

        setenv("TZ", zone, 1);
        tzset();
    }

    ~ScopedTimeZone()
    {
        if (hadZone)
            setenv("TZ", previousZone.toRawUTF8(), 1);
        else
            unsetenv("TZ");

        tzset();
    }

private:
    bool hadZone = false;
    juce::String previousZone;
};
#endif

// Independent model of what should have been credited. It never shares code
// with the processor: it walks real intervals and bisects to the exact moment
// the calendar date changes, in whatever zone is active.
class TrackedTimeOracle
{
public:
    void addInterval(const juce::Time& start, const juce::Time& end)
    {
        auto cursor = start.toMilliseconds();
        const auto endMs = end.toMilliseconds();

        while (cursor < endMs)
        {
            const auto boundary = findDateChange(cursor, endMs);
            millisPerDay[keyFor(cursor)] += boundary - cursor;
            totalMillis += boundary - cursor;
            cursor = boundary;
        }
    }

    juce::int64 getTotalSeconds() const { return totalMillis / 1000; }

    int countDaysOffBy(const KronosAudioProcessor& processor, juce::int64 toleranceSeconds, juce::String& worst) const
    {
        int mismatches = 0;
        juce::int64 worstError = 0;

        for (const auto& [key, millis] : millisPerDay)
        {
            const auto expected = millis / 1000;
            const auto actual = processor.timePerDate[key];
            const auto error = std::abs(actual - expected);

            if (error > toleranceSeconds)
                ++mismatches;

            if (error > worstError)
            {
                worstError = error;
                worst = key + " expected " + juce::String(expected) + "s got " + juce::String(actual) + "s";
            }
        }

        return mismatches;
    }

private:
    static juce::String keyFor(juce::int64 millis) { return juce::Time(millis).formatted("%Y-%m-%d"); }

    static juce::int64 findDateChange(juce::int64 start, juce::int64 end)
    {
        const auto startKey = keyFor(start);
        if (keyFor(end - 1) == startKey)
            return end;

        auto low = start;
        auto high = end - 1;

        while (high - low > 1)
        {
            const auto middle = low + (high - low) / 2;
            if (keyFor(middle) == startKey)
                low = middle;
            else
                high = middle;
        }

        return high;
    }

    std::map<juce::String, juce::int64> millisPerDay;
    juce::int64 totalMillis = 0;
};

juce::int64 sumOfDays(const KronosAudioProcessor& processor)
{
    juce::int64 sum = 0;
    for (auto it = processor.timePerDate.begin(); it != processor.timePerDate.end(); ++it)
        sum += it.getValue();

    return sum;
}

class KronosLongRunTests : public juce::UnitTest
{
public:
    KronosLongRunTests() : juce::UnitTest("Kronos Long Run Tests", "Kronos") {}

    void runTest() override
    {
        scenario("Jittered one-second ticks lose no time over a simulated week");
        {
            FakeClock clock(juce::Time(2026, 2, 2, 9, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            juce::Random random(7);
            TrackedTimeOracle oracle;

            processor.startTracking();
            const auto sessionStart = clock.now();

            for (int tick = 0; tick < 7 * 86400; ++tick)
            {
                const auto before = clock.now();
                clock.advanceMilliseconds(900 + random.nextInt(201));
                oracle.addInterval(before, clock.now());
                processor.timerCallback();
            }

            processor.stopTracking();
            emitTestIO("Output: wall=" + juce::String((clock.now() - sessionStart).inSeconds())
                     + ", tracked=" + juce::String(processor.getTotalTimeInSeconds()));
            expectEquals(processor.getTotalTimeInSeconds(), oracle.getTotalSeconds());
            expectEquals(sumOfDays(processor), processor.getTotalTimeInSeconds());
        }

        scenario("Ticks faster than one second still accumulate");
        {
            FakeClock clock(juce::Time(2026, 2, 3, 9, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            for (int tick = 0; tick < 1200; ++tick)
            {
                clock.advanceMilliseconds(500);
                processor.timerCallback();
            }
            processor.stopTracking();

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(600));
        }

        scenario("Fractions survive repeated pause and resume");
        {
            FakeClock clock(juce::Time(2026, 2, 4, 9, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            for (int cycle = 0; cycle < 1000; ++cycle)
            {
                processor.startTracking();
                clock.advanceMilliseconds(1500);
                processor.stopTracking();
                clock.advanceSeconds(10);
            }

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(1500));
        }

        scenario("Overnight sleep is not counted as session time");
        {
            FakeClock clock(juce::Time(2026, 2, 5, 22, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(60);
            processor.timerCallback();
            const auto beforeSleep = processor.getTotalTimeInSeconds();

            clock.advanceSeconds(9 * 3600); // laptop lid closed, no callbacks fire

            const auto displayedOnWake = processor.getTotalTimeInSeconds();
            processor.timerCallback();
            const auto afterWake = processor.getTotalTimeInSeconds();
            emitTestIO("Output: beforeSleep=" + juce::String(beforeSleep)
                     + ", displayedOnWake=" + juce::String(displayedOnWake)
                     + ", afterWake=" + juce::String(afterWake));

            expect(displayedOnWake <= beforeSleep + 1, "Display must not jump on wake.");
            expect(afterWake <= beforeSleep + 1, "Sleep must not be credited.");

            clock.advanceSeconds(1);
            processor.timerCallback();
            expect(processor.getTotalTimeInSeconds() > afterWake, "Tracking must resume after wake.");
        }

        scenario("Sleep before pressing pause is not counted");
        {
            FakeClock clock(juce::Time(2026, 2, 6, 22, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(5 * 3600);
            processor.stopTracking();

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(0));
        }

        scenario("A multi-minute message thread stall is still counted");
        {
            FakeClock clock(juce::Time(2026, 2, 7, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(240);
            processor.timerCallback();

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(240));
        }

        scenario("The system clock moving backwards never removes or invents time");
        {
            FakeClock clock(juce::Time(2026, 2, 8, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(100);
            processor.timerCallback();
            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(100));

            clock.advanceSeconds(-3600);
            expect(processor.getTotalTimeInSeconds() >= 100, "Display must not go backwards.");
            processor.timerCallback();
            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(100));

            clock.advanceSeconds(10);
            processor.timerCallback();
            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(110));
            expectEquals(sumOfDays(processor), static_cast<juce::int64>(110));
        }

        scenario("Host-side parameter changes while paused do not invent time");
        {
            FakeClock clock(juce::Time(2026, 2, 9, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(10);
            processor.stopTracking();
            clock.advanceSeconds(2 * 3600);

            processor.setTracking(true); // automation or a generic host panel
            const auto displayed = processor.getTotalTimeInSeconds();
            processor.timerCallback();
            clock.advanceSeconds(3);
            processor.timerCallback();
            processor.setTracking(false);
            processor.timerCallback();

            emitTestIO("Output: displayed=" + juce::String(displayed)
                     + ", total=" + juce::String(processor.getTotalTimeInSeconds()));
            expect(displayed <= 11, "Display must not show the paused interval.");
            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(13));
        }

        scenario("Host-side pause stops the clock");
        {
            FakeClock clock(juce::Time(2026, 2, 10, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(10);
            processor.timerCallback();
            processor.setTracking(false);
            processor.timerCallback();
            clock.advanceSeconds(3600);
            processor.timerCallback();
            processor.startTracking();
            clock.advanceSeconds(5);
            processor.stopTracking();

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(15));
        }

        scenario("Restoring a tracking state after a long pause does not invent time");
        {
            FakeClock clock(juce::Time(2026, 2, 11, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(30);
            processor.timerCallback();

            juce::MemoryBlock whileTracking;
            processor.getStateInformation(whileTracking);

            processor.stopTracking();
            clock.advanceSeconds(3 * 3600);

            processor.setStateInformation(whileTracking.getData(), static_cast<int>(whileTracking.getSize()));
            const auto displayed = processor.getTotalTimeInSeconds();
            clock.advanceSeconds(2);
            processor.timerCallback();
            emitTestIO("Output: displayed=" + juce::String(displayed)
                     + ", total=" + juce::String(processor.getTotalTimeInSeconds()));

            expectEquals(displayed, static_cast<juce::int64>(30));
            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(32));
        }

        scenario("Saving captures time since the last timer tick");
        {
            FakeClock clock(juce::Time(2026, 2, 12, 12, 0, 0, 0, true));
            KronosAudioProcessor source([&clock]() { return clock.now(); });

            source.startTracking();
            clock.advanceSeconds(45); // message thread busy, no ticks delivered

            juce::MemoryBlock saved;
            source.getStateInformation(saved);

            KronosAudioProcessor target([&clock]() { return clock.now(); });
            target.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            target.stopTracking();

            expectEquals(target.getTotalTimeInSeconds(), static_cast<juce::int64>(45));
        }

        scenario("Most-time order and new days stay current while tracking");
        {
            FakeClock clock(juce::Time(2026, 2, 13, 23, 59, 50, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            processor.setDateSortMode(KronosAudioProcessor::DateSortMode::MostTime);

            processor.startTracking();
            clock.advanceSeconds(5);
            processor.timerCallback();
            const auto beforeMidnight = processor.getSortedDates();
            expectEquals(beforeMidnight.size(), 1);

            for (int tick = 0; tick < 20; ++tick)
            {
                clock.advanceSeconds(1);
                processor.timerCallback();
            }

            const auto afterMidnight = processor.getSortedDates();
            expectEquals(afterMidnight.size(), 2);
            expectEquals(afterMidnight[0].formatted("%Y-%m-%d"), clock.now().formatted("%Y-%m-%d"));

            processor.setDateSortMode(KronosAudioProcessor::DateSortMode::MostRecent);
            expectEquals(processor.getSortedDates()[0].formatted("%Y-%m-%d"), clock.now().formatted("%Y-%m-%d"));
        }

        scenario("State from another plugin is rejected");
        {
            FakeClock clock(juce::Time(2026, 2, 14, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            processor.startTracking();
            clock.advanceSeconds(30);
            processor.stopTracking();
            processor.setDarkMode(false);

            juce::XmlElement foreign("SomeOtherPlugin");
            foreign.setAttribute("gain", 0.5);
            juce::MemoryBlock data;
            KronosAudioProcessor::copyXmlToBinary(foreign, data);
            processor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(30));
            expect(!processor.isDarkMode(), "Foreign state must not reset parameters.");

            juce::MemoryBlock saved;
            clock.advanceSeconds(1);
            processor.getStateInformation(saved);
            KronosAudioProcessor target([&clock]() { return clock.now(); });
            target.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
            expectEquals(target.getTotalTimeInSeconds(), static_cast<juce::int64>(30));
        }

        scenario("Damaged tracking values are sanitised on load");
        {
            FakeClock clock(juce::Time(2026, 2, 15, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            const juce::Time day(2026, 2, 10, 12, 0, 0, 0, true);
            const auto dayKey = day.formatted("%Y-%m-%d");

            juce::Array<std::pair<juce::String, juce::int64>> times;
            times.add({ dayKey, 100 });
            times.add({ dayKey, 900 });
            times.add({ "not-a-date", 50 });
            times.add({ "2026-02-30", 70 });
            times.add({ "2026-3-5", 60 });
            times.add({ "2026-03-09", -500 });

            auto state = buildStateWithTrackingData(processor, -999, { day, day, day }, times);
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(100));
            expectEquals(sumOfDays(processor), static_cast<juce::int64>(100));
            expectEquals(countDateOccurrences(processor.getSessionDates(), dayKey), 1);
            expectEquals(processor.getTimeForDate(day), static_cast<juce::int64>(100));
            expectEquals(processor.getSessionDates().size(), 2); // the valid day and today
        }

        scenario("State saved under the old root name still loads and is upgraded");
        {
            FakeClock clock(juce::Time(2026, 2, 17, 12, 0, 0, 0, true));
            KronosAudioProcessor source([&clock]() { return clock.now(); });
            const juce::Time day(2026, 2, 12, 12, 0, 0, 0, true);

            juce::Array<std::pair<juce::String, juce::int64>> times;
            times.add({ day.formatted("%Y-%m-%d"), 4000 });
            auto state = buildStateWithTrackingData(source, 4000, { day }, times);

            std::unique_ptr<juce::XmlElement> xml(KronosAudioProcessor::getXmlFromBinary(state.getData(), static_cast<int>(state.getSize())));
            xml->setTagName("KronosState");
            juce::MemoryBlock legacy;
            KronosAudioProcessor::copyXmlToBinary(*xml, legacy);

            KronosAudioProcessor target([&clock]() { return clock.now(); });
            target.setStateInformation(legacy.getData(), static_cast<int>(legacy.getSize()));
            expectEquals(target.getTotalTimeInSeconds(), static_cast<juce::int64>(4000));

            juce::MemoryBlock resaved;
            clock.advanceSeconds(1);
            target.getStateInformation(resaved);
            std::unique_ptr<juce::XmlElement> resavedXml(KronosAudioProcessor::getXmlFromBinary(resaved.getData(), static_cast<int>(resaved.getSize())));
            expect(resavedXml->hasTagName("Parameters"), "Re-saved state should use the current root.");
        }

        scenario("Timestamps alone restore when per-day history is missing");
        {
            FakeClock clock(juce::Time(2026, 2, 18, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            const juce::Time day(2026, 2, 12, 12, 0, 0, 0, true);

            auto state = buildStateWithTrackingData(processor, 500, { day }, {});
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(500));
            expect(hasDateKey(processor.getSessionDates(), day.formatted("%Y-%m-%d")), "Timestamped day should survive.");
        }

        scenario("A backup without tracking data is refused");
        {
            FakeClock clock(juce::Time(2026, 2, 19, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            processor.setDarkMode(false);

            auto paramsOnly = processor.parameters->copyState();
            paramsOnly.removeChild(paramsOnly.getChildWithName("TrackingData"), nullptr);
            std::unique_ptr<juce::XmlElement> xml(paramsOnly.createXml());
            juce::MemoryBlock data;
            KronosAudioProcessor::copyXmlToBinary(*xml, data);

            processor.setDarkMode(true);
            expect(!processor.restoreStateFromData(data.getData(), static_cast<int>(data.getSize())),
                   "Restore must report failure when no time data is present.");
            expect(processor.isDarkMode(), "A refused backup must not change parameters.");

            processor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
            expect(!processor.isDarkMode(), "Hosts may still load parameter-only state.");
        }

        scenario("Derived stats come from the real tracking data");
        {
            FakeClock clock(juce::Time(2026, 2, 21, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            expectEquals(processor.getDailyAverageSeconds(), static_cast<juce::int64>(0));
            expectEquals(processor.getDaysLogged(), 0);

            const juce::Time dayA(2026, 2, 18, 12, 0, 0, 0, true);
            const juce::Time dayB(2026, 2, 19, 12, 0, 0, 0, true);
            const juce::Time dayC(2026, 2, 20, 12, 0, 0, 0, true);

            juce::Array<std::pair<juce::String, juce::int64>> times;
            times.add({ dayA.formatted("%Y-%m-%d"), 3000 });
            times.add({ dayB.formatted("%Y-%m-%d"), 9000 });
            times.add({ dayC.formatted("%Y-%m-%d"), 0 });

            auto state = buildStateWithTrackingData(processor, 12000, { dayA, dayB, dayC }, times);
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            expectEquals(processor.getDaysLogged(), 2, "Days with no time do not count as logged.");
            expectEquals(processor.getDailyAverageSeconds(), static_cast<juce::int64>(6000));
            expectEquals(processor.getLongestDaySeconds(), static_cast<juce::int64>(9000));
            expectEquals(processor.getTodaySeconds(), static_cast<juce::int64>(0));
            expectEquals(processor.getCurrentRunSeconds(), static_cast<juce::int64>(0));

            processor.startTracking();
            clock.advanceSeconds(20);
            processor.timerCallback();
            clock.advanceMilliseconds(5500); // not yet credited by a tick

            expectEquals(processor.getTodaySeconds(), static_cast<juce::int64>(25));
            expectEquals(processor.getCurrentRunSeconds(), static_cast<juce::int64>(25));
            expectEquals(processor.getDaysLogged(), 3);
            expectEquals(processor.getDailyAverageSeconds(), static_cast<juce::int64>((12000 + 25) / 3));

            processor.stopTracking();
            clock.advanceSeconds(600);
            expectEquals(processor.getCurrentRunSeconds(), static_cast<juce::int64>(25),
                         "A paused run keeps showing what it reached.");

            processor.startTracking();
            expectEquals(processor.getCurrentRunSeconds(), static_cast<juce::int64>(0),
                         "Resuming starts a new run.");
            clock.advanceSeconds(7);
            processor.timerCallback();
            expectEquals(processor.getCurrentRunSeconds(), static_cast<juce::int64>(7));
            expectEquals(processor.getTodaySeconds(), static_cast<juce::int64>(32));
        }

        scenario("Sleep is excluded from this run and today");
        {
            FakeClock clock(juce::Time(2026, 2, 22, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            processor.startTracking();
            clock.advanceSeconds(30);
            processor.timerCallback();
            clock.advanceSeconds(3 * 3600);

            expectEquals(processor.getCurrentRunSeconds(), static_cast<juce::int64>(30));
            expectEquals(processor.getTodaySeconds(), static_cast<juce::int64>(30));
        }

        scenario("Backup summaries read totals without applying them");
        {
            FakeClock clock(juce::Time(2026, 2, 23, 10, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            const juce::Time dayA(2026, 2, 18, 12, 0, 0, 0, true);
            const juce::Time dayB(2026, 2, 19, 12, 0, 0, 0, true);

            juce::Array<std::pair<juce::String, juce::int64>> times;
            times.add({ dayA.formatted("%Y-%m-%d"), 3000 });
            times.add({ dayB.formatted("%Y-%m-%d"), 4000 });

            auto state = buildStateWithTrackingData(processor, 7000, { dayA, dayB }, times);
            const auto summary = KronosAudioProcessor::summariseBackup(state.getData(), static_cast<int>(state.getSize()));

            expect(summary.isValid, "A real backup should summarise.");
            expectEquals(summary.totalSeconds, static_cast<juce::int64>(7000));
            expectEquals(summary.daysLogged, 2);
            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(0));

            const auto revision = processor.getStateRevision();
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            expect(processor.getStateRevision() != revision, "Loading state should tell the editor to redraw.");

            const char garbage[] = "not-a-backup";
            expect(!KronosAudioProcessor::summariseBackup(garbage, static_cast<int>(sizeof(garbage))).isValid,
                   "Garbage must not summarise.");
        }

        scenario("A parameter changed right after a save reaches the next save");
        {
            FakeClock clock(juce::Time(2026, 2, 20, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            juce::MemoryBlock first;
            processor.getStateInformation(first);
            processor.setDarkMode(false);

            juce::MemoryBlock second;
            processor.getStateInformation(second); // inside the debounce window

            KronosAudioProcessor target([&clock]() { return clock.now(); });
            target.setStateInformation(second.getData(), static_cast<int>(second.getSize()));
            expect(!target.isDarkMode(), "Debounce must not hide a parameter change.");
        }

        scenario("A total lower than its own days heals from the day history");
        {
            FakeClock clock(juce::Time(2026, 2, 16, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            const juce::Time dayA(2026, 2, 10, 12, 0, 0, 0, true);
            const juce::Time dayB(2026, 2, 11, 12, 0, 0, 0, true);

            juce::Array<std::pair<juce::String, juce::int64>> times;
            times.add({ dayA.formatted("%Y-%m-%d"), 7200 });
            times.add({ dayB.formatted("%Y-%m-%d"), 3600 });

            auto state = buildStateWithTrackingData(processor, 3600, { dayA, dayB }, times);
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            expectEquals(processor.getTotalTimeInSeconds(), static_cast<juce::int64>(10800));
        }

        runTimeZoneScenarios();
        runSerialisationGrowthScenario();
        runConcurrentSaveScenario();
        runEditorPerformanceScenario();
        runSheetScenarios();
        runSnapshotsIfRequested();
    }

private:
    void scenario(const juce::String& name)
    {
        emitEmbeddedTestLog("[TEST] " + name);
        beginTest(name);
    }

    void runTimeZoneScenarios()
    {
       #if KRONOS_CAN_SIMULATE_TIME_ZONES
        scenario("Years of daily use conserve every second in every zone");
        for (const auto* zone : { "America/New_York", "Europe/London", "Asia/Kolkata",
                                  "Australia/Lord_Howe", "Pacific/Auckland" })
        {
            simulateYearsOfUse(zone, 2 * 365);
        }

        struct Transition { const char* zone; juce::Time utc; };
        const Transition transitions[] {
            { "America/New_York",    juce::Time(2026,  2,  8,  7,  0, 0, 0, false) },
            { "America/New_York",    juce::Time(2026, 10,  1,  6,  0, 0, 0, false) },
            { "Europe/London",       juce::Time(2026,  2, 29,  1,  0, 0, 0, false) },
            { "Europe/London",       juce::Time(2026,  9, 25,  1,  0, 0, 0, false) },
            { "Australia/Lord_Howe", juce::Time(2026,  9,  3, 15, 30, 0, 0, false) },
            { "America/Havana",      juce::Time(2024,  2, 10,  5,  0, 0, 0, false) },
            { "America/Havana",      juce::Time(2024, 10,  3,  5,  0, 0, 0, false) },
            { "America/Sao_Paulo",   juce::Time(2017,  9, 15,  3,  0, 0, 0, false) },
            { "America/Sao_Paulo",   juce::Time(2018,  1, 18,  2,  0, 0, 0, false) },
            { "Pacific/Apia",        juce::Time(2011, 11, 30, 10,  0, 0, 0, false) },
        };

        scenario("Clock changes split time onto the correct calendar days");
        for (const auto& transition : transitions)
            for (int seed = 1; seed <= 8; ++seed)
                simulateTransition(transition.zone, transition.utc, seed);

        scenario("Reopening a project in another time zone keeps each day intact");
        {
            juce::MemoryBlock saved;
            juce::StringArray keys;
            juce::int64 total = 0;

            {
                ScopedTimeZone newYork("America/New_York");
                FakeClock clock(juce::Time(2026, 5, 1, 23, 30, 0, 0, true));
                KronosAudioProcessor source([&clock]() { return clock.now(); });

                for (int day = 0; day < 5; ++day)
                {
                    keys.add(clock.now().formatted("%Y-%m-%d"));
                    source.startTracking();
                    for (int tick = 0; tick < 10; ++tick)
                    {
                        clock.advanceSeconds(60);
                        source.timerCallback();
                    }
                    source.stopTracking();
                    clock.advanceSeconds(86400 - 600);
                }

                clock.advanceSeconds(1);
                source.getStateInformation(saved);
                total = source.getTotalTimeInSeconds();
            }

            ScopedTimeZone tokyo("Asia/Tokyo");
            FakeClock clock(juce::Time(2026, 5, 7, 12, 0, 0, 0, false));
            KronosAudioProcessor target([&clock]() { return clock.now(); });
            target.setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));

            juce::int64 visibleTotal = 0;
            juce::StringArray visibleKeys;
            for (const auto& date : target.getSessionDates())
            {
                visibleTotal += target.getTimeForDate(date);
                visibleKeys.addIfNotAlreadyThere(date.formatted("%Y-%m-%d"));
            }

            int missingKeys = 0;
            for (const auto& key : keys)
                if (!visibleKeys.contains(key))
                    ++missingKeys;

            emitTestIO("Output: total=" + juce::String(total) + ", visible=" + juce::String(visibleTotal)
                     + ", keys=" + visibleKeys.joinIntoString(","));
            expectEquals(total, static_cast<juce::int64>(5 * 600));
            expectEquals(visibleTotal, total);
            expectEquals(missingKeys, 0);
        }
       #endif
    }

   #if KRONOS_CAN_SIMULATE_TIME_ZONES
    void simulateYearsOfUse(const char* zone, int days)
    {
        ScopedTimeZone scopedZone(zone);
        juce::Random random(juce::String(zone).hashCode());
        FakeClock clock(juce::Time(2024, 0, 1, 21, 0, 0, 0, true));
        auto now = [&clock]() { return clock.now(); };
        auto processor = std::make_unique<KronosAudioProcessor>(now);
        TrackedTimeOracle oracle;
        int reopens = 0;
        int conservationFailures = 0;

        for (int day = 0; day < days; ++day)
        {
            processor->startTracking();
            const auto sessionStart = clock.now();
            const juce::int64 sessionLength = (2 + random.nextInt(7)) * 3600000 + random.nextInt(3600000);
            juce::int64 elapsed = 0;

            while (elapsed < sessionLength)
            {
                const auto step = juce::jmin<juce::int64>(sessionLength - elapsed, 30000 + random.nextInt(210000));
                clock.advanceMilliseconds(step);
                elapsed += step;
                processor->timerCallback();
            }

            oracle.addInterval(sessionStart, clock.now());
            processor->stopTracking();

            if (sumOfDays(*processor) != processor->getTotalTimeInSeconds())
                ++conservationFailures;

            if (day % 7 == 6)
            {
                juce::MemoryBlock saved;
                clock.advanceSeconds(1);
                processor->getStateInformation(saved);
                processor = std::make_unique<KronosAudioProcessor>(now);
                processor->setStateInformation(saved.getData(), static_cast<int>(saved.getSize()));
                ++reopens;
            }

            clock.advanceMilliseconds(86400000 - sessionLength + random.nextInt(7200000) - 3600000);
        }

        juce::String worst;
        const auto tolerance = static_cast<juce::int64>(3);
        const auto daysOff = oracle.countDaysOffBy(*processor, tolerance, worst);
        const auto totalError = std::abs(processor->getTotalTimeInSeconds() - oracle.getTotalSeconds());

        emitTestIO(juce::String(zone) + ": oracle=" + juce::String(oracle.getTotalSeconds())
                 + "s, tracked=" + juce::String(processor->getTotalTimeInSeconds())
                 + "s, drift=" + juce::String(totalError) + "s, daysOff=" + juce::String(daysOff)
                 + ", worst=" + worst + ", dates=" + juce::String(processor->getSessionDates().size()));

        expectEquals(conservationFailures, 0, "Total must equal the sum of days in " + juce::String(zone));
        expect(totalError <= reopens + 1, "Drift of " + juce::String(totalError) + "s in " + juce::String(zone));
        expectEquals(daysOff, 0, "Day totals wrong in " + juce::String(zone) + ": " + worst);
    }

    void simulateTransition(const char* zone, const juce::Time& transitionUtc, int seed)
    {
        ScopedTimeZone scopedZone(zone);
        juce::Random random(seed * 7919);
        FakeClock clock(transitionUtc - juce::RelativeTime::hours(30));
        KronosAudioProcessor processor([&clock]() { return clock.now(); });
        TrackedTimeOracle oracle;

        processor.startTracking();

        for (juce::int64 elapsed = 0; elapsed < 60 * 3600000LL;)
        {
            const auto step = static_cast<juce::int64>(700 + random.nextInt(240000));
            const auto before = clock.now();
            clock.advanceMilliseconds(step);
            elapsed += step;
            oracle.addInterval(before, clock.now());
            processor.timerCallback();
        }

        processor.stopTracking();

        juce::String worst;
        const auto daysOff = oracle.countDaysOffBy(processor, 2, worst);
        const auto totalError = std::abs(processor.getTotalTimeInSeconds() - oracle.getTotalSeconds());

        if (daysOff != 0 || totalError > 1)
            emitTestIO(juce::String(zone) + " seed " + juce::String(seed) + ": drift="
                     + juce::String(totalError) + "s, worst=" + worst);

        expect(daysOff == 0, juce::String(zone) + " misattributed a day: " + worst);
        expect(totalError <= 1, juce::String(zone) + " drifted by " + juce::String(totalError) + "s");
    }
   #endif

    void runSheetScenarios()
    {
        scenario("Backup only reports success after the file is really written");
        {
            FakeClock clock(juce::Time(2026, 2, 24, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            processor.startTracking();
            clock.advanceSeconds(90);
            processor.stopTracking();

            BackupComponent sheet(processor);

            const auto folder = juce::File::createTempFile("kronos-backup-test");
            expect(folder.createDirectory().wasOk(), "Test folder should exist.");
            const auto target = folder.getChildFile("sheet.kronos");

            expect(sheet.saveBackup(target), "A writable path should save.");
            expect(sheet.getStage() == BackupComponent::Stage::saved, "Success should show only after writing.");
            expect(target.existsAsFile(), "The file should exist.");

            juce::MemoryBlock written;
            target.loadFileAsData(written);
            expectEquals(KronosAudioProcessor::summariseBackup(written.getData(), static_cast<int>(written.getSize())).totalSeconds,
                         static_cast<juce::int64>(90));

            BackupComponent failing(processor);
            const auto blocked = target.getChildFile("not-a-folder").getChildFile("x.kronos"); // a file is in the way

            expect(!failing.saveBackup(blocked), "An unwritable path should fail.");
            expect(failing.getStage() == BackupComponent::Stage::entering, "Failure must not show the success state.");
            expect(failing.getErrorText().isNotEmpty(), "Failure should explain itself in plain text.");
            expect(!blocked.existsAsFile(), "Nothing should have been written.");

            folder.deleteRecursively();
        }

        scenario("Restore confirms before replacing and survives bad files");
        {
            FakeClock clock(juce::Time(2026, 2, 25, 12, 0, 0, 0, true));
            KronosAudioProcessor source([&clock]() { return clock.now(); });
            source.startTracking();
            clock.advanceSeconds(200);
            source.stopTracking();
            clock.advanceSeconds(1);

            juce::MemoryBlock saved;
            source.getStateInformation(saved);

            const auto folder = juce::File::createTempFile("kronos-restore-test");
            expect(folder.createDirectory().wasOk(), "Test folder should exist.");
            const auto good = folder.getChildFile("good.kronos");
            const auto bad = folder.getChildFile("bad.kronos");
            good.replaceWithData(saved.getData(), saved.getSize());
            bad.replaceWithText("not a backup");

            KronosAudioProcessor target([&clock]() { return clock.now(); });
            RestoreComponent sheet(target);

            expect(!sheet.beginConfirmation(bad), "A bad file must not reach the confirm step.");
            expect(sheet.getStage() == RestoreComponent::Stage::choosing, "It should stay on the list.");
            expect(sheet.getErrorText().isNotEmpty(), "A bad file should explain itself.");

            expect(sheet.beginConfirmation(good), "A real backup should reach the confirm step.");
            expect(sheet.getStage() == RestoreComponent::Stage::confirming, "Confirm should appear first.");
            expectEquals(target.getTotalTimeInSeconds(), static_cast<juce::int64>(0), "Nothing changes before confirming.");

            sheet.cancelOrClose();
            expect(sheet.getStage() == RestoreComponent::Stage::choosing, "Keep current should go back.");
            expectEquals(target.getTotalTimeInSeconds(), static_cast<juce::int64>(0), "Keep current must not restore.");

            sheet.beginConfirmation(good);
            expect(sheet.confirmReplace(), "Replace log should apply the backup.");
            expectEquals(target.getTotalTimeInSeconds(), static_cast<juce::int64>(200));

            folder.deleteRecursively();
        }

        scenario("Every control is reachable through the scale transform at every size");
        {
            FakeClock clock(juce::Time(2026, 2, 27, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            std::unique_ptr<juce::AudioProcessorEditor> base(processor.createEditor());
            auto* editor = dynamic_cast<KronosAudioProcessorEditor*>(base.get());
            using Control = KronosAudioProcessorEditor::Control;
            editor->setVisible(true); // hit testing skips hidden components

            const auto hits = [&](Control control)
            {
                auto& button = editor->getControl(control);
                const auto centre = editor->getLocalArea(&button, button.getLocalBounds()).getCentre();
                return editor->getComponentAt(centre) == &button;
            };

            for (const auto width : { 400, 600, 1200 })
            {
                editor->setSize(width, width * 3 / 4);

                for (const auto control : { Control::play, Control::theme, Control::menu,
                                            Control::sortRecent, Control::sortLongest })
                    expect(hits(control), "Main control missed at width " + juce::String(width));

                editor->setMenuOpen(true);
                for (const auto control : { Control::backupItem, Control::restoreItem, Control::aboutItem })
                    expect(hits(control), "Menu item missed at width " + juce::String(width));
                editor->setMenuOpen(false);
            }

            // Every control keeps a 32px target at the base size
            editor->setSize(600, 450);
            editor->setMenuOpen(true);
            for (const auto control : { Control::play, Control::theme, Control::menu, Control::sortRecent,
                                        Control::sortLongest, Control::backupItem, Control::restoreItem, Control::aboutItem })
            {
                auto& button = editor->getControl(control);
                expect(button.getHeight() >= 32 && button.getWidth() >= 32, "Hit target under 32px: " + button.getTitle());
            }
            editor->setMenuOpen(false);

            // The chamfered corner of the play button is not part of the control
            auto& play = editor->getControl(Control::play);
            expect(play.hitTest(36, 36), "The play button centre should be live.");
            expect(!play.hitTest(1, 1), "The cut corner should not take clicks.");
        }

        scenario("Controls drive the processor");
        {
            FakeClock clock(juce::Time(2026, 2, 28, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            std::unique_ptr<juce::AudioProcessorEditor> base(processor.createEditor());
            auto* editor = dynamic_cast<KronosAudioProcessorEditor*>(base.get());
            using Control = KronosAudioProcessorEditor::Control;

            const auto press = [&](Control control)
            {
                if (auto* button = dynamic_cast<KronosButton*>(&editor->getControl(control)))
                    button->onActivated();
            };

            expect(!processor.isTracking(), "Test mode starts paused.");
            press(Control::play);
            expect(processor.isTracking(), "Play should start tracking.");
            press(Control::play);
            expect(!processor.isTracking(), "Play should pause tracking.");

            const auto wasDark = processor.isDarkMode();
            press(Control::theme);
            expect(processor.isDarkMode() != wasDark, "The theme button should flip the theme.");
            press(Control::theme);

            press(Control::sortLongest);
            expect(processor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostTime, "Longest should sort by time.");
            press(Control::sortRecent);
            expect(processor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostRecent, "Recent should sort by date.");

            press(Control::menu);
            press(Control::backupItem);
            expect(dynamic_cast<BackupComponent*>(editor->getOpenSheet()) != nullptr, "Back up should open its sheet.");

            auto* sheet = editor->getOpenSheet();
            sheet->onClose();
            expect(!sheet->isEnabled(), "A sheet that is closing must not take input.");
        }

        scenario("The header stays still when tracking changes");
        {
            KronosLookAndFeel lookAndFeel;
            HeaderBar header;
            header.setLookAndFeel(&lookAndFeel);
            header.setBounds(16, 10, 568, 36);

            header.setTracking(true, false, false);
            const auto rule = header.getRuleArea();
            const auto chip = header.getChipArea();

            header.setTracking(false, false, false);
            header.resized();

            expect(header.getRuleArea() == rule, "The rule must not move between states.");
            expect(header.getChipArea() == chip, "The chip must not change size between states.");
            expect(header.getRuleArea().getWidth() > 10.0f, "The rule should have room for its glint.");

            header.setLookAndFeel(nullptr);
        }

        scenario("Every session stays visible when a long list is scrolled");
        {
            FakeClock clock(juce::Time(2026, 3, 1, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });

            juce::Array<juce::Time> dates;
            juce::Array<std::pair<juce::String, juce::int64>> times;
            for (int day = 1; day <= 40; ++day)
            {
                const auto date = clock.now() - juce::RelativeTime::days(day);
                dates.add(date);
                times.add({ date.formatted("%Y-%m-%d"), 3600 });
            }

            auto state = buildStateWithTrackingData(processor, 40 * 3600, dates, times);
            processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

            std::unique_ptr<juce::AudioProcessorEditor> base(processor.createEditor());
            auto* editor = dynamic_cast<KronosAudioProcessorEditor*>(base.get());
            editor->setMotionEnabled(false);
            editor->setVisible(true);

            for (const auto row : { 0, 15, 39 })
            {
                editor->scrollSessionsToRow(row);
                const auto image = editor->createComponentSnapshot({ 30, 300, 540, 130 }, true, 1.0f);

                int inkPixels = 0;
                for (int y = 0; y < image.getHeight(); ++y)
                    for (int x = 0; x < image.getWidth(); ++x)
                        if (image.getPixelAt(x, y).getBrightness() > 0.5f)
                            ++inkPixels;

                expect(inkPixels > 500, "Rows around " + juce::String(row) + " drew only " + juce::String(inkPixels) + " bright pixels.");
            }
        }

        scenario("The default backup name is readable in the light theme");
        {
            FakeClock clock(juce::Time(2026, 3, 2, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            processor.setDarkMode(false);

            std::unique_ptr<juce::AudioProcessorEditor> base(processor.createEditor());
            auto* editor = dynamic_cast<KronosAudioProcessorEditor*>(base.get());
            editor->setVisible(true);
            editor->openSheet(KronosAudioProcessorEditor::Sheet::backup, false);

            auto* sheet = editor->getOpenSheet();
            const auto image = sheet->createComponentSnapshot({ 8, 110, 300, 26 }, true, 1.0f);

            int darkPixels = 0;
            for (int y = 0; y < image.getHeight(); ++y)
                for (int x = 0; x < image.getWidth(); ++x)
                    if (image.getPixelAt(x, y).getBrightness() < 0.4f)
                        ++darkPixels;

            expect(darkPixels > 50, "The name field drew only " + juce::String(darkPixels) + " dark pixels.");
        }

        scenario("The editor follows tracking, theme and sort state");
        {
            FakeClock clock(juce::Time(2026, 2, 26, 12, 0, 0, 0, true));
            KronosAudioProcessor processor([&clock]() { return clock.now(); });
            std::unique_ptr<juce::AudioProcessorEditor> base(processor.createEditor());
            auto* editor = dynamic_cast<KronosAudioProcessorEditor*>(base.get());
            expect(editor != nullptr, "The editor should be the Kronos editor.");

            processor.startTracking();
            processor.setDateSortMode(KronosAudioProcessor::DateSortMode::MostTime);
            processor.setDarkMode(false);
            editor->refreshNow();

            for (const auto sheet : { KronosAudioProcessorEditor::Sheet::backup,
                                      KronosAudioProcessorEditor::Sheet::restore,
                                      KronosAudioProcessorEditor::Sheet::about })
            {
                editor->openSheet(sheet, false);
                expect(editor->getOpenSheet() != nullptr, "The sheet should open.");
                expect(editor->getOpenSheet()->getKicker().isNotEmpty(), "The sheet should have a kicker.");
            }

            editor->setMenuOpen(true);
            editor->setMenuOpen(false);
            editor->setSize(1200, 900);
            editor->setSize(400, 300);

            juce::Image canvas(juce::Image::ARGB, 400, 300, true);
            juce::Graphics g(canvas);
            editor->paintEntireComponent(g, true);
            expect(canvas.getPixelAt(200, 150).getAlpha() > 0, "The editor should paint at its smallest size.");
        }
    }

    void runSnapshotsIfRequested()
    {
        const auto folder = juce::SystemStats::getEnvironmentVariable("KRONOS_SNAPSHOT_DIR", {});
        if (folder.isEmpty())
            return;

        FakeClock clock(juce::Time(2026, 9, 31, 22, 14, 7, 0, true));
        KronosAudioProcessor processor([&clock]() { return clock.now(); });

        // The prototype's sample days, padded to its 27 logged days
        const std::pair<int, juce::int64> history[] { { 1, 22360 }, { 2, 11112 }, { 4, 35283 }, { 5, 15655 }, { 7, 25351 },
                                                      { 8, 6250 }, { 10, 19800 }, { 11, 8324 }, { 12, 14000 }, { 13, 26000 },
                                                      { 15, 9100 }, { 16, 31000 }, { 18, 12400 }, { 19, 20500 }, { 20, 7300 },
                                                      { 22, 16800 }, { 23, 27400 }, { 25, 10100 }, { 26, 23900 }, { 27, 18200 },
                                                      { 29, 13300 }, { 30, 29000 }, { 32, 8800 }, { 33, 21700 }, { 34, 15200 },
                                                      { 36, 32100 } };
        juce::Array<juce::Time> dates;
        juce::Array<std::pair<juce::String, juce::int64>> times;
        juce::int64 total = 0;

        for (const auto& [daysAgo, seconds] : history)
        {
            const auto date = clock.now() - juce::RelativeTime::days(daysAgo);
            dates.add(date);
            times.add({ date.formatted("%Y-%m-%d"), seconds });
            total += seconds;
        }

        auto state = buildStateWithTrackingData(processor, total + 60000, dates, times);
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

        processor.startTracking();
        for (int tick = 0; tick < 95; ++tick)
        {
            clock.advanceSeconds(100);
            processor.timerCallback();
        }

        const auto backupFile = juce::File(folder).getChildFile("snapshot-backup.kronos");
        juce::MemoryBlock backup;
        processor.getStateInformation(backup);
        backupFile.replaceWithData(backup.getData(), backup.getSize());

        std::unique_ptr<juce::AudioProcessorEditor> base(processor.createEditor());
        auto* editor = dynamic_cast<KronosAudioProcessorEditor*>(base.get());
        editor->setMotionEnabled(false);
        editor->setVisible(true);

        const auto capture = [&](const juce::String& name)
        {
            for (const auto width : { 600, 1200 })
            {
                editor->setSize(width, width * 3 / 4);
                editor->refreshNow();

                const auto image = editor->createComponentSnapshot(editor->getLocalBounds(), true, 1.0f);
                juce::File file(juce::File(folder).getChildFile(name + "-" + juce::String(width) + ".png"));
                file.deleteFile();
                juce::FileOutputStream stream(file);
                juce::PNGImageFormat().writeImageToStream(image, stream);
            }
        };

        using Sheet = KronosAudioProcessorEditor::Sheet;

        capture("dark-tracking");

        processor.setDarkMode(false);
        editor->setMenuOpen(true);
        capture("light-menu");
        editor->setMenuOpen(false);
        processor.setDarkMode(true);

        editor->openSheet(Sheet::backup, false);
        capture("backup");

        editor->openSheet(Sheet::restore, false);
        if (auto* sheet = dynamic_cast<RestoreComponent*>(editor->getOpenSheet()))
            sheet->beginConfirmation(backupFile);
        capture("restore-confirm");

        editor->openSheet(Sheet::about, false);
        capture("about");

        std::unique_ptr<juce::AudioProcessorEditor> pausedBase(nullptr);
        base.reset();
        processor.stopTracking();
        pausedBase.reset(processor.createEditor());
        auto* pausedEditor = dynamic_cast<KronosAudioProcessorEditor*>(pausedBase.get());
        pausedEditor->setMotionEnabled(false);
        pausedEditor->setVisible(true);
        editor = pausedEditor;
        capture("paused");
    }

    void runEditorPerformanceScenario()
    {
        scenario("Editor open and paint cost with a year of history");

        FakeClock clock(juce::Time(2026, 0, 1, 12, 0, 0, 0, true));
        KronosAudioProcessor processor([&clock]() { return clock.now(); });

        juce::Array<juce::Time> dates;
        juce::Array<std::pair<juce::String, juce::int64>> times;
        for (int day = 1; day <= 365; ++day)
        {
            const auto date = clock.now() - juce::RelativeTime::days(day);
            dates.add(date);
            times.add({ date.formatted("%Y-%m-%d"), 3600 + (day * 37) % 7200 });
        }

        auto state = buildStateWithTrackingData(processor, 0, dates, times);
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

        const auto openStart = juce::Time::getMillisecondCounterHiRes();
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        const auto openMs = juce::Time::getMillisecondCounterHiRes() - openStart;

        juce::Image canvas(juce::Image::ARGB, 1200, 900, true);

        for (const auto dark : { true, false })
        {
            processor.setDarkMode(dark);
            juce::Graphics g(canvas);
            g.addTransform(juce::AffineTransform::scale(2.0f));

            const auto paintStart = juce::Time::getMillisecondCounterHiRes();
            for (int paint = 0; paint < 20; ++paint)
                editor->paintEntireComponent(g, true);

            emitTestIO(juce::String(dark ? "Dark" : "Light") + " paintMs="
                     + juce::String((juce::Time::getMillisecondCounterHiRes() - paintStart) / 20.0, 2));

            const auto snapshotDir = juce::SystemStats::getEnvironmentVariable("KRONOS_SNAPSHOT_DIR", {});
            if (snapshotDir.isNotEmpty())
            {
                juce::FileOutputStream stream(juce::File(snapshotDir).getChildFile(dark ? "dark.png" : "light.png"));
                stream.setPosition(0);
                stream.truncate();
                juce::PNGImageFormat().writeImageToStream(canvas, stream);
            }
        }

        emitTestIO("Output: editorOpenMs=" + juce::String(openMs, 1));
        expect(canvas.getPixelAt(600, 450).getAlpha() > 0, "The editor should have painted something.");
    }

    void runSerialisationGrowthScenario()
    {
        scenario("Thirty years of history stay small and responsive");

        FakeClock clock(juce::Time(2026, 0, 1, 12, 0, 0, 0, true));
        KronosAudioProcessor processor([&clock]() { return clock.now(); });

        juce::Array<juce::Time> dates;
        juce::Array<std::pair<juce::String, juce::int64>> times;
        for (int day = 1; day <= 30 * 365; ++day)
        {
            const auto date = clock.now() - juce::RelativeTime::days(day);
            dates.add(date);
            times.add({ date.formatted("%Y-%m-%d"), 3600 + (day * 37) % 7200 });
        }

        auto state = buildStateWithTrackingData(processor, 0, dates, times);

        const auto loadStart = juce::Time::getMillisecondCounterHiRes();
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        const auto loadMs = juce::Time::getMillisecondCounterHiRes() - loadStart;

        processor.setDateSortMode(KronosAudioProcessor::DateSortMode::MostTime);
        processor.startTracking();

        double worstTickMs = 0.0;
        for (int tick = 0; tick < 50; ++tick)
        {
            clock.advanceSeconds(1);
            const auto tickStart = juce::Time::getMillisecondCounterHiRes();
            processor.timerCallback();
            juce::ignoreUnused(processor.getSortedDates());
            worstTickMs = juce::jmax(worstTickMs, juce::Time::getMillisecondCounterHiRes() - tickStart);
        }

        juce::MemoryBlock saved;
        const auto saveStart = juce::Time::getMillisecondCounterHiRes();
        processor.getStateInformation(saved);
        const auto saveMs = juce::Time::getMillisecondCounterHiRes() - saveStart;

        emitTestIO("Output: days=" + juce::String(processor.getSessionDates().size())
                 + ", bytes=" + juce::String(static_cast<juce::int64>(saved.getSize()))
                 + ", loadMs=" + juce::String(loadMs, 1)
                 + ", saveMs=" + juce::String(saveMs, 1)
                 + ", worstTickAndSortMs=" + juce::String(worstTickMs, 2));

        expect(saved.getSize() < 2 * 1024 * 1024, "State should stay under 2 MB after thirty years.");
        expect(worstTickMs < 250.0, "A timer tick must never stall the message thread.");
    }

    void runConcurrentSaveScenario()
    {
        scenario("Hosts may save from another thread while the timer is running");

        FakeClock clock(juce::Time(2026, 0, 1, 12, 0, 0, 0, true));
        std::atomic<juce::int64> nowMs { clock.now().toMilliseconds() };
        KronosAudioProcessor processor([&nowMs]() { return juce::Time(nowMs.load()); });
        processor.startTracking();

        std::atomic<bool> keepSaving { true };
        std::atomic<int> saves { 0 };
        std::atomic<int> sorts { 0 };

        std::thread saver([&]()
        {
            while (keepSaving.load())
            {
                juce::MemoryBlock data;
                processor.getStateInformation(data);
                juce::ignoreUnused(processor.getTotalTimeInSeconds());
                ++saves;
            }
        });

        std::thread reader([&]()
        {
            while (keepSaving.load())
            {
                juce::ignoreUnused(processor.getSortedDates());
                ++sorts;
            }
        });

        // One credited second per tick, with a jump to a new calendar day every
        // thousand ticks so both containers grow while the other threads read.
        juce::int64 expectedSeconds = 0;

        for (int tick = 0; tick < 200000; ++tick)
        {
            nowMs += 1000;
            processor.timerCallback();
            ++expectedSeconds;

            if (tick % 1000 == 999)
            {
                nowMs += 86400000;
                processor.timerCallback(); // the jump is a gap and credits nothing
            }
        }

        keepSaving = false;
        saver.join();
        reader.join();

        emitTestIO("Output: saves=" + juce::String(saves.load()) + ", sorts=" + juce::String(sorts.load())
                 + ", days=" + juce::String(processor.getSessionDates().size()));
        expectEquals(processor.getTotalTimeInSeconds(), expectedSeconds);
        expectEquals(sumOfDays(processor), expectedSeconds);
        expect(processor.getSessionDates().size() > 150, "The scenario should have spanned many days.");
    }
};

KronosLongRunTests kronosLongRunTests;
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



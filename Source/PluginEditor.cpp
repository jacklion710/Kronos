/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#include "AboutComponent.h"
#include "BackupComponent.h"
#include "RestoreComponent.h"
#if JUCE_WINDOWS
    #include "BinaryData.h"
#endif

//==============================================================================
// Performance: O(1) - Constructor with multiple SVG loads and UI initialization
// Memory: High initial cost for SVG caching
// CPU: Moderate due to SVG processing and UI setup
KronosAudioProcessorEditor::KronosAudioProcessorEditor (KronosAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setResizable(true, true); // First true enables resizing, second true enables fixed aspect ratio
    setResizeLimits(400, 300, 1200, 900); // Min and max sizes
    getConstrainer()->setFixedAspectRatio(600.0f / 450.0f); // Maintain aspect ratio based on default size

    // Apply the custom look and feel and set initial theme
    setLookAndFeel(&customLookAndFeel);
    customLookAndFeel.setDarkMode(audioProcessor.isDarkMode());
    
    // Create fonts
    auto asteraFontLarge = juce::Font(36.0f);
    auto asteraFontSmall = juce::Font(16.0f);
    asteraFontLarge.setTypefaceName("ASTERA");
    asteraFontSmall.setTypefaceName("ASTERA");
    
    // Add the time label
    addAndMakeVisible(hoursLabel);
    addAndMakeVisible(minutesLabel);
    addAndMakeVisible(secondsLabel);

    hoursLabel.setFont(asteraFontLarge);
    minutesLabel.setFont(asteraFontLarge);
    secondsLabel.setFont(asteraFontLarge);

    hoursLabel.setJustificationType(juce::Justification::centred);
    minutesLabel.setJustificationType(juce::Justification::centred);
    secondsLabel.setJustificationType(juce::Justification::centred);
    
    // Setup date labels
    for (int i = 0; i < 3; ++i)
    {
        addAndMakeVisible(dateLabels[i]);
        dateLabels[i].setFont(asteraFontSmall);
        dateLabels[i].setJustificationType(juce::Justification::centred);
        // Set initial color based on theme
        dateLabels[i].setColour(juce::Label::textColourId, 
            audioProcessor.isDarkMode() ? 
                juce::Colour(0xE6, 0xE6, 0xFF) :  // Light blue-white for dark mode
                juce::Colour(0xE6, 0xD5, 0xBF));  // Beige for light mode
    }
    
    // Cache ALL SVGs first - Dark Mode
    backgroundSvgCache = juce::Drawable::createFromImageData(BinaryData::Background_Dark_svg, 
                                                           BinaryData::Background_Dark_svgSize);

    timeDisplaySvgCache = juce::Drawable::createFromImageData(BinaryData::Time_Display_Dark_svg, 
                                                            BinaryData::Time_Display_Dark_svgSize);

    previousSessionsSvgCache = juce::Drawable::createFromImageData(BinaryData::Previous_Sessions_Dark_svg, 
                                                                 BinaryData::Previous_Sessions_Dark_svgSize);
    headerSvgCache = juce::Drawable::createFromImageData(BinaryData::Header_Dark_svg,
                                                       BinaryData::Header_Dark_svgSize);
    playSvgCache = juce::Drawable::createFromImageData(BinaryData::Play_Button_Dark_svg, 
                                                      BinaryData::Play_Button_Dark_svgSize);
    playPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Play_Button_Pressed_Dark_svg, 
                                                            BinaryData::Play_Button_Pressed_Dark_svgSize);
    pauseSvgCache = juce::Drawable::createFromImageData(BinaryData::Pause_Button_Dark_svg, 
                                                      BinaryData::Pause_Button_Dark_svgSize);
    pausePressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Pause_Button_Pressed_Dark_svg, 
                                                             BinaryData::Pause_Button_Pressed_Dark_svgSize);

    // Cache Light Mode variants
    backgroundLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Background_Light_svg, 
                                                           BinaryData::Background_Light_svgSize);
    timeDisplayLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Time_Display_Light_svg, 
                                                            BinaryData::Time_Display_Light_svgSize);
    previousSessionsLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Previous_Sessions_Light_svg, 
                                                                 BinaryData::Previous_Sessions_Light_svgSize);
    headerLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Header_Light_svg,
                                                       BinaryData::Header_Light_svgSize);
    playLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Play_Button_Light_svg, 
                                                      BinaryData::Play_Button_Light_svgSize);
    playPressedLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Play_Button_Pressed_Light_svg, 
                                                            BinaryData::Play_Button_Pressed_Light_svgSize);
    pauseLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Pause_Button_Light_svg, 
                                                      BinaryData::Pause_Button_Light_svgSize);
    pausePressedLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Pause_Button_Pressed_Light_svg, 
                                                             BinaryData::Pause_Button_Pressed_Light_svgSize);

    // Normalize all button-related SVGs before initializing buttons
    playSvgCache = createNormalizedDrawable(playSvgCache.get(), targetButtonSize);
    playPressedSvgCache = createNormalizedDrawable(playPressedSvgCache.get(), targetButtonSize * 0.95f);
    pauseSvgCache = createNormalizedDrawable(pauseSvgCache.get(), targetButtonSize);
    pausePressedSvgCache = createNormalizedDrawable(pausePressedSvgCache.get(), targetButtonSize * 0.95f);

    playLightSvgCache = createNormalizedDrawable(playLightSvgCache.get(), targetButtonSize);
    playPressedLightSvgCache = createNormalizedDrawable(playPressedLightSvgCache.get(), targetButtonSize * 0.95f);
    pauseLightSvgCache = createNormalizedDrawable(pauseLightSvgCache.get(), targetButtonSize);
    pausePressedLightSvgCache = createNormalizedDrawable(pausePressedLightSvgCache.get(), targetButtonSize * 0.95f);

    // THEN initialize play/pause button
    playPauseButton.setButtonText("");
    addAndMakeVisible(playPauseButton);
    playPauseButton.setClickingTogglesState(true);  // Enable toggling

    // Make button background transparent
    playPauseButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    playPauseButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Add click handler for play/pause button
    playPauseButton.onClick = [this]() {
        if (audioProcessor.isTracking()) {
            audioProcessor.stopTracking();
        } else {
            audioProcessor.startTracking();
        }
        updateButtonImages();
    };

    // Set initial toggle state and images
    playPauseButton.setToggleState(audioProcessor.isTracking(), juce::dontSendNotification);
    updateButtonImages();
    
    // Initialize theme toggle button
    themeToggleButton.setButtonText("");
    themeToggleButton.setClickingTogglesState(true);
    themeToggleButton.setToggleState(!audioProcessor.isDarkMode(), juce::dontSendNotification);
    addAndMakeVisible(themeToggleButton);

    // Make button background transparent
    themeToggleButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    themeToggleButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // First load SVGs
    darkModeSvgCache = juce::Drawable::createFromImageData(BinaryData::Dark_Mode_Button_svg, 
                                                        BinaryData::Dark_Mode_Button_svgSize);

    darkModePressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Dark_Mode_Button_Pressed_svg, 
                                                                 BinaryData::Dark_Mode_Button_Pressed_svgSize);

    lightModeSvgCache = juce::Drawable::createFromImageData(BinaryData::Light_Mode_Button_svg, 
                                                         BinaryData::Light_Mode_Button_svgSize);

    lightModePressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Light_Mode_Button_Pressed_svg, 
                                                                BinaryData::Light_Mode_Button_Pressed_svgSize);

    // Then normalize them
    if (darkModeSvgCache != nullptr)
        darkModeSvgCache = createNormalizedDrawable(darkModeSvgCache.get(), targetButtonSize);
    if (darkModePressedSvgCache != nullptr)
        darkModePressedSvgCache = createNormalizedDrawable(darkModePressedSvgCache.get(), targetButtonSize * 0.95f);
    if (lightModeSvgCache != nullptr)
        lightModeSvgCache = createNormalizedDrawable(lightModeSvgCache.get(), targetButtonSize);
    if (lightModePressedSvgCache != nullptr)
        lightModePressedSvgCache = createNormalizedDrawable(lightModePressedSvgCache.get(), targetButtonSize * 0.95f);

    // Initialize the button
    themeToggleButton.setButtonText("");
    themeToggleButton.setClickingTogglesState(true);
    themeToggleButton.setToggleState(!audioProcessor.isDarkMode(), juce::dontSendNotification);
    addAndMakeVisible(themeToggleButton);

    // Make button background transparent
    themeToggleButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    themeToggleButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Set initial images
    bool isDark = audioProcessor.isDarkMode();

    themeToggleButton.setImages(
        // Show opposite mode icon (light icon in dark mode, dark icon in light mode)
        !isDark ? darkModeSvgCache.get() : lightModeSvgCache.get(),           // normal
        nullptr,                                                               // over
        !isDark ? darkModePressedSvgCache.get() : lightModePressedSvgCache.get(), // down
        nullptr,                                                               // disabled
        !isDark ? darkModeSvgCache.get() : lightModeSvgCache.get(),           // normal (on)
        nullptr,                                                               // over (on)
        !isDark ? darkModePressedSvgCache.get() : lightModePressedSvgCache.get(), // down (on)
        nullptr                                                                // disabled (on) (use normal)
    );

    themeToggleButton.onClick = [this]() {
        bool newState = !audioProcessor.isDarkMode();
        audioProcessor.setDarkMode(newState);
        updateThemeButtonImages();
        updateButtonImages();
        updateSortButtonImages();
        updateScrollButtonImages();
        updateScrollButtonStates();
        repaint();
    };

    // Set a fixed size for our editor
    setSize(600, 450);
    
    // Start the timer for updates
    startTimerHz(1);

    sortModeButton.setVisible(true);
    sortModeButton.setButtonText("");
    sortModeButton.setClickingTogglesState(true);
    addAndMakeVisible(sortModeButton);

    // Make button background transparent
    sortModeButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    sortModeButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Add the click handler
    sortModeButton.onClick = [this]() {
        auto currentMode = audioProcessor.getDateSortMode();
        auto newMode = (currentMode == KronosAudioProcessor::DateSortMode::MostRecent) ?
            KronosAudioProcessor::DateSortMode::MostTime :
            KronosAudioProcessor::DateSortMode::MostRecent;
        audioProcessor.setDateSortMode(newMode);
        updateSortButtonImages();
        updateDateLabels();
    };

    // Initialize scroll buttons with the smallest triangles
    addAndMakeVisible(scrollUpButton);
    addAndMakeVisible(scrollDownButton);

    // Make button backgrounds transparent
    scrollUpButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    scrollUpButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);
    scrollDownButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    scrollDownButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Add the unit labels
    addAndMakeVisible(hourUnitLabel);
    addAndMakeVisible(minuteUnitLabel);
    addAndMakeVisible(secondUnitLabel);

    hourUnitLabel.setFont(asteraFontSmall.withHeight(32.0f));
    minuteUnitLabel.setFont(asteraFontSmall.withHeight(32.0f));
    secondUnitLabel.setFont(asteraFontSmall.withHeight(32.0f));

    juce::Colour labelColor(0xE6, 0xE6, 0xFF);  // #E6E6FF
    hourUnitLabel.setColour(juce::Label::textColourId, labelColor);
    minuteUnitLabel.setColour(juce::Label::textColourId, labelColor);
    secondUnitLabel.setColour(juce::Label::textColourId, labelColor);

    hourUnitLabel.setJustificationType(juce::Justification::centred);
    minuteUnitLabel.setJustificationType(juce::Justification::centred);
    secondUnitLabel.setJustificationType(juce::Justification::centred);

    hourUnitLabel.setText("HOURS", juce::dontSendNotification);
    minuteUnitLabel.setText("MINUTES", juce::dontSendNotification);
    secondUnitLabel.setText("SECONDS", juce::dontSendNotification);

    juce::Colour textColor(0xE6, 0xE6, 0xFF);  // Slightly blue-tinted white (#E6E6FF)
    
    hoursLabel.setColour(juce::Label::textColourId, textColor);
    minutesLabel.setColour(juce::Label::textColourId, textColor);
    secondsLabel.setColour(juce::Label::textColourId, textColor);

    // Apply glowing effect to unit labels
    hourUnitLabel.setLookAndFeel(&customLookAndFeel.glowingLabelLookAndFeel);
    minuteUnitLabel.setLookAndFeel(&customLookAndFeel.glowingLabelLookAndFeel);
    secondUnitLabel.setLookAndFeel(&customLookAndFeel.glowingLabelLookAndFeel);

    hoursLabel.setName("TimeLabel");
    minutesLabel.setName("TimeLabel");
    secondsLabel.setName("TimeLabel");

    // Set names for date labels
    for (auto& label : dateLabels)
    {
        label.setName("DateLabel");
    }

    menuButton.setButtonText("...");
    addAndMakeVisible(menuButton);
    menuButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    menuButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xE6, 0xE6, 0xFF));  // Match other text
    menuButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xE6, 0xE6, 0xFF));   // Match when pressed
    menuButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);    // Remove border
    menuButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);  // Remove press color
    menuButton.onClick = [this]() {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&customLookAndFeel);
        menu.addItem(1, "About");
        menu.addItem(2, "Backup");
        menu.addItem(3, "Restore");
        
        // Set minimum width for the popup menu so text doesn't get cut off
        juce::PopupMenu::Options options = juce::PopupMenu::Options()
            .withMinimumWidth(112)  // Set a minimum width in pixels
            .withMaximumNumColumns(1)
            .withTargetComponent(&menuButton);
            
        menu.showMenuAsync(options,
            [this](int result) {
                if (result == 1)
                {
                    aboutComponent = std::make_unique<AboutComponent>(audioProcessor);
                    addAndMakeVisible(aboutComponent.get());
                    aboutComponent->setBounds(getLocalBounds());
                    aboutComponent->setVisible(true);
                    aboutComponent->toFront(true);
                    
                    // Set up the close button handler
                    aboutComponent->closeButton.onClick = [this]() {
                        aboutComponent->setVisible(false);
                    };
                }
                else if (result == 2) // Backup
                {
                    backupComponent = std::make_unique<BackupComponent>(audioProcessor);
                    addAndMakeVisible(backupComponent.get());
                    backupComponent->setBounds(getLocalBounds());
                    backupComponent->setVisible(true);
                    backupComponent->toFront(true);
                    
                    // Set up the close button handler
                    backupComponent->closeButton.onClick = [this]() {
                        backupComponent->setVisible(false);
                    };
                }
                else if (result == 3) // Restore
                {
                    restoreComponent = std::make_unique<RestoreComponent>(audioProcessor);
                    addAndMakeVisible(restoreComponent.get());
                    restoreComponent->setBounds(getLocalBounds());
                    restoreComponent->setVisible(true);
                    restoreComponent->toFront(true);
                    
                    // Set up the close button handler
                    restoreComponent->closeButton.onClick = [this]() {
                        restoreComponent->setVisible(false);
                    };
                }
            });
    };

    // Load Sort button SVGs
    sortTimeDarkSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Time_Dark_svg, 
                                                       BinaryData::Sort_Time_Dark_svgSize);

    // Initialize the button
    addAndMakeVisible(sortModeButton);
    sortModeButton.setVisible(true);
    sortModeButton.setButtonText("");
    sortModeButton.setClickingTogglesState(true);

    // Make button background transparent
    sortModeButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    sortModeButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Time Sort SVGs
    sortTimeDarkSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Time_Dark_svg, 
                                                         BinaryData::Sort_Time_Dark_svgSize);

    sortTimeLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Time_Light_svg, 
                                                          BinaryData::Sort_Time_Light_svgSize);

    sortTimeDarkPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Time_Pressed_Dark_svg, 
                                                               BinaryData::Sort_Time_Pressed_Dark_svgSize);

    sortTimeLightPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Time_Pressed_Light_svg, 
                                                                BinaryData::Sort_Time_Pressed_Light_svgSize);

    // Recency Sort SVGs
    sortRecencyDarkSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Recency_Dark_svg, 
                                                           BinaryData::Sort_Recency_Dark_svgSize);

    sortRecencyLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Recency_Light_svg, 
                                                            BinaryData::Sort_Recency_Light_svgSize);

    sortRecencyDarkPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Recency_Pressed_Dark_svg, 
                                                                  BinaryData::Sort_Recency_Pressed_Dark_svgSize);

    sortRecencyLightPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Sort_Recency_Pressed_Light_svg, 
                                                                   BinaryData::Sort_Recency_Pressed_Light_svgSize);
    // Normalize all loaded SVGs
    if (sortTimeDarkSvgCache != nullptr)
        sortTimeDarkSvgCache = createNormalizedDrawable(sortTimeDarkSvgCache.get(), targetButtonSize);
    if (sortTimeLightSvgCache != nullptr)
        sortTimeLightSvgCache = createNormalizedDrawable(sortTimeLightSvgCache.get(), targetButtonSize);
    if (sortTimeDarkPressedSvgCache != nullptr)
        sortTimeDarkPressedSvgCache = createNormalizedDrawable(sortTimeDarkPressedSvgCache.get(), targetButtonSize * 0.95f);
    if (sortTimeLightPressedSvgCache != nullptr)
        sortTimeLightPressedSvgCache = createNormalizedDrawable(sortTimeLightPressedSvgCache.get(), targetButtonSize * 0.95f);
    if (sortRecencyDarkSvgCache != nullptr)
        sortRecencyDarkSvgCache = createNormalizedDrawable(sortRecencyDarkSvgCache.get(), targetButtonSize);
    if (sortRecencyLightSvgCache != nullptr)
        sortRecencyLightSvgCache = createNormalizedDrawable(sortRecencyLightSvgCache.get(), targetButtonSize);
    if (sortRecencyDarkPressedSvgCache != nullptr)
        sortRecencyDarkPressedSvgCache = createNormalizedDrawable(sortRecencyDarkPressedSvgCache.get(), targetButtonSize * 0.95f);
    if (sortRecencyLightPressedSvgCache != nullptr)
        sortRecencyLightPressedSvgCache = createNormalizedDrawable(sortRecencyLightPressedSvgCache.get(), targetButtonSize * 0.95f);

    // Initialize the button
    sortModeButton.setVisible(true);
    sortModeButton.setButtonText("");
    sortModeButton.setClickingTogglesState(true);
    addAndMakeVisible(sortModeButton);

    // Make button background transparent (changed from red)
    sortModeButton.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    sortModeButton.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);

    // Load arrow SVGs
    upArrowDarkSvgCache = juce::Drawable::createFromImageData(BinaryData::Up_Arrow_Dark_svg, 
                                                           BinaryData::Up_Arrow_Dark_svgSize);
    upArrowDarkPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Up_Arrow_Pressed_Dark_svg, 
                                                                  BinaryData::Up_Arrow_Pressed_Dark_svgSize);
    upArrowLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Up_Arrow_Light_svg, 
                                                            BinaryData::Up_Arrow_Light_svgSize);
    upArrowLightPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Up_Arrow_Pressed_Light_svg, 
                                                                     BinaryData::Up_Arrow_Pressed_Light_svgSize);
    downArrowDarkSvgCache = juce::Drawable::createFromImageData(BinaryData::Down_Arrow_Dark_svg, 
                                                             BinaryData::Down_Arrow_Dark_svgSize);
    downArrowDarkPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Down_Arrow_Pressed_Dark_svg, 
                                                                    BinaryData::Down_Arrow_Pressed_Dark_svgSize);
    downArrowLightSvgCache = juce::Drawable::createFromImageData(BinaryData::Down_Arrow_Light_svg, 
                                                                BinaryData::Down_Arrow_Light_svgSize);
    downArrowLightPressedSvgCache = juce::Drawable::createFromImageData(BinaryData::Down_Arrow_Pressed_Light_svg, 
                                                                       BinaryData::Down_Arrow_Pressed_Light_svgSize);

    // Normalize arrow SVGs
    if (upArrowDarkSvgCache != nullptr)
        upArrowDarkSvgCache = createNormalizedDrawable(upArrowDarkSvgCache.get(), targetButtonSize);
    if (upArrowDarkPressedSvgCache != nullptr)
        upArrowDarkPressedSvgCache = createNormalizedDrawable(upArrowDarkPressedSvgCache.get(), targetButtonSize * 0.95f);
    if (upArrowLightSvgCache != nullptr)
        upArrowLightSvgCache = createNormalizedDrawable(upArrowLightSvgCache.get(), targetButtonSize);
    if (upArrowLightPressedSvgCache != nullptr)
        upArrowLightPressedSvgCache = createNormalizedDrawable(upArrowLightPressedSvgCache.get(), targetButtonSize * 0.95f);
    if (downArrowDarkSvgCache != nullptr)
        downArrowDarkSvgCache = createNormalizedDrawable(downArrowDarkSvgCache.get(), targetButtonSize);
    if (downArrowDarkPressedSvgCache != nullptr)
        downArrowDarkPressedSvgCache = createNormalizedDrawable(downArrowDarkPressedSvgCache.get(), targetButtonSize * 0.95f);
    if (downArrowLightSvgCache != nullptr)
        downArrowLightSvgCache = createNormalizedDrawable(downArrowLightSvgCache.get(), targetButtonSize);
    if (downArrowLightPressedSvgCache != nullptr)
        downArrowLightPressedSvgCache = createNormalizedDrawable(downArrowLightPressedSvgCache.get(), targetButtonSize * 0.95f);

    // Initialize scroll position and update button states
    isAtTop = true;  // Explicitly set initial state
    isAtBottom = false;
    scrollOffset = 0.0f;
    constrainScrollOffset();  // This will update the button states

    // Initialize the buttons
    addAndMakeVisible(scrollUpButton);
    addAndMakeVisible(scrollDownButton);
    scrollUpButton.setVisible(true);
    scrollDownButton.setVisible(true);

    // Update scroll button images initially
    updateScrollButtonImages();
    updateScrollButtonStates();

    // After loading and normalizing the sort button SVGs in the constructor:
    auto& normalImage = isDark ? sortTimeDarkSvgCache : sortTimeLightSvgCache;
    auto& pressedImage = isDark ? sortTimeDarkPressedSvgCache : sortTimeLightPressedSvgCache;

    if (normalImage != nullptr && pressedImage != nullptr)
    {
        sortModeButton.setImages(
            normalImage.get(),          // normal
            normalImage.get(),          // over
            pressedImage.get(),         // down
            normalImage.get(),          // disabled
            normalImage.get(),          // normal (on)
            normalImage.get(),          // over (on)
            pressedImage.get(),         // down (on)
            normalImage.get()           // disabled (on)
        );
    }

    // Add click handlers for scroll buttons
    scrollUpButton.onClick = [this]() {
        if (!isAtTop) {
            scrollOffset = std::max(0.0f, scrollOffset - dateHeight);
            constrainScrollOffset();
            updateDateLabels();
            repaint();
        }
    };

    scrollDownButton.onClick = [this]() {
        if (!isAtBottom) {
            auto dates = audioProcessor.getSortedDates();
            float maxScroll = std::max(0.0f, (dates.size() - visibleDates) * dateHeight);
            scrollOffset = std::min(maxScroll, scrollOffset + dateHeight);
            constrainScrollOffset();
            updateDateLabels();
            repaint();
        }
    };

    // Add parameter listeners
    audioProcessor.parameters->addParameterListener("tracking", this);
    audioProcessor.parameters->addParameterListener("darkMode", this);
    audioProcessor.parameters->addParameterListener("dateSortMode", this);

    // Pre-process grit texture
    auto gritImage = juce::ImageCache::getFromMemory(BinaryData::Grit_jpg, BinaryData::Grit_jpgSize);
    if (gritImage.isValid())
    {
        gritTexture = gritImage.convertedToFormat(juce::Image::ARGB);
    }

    // Replace manual title image with styled label
    addAndMakeVisible(titleLabel);
    titleLabel.setText("KRONOS", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setLookAndFeel(&customLookAndFeel);
    titleLabel.setName("TitleLabel");  // Important for LookAndFeel recognition
    titleLabel.setComponentID("GlowingTitle");  // Enable glow effect
}

// Performance: O(1) - Simple timer callback with basic arithmetic
// CPU: Low - Called frequently (1Hz) but performs minimal work
void KronosAudioProcessorEditor::timerCallback()
{
    // Handle regular timer updates
    auto totalSeconds = audioProcessor.getTotalTimeInSeconds();
    auto hours = totalSeconds / 3600;
    auto minutes = (totalSeconds % 3600) / 60;
    auto seconds = totalSeconds % 60;
    
    // For the main display, show actual hours without capping
    hoursLabel.setText(juce::String::formatted("%02d", (int)hours), 
                  juce::dontSendNotification);
    minutesLabel.setText(juce::String::formatted("%02d", (int)minutes), 
                    juce::dontSendNotification);
    secondsLabel.setText(juce::String::formatted("%02d", (int)seconds), 
                    juce::dontSendNotification);
    
    // Get the bounds of the Previous Sessions panel
    auto bounds = getLocalBounds();
    auto bottomSection = bounds.removeFromBottom(160);
    bottomSection.removeFromTop(margin * 5); 
    bottomSection.removeFromBottom(margin);
    
    // Update date labels with scrolling
    auto dates = audioProcessor.getSortedDates();
    for (int i = 0; i < 3; ++i)
    {
        int dateIndex = i + (int)(scrollOffset / dateHeight);
        
        if (dateIndex < dates.size())
        {
            auto date = dates[dateIndex];
            
            // Calculate position within the Previous Sessions panel
            float y = bottomSection.getY() + (i * dateHeight);
            
            float barWidth = 70.0f;
            float startX = (getWidth() - barWidth) / 2.0f;
            dateLabels[i].setBounds(startX - 100, y, 100, dateHeight);
            dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - ", 
                                juce::dontSendNotification);
            
            // Only show label if it's within the Previous Sessions panel
            dateLabels[i].setVisible(y >= bottomSection.getY() && 
                                   y + dateHeight <= bottomSection.getBottom());
        }
        else
        {
            dateLabels[i].setVisible(false);
        }
    }

    // Force update of date labels and repaint
    updateDateLabels();
    repaint(previousSessionsBounds.toNearestInt());
}

// Performance: O(1) - UI layout calculations
// CPU: Low-Moderate - Called on window resize
// Note: Contains many floating point calculations
void KronosAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Calculate scale factors based on default size
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    scale = juce::jmin(widthScale, heightScale);  // Update the member variable
    
    // Scale margins and sizes
    auto scaledMargin = margin * scale;
    auto scaledButtonHeight = 60 * scale;
    auto scaledDateHeight = 25 * scale;
    auto scaledTitleHeight = 80 * scale;
    auto scaledPlayPauseWidth = 60 * scale;
    auto scaledPreviousSessionsHeight = 160 * scale;
    auto scaledSortButtonSize = 50 * scale;
    auto scaledTimeWidth = 400.0f * scale;
    
    // Store original bottomSection for later use
    auto originalBottomSection = bounds.removeFromBottom(scaledPreviousSessionsHeight);

    // Title area
    bounds.removeFromTop(scaledTitleHeight);
    bounds.removeFromTop(scaledMargin * 3);

    // Time display area - store the bounds
    timeDisplayBounds = bounds.removeFromTop(scaledButtonHeight * 2.5);

    // Move to bottom section for Previous Sessions panel
    auto bottomSection = bounds.removeFromBottom(scaledPreviousSessionsHeight);
    bottomSection.removeFromTop(scaledMargin * 5);
    bottomSection.removeFromBottom(scaledMargin);

    // Position date labels in the bottom section
    for (int i = 0; i < 3; ++i)
    {
        dateLabels[i].setBounds(bottomSection.removeFromTop(scaledDateHeight));
    }

    // Calculate center positions
    auto centerX = getWidth() / 2;
    
    // Position play button halfway between window edge and time display
    auto timeDisplayLeft = centerX - (scaledTimeWidth / 2);
    auto playButtonX = timeDisplayLeft / 2 - (scaledPlayPauseWidth / 2);
    auto playButtonY = timeDisplayBounds.getCentreY() - (scaledButtonHeight / 3.3);
    playPauseButton.setBounds(playButtonX, playButtonY, scaledPlayPauseWidth, scaledButtonHeight);

    // Center time labels
    auto timeLabelWidth = scaledTimeWidth - scaledMargin * 2;
    auto timeLabelHeight = scaledButtonHeight * 2;
    auto labelWidth = timeLabelWidth / 3;
    auto labelHeight = timeLabelHeight;

    // Scale fonts
    float scaledLargeFont = 36.0f * scale;
    float scaledSmallFont = 16.0f * scale;
    float scaledUnitFont = 36.0f * scale;  // Increased to match constructor
    
    auto asteraFontLarge = juce::Font("ASTERA", scaledLargeFont, juce::Font::plain);
    auto asteraFontSmall = juce::Font("ASTERA", scaledSmallFont, juce::Font::plain);
    auto asteraFontUnit = juce::Font("ASTERA", scaledUnitFont, juce::Font::plain);
    
    // Update fonts for all labels
    hoursLabel.setFont(asteraFontLarge);
    minutesLabel.setFont(asteraFontLarge);
    secondsLabel.setFont(asteraFontLarge);
    
    hourUnitLabel.setFont(asteraFontUnit);
    minuteUnitLabel.setFont(asteraFontUnit);
    secondUnitLabel.setFont(asteraFontUnit);
    
    for (auto& label : dateLabels)
    {
        label.setFont(asteraFontSmall);
    }

    // Position the time labels
    hoursLabel.setBounds(centerX - timeLabelWidth/2 - 2.5 * scale,
                        timeDisplayBounds.getCentreY() - (labelHeight / 2),
                        labelWidth,
                        labelHeight);

    minutesLabel.setBounds(centerX - labelWidth/2,
                          timeDisplayBounds.getCentreY() - (labelHeight / 2),
                          labelWidth,
                          labelHeight);

    secondsLabel.setBounds(centerX + timeLabelWidth/2 - labelWidth + 2.5 * scale,
                          timeDisplayBounds.getCentreY() - (labelHeight / 2),
                          labelWidth,
                          labelHeight);

    // Position unit labels
    float unitLabelHeight = 37.5f * scale;
    float unitLabelOffset = -8.0f * scale;

    hourUnitLabel.setBounds(hoursLabel.getX() - 2 * scale,
                           hoursLabel.getY() - unitLabelHeight - unitLabelOffset,
                           hoursLabel.getWidth(),
                           unitLabelHeight);

    minuteUnitLabel.setBounds(minutesLabel.getX(),
                             minutesLabel.getY() - unitLabelHeight - unitLabelOffset,
                             minutesLabel.getWidth(),
                             unitLabelHeight);

    secondUnitLabel.setBounds(secondsLabel.getX() + 2 * scale,
                             secondsLabel.getY() - unitLabelHeight - unitLabelOffset,
                             secondsLabel.getWidth(),
                             unitLabelHeight);

    // Position theme toggle button
    auto scaledThemeButtonSize = 50 * scale;
    themeToggleButton.setBounds(getWidth() - scaledThemeButtonSize - scaledMargin,
                               getHeight() - scaledThemeButtonSize - scaledMargin,
                               scaledThemeButtonSize, scaledThemeButtonSize);

    // Position sort button
    sortModeButton.setBounds(
        scaledMargin + 90 * scale, 
        originalBottomSection.getCentreY() - (scaledSortButtonSize / 2),
        scaledSortButtonSize,
        scaledSortButtonSize
    );

    // Position scroll buttons
    int scaledScrollButtonSize = 25 * scale;
    int buttonX = getWidth() - scaledScrollButtonSize - (scaledMargin * 12.5);  
    int buttonsY = getHeight() - scaledPreviousSessionsHeight + (scaledMargin * 6);
    
    scrollUpButton.setBounds(buttonX, buttonsY, scaledScrollButtonSize, scaledScrollButtonSize);
    scrollDownButton.setBounds(buttonX, buttonsY + scaledScrollButtonSize + 5 * scale, 
                             scaledScrollButtonSize, scaledScrollButtonSize);

    // Position menu button
    auto scaledMenuButtonSize = 30 * scale;
    menuButton.setBounds(getWidth() - scaledMenuButtonSize - 10 * scale,
                        0.5f * scale,
                        scaledMenuButtonSize, 
                        scaledMenuButtonSize);

    // Cache bounds for paint()
    previousSessionsBounds = juce::Rectangle<float>(
        getWidth()/2.0f - 150.0f * scale,
        getHeight() - 140.0f * scale - 10.0f * scale,
        300.0f * scale,
        140.0f * scale
    );

    headerBounds = juce::Rectangle<float>(
        (getWidth() - 400.0f * scale)/2.0f,
        10.0f * scale,
        400.0f * scale,
        60.0f * scale
    );

    // Position title label using LookAndFeel
    titleLabel.setBounds(
        getWidth()/2 - 150 * scale,
        static_cast<int>(17 * scale),
        300 * scale,
        50 * scale
    );

    // Update AboutComponent bounds if it exists and is visible
    if (aboutComponent != nullptr && aboutComponent->isVisible())
    {
        aboutComponent->setBounds(getLocalBounds());
    }
    
    // Update BackupComponent bounds if it exists and is visible
    if (backupComponent != nullptr && backupComponent->isVisible())
    {
        backupComponent->setBounds(getLocalBounds());
    }
    
    // Update RestoreComponent bounds if it exists and is visible
    if (restoreComponent != nullptr && restoreComponent->isVisible())
    {
        restoreComponent->setBounds(getLocalBounds());
    }
}

// Performance: O(1) - Simple cleanup operations
// Memory: Frees cached SVGs and UI components
KronosAudioProcessorEditor::~KronosAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
    playPauseButton.onClick = nullptr;
    hoursLabel.setLookAndFeel(nullptr);
    minutesLabel.setLookAndFeel(nullptr);
    secondsLabel.setLookAndFeel(nullptr);
    hourUnitLabel.setLookAndFeel(nullptr);
    minuteUnitLabel.setLookAndFeel(nullptr);
    secondUnitLabel.setLookAndFeel(nullptr);
    menuButton.setLookAndFeel(nullptr);

    // Ensure backup and restore components are properly cleaned up
    if (backupComponent != nullptr)
    {
        backupComponent->setVisible(false);
        removeChildComponent(backupComponent.get());
    }
    
    if (restoreComponent != nullptr)
    {
        restoreComponent->setVisible(false);
        removeChildComponent(restoreComponent.get());
    }

    // Remove parameter listeners
    audioProcessor.parameters->removeParameterListener("tracking", this);
    audioProcessor.parameters->removeParameterListener("darkMode", this);
    audioProcessor.parameters->removeParameterListener("dateSortMode", this);
}

// Performance: O(n) where n is number of UI elements to draw
// CPU: High - Called frequently for UI updates
// GPU: Moderate - SVG rendering and gradient operations
// Note: Try to optimize to O(log n)
void KronosAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Paint duration enter
    auto paintStart = juce::Time::getMillisecondCounterHiRes();

    // Pre-calculate frequently used values
    const auto& processor = audioProcessor;
    const bool isDarkMode = processor.isDarkMode();
    const auto bounds = getLocalBounds().toFloat();
    const float borderThickness = 4.0f * scale;
    
    // Cache background drawing
    if (auto* bg = isDarkMode ? backgroundSvgCache.get() : backgroundLightSvgCache.get())
    {
        bg->drawWithin(g, bounds, juce::RectanglePlacement::stretchToFit, 1.0f);
    }

    // Restore metallic border gradient
    juce::ColourGradient borderGradient(
        isDarkMode ? juce::Colour(130, 130, 130) : juce::Colour(160, 140, 120),
        bounds.getBottomLeft(),
        isDarkMode ? juce::Colour(130, 130, 130) : juce::Colour(160, 140, 120),
        bounds.getTopRight(),
        false
    );
    
    borderGradient.addColour(0.25, isDarkMode ? juce::Colour(40, 40, 40) : juce::Colour(100, 85, 70));
    borderGradient.addColour(0.5, isDarkMode ? juce::Colour(40, 40, 40) : juce::Colour(100, 85, 70));
    borderGradient.addColour(0.75, isDarkMode ? juce::Colour(130, 130, 130) : juce::Colour(160, 140, 120));
    
    g.setGradientFill(borderGradient);
    g.drawRoundedRectangle(bounds.reduced(borderThickness * 0.5f), 2.0f, borderThickness);


    // Draw grit texture overlay with platform-specific handling
    if (gritTexture.isValid())
    {
        auto gritBounds = bounds.reduced(borderThickness + 1.0f);
        
        #if JUCE_WINDOWS
            g.setOpacity(0.07f);
            g.drawImageWithin(gritTexture,
                            gritBounds.getX(),
                            gritBounds.getY(),
                            gritBounds.getWidth(),
                            gritBounds.getHeight(),
                            juce::RectanglePlacement::stretchToFit,
                            false);
            g.setOpacity(1.0f);
        #else
            // Original Mac handling
            juce::Image gritCopy = gritTexture.createCopy();
            gritCopy.multiplyAllAlphas(0.07f);
            g.drawImage(gritCopy,
                       gritBounds.getX(),
                       gritBounds.getY(),
                       gritBounds.getWidth(),
                       gritBounds.getHeight(),
                       0,
                       0,
                       gritTexture.getWidth(),
                       gritTexture.getHeight());
        #endif
    }

    // Fixed time display scaling with solid background
    if (auto* timeDisplay = isDarkMode ? timeDisplaySvgCache.get() : timeDisplayLightSvgCache.get())
    {
        // Calculate the desired bounds for the time display
        const float desiredWidth = 400.0f * scale;
        const float desiredHeight = 200.0f * scale;
        const auto timeDisplayArea = juce::Rectangle<float>(
            (getWidth() - desiredWidth) / 2.0f,
            timeDisplayBounds.getCentreY() - desiredHeight/2,
            desiredWidth,
            desiredHeight
        );

        // Ensure solid opacity
        timeDisplay->drawWithin(g, timeDisplayArea, 
            juce::RectanglePlacement::stretchToFit, 
            1.0f);
        g.setOpacity(1.0f);
    }
    
    // Cached SVG drawing - fix theme handling
    if (auto* previousSessions = audioProcessor.isDarkMode() ? 
        previousSessionsSvgCache.get() : previousSessionsLightSvgCache.get())
    {
        previousSessions->drawWithin(g, previousSessionsBounds,
                                   juce::RectanglePlacement::centred, 1.0f);
    }

    if (auto* header = audioProcessor.isDarkMode() ? 
        headerSvgCache.get() : headerLightSvgCache.get())
    {
        header->drawWithin(g, headerBounds,
                         juce::RectanglePlacement::centred, 1.0f);
    }

    // Optimized time bars using batch rendering
    drawTimeBars(g);

    // paint duration exit
    DBG("Paint duration: " << (juce::Time::getMillisecondCounterHiRes() - paintStart) << " ms");
}

// Performance: O(1) - Simple text update
// CPU: Very Low
void KronosAudioProcessorEditor::updateSortButtonText()
{
    if (audioProcessor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostRecent)
        sortModeButton.setButtonText("Sort by Time");
    else
        sortModeButton.setButtonText("Sort by Date");
}

// Performance: O(n) where n is number of visible date labels
// CPU: Low - Simple text formatting and UI updates
void KronosAudioProcessorEditor::updateDateLabels()
{
    auto dates = audioProcessor.getSortedDates();
    int firstVisibleIndex = static_cast<int>(scrollOffset / dateHeight);
    
    for (int i = 0; i < 3; ++i)
    {
        int dateIndex = i + firstVisibleIndex;
        if (dateIndex < dates.size())
        {
            auto date = dates[dateIndex];
            auto timeSpent = audioProcessor.getTimeForDate(date);
            
            dateLabels[i].setVisible(true);
            auto hours = timeSpent / 3600;
            auto minutes = (timeSpent % 3600) / 60;
            auto seconds = timeSpent % 60;
            
            juce::String timeStr = juce::String::formatted("%02d:%02d:%02d", 
                                                          (int)hours, 
                                                          (int)minutes, 
                                                          (int)seconds);
            
            dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - " + timeStr, 
                                 juce::dontSendNotification);
        }
        else
        {
            dateLabels[i].setVisible(false);
        }
    }
    repaint();
}

// Performance: O(1) - Simple floating point division
// CPU: Very Low
float KronosAudioProcessorEditor::getTimeRatio(juce::int64 time, juce::int64 maxTime) const
{
    if (maxTime == 0) return 0.0f;
    return static_cast<float>(time) / static_cast<float>(maxTime);
}

// Performance: O(n) where n is number of visible time bars
// CPU: Moderate - Multiple drawing operations and text formatting
// GPU: Moderate - Gradient fills and text rendering
// Note: Try to optimize to O(log n)
void KronosAudioProcessorEditor::drawTimeBars(juce::Graphics& g)
{
    auto dates = audioProcessor.getSortedDates();
    if (dates.isEmpty()) return;

    // Find maximum time and debug output
    juce::int64 maxTime = 0;
    for (const auto& date : dates)
    {
        auto timeSpent = audioProcessor.getTimeForDate(date);
        maxTime = juce::jmax(maxTime, timeSpent);
    }

    // Get the bounds of the Previous Sessions panel
    auto bounds = getLocalBounds();
    auto bottomSection = bounds.removeFromBottom(160 * scale);
    bottomSection.removeFromTop(margin * 5 * scale);
    bottomSection.removeFromBottom(margin * scale);

    // Draw bars for visible dates
    for (int i = 0; i < 3; ++i)
    {
        int dateIndex = i + static_cast<int>(scrollOffset / dateHeight);
        
        if (dateIndex < dates.size())
        {
            auto date = dates[dateIndex];
            auto timeSpent = audioProcessor.getTimeForDate(date);
            float ratio = getTimeRatio(timeSpent, maxTime);
            
            // Calculate positions with scaling
            float barHeight = 16.0f * scale;
            float maxBarWidth = 100.0f * scale;
            float dateWidth = 120.0f * scale;
            float dashWidth = 20.0f * scale;
            float totalWidth = dateWidth + dashWidth + maxBarWidth;
            float startX = (getWidth() - totalWidth) / 2.0f - (7.0f * scale);
            
            float y = bottomSection.getY() + (i * dateHeight * scale);
            
            if (y >= bottomSection.getY() && y + barHeight <= bottomSection.getBottom())
            {
                // Position date label and set text (just the date)
                dateLabels[i].setBounds(startX, y, dateWidth + dashWidth, dateHeight * scale);
                dateLabels[i].setText(date.formatted("%m-%d-%Y") + " - ", juce::dontSendNotification);
                dateLabels[i].setVisible(true);
                
                // Draw bar background
                float barX = startX + dateWidth + dashWidth;
                float barY = y + (dateHeight * scale - barHeight) / 2;
                
                // Draw background bar (dark grey)
                g.setColour(juce::Colour(40, 40, 40));
                g.fillRoundedRectangle(barX, barY, maxBarWidth, barHeight, 3.0f * scale);
                
                // Calculate filled width and ensure it's properly scaled
                float filledWidth = maxBarWidth * ratio;
                
                // Only apply minimum width if there's actual time
                if (timeSpent > 0) {
                    filledWidth = juce::jmax(2.0f * scale, filledWidth);
                }
                
                // Draw the filled portion
                if (audioProcessor.isDarkMode()) {
                    g.setColour(juce::Colour(64, 64, 255));  // Blue for dark mode
                } else {
                    g.setColour(juce::Colour(255, 140, 0));  // Orange for light mode
                }
                
                g.fillRoundedRectangle(barX, barY, filledWidth, barHeight, 3.0f * scale);

                // Format time text for the bar
                juce::String timeStr;
                if (timeSpent >= 360000) // Over 99:59:59
                {
                    auto displayHours = timeSpent / 3600;
                    timeStr = juce::String::formatted("%d:00:00", (int)displayHours);
                }
                else
                {
                    auto hours = timeSpent / 3600;
                    auto minutes = (timeSpent % 3600) / 60;
                    auto seconds = timeSpent % 60;
                    timeStr = juce::String::formatted("%02d:%02d:%02d", 
                                                    (int)hours, 
                                                    (int)minutes, 
                                                    (int)seconds);
                }
                
                // Set up text style with scaling
                g.setFont(juce::Font("ASTERA", 14.0f * scale, juce::Font::plain));
                float textY = y + (dateHeight * scale - barHeight) / 2 + (2.0f * scale);
                auto textBounds = juce::Rectangle<float>(barX, textY, maxBarWidth, barHeight);

                // Draw stroke (shadow) effect
                g.setColour(juce::Colours::black);
                float strokeSize = 0.8f * scale;
                float positions[][2] = {
                    {-strokeSize, -strokeSize},
                    {-strokeSize, strokeSize},
                    {strokeSize, -strokeSize},
                    {strokeSize, strokeSize},
                    {0, strokeSize},
                    {0, -strokeSize},
                    {strokeSize, 0},
                    {-strokeSize, 0}
                };

                for (auto& pos : positions)
                {
                    auto offsetBounds = textBounds.translated(pos[0], pos[1]);
                    g.drawText(timeStr, offsetBounds.toNearestInt(), 
                             juce::Justification::centred, true);
                }

                // Draw main text
                g.setColour(juce::Colour(0xE6, 0xE6, 0xFF));  // Light blue-white color
                g.drawText(timeStr, textBounds.toNearestInt(), 
                         juce::Justification::centred, true);
            }
            else
            {
                dateLabels[i].setVisible(false);
            }
        }
        else
        {
            dateLabels[i].setVisible(false);
        }
    }
}

// Performance: O(1) - Simple bounds checking and state updates
// CPU: Very Low
void KronosAudioProcessorEditor::constrainScrollOffset()
{
    auto dates = audioProcessor.getSortedDates();
    float maxScroll = juce::jmax(0.0f, (dates.size() - visibleDates) * dateHeight);
    scrollOffset = juce::jlimit(0.0f, maxScroll, scrollOffset);
    
    // Update scroll position states
    isAtTop = (scrollOffset <= 0.0f);
    isAtBottom = (scrollOffset >= maxScroll && dates.size() > visibleDates);
    
    // Update button appearances
    updateScrollButtonStates();
}

// Performance: O(1) - Simple event handler
// CPU: Very Low
void KronosAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    // This override is required as part of JUCE's Button::Listener interface.
    // However, we prefer to use button-specific callbacks (button.onClick) 
    // for clearer, more maintainable code and better encapsulation.
    // This also avoids the need to check which button was clicked.
    // DBG("Button clicked: " << button->getName());
}

// Performance: O(1) - Simple event handler
// CPU: Very Low
void KronosAudioProcessorEditor::mouseUp(const juce::MouseEvent& event)
{
    // This override is required as part of JUCE's MouseListener interface.
    // We prefer to use component-specific callbacks for better encapsulation
    // and clearer event handling. Mouse events are primarily handled through
    // individual component callbacks.
    // DBG("Mouse up event: " << event.eventComponent->getName());
}

// Performance: O(1) - Image asset switching
// CPU: Low - Simple pointer assignments
// Memory: Uses pre-cached images
void KronosAudioProcessorEditor::updateButtonImages()
{
    auto startTime = juce::Time::getMillisecondCounterHiRes();
    
    bool isDark = audioProcessor.isDarkMode();
    
    // Get the correct themed assets
    auto& currentPlay = isDark ? playSvgCache : playLightSvgCache;
    auto& currentPlayPressed = isDark ? playPressedSvgCache : playPressedLightSvgCache;
    auto& currentPause = isDark ? pauseSvgCache : pauseLightSvgCache;
    auto& currentPausePressed = isDark ? pausePressedSvgCache : pausePressedLightSvgCache;
    
    // Set the button images based on tracking state
    if (audioProcessor.isTracking())
    {
        playPauseButton.setImages(currentPause.get(),            // normal
                                nullptr,                         // over
                                currentPausePressed.get(),       // down
                                nullptr,                         // disabled
                                currentPause.get(),              // normal on
                                nullptr,                         // over on
                                currentPausePressed.get(),       // down on
                                nullptr);                        // disabled on
    }
    else
    {
        playPauseButton.setImages(currentPlay.get(),            // normal
                                nullptr,                         // over
                                currentPlayPressed.get(),        // down
                                nullptr,                         // disabled
                                currentPlay.get(),               // normal on
                                nullptr,                         // over on
                                currentPlayPressed.get(),        // down on
                                nullptr);                        // disabled on
    }

    auto endTime = juce::Time::getMillisecondCounterHiRes();
    DBG("Button image update time: " << (endTime - startTime) << "ms");
}

// Performance: O(1) - Simple drawable transformation
// CPU: Low - One-time transform calculation
// Memory: Creates new drawable instance
std::unique_ptr<juce::Drawable> KronosAudioProcessorEditor::createNormalizedDrawable(juce::Drawable* source, float targetSize)
{
    if (source == nullptr) return nullptr;
    
    std::unique_ptr<juce::Drawable> drawable(source->createCopy());
    auto bounds = drawable->getBounds();
    
    // Calculate scale to fit target size while maintaining aspect ratio
    float scale = targetSize / juce::jmax(bounds.getWidth(), bounds.getHeight());
    
    auto transform = juce::AffineTransform::scale(scale, scale);
    drawable->setTransform(transform);
    
    return drawable;
}

// Performance: O(1) - Simple image asset switching
// CPU: Low - Pointer assignments
// Memory: Uses pre-cached images
void KronosAudioProcessorEditor::updateThemeButtonImages()
{
    bool isDark = audioProcessor.isDarkMode();
    
    themeToggleButton.setImages(
        // Show opposite mode icon
        !isDark ? darkModeSvgCache.get() : lightModeSvgCache.get(),           // normal
        nullptr,                                                               // over
        !isDark ? darkModePressedSvgCache.get() : lightModePressedSvgCache.get(), // down
        nullptr,                                                               // disabled
        !isDark ? darkModeSvgCache.get() : lightModeSvgCache.get(),           // normal (on)
        nullptr,                                                               // over (on)
        !isDark ? darkModePressedSvgCache.get() : lightModePressedSvgCache.get(), // down (on)
        nullptr                                                                // disabled (on)
    );
}

// Performance: O(1) - Simple image asset switching
// CPU: Low - Pointer assignments
// Memory: Uses pre-cached images
void KronosAudioProcessorEditor::updateSortButtonImages()
{
    bool isDark = audioProcessor.isDarkMode();
    bool isTimeSort = audioProcessor.getDateSortMode() == KronosAudioProcessor::DateSortMode::MostTime;
    
    std::unique_ptr<juce::Drawable>& normalImage = isDark ? 
        (isTimeSort ? sortRecencyDarkSvgCache : sortTimeDarkSvgCache) :
        (isTimeSort ? sortRecencyLightSvgCache : sortTimeLightSvgCache);
    
    std::unique_ptr<juce::Drawable>& pressedImage = isDark ? 
        (isTimeSort ? sortRecencyDarkPressedSvgCache : sortTimeDarkPressedSvgCache) :
        (isTimeSort ? sortRecencyLightPressedSvgCache : sortTimeLightPressedSvgCache);
    
    if (normalImage != nullptr && pressedImage != nullptr)
    {
        sortModeButton.setImages(
            normalImage.get(),          // normal
            normalImage.get(),          // over (use normal)
            pressedImage.get(),         // down
            normalImage.get(),          // disabled (use normal)
            normalImage.get(),          // normal (on)
            normalImage.get(),          // over (on)
            pressedImage.get(),         // down (on)
            normalImage.get()           // disabled (on) (use normal)
        );
    }
    else
    {
        DBG("Failed to set images - one or more images are null");
    }
}

// Performance: O(1) - Simple image asset switching
// CPU: Low - Pointer assignments
// Memory: Uses pre-cached images
void KronosAudioProcessorEditor::updateScrollButtonImages()
{
    bool isDark = audioProcessor.isDarkMode();
    
    // Get the correct themed assets
    auto& currentUpArrow = isDark ? upArrowDarkSvgCache : upArrowLightSvgCache;
    auto& currentUpArrowPressed = isDark ? upArrowDarkPressedSvgCache : upArrowLightPressedSvgCache;
    auto& currentDownArrow = isDark ? downArrowDarkSvgCache : downArrowLightSvgCache;
    auto& currentDownArrowPressed = isDark ? downArrowDarkPressedSvgCache : downArrowLightPressedSvgCache;
    
    if (currentUpArrow != nullptr && currentUpArrowPressed != nullptr)
    {
        scrollUpButton.setImages(
            currentUpArrow.get(),          // normal
            currentUpArrow.get(),          // over (use normal)
            currentUpArrowPressed.get(),    // down
            currentUpArrow.get(),          // disabled (use normal)
            currentUpArrow.get(),          // normal (on)
            currentUpArrow.get(),          // over (on) (use normal)
            currentUpArrowPressed.get(),    // down (on)
            currentUpArrow.get()           // disabled (on) (use normal)
        );
    }
    
    if (currentDownArrow != nullptr && currentDownArrowPressed != nullptr)
    {
        scrollDownButton.setImages(
            currentDownArrow.get(),         // normal
            currentDownArrow.get(),         // over (use normal)
            currentDownArrowPressed.get(),   // down
            currentDownArrow.get(),         // disabled (use normal)
            currentDownArrow.get(),         // normal (on)
            currentDownArrow.get(),         // over (on) (use normal)
            currentDownArrowPressed.get(),   // down (on)
            currentDownArrow.get()          // disabled (on) (use normal)
        );
    }
}

// Performance: O(1) - Simple state updates and image switching
// CPU: Low - Conditional checks and pointer assignments
// Memory: Uses pre-cached images
void KronosAudioProcessorEditor::updateScrollButtonStates()
{
    bool isDark = audioProcessor.isDarkMode();
    auto dates = audioProcessor.getSortedDates();
    bool hasEnoughDates = dates.size() >= 4;
    
    // Up button
    auto& upNormal = isDark ? upArrowDarkSvgCache : upArrowLightSvgCache;
    auto& upPressed = isDark ? upArrowDarkPressedSvgCache : upArrowLightPressedSvgCache;
    
    if (upNormal != nullptr && upPressed != nullptr)
    {
        // Use pressed state if at top OR not enough dates
        auto& currentImage = (isAtTop || !hasEnoughDates) ? upPressed : upNormal;
        scrollUpButton.setImages(
            currentImage.get(),          // normal
            currentImage.get(),          // over
            upPressed.get(),             // down
            currentImage.get(),          // disabled
            currentImage.get(),          // normal (on)
            currentImage.get(),          // over (on)
            upPressed.get(),             // down (on)
            currentImage.get()           // disabled (on)
        );
    }
    
    // Down button
    auto& downNormal = isDark ? downArrowDarkSvgCache : downArrowLightSvgCache;
    auto& downPressed = isDark ? downArrowDarkPressedSvgCache : downArrowLightPressedSvgCache;
    
    if (downNormal != nullptr && downPressed != nullptr)
    {
        // Use pressed state if at bottom OR not enough dates
        auto& currentImage = (isAtBottom || !hasEnoughDates) ? downPressed : downNormal;
        scrollDownButton.setImages(
            currentImage.get(),          // normal
            currentImage.get(),          // over
            downPressed.get(),           // down
            currentImage.get(),          // disabled
            currentImage.get(),          // normal (on)
            currentImage.get(),          // over (on)
            downPressed.get(),           // down (on)
            currentImage.get()           // disabled (on)
        );
    }
}

// Performance: O(1) - Simple parameter update handler
// CPU: Low - Conditional checks and UI updates
void KronosAudioProcessorEditor::applyParameterChange(const juce::String& parameterID, float newValue)
{
    // Parameter change duration enter
    auto paramChangeStart = juce::Time::getMillisecondCounterHiRes();
    
    if (parameterID == "tracking")
    {
        playPauseButton.setToggleState(newValue >= 0.5f, juce::dontSendNotification);
        updateButtonImages();
    }
    else if (parameterID == "darkMode")
    {
        customLookAndFeel.setDarkMode(newValue >= 0.5f);
        updateThemeButtonImages();
        updateButtonImages();
        updateSortButtonImages();
        updateScrollButtonImages();
        updateScrollButtonStates();
        repaint();
        // Force refresh header SVG
        headerSvgCache.reset();
        headerLightSvgCache.reset();

        // Reload header assets after reset
        bool isDark = newValue >= 0.5f;
        headerSvgCache = juce::Drawable::createFromImageData(
            isDark ? BinaryData::Header_Dark_svg : BinaryData::Header_Light_svg,
            isDark ? BinaryData::Header_Dark_svgSize : BinaryData::Header_Light_svgSize
        );
        if (!isDark) {
            headerLightSvgCache = std::move(headerSvgCache);
        }
        
        // Update header bounds
        headerBounds = juce::Rectangle<float>(
            (getWidth() - 400.0f * scale)/2.0f,
            10.0f * scale,
            400.0f * scale,
            60.0f * scale
        );
    }
    else if (parameterID == "dateSortMode")
    {
        updateSortButtonImages();
        updateDateLabels();
    }

    // Parameter change duration exit
    DBG("Parameter change handling took: " << (juce::Time::getMillisecondCounterHiRes() - paramChangeStart) << " ms");
}

void KronosAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        applyParameterChange(parameterID, newValue);
        return;
    }

    juce::MessageManager::callAsync([safeThis = juce::Component::SafePointer<KronosAudioProcessorEditor>(this),
                                     parameterID,
                                     newValue]() {
        if (safeThis != nullptr)
            safeThis->applyParameterChange(parameterID, newValue);
    });
}

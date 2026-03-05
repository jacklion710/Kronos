#include "BackupComponent.h"

// Custom LookAndFeel implementation for backup component
class BackupLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override
    {
        // Special handling for close button
        if (button.getButtonText() == "x") {
            // Use a larger font for the close button - matching AboutComponent
            return juce::Font(juce::Font::getDefaultSansSerifFontName(), 20.0f, juce::Font::plain);
        }
        // Original behavior for other buttons
        return juce::Font("ASTERA", 18.0f, juce::Font::plain);
    }
};

/**
 * Constructor - Initialize UI components and set up styling
 * 
 * @param p Reference to the audio processor for accessing plugin state
 */
BackupComponent::BackupComponent(KronosAudioProcessor& p)
    : audioProcessor(p), isConfirmationMode(false), confirmationMessage("")
{
    // Close button setup
    closeButton.setButtonText("x");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(closeButton);

    // Set up the component with custom look and feel (only for backup button)
    customLookAndFeel = std::make_unique<BackupLookAndFeel>();
    setLookAndFeel(customLookAndFeel.get());
    
    // Configure and add title label
    titleLabel.setText("Backup Session Data", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("ASTERA", 24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xE6, 0xE6, 0xFF));
    addAndMakeVisible(titleLabel);
    
    // Configure instruction label
    instructionLabel.setText("Enter a project name for this backup:", juce::dontSendNotification);
    instructionLabel.setFont(juce::Font("ASTERA", 16.0f, juce::Font::plain));
    instructionLabel.setJustificationType(juce::Justification::centred);
    instructionLabel.setColour(juce::Label::textColourId, juce::Colour(0xE6, 0xE6, 0xFF));
    addAndMakeVisible(instructionLabel);
    
    // Configure and add project name editor
    projectNameEditor.setMultiLine(false);
    projectNameEditor.setReturnKeyStartsNewLine(false);
    projectNameEditor.setReadOnly(false);
    projectNameEditor.setScrollbarsShown(false);
    projectNameEditor.setCaretVisible(true);
    projectNameEditor.setPopupMenuEnabled(true);
    projectNameEditor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 18.0f, juce::Font::plain));
    projectNameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0x30303030));
    projectNameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xFFE6E6FF));
    projectNameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF505050));
    projectNameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xFF909090));
    projectNameEditor.setJustification(juce::Justification::centred);
    projectNameEditor.setText("MY PROJECT", juce::dontSendNotification);
    addAndMakeVisible(projectNameEditor);
    
    // Configure and add backup button
    backupButton.setButtonText("Create Backup");
    backupButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF404080));
    backupButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    backupButton.onClick = [this] { initiateBackup(); };
    addAndMakeVisible(backupButton);

    // Configure confirmation buttons (initially invisible)
    confirmButton.setButtonText("Overwrite");
    confirmButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF404080));
    confirmButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    confirmButton.onClick = [this] { confirmBackup(); };
    addAndMakeVisible(confirmButton);
    confirmButton.setVisible(false);

    cancelButton.setButtonText("Cancel");
    cancelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF404080));
    cancelButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    cancelButton.onClick = [this] { cancelConfirmation(); };
    addAndMakeVisible(cancelButton);
    cancelButton.setVisible(false);
}

/**
 * Destructor - Clean up resources
 */
BackupComponent::~BackupComponent()
{
    setLookAndFeel(nullptr);
}

/**
 * Paint method - Draw the component background and styling
 * 
 * @param g Graphics context to use for drawing
 */
void BackupComponent::paint(juce::Graphics& g)
{
    // Calculate scale factor
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    float scale = juce::jmin(widthScale, heightScale);

    // Background
    g.fillAll(juce::Colour(0xFF1A1A1A));
    
    // Text color
    g.setColour(juce::Colours::white);
    
    auto asteraFont = juce::Font("ASTERA", 24.0f * scale, juce::Font::plain);
    auto contentBounds = getLocalBounds().reduced(20 * scale);
    
    // Title
    g.setFont(asteraFont);
    g.drawText(isConfirmationMode ? "CONFIRM BACKUP" : "BACKUP SESSION DATA", 
               contentBounds.removeFromTop(40 * scale), juce::Justification::centred, true);
    
    contentBounds.removeFromTop(30 * scale);

    if (isConfirmationMode)
    {
        // Draw confirmation message
        g.setFont(asteraFont.withHeight(16.0f * scale));
        auto messageArea = contentBounds;
        messageArea.removeFromTop(60 * scale); // Add some space after title, but not too much
        messageArea.setHeight(120 * scale); // Set a fixed height for the message area
        g.drawFittedText(confirmationMessage,
                        messageArea,
                        juce::Justification::centred,
                        3);
    }
    else
    {
        // Project name instruction
        g.setFont(asteraFont.withHeight(20.0f * scale));
        g.drawText("ENTER A PROJECT NAME FOR THIS BACKUP:", 
                   contentBounds.removeFromTop(30 * scale), 
                   juce::Justification::centred, true);
        
        contentBounds.removeFromTop(50 * scale);
        contentBounds.removeFromTop(60 * scale);
    }
    
    // Draw metallic border
    float borderThickness = 2.0f * scale;
    juce::ColourGradient borderGradient(
        juce::Colour(180, 180, 180),
        getLocalBounds().getTopLeft().toFloat(),
        juce::Colour(100, 100, 100),
        getLocalBounds().getBottomRight().toFloat(),
        false);
    
    auto borderBounds = getLocalBounds().toFloat();
    g.setGradientFill(borderGradient);
    g.drawRect(borderBounds, borderThickness);
}

/**
 * Resize method - Position components based on size
 */
void BackupComponent::resized()
{
    // Calculate scale factor based on parent size
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    float scale = juce::jmin(widthScale, heightScale);

    // Position close button relative to parent bounds with scaling
    closeButton.setBounds(getWidth() - (25 * scale), 5 * scale, 20 * scale, 20 * scale);
    
    if (isConfirmationMode)
    {
        // Hide normal UI elements
        projectNameEditor.setVisible(false);
        backupButton.setVisible(false);

        // Position confirmation buttons
        int buttonWidth = 200 * scale;
        int buttonHeight = 60 * scale;
        int buttonSpacing = 20 * scale;
        int totalWidth = (buttonWidth * 2) + buttonSpacing;
        int startX = (getWidth() - totalWidth) / 2;
        int buttonY = getHeight() - buttonHeight - (60 * scale);

        confirmButton.setBounds(startX, buttonY, buttonWidth, buttonHeight);
        cancelButton.setBounds(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);

        confirmButton.setVisible(true);
        cancelButton.setVisible(true);
    }
    else
    {
        // Show normal UI elements
        projectNameEditor.setVisible(true);
        backupButton.setVisible(true);
        confirmButton.setVisible(false);
        cancelButton.setVisible(false);

        // Position project name input
        int inputWidth = 480 * scale;
        int inputHeight = 40 * scale;
        int inputX = (getWidth() - inputWidth) / 2;
        int inputY = 140 * scale;
        
        projectNameEditor.setBounds(inputX, inputY, inputWidth, inputHeight);
        
        // Position backup button
        int buttonWidth = 200 * scale;
        int buttonHeight = 60 * scale;
        int buttonX = (getWidth() - buttonWidth) / 2;
        int buttonY = getHeight() - buttonHeight - (60 * scale);
        
        backupButton.setBounds(buttonX, buttonY, buttonWidth, buttonHeight);
    }
}

/**
 * Get the platform-specific backup directory path
 * 
 * @return A string with the full backup directory path
 */
juce::String BackupComponent::getBackupDirectory()
{
    // Get appropriate directory for the platform
    juce::File backupDir;
    
    #if JUCE_MAC
        backupDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile("Application Support/KronosTimeTracker/Backups");
    #elif JUCE_WINDOWS
        backupDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile("KronosTimeTracker/Backups");
    #else
        backupDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                    .getChildFile(".kronostimetracker/backups");
    #endif
    
    // Ensure directory exists
    backupDir.createDirectory();
    
    return backupDir.getFullPathName();
}

void BackupComponent::initiateBackup()
{
    // Get project name from the text editor
    juce::String projectName = projectNameEditor.getText();
    
    // Sanitize the filename
    juce::String sanitizedName = "";
    const juce::String invalidChars = "\\/*?\"<>|:";
    
    for (auto c : projectName)
    {
        if (invalidChars.containsChar(c))
            sanitizedName += '_';
        else
            sanitizedName += c;
    }
    
    projectName = sanitizedName.isEmpty() ? "KronosBackup" : sanitizedName;
    
    // Create backup filename
    juce::String filename = projectName + ".kronos";
    
    // Create full path
    juce::File backupDir(getBackupDirectory());
    juce::File backupFile = backupDir.getChildFile(filename);
    
    if (backupFile.existsAsFile())
    {
        // Show confirmation UI
        confirmationMessage = "A backup file named '" + filename + "' already exists.\n\n"
                            "Do you want to overwrite it?";
        pendingBackupFile = backupFile;
        isConfirmationMode = true;
        resized(); // Trigger UI update
        repaint();
    }
    else
    {
        // Proceed with backup
        saveBackup(backupFile);
    }
}

void BackupComponent::confirmBackup()
{
    if (pendingBackupFile != juce::File())
    {
        saveBackup(pendingBackupFile);
        resetConfirmationState();
    }
}

void BackupComponent::cancelConfirmation()
{
    resetConfirmationState();
}

void BackupComponent::resetConfirmationState()
{
    isConfirmationMode = false;
    confirmationMessage = "";
    pendingBackupFile = juce::File();
    resized();
    repaint();
}

void BackupComponent::saveBackup(const juce::File& backupFile)
{
    juce::MemoryBlock memoryBlock;
    audioProcessor.getStateInformation(memoryBlock);

    if (memoryBlock.getSize() == 0)
    {
        titleLabel.setText("Backup Failed", juce::dontSendNotification);
        startTimer(2000);
        return;
    }
    
    if (backupFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize()))
    {
        titleLabel.setText("Backup Successful", juce::dontSendNotification);
        startTimer(2000);
    }
    else
    {
        titleLabel.setText("Backup Failed", juce::dontSendNotification);
        startTimer(2000);
    }
}

void BackupComponent::timerCallback()
{
    stopTimer();
    setVisible(false);
}

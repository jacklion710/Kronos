#include "RestoreComponent.h"

/**
 * Constructor - Initialize UI components and set up styling
 * 
 * @param p Reference to the audio processor for accessing plugin state
 */
RestoreComponent::RestoreComponent(KronosAudioProcessor& p)
    : audioProcessor(p), isConfirmationMode(false), isSuccessMode(false),
      confirmationMessage(""), statusMessage("")
{
    // Set up the component
    setLookAndFeel(&lookAndFeel);
    
    // Configure and add the close button
    closeButton.setButtonText("x");
    closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    addAndMakeVisible(closeButton);
    
    // Configure and add title label
    titleLabel.setText("Restore Session Data", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("ASTERA", 24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE6E6FF));
    addAndMakeVisible(titleLabel);
    
    // Configure instruction label
    instructionLabel.setText("Select a backup to restore:", juce::dontSendNotification);
    instructionLabel.setFont(juce::Font("ASTERA", 16.0f, juce::Font::plain));
    instructionLabel.setJustificationType(juce::Justification::centred);
    instructionLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFE6E6FF));
    addAndMakeVisible(instructionLabel);
    
    // Configure and add backup listbox
    backupListBox.setModel(this);
    backupListBox.setRowHeight(50);
    backupListBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xFF202030));
    backupListBox.setColour(juce::ListBox::outlineColourId, juce::Colour(0xFF404060));
    backupListBox.setColour(juce::ListBox::textColourId, juce::Colour(0xFFE6E6FF));
    addAndMakeVisible(backupListBox);
    
    // Configure and add restore button
    restoreButton.setButtonText("RESTORE");
    restoreButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF404080));
    restoreButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    restoreButton.onClick = [this] { performRestore(); };
    restoreButton.setEnabled(false); // Disabled until selection is made
    addAndMakeVisible(restoreButton);
    
    // Add browse button
    browseButton.setButtonText("BROWSE");
    browseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF404080));
    browseButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    browseButton.onClick = [this] { browseForBackup(); };
    addAndMakeVisible(browseButton);
    
    // Configure confirmation buttons (initially invisible)
    confirmButton.setButtonText("RESTORE");
    confirmButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF404080));
    confirmButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    confirmButton.onClick = [this] { confirmRestore(); };
    addAndMakeVisible(confirmButton);
    confirmButton.setVisible(false);

    cancelButton.setButtonText("CANCEL");
    cancelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF404080));
    cancelButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE6E6FF));
    cancelButton.onClick = [this] { cancelConfirmation(); };
    addAndMakeVisible(cancelButton);
    cancelButton.setVisible(false);
    
    // Populate the list of backups
    refreshBackupList();
}

/**
 * Destructor - Clean up resources
 */
RestoreComponent::~RestoreComponent()
{
    setLookAndFeel(nullptr);
}

/**
 * Paint method - Draw the component background and styling
 * 
 * @param g Graphics context to use for drawing
 */
void RestoreComponent::paint(juce::Graphics& g)
{
    // Fill background with semi-transparent color for modal effect
    g.fillAll(juce::Colour(0xDD1A1A1A));
    
    // Calculate scale factor
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    float scale = juce::jmin(widthScale, heightScale);

    // Text color
    g.setColour(juce::Colours::white);
    
    auto asteraFont = juce::Font("ASTERA", 24.0f * scale, juce::Font::plain);
    auto contentBounds = getLocalBounds().reduced(20 * scale);
    
    // Title
    g.setFont(asteraFont);
    if (isSuccessMode)
        g.drawText("RESTORE " + statusMessage, 
                   contentBounds.removeFromTop(40 * scale), 
                   juce::Justification::centred, true);
    else
        g.drawText(isConfirmationMode ? "CONFIRM RESTORE" : "RESTORE SESSION DATA", 
                   contentBounds.removeFromTop(40 * scale), 
                   juce::Justification::centred, true);
    
    contentBounds.removeFromTop(30 * scale);

    if (isConfirmationMode)
    {
        // Draw confirmation message
        g.setFont(asteraFont.withHeight(16.0f * scale));
        auto messageArea = contentBounds;
        messageArea.removeFromTop(60 * scale);
        messageArea.setHeight(120 * scale);
        g.drawFittedText(confirmationMessage,
                        messageArea,
                        juce::Justification::centred,
                        3);
    }
    else if (isSuccessMode)
    {
        // Draw success/error message
        g.setFont(asteraFont.withHeight(16.0f * scale));
        g.drawFittedText(statusMessage,
                        contentBounds.reduced(20),
                        juce::Justification::centred,
                        3);
    }
    else
    {
        // Instructions
        g.setFont(asteraFont.withHeight(20.0f * scale));
        g.drawText("SELECT A BACKUP TO RESTORE:", 
                   contentBounds.removeFromTop(30 * scale), 
                   juce::Justification::centred, true);
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
void RestoreComponent::resized()
{
    // Calculate scale factor based on parent size
    float widthScale = getWidth() / 600.0f;
    float heightScale = getHeight() / 450.0f;
    float scale = juce::jmin(widthScale, heightScale);

    // Position close button relative to parent bounds with scaling
    closeButton.setBounds(getWidth() - (25 * scale), 5 * scale, 20 * scale, 20 * scale);
    
    auto contentBounds = getLocalBounds().reduced(20 * scale);
    
    // Reserve space for title and instructions (120 pixels at default scale)
    contentBounds.removeFromTop(120 * scale);
    
    if (isConfirmationMode)
    {
        // Hide normal UI elements
        backupListBox.setVisible(false);
        restoreButton.setVisible(false);
        browseButton.setVisible(false);

        // Position confirmation buttons
        int buttonWidth = 200 * scale;
        int buttonHeight = 60 * scale;
        int buttonSpacing = 20 * scale;
        int totalWidth = (buttonWidth * 2) + buttonSpacing;
        int startX = (getWidth() - totalWidth) / 2;
        
        // Position buttons relative to bottom of content bounds
        int buttonY = getHeight() - buttonHeight - (40 * scale);

        confirmButton.setBounds(startX, buttonY, buttonWidth, buttonHeight);
        cancelButton.setBounds(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);

        confirmButton.setVisible(true);
        cancelButton.setVisible(true);
    }
    else if (isSuccessMode)
    {
        // Hide all interactive elements
        backupListBox.setVisible(false);
        restoreButton.setVisible(false);
        browseButton.setVisible(false);
        confirmButton.setVisible(false);
        cancelButton.setVisible(false);
    }
    else
    {
        // Show normal UI elements
        backupListBox.setVisible(true);
        restoreButton.setVisible(true);
        browseButton.setVisible(true);
        confirmButton.setVisible(false);
        cancelButton.setVisible(false);

        // Reserve space for buttons at bottom
        int buttonHeight = 60 * scale;
        int bottomPadding = 40 * scale;
        auto bottomSection = contentBounds.removeFromBottom(buttonHeight + bottomPadding);
        
        // Position buttons
        int buttonWidth = 200 * scale;
        int buttonSpacing = 20 * scale;
        int totalWidth = (buttonWidth * 2) + buttonSpacing;
        int startX = (getWidth() - totalWidth) / 2;
        
        restoreButton.setBounds(startX, bottomSection.getY(), buttonWidth, buttonHeight);
        browseButton.setBounds(startX + buttonWidth + buttonSpacing, bottomSection.getY(), buttonWidth, buttonHeight);

        // Position listbox in remaining space
        backupListBox.setBounds(contentBounds.reduced(20 * scale));
    }
}

/**
 * Refresh the list of available backup files
 */
void RestoreComponent::refreshBackupList()
{
    // Clear current list
    backupFiles.clear();
    
    // Get the backup directory
    juce::File backupDir(getBackupDirectory());
    
    // Check if directory exists
    if (backupDir.exists() && backupDir.isDirectory())
    {
        // Get all files with .kronos extension
        juce::Array<juce::File> files;
        backupDir.findChildFiles(files, juce::File::findFiles, false, "*.kronos");
        
        // Sort manually - create a new array and add files in sorted order
        juce::Array<juce::File> sortedFiles;
        
        // First, add all files to our new array
        for (auto& file : files)
        {
            sortedFiles.add(file);
        }
        
        // Now manually sort the array using bubble sort (simple but effective for small lists)
        bool swapped;
        for (int i = 0; i < sortedFiles.size() - 1; i++)
        {
            swapped = false;
            for (int j = 0; j < sortedFiles.size() - i - 1; j++)
            {
                // Compare creation times - sort newest (largest time) first
                auto timeA = sortedFiles[j].getCreationTime().toMilliseconds();
                auto timeB = sortedFiles[j + 1].getCreationTime().toMilliseconds();
                
                if (timeA < timeB) // timeB is newer
                {
                    // Swap them
                    juce::File temp = sortedFiles[j];
                    sortedFiles.set(j, sortedFiles[j + 1]);
                    sortedFiles.set(j + 1, temp);
                    swapped = true;
                }
            }
            
            // If no swapping occurred in this pass, array is sorted
            if (!swapped)
                break;
        }
        
        // Store the sorted files
        backupFiles = sortedFiles;
    }
    
    // Update the listbox
    backupListBox.updateContent();
    backupListBox.repaint();
}

/**
 * Get the platform-specific backup directory path
 * 
 * @return A string with the full backup directory path
 */
juce::String RestoreComponent::getBackupDirectory()
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
    
    return backupDir.getFullPathName();
}

void RestoreComponent::initiateRestore()
{
    int selectedRow = backupListBox.getSelectedRow();
    if (selectedRow >= 0 && selectedRow < backupFiles.size())
    {
        juce::File selectedFile = backupFiles[selectedRow];
        juce::MemoryBlock memoryBlock;
        
                            if (selectedFile.loadFileAsData(memoryBlock))
                            {
            confirmationMessage = "Are you sure you want to restore from:\n\n" + 
                                selectedFile.getFileName() + "\n\n" +
                                "This will replace your current session data.";
            pendingRestoreFile = selectedFile;
            pendingRestoreData = memoryBlock;
            isConfirmationMode = true;
            resized();
            repaint();
                            }
                            else
                            {
            showErrorMessage("Could not read backup file:\n" + selectedFile.getFullPathName());
        }
    }
}

void RestoreComponent::confirmRestore()
{
    if (pendingRestoreFile != juce::File())
    {
        performRestore(pendingRestoreFile, pendingRestoreData);
        resetConfirmationState();
    }
}

void RestoreComponent::cancelConfirmation()
{
    resetConfirmationState();
}

void RestoreComponent::resetConfirmationState()
{
    isConfirmationMode = false;
    isSuccessMode = false;
    confirmationMessage = "";
    statusMessage = "";
    pendingRestoreFile = juce::File();
    pendingRestoreData.reset();
    resized();
    repaint();
}

void RestoreComponent::performRestore(const juce::File& file, const juce::MemoryBlock& data)
{
    audioProcessor.setStateInformation(data.getData(), static_cast<int>(data.getSize()));
    showSuccessMessage("Session data has been restored from:\n" + file.getFullPathName());
}

void RestoreComponent::showSuccessMessage(const juce::String& message)
{
    isSuccessMode = true;
    statusMessage = "SUCCESSFUL\n\n" + message;
    resized();
    repaint();
    startTimer(2000); // Close window after 2 seconds
}

void RestoreComponent::showErrorMessage(const juce::String& message)
{
    isSuccessMode = true;
    statusMessage = "FAILED\n\n" + message;
    resized();
    repaint();
    startTimer(2000);
}

void RestoreComponent::timerCallback()
{
    stopTimer();
    setVisible(false);
}

// Update performRestore to use the new flow
void RestoreComponent::performRestore()
{
    initiateRestore();
}

// Update browseForBackup to use the new confirmation flow
void RestoreComponent::browseForBackup()
{
    juce::FileChooser chooser("Select Backup File",
                             juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                             "*.kronos");
    
    auto chooserFlags = juce::FileBrowserComponent::openMode | 
                       juce::FileBrowserComponent::canSelectFiles;
                       
    chooser.launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        auto selectedFile = chooser.getResult();
        if (selectedFile.existsAsFile())
        {
            juce::MemoryBlock memoryBlock;
            if (selectedFile.loadFileAsData(memoryBlock))
            {
                confirmationMessage = "Are you sure you want to restore from:\n\n" + 
                                    selectedFile.getFileName() + "\n\n" +
                                    "This will replace your current session data.";
                pendingRestoreFile = selectedFile;
                pendingRestoreData = memoryBlock;
                isConfirmationMode = true;
                resized();
                repaint();
            }
            else
            {
                showErrorMessage("Could not read backup file:\n" + selectedFile.getFullPathName());
            }
        }
    });
}

//==============================================================================
// ListBoxModel implementation
//==============================================================================

int RestoreComponent::getNumRows()
{
    return backupFiles.size();
}

void RestoreComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, 
                                       int width, int height, bool rowIsSelected)
{
    // Check valid row
    if (rowNumber < 0 || rowNumber >= backupFiles.size())
        return;
    
    // Get the file
    juce::File file = backupFiles[rowNumber];
    
    // Background
    if (rowIsSelected)
        g.fillAll(juce::Colour(0xFF6060A0));
    else if (rowNumber % 2)
        g.fillAll(juce::Colour(0xFF303040));
    
    // Text color
    g.setColour(juce::Colour(0xFFE6E6FF));
    g.setFont(juce::Font("ASTERA", 14.0f, juce::Font::plain));
    
    // Extract project name and date from filename
    juce::String filename = file.getFileNameWithoutExtension();
    juce::String projectName = filename;
    
    // Draw project name
    g.drawText(projectName, 10, 0, width - 20, height / 2, juce::Justification::bottomLeft, true);
    
    // Draw date in smaller font
    g.setFont(juce::Font("ASTERA", 12.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xFFBBBBDD));
    g.drawText(file.getCreationTime().formatted("%Y-%m-%d %H:%M:%S"), 
             10, height / 2, width - 20, height / 2, juce::Justification::topLeft, true);
}

void RestoreComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    // Double-click performs restore
    if (row >= 0 && row < backupFiles.size())
    {
        performRestore();
    }
}

void RestoreComponent::selectedRowsChanged(int lastRowSelected)
{
    // Enable/disable restore button based on selection
    restoreButton.setEnabled(lastRowSelected >= 0 && lastRowSelected < backupFiles.size());
} 
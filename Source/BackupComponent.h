#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * BackupComponent - Modal dialog for backing up plugin state data
 * 
 * Allows users to enter a project name and displays the location
 * where the backup will be saved.
 */
class BackupComponent : public juce::Component,
                       public juce::Timer
{
public:
    // Constructor takes a reference to the processor to access plugin state
    BackupComponent(KronosAudioProcessor& p);
    ~BackupComponent() override;

    // JUCE Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Close button for the modal dialog
    juce::TextButton closeButton;

private:
    // Reference to the audio processor
    KronosAudioProcessor& audioProcessor;
    
    // UI Elements
    juce::Label titleLabel;
    juce::Label instructionLabel;
    juce::Label locationLabel;
    juce::TextEditor projectNameEditor;
    juce::TextButton backupButton;
    
    // Custom look and feel
    std::unique_ptr<juce::LookAndFeel> customLookAndFeel;
    
    // New UI elements for confirmation
    juce::TextButton confirmButton;
    juce::TextButton cancelButton;
    
    // State management
    bool isConfirmationMode;
    juce::String confirmationMessage;
    juce::File pendingBackupFile;
    
    // Setup and positioning methods
    void setupComponents();
    void positionComponents();
    
    // Helper methods
    void performBackup();
    juce::String getBackupDirectory();
    void initiateBackup();
    void confirmBackup();
    void cancelConfirmation();
    void resetConfirmationState();
    void saveBackup(const juce::File& backupFile);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackupComponent)
}; 
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * RestoreComponent - Modal dialog for restoring plugin state data
 * 
 * Allows users to select a previous backup to restore, which will
 * overwrite the current plugin state.
 */
class RestoreComponent : public juce::Component,
                         public juce::ListBoxModel,
                         public juce::Timer
{
public:
    // Constructor takes a reference to the processor to access plugin state
    RestoreComponent(KronosAudioProcessor&);
    ~RestoreComponent() override;

    // JUCE Component overrides
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Close button for the modal dialog
    juce::TextButton closeButton;

    // ListBoxModel implementation
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

private:
    // Reference to the audio processor
    KronosAudioProcessor& audioProcessor;
    
    // UI Elements
    juce::Label titleLabel;
    juce::Label instructionLabel;
    juce::ListBox backupListBox;
    juce::TextButton restoreButton;
    juce::TextButton browseButton;
    
    // Confirmation UI elements
    juce::TextButton confirmButton;
    juce::TextButton cancelButton;
    
    // State management
    bool isConfirmationMode;
    bool isSuccessMode;
    juce::String confirmationMessage;
    juce::String statusMessage;
    juce::File pendingRestoreFile;
    juce::MemoryBlock pendingRestoreData;
    
    // Data storage
    juce::Array<juce::File> backupFiles;
    
    // Methods
    void refreshBackupList();
    void performRestore();
    juce::String getBackupDirectory();
    void browseForBackup();
    void initiateRestore();
    void confirmRestore();
    void cancelConfirmation();
    void resetConfirmationState();
    void performRestore(const juce::File& file, const juce::MemoryBlock& data);
    void showSuccessMessage(const juce::String& message);
    void showErrorMessage(const juce::String& message);
    
    // The LookAndFeel instance to use
    juce::LookAndFeel_V4 lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RestoreComponent)
}; 
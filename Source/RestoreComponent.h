#pragma once

#include <JuceHeader.h>
#include "KronosWidgets.h"
#include "PluginProcessor.h"

/**
 * RestoreComponent - Sheet for restoring plugin state data
 *
 * Pick a backup, then compare it with the current log before replacing it.
 */
class RestoreComponent : public KronosSheet,
                         private juce::ListBoxModel
{
public:
    enum class Stage { choosing, confirming };

    explicit RestoreComponent(KronosAudioProcessor&);

    juce::String getKicker() const override;
    juce::String getSheetTitle() const override;
    int getPreferredHeight() const override;
    void focusDefaultControl() override;
    void cancelOrClose() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    Stage getStage() const { return stage; }
    juce::String getErrorText() const { return errorText; }

    // Reads a backup and moves to the confirm step if it is valid.
    bool beginConfirmation(const juce::File& file);
    bool confirmReplace();

private:
    struct Backup
    {
        juce::File file;
        KronosAudioProcessor::BackupSummary summary;
    };

    int getNumRows() override { return backups.size(); }
    void paintListBoxItem(int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;

    void setStage(Stage newStage);
    void refreshBackupList();
    juce::String getBackupDirectory() const;
    void browseForBackup();
    void showError(const juce::String& message);

    KronosAudioProcessor& audioProcessor;

    KronosListBox backupListBox;
    KronosButton browseButton { "Browse", KronosButton::Style::secondary };
    KronosButton cancelButton { "Cancel", KronosButton::Style::secondary };
    KronosButton restoreButton { "Restore", KronosButton::Style::primary };
    KronosButton keepButton { "Keep current", KronosButton::Style::emphasised };
    KronosButton replaceButton { "Replace log", KronosButton::Style::primary };

    Stage stage = Stage::choosing;
    juce::String errorText;
    juce::Array<Backup> backups;
    juce::File pendingRestoreFile;
    juce::MemoryBlock pendingRestoreData;
    KronosAudioProcessor::BackupSummary pendingSummary;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RestoreComponent)
};

#pragma once

#include <JuceHeader.h>
#include "KronosWidgets.h"
#include "PluginProcessor.h"

/**
 * BackupComponent - Sheet for backing up plugin state data
 *
 * The success state is only shown after the file has actually been written.
 */
class BackupComponent : public KronosSheet
{
public:
    enum class Stage { entering, confirmingOverwrite, saved };

    explicit BackupComponent(KronosAudioProcessor& p);

    juce::String getKicker() const override;
    juce::String getSheetTitle() const override;
    int getPreferredHeight() const override;
    void focusDefaultControl() override;
    void cancelOrClose() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void parentHierarchyChanged() override;

    Stage getStage() const { return stage; }
    juce::String getErrorText() const { return errorText; }
    void setProjectName(const juce::String& name) { projectNameEditor.setText(name, true); }

    // Writes the current state and moves to the saved stage, or reports why not.
    bool saveBackup(const juce::File& backupFile);

private:
    void setStage(Stage newStage);
    juce::File getBackupFile() const;
    juce::String getFileName() const;
    juce::String getBackupDirectory() const;
    void initiateBackup();

    KronosAudioProcessor& audioProcessor;

    juce::TextEditor projectNameEditor;
    KronosButton cancelButton { "Cancel", KronosButton::Style::secondary };
    KronosButton saveButton { "Save backup", KronosButton::Style::primary };
    KronosButton overwriteButton { "Overwrite", KronosButton::Style::primary };
    KronosButton revealButton { "Show in folder", KronosButton::Style::secondary };
    KronosButton doneButton { "Done", KronosButton::Style::primary };

    Stage stage = Stage::entering;
    juce::String errorText;
    juce::File savedFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BackupComponent)
};

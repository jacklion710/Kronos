#include "BackupComponent.h"

using KLF = KronosLookAndFeel;

BackupComponent::BackupComponent(KronosAudioProcessor& p) : audioProcessor(p)
{
    projectNameEditor.setTitle("Project name");
    projectNameEditor.setMultiLine(false);
    projectNameEditor.setReturnKeyStartsNewLine(false);
    projectNameEditor.setScrollbarsShown(false);
    projectNameEditor.setFont(KLF::font(KLF::Face::monoRegular, 13.0f, 0.06f));
    projectNameEditor.setIndents(12, 10);
    projectNameEditor.setBorder(juce::BorderSize<int>(0));
    projectNameEditor.setText("MY PROJECT", juce::dontSendNotification);
    projectNameEditor.onTextChange = [this] { errorText.clear(); repaint(); };
    projectNameEditor.onReturnKey = [this] { initiateBackup(); };
    projectNameEditor.onEscapeKey = [this] { cancelOrClose(); };
    addAndMakeVisible(projectNameEditor);

    for (auto* button : { &cancelButton, &saveButton, &overwriteButton, &revealButton, &doneButton })
    {
        button->setButtonText(button->getName());
        addChildComponent(*button);
    }

    cancelButton.onActivated = [this] { cancelOrClose(); };
    saveButton.onActivated = [this] { initiateBackup(); };
    overwriteButton.onActivated = [this] { saveBackup(getBackupFile()); };
    revealButton.onActivated = [this] { savedFile.revealToUser(); };
    doneButton.onActivated = [this] { if (onClose) onClose(); };

    setStage(Stage::entering);
}

void BackupComponent::lookAndFeelChanged()
{
    // A sheet that is detached or closing has no Kronos colours to read
    if (! getLookAndFeel().isColourSpecified(KLF::textColourId))
        return;

    // Text that is already in the field keeps its colour unless told otherwise
    projectNameEditor.applyColourToAllText(findColour(KLF::textColourId), true);
}

void BackupComponent::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

juce::String BackupComponent::getKicker() const
{
    return "BACKUP";
}

juce::String BackupComponent::getSheetTitle() const
{
    return stage == Stage::saved ? "Backup saved" : "Back up session data";
}

int BackupComponent::getPreferredHeight() const
{
    switch (stage)
    {
        case Stage::confirmingOverwrite: return 128;
        case Stage::saved:               return 152;
        case Stage::entering:            break;
    }

    return errorText.isEmpty() ? 220 : 256;
}

void BackupComponent::focusDefaultControl()
{
    if (stage == Stage::entering && projectNameEditor.isShowing())
    {
        projectNameEditor.grabKeyboardFocus();
        projectNameEditor.selectAll();
    }
    else if (stage == Stage::saved && doneButton.isShowing())
    {
        doneButton.grabKeyboardFocus();
    }
    else if (stage == Stage::confirmingOverwrite && cancelButton.isShowing())
    {
        cancelButton.grabKeyboardFocus();
    }
}

void BackupComponent::cancelOrClose()
{
    if (stage == Stage::confirmingOverwrite)
        setStage(Stage::entering);
    else if (onClose)
        onClose();
}

void BackupComponent::setStage(Stage newStage)
{
    stage = newStage;

    projectNameEditor.setVisible(stage == Stage::entering);
    saveButton.setVisible(stage == Stage::entering);
    cancelButton.setVisible(stage != Stage::saved);
    overwriteButton.setVisible(stage == Stage::confirmingOverwrite);
    revealButton.setVisible(stage == Stage::saved);
    doneButton.setVisible(stage == Stage::saved);

    resized();
    repaint();

    if (onLayoutChanged)
        onLayoutChanged();

    focusDefaultControl();
}

/**
 * Get the platform-specific backup directory path
 *
 * @return A string with the full backup directory path
 */
juce::String BackupComponent::getBackupDirectory() const
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

juce::String BackupComponent::getFileName() const
{
    // Sanitize the filename
    juce::String sanitizedName;
    const juce::String invalidChars = "\\/*?\"<>|:";

    for (auto c : projectNameEditor.getText())
        sanitizedName += invalidChars.containsChar(c) ? juce::juce_wchar('_') : c;

    return (sanitizedName.isEmpty() ? juce::String("KronosBackup") : sanitizedName) + ".kronos";
}

juce::File BackupComponent::getBackupFile() const
{
    return juce::File(getBackupDirectory()).getChildFile(getFileName());
}

void BackupComponent::initiateBackup()
{
    const auto backupFile = getBackupFile();

    if (backupFile.existsAsFile())
        setStage(Stage::confirmingOverwrite);
    else
        saveBackup(backupFile);
}

bool BackupComponent::saveBackup(const juce::File& backupFile)
{
    juce::MemoryBlock memoryBlock;
    audioProcessor.getStateInformation(memoryBlock);

    const auto directoryReady = backupFile.getParentDirectory().createDirectory().wasOk();

    if (memoryBlock.getSize() > 0 && directoryReady
        && backupFile.replaceWithData(memoryBlock.getData(), memoryBlock.getSize()))
    {
        errorText.clear();
        savedFile = backupFile;
        setStage(Stage::saved);
        return true;
    }

    errorText = "The backup could not be saved. Check that this folder is writable: "
              + backupFile.getParentDirectory().getFullPathName();
    setStage(Stage::entering);
    return false;
}

void BackupComponent::resized()
{
    auto area = getLocalBounds();
    auto footer = area.removeFromBottom(36);

    if (stage == Stage::entering)
    {
        projectNameEditor.setBounds(0, 104, getWidth(), 38);

        saveButton.setBounds(footer.removeFromRight(122));
        footer.removeFromRight(8);
        cancelButton.setBounds(footer.removeFromRight(84));
    }
    else if (stage == Stage::confirmingOverwrite)
    {
        overwriteButton.setBounds(footer.removeFromRight(112));
        footer.removeFromRight(8);
        cancelButton.setBounds(footer.removeFromRight(84));
    }
    else
    {
        doneButton.setBounds(footer.removeFromRight(84));
        footer.removeFromRight(8);
        revealButton.setBounds(footer.removeFromRight(130));
    }
}

void BackupComponent::paint(juce::Graphics& g)
{
    const auto text = findColour(KLF::textColourId);
    const auto muted = findColour(KLF::mutedColourId);
    const auto faint = findColour(KLF::faintColourId);
    const auto width = static_cast<float>(getWidth());
    const auto titleFont = KLF::font(KLF::Face::sansSemibold, 22.0f);
    const auto bodyFont = KLF::font(KLF::Face::sansRegular, 13.0f);
    const auto monoSmall = KLF::font(KLF::Face::monoRegular, 11.0f);

    if (stage == Stage::entering)
    {
        g.setColour(text);
        g.setFont(titleFont);
        g.drawText(getSheetTitle(), juce::Rectangle<float>(0.0f, 0.0f, width, 28.0f), juce::Justification::centredLeft, false);

        drawParagraph(g, juce::String(juce::CharPointer_UTF8("Saves this project\xe2\x80\x99s full time log as a .kronos file you can restore into any session.")),
                      bodyFont, muted, { 0.0f, 32.0f, width, 38.0f });

        g.setColour(muted);
        g.setFont(KLF::font(KLF::Face::sansSemibold, 11.0f, 0.12f));
        g.drawText("PROJECT NAME", juce::Rectangle<float>(0.0f, 84.0f, width, 14.0f), juce::Justification::centredLeft, false);

        g.setColour(faint);
        g.setFont(monoSmall);
        const auto summary = KronosArt::formatDuration(audioProcessor.getTotalTimeInSeconds())
                           + juce::String(juce::CharPointer_UTF8(" \xc2\xb7 "))
                           + KronosArt::formatDays(audioProcessor.getDaysLogged());
        const auto summaryWidth = monoSmall.getStringWidthFloat(summary);
        const juce::Rectangle<float> previewRow(0.0f, 152.0f, width, 14.0f);

        g.drawText(summary, previewRow, juce::Justification::centredRight, false);
        g.drawFittedText(getFileName(), previewRow.withTrimmedRight(summaryWidth + 12.0f).toNearestInt(),
                         juce::Justification::centredLeft, 1, 1.0f);

        if (errorText.isNotEmpty())
            drawParagraph(g, errorText, KLF::font(KLF::Face::sansRegular, 12.0f), findColour(KLF::accentTextColourId),
                          { 0.0f, 172.0f, width, 34.0f });
    }
    else if (stage == Stage::confirmingOverwrite)
    {
        g.setColour(text);
        g.setFont(titleFont);
        g.drawText("Replace the existing backup?", juce::Rectangle<float>(0.0f, 0.0f, width, 28.0f), juce::Justification::centredLeft, false);

        drawParagraph(g, "A backup named " + getFileName() + " already exists. Overwriting replaces it.",
                      bodyFont, muted, { 0.0f, 34.0f, width, 40.0f });
    }
    else
    {
        const auto iconArea = juce::Rectangle<float>(0.0f, 0.0f, 28.0f, 28.0f);
        g.setColour(findColour(KLF::accentColourId));
        g.strokePath(KronosArt::checkIcon(), juce::PathStrokeType(1.6f), KronosArt::fit(28.0f, iconArea));

        g.setColour(text);
        g.setFont(titleFont);
        g.drawText(getSheetTitle(), juce::Rectangle<float>(40.0f, 0.0f, width - 40.0f, 28.0f), juce::Justification::centredLeft, false);

        const juce::Rectangle<float> box(0.0f, 42.0f, width, 60.0f);
        g.setColour(findColour(KLF::surfaceColourId));
        g.fillRect(box);
        g.setColour(findColour(KLF::lineColourId));
        g.drawRect(box, 1.0f);

        g.setColour(text);
        g.setFont(KLF::font(KLF::Face::monoRegular, 12.0f));
        g.drawFittedText(savedFile.getFileName(), juce::Rectangle<int>(14, 53, getWidth() - 28, 16),
                         juce::Justification::centredLeft, 1, 1.0f);

        g.setColour(faint);
        g.setFont(monoSmall);
        g.drawFittedText(savedFile.getParentDirectory().getFullPathName(), juce::Rectangle<int>(14, 75, getWidth() - 28, 16),
                         juce::Justification::centredLeft, 1, 0.7f);
    }
}

#include "RestoreComponent.h"

using KLF = KronosLookAndFeel;

/**
 * Constructor - Initialize UI components and set up styling
 *
 * @param p Reference to the audio processor for accessing plugin state
 */
RestoreComponent::RestoreComponent(KronosAudioProcessor& p) : audioProcessor(p)
{
    backupListBox.setTitle("Backups");
    backupListBox.setModel(this);
    backupListBox.setRowHeight(40);
    backupListBox.getViewport()->setScrollBarThickness(6);
    addAndMakeVisible(backupListBox);

    browseButton.setButtonText(juce::String(juce::CharPointer_UTF8("Browse\xe2\x80\xa6")));

    for (auto* button : { &cancelButton, &restoreButton, &keepButton, &replaceButton })
        button->setButtonText(button->getName());

    for (auto* button : { &browseButton, &cancelButton, &restoreButton, &keepButton, &replaceButton })
        addChildComponent(*button);

    browseButton.onActivated = [this] { browseForBackup(); };
    cancelButton.onActivated = [this] { cancelOrClose(); };
    restoreButton.onActivated = [this]
    {
        const auto row = backupListBox.getSelectedRow();
        if (juce::isPositiveAndBelow(row, backups.size()))
            beginConfirmation(backups.getReference(row).file);
    };
    keepButton.onActivated = [this] { if (onClose) onClose(); };
    replaceButton.onActivated = [this] { confirmReplace(); };

    refreshBackupList();
    setStage(Stage::choosing);
}

juce::String RestoreComponent::getKicker() const
{
    return juce::String(juce::CharPointer_UTF8("RESTORE \xc2\xb7 \xce\x91\xce\x9d\xce\x91\xce\x9c\xce\x9d\xce\x97\xce\xa3\xce\x99\xce\xa3"));
}

juce::String RestoreComponent::getSheetTitle() const
{
    return stage == Stage::confirming ? juce::String(juce::CharPointer_UTF8("Replace this project\xe2\x80\x99s log?"))
                                      : juce::String("Restore session data");
}

int RestoreComponent::getPreferredHeight() const
{
    if (stage == Stage::confirming)
        return errorText.isEmpty() ? 192 : 214;

    return errorText.isEmpty() ? 212 : 234;
}

void RestoreComponent::focusDefaultControl()
{
    // Keeping the log is the default: replacing it should take a deliberate move
    if (stage == Stage::confirming && keepButton.isShowing())
        keepButton.grabKeyboardFocus();
    else if (stage == Stage::choosing && backupListBox.isShowing() && !backups.isEmpty())
        backupListBox.grabKeyboardFocus();
    else if (stage == Stage::choosing && browseButton.isShowing())
        browseButton.grabKeyboardFocus();
}

void RestoreComponent::cancelOrClose()
{
    if (stage == Stage::confirming)
    {
        pendingRestoreFile = juce::File();
        pendingRestoreData.reset();
        setStage(Stage::choosing);
    }
    else if (onClose)
    {
        onClose();
    }
}

void RestoreComponent::setStage(Stage newStage)
{
    stage = newStage;
    errorText.clear();

    const auto choosing = stage == Stage::choosing;
    backupListBox.setVisible(choosing);
    browseButton.setVisible(choosing);
    cancelButton.setVisible(choosing);
    restoreButton.setVisible(choosing);
    keepButton.setVisible(!choosing);
    replaceButton.setVisible(!choosing);

    resized();
    repaint();

    if (onLayoutChanged)
        onLayoutChanged();

    focusDefaultControl();
}

void RestoreComponent::resized()
{
    auto area = getLocalBounds();
    auto footer = area.removeFromBottom(36);

    if (stage == Stage::choosing)
    {
        backupListBox.setBounds(1, 41, getWidth() - 2, 122);

        browseButton.setBounds(footer.removeFromLeft(96));
        restoreButton.setBounds(footer.removeFromRight(88));
        footer.removeFromRight(8);
        cancelButton.setBounds(footer.removeFromRight(84));
    }
    else
    {
        replaceButton.setBounds(footer.removeFromRight(112));
        footer.removeFromRight(8);
        keepButton.setBounds(footer.removeFromRight(112));
    }
}

void RestoreComponent::paint(juce::Graphics& g)
{
    const auto text = findColour(KLF::textColourId);
    const auto muted = findColour(KLF::mutedColourId);
    const auto faint = findColour(KLF::faintColourId);
    const auto line = findColour(KLF::lineColourId);
    const auto width = static_cast<float>(getWidth());

    g.setColour(text);
    g.setFont(KLF::font(KLF::Face::sansSemibold, 22.0f));
    g.drawText(getSheetTitle(), juce::Rectangle<float>(0.0f, 0.0f, width, 28.0f), juce::Justification::centredLeft, false);

    if (stage == Stage::choosing)
    {
        g.setColour(line);
        g.drawRect(juce::Rectangle<float>(0.0f, 40.0f, width, 124.0f), 1.0f);

        if (backups.isEmpty())
        {
            g.setColour(faint);
            g.setFont(KLF::font(KLF::Face::sansRegular, 13.0f));
            g.drawText(juce::String(juce::CharPointer_UTF8("No backups in the Kronos folder yet. Use Browse\xe2\x80\xa6 to pick a file.")),
                       juce::Rectangle<float>(16.0f, 40.0f, width - 32.0f, 124.0f), juce::Justification::centred, false);
        }

        if (errorText.isNotEmpty())
            drawParagraph(g, errorText, KLF::font(KLF::Face::sansRegular, 12.0f), findColour(KLF::accentTextColourId),
                          { 0.0f, 170.0f, width, 18.0f });

        return;
    }

    drawParagraph(g, "The current log is overwritten. Back it up first if you may want it again.",
                  KLF::font(KLF::Face::sansRegular, 13.0f), muted, { 0.0f, 32.0f, width, 38.0f });

    const juce::Rectangle<float> grid(0.0f, 64.0f, width, 76.0f);
    g.setColour(line);
    g.fillRect(grid);

    const auto columnWidth = (width - 3.0f) / 2.0f;
    const auto drawColumn = [&](float x, const juce::String& label, juce::Colour labelColour,
                                juce::int64 seconds, const juce::String& caption)
    {
        const juce::Rectangle<float> cell(x, grid.getY() + 1.0f, columnWidth, grid.getHeight() - 2.0f);
        g.setColour(findColour(KLF::surfaceColourId));
        g.fillRect(cell);

        g.setColour(labelColour);
        g.setFont(KLF::font(KLF::Face::sansSemibold, 11.0f, 0.12f));
        g.drawText(label, cell.withTrimmedLeft(12.0f).withY(cell.getY() + 10.0f).withHeight(14.0f),
                   juce::Justification::centredLeft, false);

        g.setColour(text);
        g.setFont(KLF::font(KLF::Face::monoRegular, 16.0f));
        g.drawText(KronosArt::formatDuration(seconds), cell.withTrimmedLeft(12.0f).withY(cell.getY() + 28.0f).withHeight(20.0f),
                   juce::Justification::centredLeft, false);

        g.setColour(faint);
        g.setFont(KLF::font(KLF::Face::sansRegular, 11.0f));
        g.drawFittedText(caption, cell.withTrimmedLeft(12.0f).withTrimmedRight(8.0f).withY(cell.getY() + 52.0f).withHeight(14.0f).toNearestInt(),
                         juce::Justification::centredLeft, 1, 0.8f);
    };

    drawColumn(1.0f, "CURRENT", muted, audioProcessor.getTotalTimeInSeconds(),
               KronosArt::formatDays(audioProcessor.getDaysLogged()));
    drawColumn(columnWidth + 2.0f, "FROM BACKUP", findColour(KLF::accentTextColourId), pendingSummary.totalSeconds,
               pendingRestoreFile.getFileName());

    if (errorText.isNotEmpty())
        drawParagraph(g, errorText, KLF::font(KLF::Face::sansRegular, 12.0f), findColour(KLF::accentTextColourId),
                      { 0.0f, 148.0f, width, 18.0f });
}

/**
 * Refresh the list of available backup files
 */
void RestoreComponent::refreshBackupList()
{
    backups.clear();

    juce::File backupDir(getBackupDirectory());

    if (backupDir.isDirectory())
    {
        // Get all files with .kronos extension
        auto files = backupDir.findChildFiles(juce::File::findFiles, false, "*.kronos");

        // Sort newest first
        std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b)
        {
            return a.getCreationTime() > b.getCreationTime();
        });

        for (const auto& file : files)
        {
            juce::MemoryBlock data;
            Backup backup { file, {} };

            if (file.loadFileAsData(data))
                backup.summary = KronosAudioProcessor::summariseBackup(data.getData(), static_cast<int>(data.getSize()));

            backups.add(backup);
        }
    }

    backupListBox.updateContent();
    backupListBox.repaint();

    if (!backups.isEmpty())
        backupListBox.selectRow(0);

    restoreButton.setEnabled(!backups.isEmpty());
}

/**
 * Get the platform-specific backup directory path
 *
 * @return A string with the full backup directory path
 */
juce::String RestoreComponent::getBackupDirectory() const
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

bool RestoreComponent::beginConfirmation(const juce::File& file)
{
    juce::MemoryBlock data;

    if (!file.loadFileAsData(data))
    {
        showError("Could not read " + file.getFileName());
        return false;
    }

    const auto summary = KronosAudioProcessor::summariseBackup(data.getData(), static_cast<int>(data.getSize()));

    if (!summary.isValid)
    {
        showError(file.getFileName() + " is not a valid Kronos backup.");
        return false;
    }

    pendingRestoreFile = file;
    pendingRestoreData = data;
    pendingSummary = summary;
    setStage(Stage::confirming);
    return true;
}

bool RestoreComponent::confirmReplace()
{
    if (pendingRestoreFile == juce::File())
        return false;

    if (audioProcessor.restoreStateFromData(pendingRestoreData.getData(), static_cast<int>(pendingRestoreData.getSize())))
    {
        if (onClose)
            onClose();

        return true;
    }

    showError(pendingRestoreFile.getFileName() + " could not be restored. Your log was not changed.");
    return false;
}

void RestoreComponent::showError(const juce::String& message)
{
    errorText = message;

    if (onLayoutChanged)
        onLayoutChanged();

    resized();
    repaint();
}

void RestoreComponent::browseForBackup()
{
    fileChooser = std::make_unique<juce::FileChooser>("Select Backup File",
                                                      juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                                      "*.kronos");

    auto chooserFlags = juce::FileBrowserComponent::openMode |
                       juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        auto selectedFile = chooser.getResult();
        if (selectedFile.existsAsFile())
            beginConfirmation(selectedFile);
    });
}

//==============================================================================
// ListBoxModel implementation
//==============================================================================

void RestoreComponent::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                        int width, int height, bool rowIsSelected)
{
    if (!juce::isPositiveAndBelow(rowNumber, backups.size()))
        return;

    const auto& backup = backups.getReference(rowNumber);
    const auto accent = findColour(KLF::accentColourId);
    const auto faint = findColour(KLF::faintColourId);
    const auto bounds = juce::Rectangle<int>(width, height).toFloat();

    if (rowIsSelected)
    {
        g.setColour(findColour(KLF::surfaceColourId));
        g.fillRect(bounds);
    }

    g.setColour(findColour(KLF::lineColourId));
    g.fillRect(bounds.withTop(bounds.getBottom() - 1.0f));

    const auto marker = juce::Rectangle<float>(12.0f, (bounds.getHeight() - 10.0f) * 0.5f, 10.0f, 10.0f);

    if (rowIsSelected)
    {
        g.setColour(accent);
        g.fillRect(marker);
    }
    else
    {
        g.setColour(faint);
        g.drawRect(marker, 1.0f);
    }

    const auto totalText = backup.summary.isValid ? KronosArt::formatDuration(backup.summary.totalSeconds)
                                                  : juce::String("Unreadable");
    const auto totalFont = KLF::font(KLF::Face::monoRegular, 12.0f);
    const auto totalWidth = totalFont.getStringWidthFloat(totalText) + 2.0f;
    const auto textArea = bounds.withLeft(34.0f).withTrimmedRight(totalWidth + 24.0f);

    g.setColour(findColour(KLF::textColourId));
    g.setFont(totalFont);
    g.drawFittedText(backup.file.getFileName(), textArea.withTrimmedTop(5.0f).withHeight(16.0f).toNearestInt(),
                     juce::Justification::centredLeft, 1, 0.8f);

    const auto created = backup.file.getCreationTime();
    g.setColour(faint);
    g.setFont(KLF::font(KLF::Face::sansRegular, 11.0f));
    g.drawText(KronosArt::formatDate(created) + juce::String(juce::CharPointer_UTF8(" \xc2\xb7 ")) + created.formatted("%H:%M"),
               textArea.withTrimmedTop(21.0f).withHeight(14.0f), juce::Justification::centredLeft, false);

    g.setColour(findColour(KLF::mutedColourId));
    g.setFont(totalFont);
    g.drawText(totalText, bounds.withTrimmedRight(12.0f), juce::Justification::centredRight, false);
}

void RestoreComponent::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (juce::isPositiveAndBelow(row, backups.size()))
        beginConfirmation(backups.getReference(row).file);
}

void RestoreComponent::selectedRowsChanged(int lastRowSelected)
{
    restoreButton.setEnabled(juce::isPositiveAndBelow(lastRowSelected, backups.size()));
}

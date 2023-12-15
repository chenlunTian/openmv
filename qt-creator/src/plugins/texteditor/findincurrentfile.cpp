// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findincurrentfile.h"

#include "textdocument.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/filesearch.h>
#include <utils/fileutils.h>

#include <QSettings>

// OPENMV-DIFF //
#include <QLabel>
// OPENMV-DIFF //

namespace TextEditor::Internal {

FindInCurrentFile::FindInCurrentFile()
{
    connect(Core::EditorManager::instance(), &Core::EditorManager::currentEditorChanged,
            this, &FindInCurrentFile::handleFileChange);
    handleFileChange(Core::EditorManager::currentEditor());
}

QString FindInCurrentFile::id() const
{
    return QLatin1String("Current File");
}

QString FindInCurrentFile::displayName() const
{
    return Tr::tr("Current File");
}

Utils::FileIterator *FindInCurrentFile::files(const QStringList &nameFilters,
                                              const QStringList &exclusionFilters,
                                              const QVariant &additionalParameters) const
{
    Q_UNUSED(nameFilters)
    Q_UNUSED(exclusionFilters)
    const auto fileName = Utils::FilePath::fromVariant(additionalParameters);
    QMap<Utils::FilePath, QTextCodec *> openEditorEncodings
        = TextDocument::openedTextDocumentEncodings();
    QTextCodec *codec = openEditorEncodings.value(fileName);
    if (!codec)
        codec = Core::EditorManager::defaultTextCodec();
    return new Utils::FileListIterator({fileName}, {codec});
}

QVariant FindInCurrentFile::additionalParameters() const
{
    return m_currentDocument->filePath().toVariant();
}

QString FindInCurrentFile::label() const
{
    return Tr::tr("File \"%1\":").arg(m_currentDocument->filePath().fileName());
}

QString FindInCurrentFile::toolTip() const
{
    // %2 is filled by BaseFileFind::runNewSearch
    return Tr::tr("File path: %1\n%2").arg(m_currentDocument->filePath().toUserOutput());
}

// OPENMV-DIFF //
QWidget *FindInCurrentFile::createConfigWidget()
{
    if (!m_configWidget) {
        QLabel *label = new QLabel(Tr::tr("Please note that this only searches files that have been saved to disk."));
        label->setAlignment(Qt::AlignRight);
        m_configWidget = label;
    }
    return m_configWidget;
}
// OPENMV-DIFF //

bool FindInCurrentFile::isEnabled() const
{
    return m_currentDocument && !m_currentDocument->filePath().isEmpty();
}

void FindInCurrentFile::handleFileChange(Core::IEditor *editor)
{
    if (!editor) {
        m_currentDocument = nullptr;
        emit enabledChanged(isEnabled());
    } else {
        Core::IDocument *document = editor->document();
        if (document != m_currentDocument) {
            m_currentDocument = document;
            emit enabledChanged(isEnabled());
        }
    }
}


void FindInCurrentFile::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInCurrentFile"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInCurrentFile::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInCurrentFile"));
    readCommonSettings(settings, "*", "");
    settings->endGroup();
}

} // TextEditor::Internal

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outputwindow.h"

#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "coreplugin.h"
#include "coreplugintr.h"
#include "editormanager/editormanager.h"
#include "find/basetextfind.h"
#include "icore.h"

#include <aggregation/aggregate.h>

#include <utils/outputformatter.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QCursor>
#include <QElapsedTimer>
#include <QHash>
#include <QMimeData>
#include <QPair>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTimer>

// OPENMV-DIFF //
#include <utils/fadingindicator.h>
#include <utils/fileutils.h>
#include <extensionsystem/pluginmanager.h>
#include <QApplication>
#include <QRegularExpression>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
// OPENMV-DIFF //

#ifdef WITH_TESTS
#include <QtTest>
#endif

#include <numeric>

const int chunkSize = 10000;

using namespace Utils;

namespace Core {

namespace Internal {

class OutputWindowPrivate
{
public:
    explicit OutputWindowPrivate(QTextDocument *document)
        : cursor(document)
    {
    }

    QString settingsKey;
    OutputFormatter formatter;
    QList<QPair<QString, OutputFormat>> queuedOutput;
    QTimer queueTimer;

    bool flushRequested = false;
    bool scrollToBottom = true;
    bool linksActive = true;
    bool zoomEnabled = false;
    float originalFontSize = 0.;
    bool originalReadOnly = false;
    int maxCharCount = Core::Constants::DEFAULT_MAX_CHAR_COUNT;
    Qt::MouseButton mouseButtonPressed = Qt::NoButton;
    QTextCursor cursor;
    QString filterText;
    int lastFilteredBlockNumber = -1;
    QPalette originalPalette;
    OutputWindow::FilterModeFlags filterMode = OutputWindow::FilterModeFlag::Default;
    QTimer scrollTimer;
    QElapsedTimer lastMessage;
    QHash<unsigned int, QPair<int, int>> taskPositions;
    // OPENMV-DIFF //
    bool enforceNewline = false;
    int tabWidth = 4;
    // OPENMV-DIFF //
};

} // namespace Internal

/*******************/

OutputWindow::OutputWindow(Context context, const QString &settingsKey, QWidget *parent)
    : QPlainTextEdit(parent)
    , d(new Internal::OutputWindowPrivate(document()))
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setCenterOnScroll(false);
    setFrameShape(QFrame::NoFrame);
    setMouseTracking(true);
    setUndoRedoEnabled(false);
    d->formatter.setPlainTextEdit(this);

    d->queueTimer.setSingleShot(true);
    d->queueTimer.setInterval(10);
    connect(&d->queueTimer, &QTimer::timeout, this, &OutputWindow::handleNextOutputChunk);

    d->settingsKey = settingsKey;

    auto outputWindowContext = new IContext(this);
    outputWindowContext->setContext(context);
    outputWindowContext->setWidget(this);
    ICore::addContextObject(outputWindowContext);

    auto undoAction = new QAction(this);
    auto redoAction = new QAction(this);
    auto cutAction = new QAction(this);
    auto copyAction = new QAction(this);
    auto pasteAction = new QAction(this);
    auto selectAllAction = new QAction(this);

    ActionManager::registerAction(undoAction, Constants::UNDO, context);
    ActionManager::registerAction(redoAction, Constants::REDO, context);
    ActionManager::registerAction(cutAction, Constants::CUT, context);
    ActionManager::registerAction(copyAction, Constants::COPY, context);
    ActionManager::registerAction(pasteAction, Constants::PASTE, context);
    ActionManager::registerAction(selectAllAction, Constants::SELECTALL, context);

    connect(undoAction, &QAction::triggered, this, &QPlainTextEdit::undo);
    connect(redoAction, &QAction::triggered, this, &QPlainTextEdit::redo);
    connect(cutAction, &QAction::triggered, this, &QPlainTextEdit::cut);
    connect(copyAction, &QAction::triggered, this, &QPlainTextEdit::copy);
    connect(pasteAction, &QAction::triggered, this, &QPlainTextEdit::paste);
    connect(selectAllAction, &QAction::triggered, this, &QPlainTextEdit::selectAll);
    connect(this, &QPlainTextEdit::blockCountChanged, this, [this] {
        if (!d->filterText.isEmpty())
            filterNewContent();
    });

    connect(this, &QPlainTextEdit::undoAvailable, undoAction, &QAction::setEnabled);
    connect(this, &QPlainTextEdit::redoAvailable, redoAction, &QAction::setEnabled);
    connect(this, &QPlainTextEdit::copyAvailable, cutAction, &QAction::setEnabled);  // OutputWindow never read-only
    connect(this, &QPlainTextEdit::copyAvailable, copyAction, &QAction::setEnabled);
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested, this, [this] {
        if (!d->settingsKey.isEmpty())
            Core::ICore::settings()->setValueWithDefault(d->settingsKey, fontZoom(), 0.f);
    });

    connect(outputFormatter(), &OutputFormatter::openInEditorRequested, this, [](const Link &link) {
        EditorManager::openEditorAt(link);
    });

    connect(verticalScrollBar(), &QAbstractSlider::actionTriggered,
            this, &OutputWindow::updateAutoScroll);

    // For when "Find" changes the position; see QTCREATORBUG-26100.
    connect(this, &QPlainTextEdit::selectionChanged, this, &OutputWindow::updateAutoScroll,
            Qt::QueuedConnection);

    undoAction->setEnabled(false);
    redoAction->setEnabled(false);
    cutAction->setEnabled(false);
    copyAction->setEnabled(false);

    d->scrollTimer.setInterval(10);
    d->scrollTimer.setSingleShot(true);
    connect(&d->scrollTimer, &QTimer::timeout,
            this, &OutputWindow::scrollToBottom);
    d->lastMessage.start();

    d->originalFontSize = font().pointSizeF();

    if (!d->settingsKey.isEmpty()) {
        float zoom = Core::ICore::settings()->value(d->settingsKey).toFloat();
        setFontZoom(zoom);
    }

    // Let selected text be colored as if the text edit was editable,
    // otherwise the highlight for searching is too light
    QPalette p = palette();
    QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
    p.setColor(QPalette::Highlight, activeHighlight);
    QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
    p.setColor(QPalette::HighlightedText, activeHighlightedText);
    setPalette(p);

    auto agg = new Aggregation::Aggregate;
    agg->add(this);
    agg->add(new BaseTextFind(this));
    // OPENMV-DIFF //
    m_parser = new OpenMVPluginEscapeCodeParser(this);
    // OPENMV-DIFF //
}

OutputWindow::~OutputWindow()
{
    delete d;
}

void OutputWindow::mousePressEvent(QMouseEvent *e)
{
    d->mouseButtonPressed = e->button();
    QPlainTextEdit::mousePressEvent(e);
}

void OutputWindow::handleLink(const QPoint &pos)
{
    const QString href = anchorAt(pos);
    if (!href.isEmpty())
        d->formatter.handleLink(href);
}

void OutputWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (d->linksActive && d->mouseButtonPressed == Qt::LeftButton)
        handleLink(e->pos());

    // Mouse was released, activate links again
    d->linksActive = true;
    d->mouseButtonPressed = Qt::NoButton;

    QPlainTextEdit::mouseReleaseEvent(e);
}

void OutputWindow::mouseMoveEvent(QMouseEvent *e)
{
    // Cursor was dragged to make a selection, deactivate links
    if (d->mouseButtonPressed != Qt::NoButton && textCursor().hasSelection())
        d->linksActive = false;

    if (!d->linksActive || anchorAt(e->pos()).isEmpty())
        viewport()->setCursor(Qt::IBeamCursor);
    else
        viewport()->setCursor(Qt::PointingHandCursor);
    QPlainTextEdit::mouseMoveEvent(e);
}

void OutputWindow::resizeEvent(QResizeEvent *e)
{
    //Keep scrollbar at bottom of window while resizing, to ensure we keep scrolling
    //This can happen if window is resized while building, or if the horizontal scrollbar appears
    QPlainTextEdit::resizeEvent(e);
    if (d->scrollToBottom)
        scrollToBottom();
}

void OutputWindow::keyPressEvent(QKeyEvent *ev)
{
    QPlainTextEdit::keyPressEvent(ev);

    //Ensure we scroll also on Ctrl+Home or Ctrl+End
    if (ev->matches(QKeySequence::MoveToStartOfDocument))
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
    else if (ev->matches(QKeySequence::MoveToEndOfDocument))
        verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
}

void OutputWindow::setLineParsers(const QList<OutputLineParser *> &parsers)
{
    reset();
    d->formatter.setLineParsers(parsers);
}

OutputFormatter *OutputWindow::outputFormatter() const
{
    return &d->formatter;
}

void OutputWindow::showEvent(QShowEvent *e)
{
    QPlainTextEdit::showEvent(e);
    if (d->scrollToBottom)
        scrollToBottom();
}

void OutputWindow::wheelEvent(QWheelEvent *e)
{
    if (d->zoomEnabled) {
        if (e->modifiers() & Qt::ControlModifier) {
            float delta = e->angleDelta().y() / 120.f;

            // Workaround for QTCREATORBUG-22721, remove when properly fixed in Qt
            const float newSize = float(font().pointSizeF()) + delta;
            if (delta < 0.f && newSize < 4.f)
                return;

            zoomInF(delta);
            emit wheelZoom();
            // OPENMV-DIFF //
            Utils::FadingIndicator::showText(this, Tr::tr("Zoom: %1%").arg(int(100 * (font().pointSizeF() / d->originalFontSize))), Utils::FadingIndicator::SmallText);
            // OPENMV-DIFF //
            return;
        }
    }
    QAbstractScrollArea::wheelEvent(e);
    updateAutoScroll();
    updateMicroFocus();
}

void OutputWindow::setBaseFont(const QFont &newFont)
{
    float zoom = fontZoom();
    d->originalFontSize = newFont.pointSizeF();
    QFont tmp = newFont;
    float newZoom = qMax(d->originalFontSize + zoom, 4.0f);
    tmp.setPointSizeF(newZoom);
    setFont(tmp);
}

float OutputWindow::fontZoom() const
{
    return font().pointSizeF() - d->originalFontSize;
}

void OutputWindow::setFontZoom(float zoom)
{
    QFont f = font();
    if (f.pointSizeF() == d->originalFontSize + zoom)
        return;
    float newZoom = qMax(d->originalFontSize + zoom, 4.0f);
    f.setPointSizeF(newZoom);
    setFont(f);
}

void OutputWindow::setWheelZoomEnabled(bool enabled)
{
    d->zoomEnabled = enabled;
}

void OutputWindow::updateFilterProperties(
        const QString &filterText,
        Qt::CaseSensitivity caseSensitivity,
        bool isRegexp,
        bool isInverted
        )
{
    FilterModeFlags flags;
    flags.setFlag(FilterModeFlag::CaseSensitive, caseSensitivity == Qt::CaseSensitive)
            .setFlag(FilterModeFlag::RegExp, isRegexp)
            .setFlag(FilterModeFlag::Inverted, isInverted);
    if (d->filterMode == flags && d->filterText == filterText)
        return;
    d->lastFilteredBlockNumber = -1;
    if (d->filterText != filterText) {
        const bool filterTextWasEmpty = d->filterText.isEmpty();
        d->filterText = filterText;

        // Update textedit's background color
        if (filterText.isEmpty() && !filterTextWasEmpty) {
            setPalette(d->originalPalette);
            setReadOnly(d->originalReadOnly);
        }
        if (!filterText.isEmpty() && filterTextWasEmpty) {
            d->originalReadOnly = isReadOnly();
            setReadOnly(true);
            const auto newBgColor = [this] {
                const QColor currentColor = palette().color(QPalette::Base);
                const int factor = 120;
                return currentColor.value() < 128 ? currentColor.lighter(factor)
                                                  : currentColor.darker(factor);
            };
            QPalette p = palette();
            p.setColor(QPalette::Base, newBgColor());
            setPalette(p);
        }
    }
    d->filterMode = flags;
    filterNewContent();
}

void OutputWindow::filterNewContent()
{
    QTextBlock lastBlock = document()->findBlockByNumber(d->lastFilteredBlockNumber);
    if (!lastBlock.isValid())
        lastBlock = document()->begin();

    const bool invert = d->filterMode.testFlag(FilterModeFlag::Inverted);
    if (d->filterMode.testFlag(OutputWindow::FilterModeFlag::RegExp)) {
        QRegularExpression regExp(d->filterText);
        if (!d->filterMode.testFlag(OutputWindow::FilterModeFlag::CaseSensitive))
            regExp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);

        for (; lastBlock != document()->end(); lastBlock = lastBlock.next())
            lastBlock.setVisible(d->filterText.isEmpty()
                                 || regExp.match(lastBlock.text()).hasMatch() != invert);
    } else {
        if (d->filterMode.testFlag(OutputWindow::FilterModeFlag::CaseSensitive)) {
            for (; lastBlock != document()->end(); lastBlock = lastBlock.next())
                lastBlock.setVisible(d->filterText.isEmpty()
                                     || lastBlock.text().contains(d->filterText) != invert);
        } else {
            for (; lastBlock != document()->end(); lastBlock = lastBlock.next()) {
                lastBlock.setVisible(d->filterText.isEmpty() || lastBlock.text().toLower()
                                     .contains(d->filterText.toLower()) != invert);
            }
        }
    }

    d->lastFilteredBlockNumber = document()->lastBlock().blockNumber();

    // FIXME: Why on earth is this necessary? We should probably do something else instead...
    setDocument(document());

    if (d->scrollToBottom)
        scrollToBottom();
}

void OutputWindow::handleNextOutputChunk()
{
    QTC_ASSERT(!d->queuedOutput.isEmpty(), return);
    auto &chunk = d->queuedOutput.first();
    if (chunk.first.size() <= chunkSize) {
        handleOutputChunk(chunk.first, chunk.second);
        d->queuedOutput.removeFirst();
    } else {
        handleOutputChunk(chunk.first.left(chunkSize), chunk.second);
        chunk.first.remove(0, chunkSize);
    }
    if (!d->queuedOutput.isEmpty())
        d->queueTimer.start();
    else if (d->flushRequested) {
        d->formatter.flush();
        d->flushRequested = false;
    }
}

void OutputWindow::handleOutputChunk(const QString &output, OutputFormat format)
{
    QString out = output;
    if (out.size() > d->maxCharCount) {
        // Current chunk alone exceeds limit, we need to cut it.
        const int elided = out.size() - d->maxCharCount;
        out = out.left(d->maxCharCount / 2)
                + "[[[... "
                + Tr::tr("Elided %n characters due to Application Output settings", nullptr, elided)
                + " ...]]]"
                + out.right(d->maxCharCount / 2);
        setMaximumBlockCount(out.count('\n') + 1);
    } else {
        int plannedChars = document()->characterCount() + out.size();
        if (plannedChars > d->maxCharCount) {
            int plannedBlockCount = document()->blockCount();
            QTextBlock tb = document()->firstBlock();
            while (tb.isValid() && plannedChars > d->maxCharCount && plannedBlockCount > 1) {
                plannedChars -= tb.length();
                plannedBlockCount -= 1;
                tb = tb.next();
            }
            setMaximumBlockCount(plannedBlockCount);
        } else {
            setMaximumBlockCount(-1);
        }
    }

    d->formatter.appendMessage(out, format);

    if (d->scrollToBottom) {
        if (d->lastMessage.elapsed() < 5) {
            d->scrollTimer.start();
        } else {
            d->scrollTimer.stop();
            scrollToBottom();
        }
    }

    d->lastMessage.start();
    enableUndoRedo();
}

void OutputWindow::updateAutoScroll()
{
    d->scrollToBottom = verticalScrollBar()->sliderPosition() >= verticalScrollBar()->maximum() - 1;
}

void OutputWindow::setMaxCharCount(int count)
{
    d->maxCharCount = count;
    setMaximumBlockCount(count / 100);
}

int OutputWindow::maxCharCount() const
{
    return d->maxCharCount;
}

void OutputWindow::appendMessage(const QString &output, OutputFormat format)
{
    if (d->queuedOutput.isEmpty() || d->queuedOutput.last().second != format)
        d->queuedOutput.push_back({output, format});
    else
        d->queuedOutput.last().first.append(output);
    if (!d->queueTimer.isActive())
        d->queueTimer.start();
}

void OutputWindow::registerPositionOf(unsigned taskId, int linkedOutputLines, int skipLines,
                                      int offset)
{
    if (linkedOutputLines <= 0)
        return;

    const int blocknumber = document()->blockCount() - offset;
    const int firstLine = blocknumber - linkedOutputLines - skipLines;
    const int lastLine = firstLine + linkedOutputLines - 1;

    d->taskPositions.insert(taskId, {firstLine, lastLine});
}

bool OutputWindow::knowsPositionOf(unsigned taskId) const
{
    return d->taskPositions.contains(taskId);
}

void OutputWindow::showPositionOf(unsigned taskId)
{
    QPair<int, int> position = d->taskPositions.value(taskId);
    QTextCursor newCursor(document()->findBlockByNumber(position.second));

    // Move cursor to end of last line of interest:
    newCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::MoveAnchor);
    setTextCursor(newCursor);

    // Move cursor and select lines:
    newCursor.setPosition(document()->findBlockByNumber(position.first).position(),
                          QTextCursor::KeepAnchor);
    setTextCursor(newCursor);

    // Center cursor now:
    centerCursor();
}

QMimeData *OutputWindow::createMimeDataFromSelection() const
{
    const auto mimeData = new QMimeData;
    QString content;
    const int selStart = textCursor().selectionStart();
    const int selEnd = textCursor().selectionEnd();
    const QTextBlock firstBlock = document()->findBlock(selStart);
    const QTextBlock lastBlock = document()->findBlock(selEnd);
    for (QTextBlock curBlock = firstBlock; curBlock != lastBlock; curBlock = curBlock.next()) {
        if (!curBlock.isVisible())
            continue;
        if (curBlock == firstBlock)
            content += curBlock.text().mid(selStart - firstBlock.position());
        else
            content += curBlock.text();
        content += '\n';
    }
    if (lastBlock.isValid() && lastBlock.isVisible()) {
        if (firstBlock == lastBlock)
            content = textCursor().selectedText();
        else
            content += lastBlock.text().mid(0, selEnd - lastBlock.position());
    }
    mimeData->setText(content);
    return mimeData;
}

void OutputWindow::clear()
{
    d->formatter.clear();
    d->scrollToBottom = true;
    d->taskPositions.clear();
}

void OutputWindow::flush()
{
    const int totalQueuedSize = std::accumulate(d->queuedOutput.cbegin(), d->queuedOutput.cend(), 0,
            [](int val,  const QPair<QString, OutputFormat> &c) { return val + c.first.size(); });
    if (totalQueuedSize > 5 * chunkSize) {
        d->flushRequested = true;
        return;
    }
    d->queueTimer.stop();
    for (const auto &chunk : std::as_const(d->queuedOutput))
        handleOutputChunk(chunk.first, chunk.second);
    d->queuedOutput.clear();
    d->formatter.flush();
}

void OutputWindow::reset()
{
    flush();
    d->queueTimer.stop();
    d->formatter.reset();
    d->scrollToBottom = true;
    if (!d->queuedOutput.isEmpty()) {
        d->queuedOutput.clear();
        d->formatter.appendMessage(Tr::tr("[Discarding excessive amount of pending output.]\n"),
                                   ErrorMessageFormat);
    }
    d->flushRequested = false;
}

void OutputWindow::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    // QPlainTextEdit destroys the first calls value in case of multiline
    // text, so make sure that the scroll bar actually gets the value set.
    // Is a noop if the first call succeeded.
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

void OutputWindow::grayOutOldContent()
{
    // OPENMV-DIFF //
    if(document()->isEmpty())
        return;
    d->enforceNewline = true;
    const bool atBottom = isScrollbarAtBottom();
    // OPENMV-DIFF //
    if (!d->cursor.atEnd())
        d->cursor.movePosition(QTextCursor::End);
    QTextCharFormat endFormat = d->cursor.charFormat();

    d->cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    const QColor bkgColor = palette().base().color();
    const QColor fgdColor = palette().text().color();
    double bkgFactor = 0.50;
    double fgdFactor = 1.-bkgFactor;
    format.setForeground(QColor((bkgFactor * bkgColor.red() + fgdFactor * fgdColor.red()),
                             (bkgFactor * bkgColor.green() + fgdFactor * fgdColor.green()),
                             (bkgFactor * bkgColor.blue() + fgdFactor * fgdColor.blue()) ));
    d->cursor.mergeCharFormat(format);

    d->cursor.movePosition(QTextCursor::End);
    d->cursor.setCharFormat(endFormat);
    d->cursor.insertBlock(QTextBlockFormat());
    // OPENMV-DIFF //
    if (atBottom)
        scrollToBottom();
    // OPENMV-DIFF //
}

void OutputWindow::enableUndoRedo()
{
    setMaximumBlockCount(0);
    setUndoRedoEnabled(true);
}

void OutputWindow::setWordWrapEnabled(bool wrap)
{
    if (wrap)
        setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    else
        setWordWrapMode(QTextOption::NoWrap);
}

// OPENMV-DIFF //
QString OutputWindow::doNewlineEnforcement(const QString &out)
{
    d->scrollToBottom = true;
    QString s = out;
    if (d->enforceNewline) {
        s.prepend(QLatin1Char('\n'));
        d->enforceNewline = false;
    }

    if (s.endsWith(QLatin1Char('\n'))) {
        d->enforceNewline = true; // make appendOutputInline put in a newline next time
        s.chop(1);
    }

    return s;
}

void OutputWindow::appendText(const QString &textIn)
{
    foreach(const Utils::FormattedText &parsedText, m_handler.parseText(Utils::FormattedText(textIn)))
    {
        if(parsedText.text.isEmpty())
        {
            m_parser->parseEscapeCodes(m_handler.getEscapeCodes());
        }

        QTextCharFormat format = parsedText.format;
        QString text = QString(parsedText.text).replace(QLatin1String("\r\n"), QLatin1String("\n"));
        int index = text.indexOf(QStringLiteral("Traceback (most recent call last):\n"));

        if(index != -1)
        {
            m_handler.endFormatScope();
            m_parser->resetParser();
            appendText(text.left(index));
            grayOutOldContent();
            format = QTextCharFormat();
            text = text.mid(index);
        }

        text = text.replace(QStringLiteral("MPY: soft reboot\n"), QStringLiteral(""));

        QChar lastChar = QChar();
        QString string;
        int column = 0;

        for(int i = 0, j = text.size(); i < j; i++)
        {
            switch(text.at(i).unicode())
            {
                case 15:
                case 17: // XON
                case 19: // XOFF
                case 23:
                case 24:
                case 25:
                case 26:
                case 27: // Escape - AnsiEscapeCodeHandler
                case 28:
                case 29:
                case 30:
                case 31:
                {
                    break;
                }

                case 0: // Null
                {
                    break;
                }

                case 1: // Home Cursor
                {
                    break;
                }

                case 2: // Move Cursor Left
                {
                    break;
                }

                case 3: // Clear Screen
                {
                    break;
                }

                case 4:
                case 127: // Delete
                {
                    break;
                }

                case 5: // End Cursor
                {
                    break;
                }

                case 6: // Move Cursor Right
                {
                    break;
                }

                case 7: // Beep Speaker
                {
                    QApplication::beep();
                    break;
                }

                case 8: // Backspace
                {
                    break;
                }

                case 9: // Tab
                {
                    for(int k = d->tabWidth - (column % d->tabWidth); k > 0; k--)
                    {
                        string.append(QLatin1Char(' '));
                        column += 1;
                    }

                    break;
                }

                case 10: // Line Feed
                {
                    if(lastChar.unicode() != '\r')
                    {
                        string.append(QLatin1Char('\n'));
                        column = 0;
                    }

                    break;
                }

                case 11: // Clear to end of line.
                {
                    break;
                }

                case 12: // Clear lines below.
                {
                    break;
                }

                case 13: // Carriage Return
                {
                    string.append(QLatin1Char('\n'));
                    column = 0;
                    break;
                }

                case 14: // Move Cursor Down
                {
                    break;
                }

                case 16: // Move Cursor Up
                {
                    break;
                }

                case 18: // Move to start of line.
                {
                    break;
                }

                case 20: // Move to end of line.
                {
                    break;
                }

                case 21: // Clear to start of line.
                {
                    break;
                }

                case 22: // Clear lines above.
                {
                    break;
                }

                default:
                {
                    string.append(text.at(i));
                    column += 1;
                    break;
                }
            }

            lastChar = text.at(i);
        }

        if(d->cursor.document()->isEmpty())
        {
            string.remove(QRegularExpression(QStringLiteral("^\\s+")));
        }

        text = m_parser->parseText(string);

        if(text.isEmpty())
        {
            continue;
        }

        const bool atBottom = isScrollbarAtBottom();
        if (!d->cursor.atEnd())
            d->cursor.movePosition(QTextCursor::End);
        d->cursor.beginEditBlock();
        d->cursor.insertText(doNewlineEnforcement(text), format);

        d->cursor.endEditBlock();
        if (atBottom)
            scrollToBottom();
    }
}

bool OutputWindow::isScrollbarAtBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

static const char settingsGroup[] = "OutputWindow";
static const char saveLogFilePath[] = "SaveLogFilePath";

void OutputWindow::save()
{
    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QLatin1String(settingsGroup));

    QString path =
        QFileDialog::getSaveFileName(Core::ICore::dialogParent(), Tr::tr("Save Log"),
            settings->value(QLatin1String(saveLogFilePath), QDir::homePath()).toString(),
            Tr::tr("Text Files (*.txt);;All files (*)"));

    if(!path.isEmpty())
    {
        Utils::FileSaver file(Utils::FilePath::fromString(path));

        if(!file.hasError())
        {
            if((!file.write(toPlainText().toUtf8())) || (!file.finalize()))
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    Tr::tr("Save Log"),
                    Tr::tr("Error: %L1!").arg(file.errorString()));
            }
            else
            {
                settings->setValue(QLatin1String(saveLogFilePath), path);
            }
        }
        else
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                Tr::tr("Save Log"),
                Tr::tr("Error: %L1!").arg(file.errorString()));
        }
    }

    settings->endGroup();
}

void OutputWindow::setTabSettings(int tabWidth)
{
    d->tabWidth = tabWidth;
}
// OPENMV-DIFF //

#ifdef WITH_TESTS

// Handles all lines starting with "A" and the following ones up to and including the next
// one starting with "A".
class TestFormatterA : public OutputLineParser
{
private:
    Result handleLine(const QString &text, OutputFormat) override
    {
        static const QString replacement = "handled by A";
        if (m_handling) {
            if (text.startsWith("A")) {
                m_handling = false;
                return {Status::Done, {}, replacement};
            }
            return {Status::InProgress, {}, replacement};
        }
        if (text.startsWith("A")) {
            m_handling = true;
            return {Status::InProgress, {}, replacement};
        }
        return Status::NotHandled;
    }

    bool m_handling = false;
};

// Handles all lines starting with "B". No continuation logic.
class TestFormatterB : public OutputLineParser
{
private:
    Result handleLine(const QString &text, OutputFormat) override
    {
        if (text.startsWith("B"))
            return {Status::Done, {}, QString("handled by B")};
        return Status::NotHandled;
    }
};

void Internal::CorePlugin::testOutputFormatter()
{
    const QString input =
            "B to be handled by B\r\r\n"
            "not to be handled\n\n\n\n"
            "A to be handled by A\n"
            "continuation for A\r\n"
            "B looks like B, but still continuation for A\r\n"
            "A end of A\n"
            "A next A\n"
            "A end of next A\n"
            " A trick\r\n"
            "line with \r embedded carriage return\n"
            "B to be handled by B\n";
    const QString output =
            "handled by B\n"
            "not to be handled\n\n\n\n"
            "handled by A\n"
            "handled by A\n"
            "handled by A\n"
            "handled by A\n"
            "handled by A\n"
            "handled by A\n"
            " A trick\n"
            " embedded carriage return\n"
            "handled by B\n";

    // Stress-test the implementation by providing the input in chunks, splitting at all possible
    // offsets.
    for (int i = 0; i < input.length(); ++i) {
        OutputFormatter formatter;
        QPlainTextEdit textEdit;
        formatter.setPlainTextEdit(&textEdit);
        formatter.setLineParsers({new TestFormatterB, new TestFormatterA});
        formatter.appendMessage(input.left(i), StdOutFormat);
        formatter.appendMessage(input.mid(i), StdOutFormat);
        formatter.flush();
        QCOMPARE(textEdit.toPlainText(), output);
    }
}
#endif // WITH_TESTS
} // namespace Core

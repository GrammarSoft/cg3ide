/*
* Copyright 2013-2023, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com> for GrammarSoft ApS (https://grammarsoft.com/)
* Development funded by Tony Berber Sardinha (http://www2.lael.pucsp.br/~tony/), São Paulo Catholic University (http://pucsp.br/), CEPRIL (http://www2.lael.pucsp.br/corpora/), CNPq (http://cnpq.br/), FAPESP (http://fapesp.br/)
*
* This file is part of CG-3 IDE
*
* CG-3 IDE is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CG-3 IDE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CG-3 IDE.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "GrammarEditor.hpp"
#include "ui_GrammarEditor.h"
#include "GotoLine.hpp"
#include "inlines.hpp"
#include "version.hpp"
#include <algorithm>

GrammarEditor::GrammarEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::GrammarEditor),
    check_timer(new QTimer),
    hilite_timer(new QTimer),
    rxTrace(CG_TRACE_RX),
    rxReading(CG_READING_RX),
    rxReading2(CG_READING_RX2),
    previewIn_dirty(true),
    previewIn_run(false),
    previewOut_run(false),
    cur_file_check(false)

{
    QTemporaryFile tmpf(QDir(QDir::tempPath()).filePath("cg3ide-XXXXXX-") + QVariant(qrand()).toString());
    if (tmpf.open()) {
        checker.txtGrammar = tmpf.fileName() + ".cg3";
        checker.binGrammar = tmpf.fileName() + ".cg3b";
        checker.inputFile = tmpf.fileName() + ".txt";
        tmpf.remove();
    }
    else {
        QMessageBox::critical(this, tr("Creating temporaries failed!"), tr("Failed to create temporary files! Make sure you have write access to the temporary folder."));
        close();
        return;
    }

    ui->setupUi(this);
    reTitle();

    defGrammar = ui->editGrammar->toPlainText();

    QSettings settings;
    restoreGeometry(settings.value("editor/geometry").toByteArray());
    restoreState(settings.value("editor/state").toByteArray());

    auto tabwidth = QFontMetrics(ui->editGrammar->font()).width('x')*3;
    ui->editGrammar->setTabStopWidth(tabwidth);
    tabwidth = QFontMetrics(ui->editStdin->font()).width('x')*3;
    ui->editStdin->setTabStopWidth(tabwidth);
    ui->editStdinPreview->setTabStopWidth(tabwidth);
    ui->editStdout->setTabStopWidth(tabwidth);
    ui->editStderr->setTabStopWidth(tabwidth);
    ui->editStderrPreviewInput->setTabStopWidth(tabwidth);
    ui->editStderrPreviewOutput->setTabStopWidth(tabwidth);

    ui->actWrapGrammar->setChecked(settings.value("editor/wrapgrammar", true).toBool());
    ui->actWrapIO->setChecked(settings.value("editor/wrapio", false).toBool());
    ui->optHideTags->setChecked(settings.value("editor/hidetags", true).toBool());
    ui->optHideRemoved->setChecked(settings.value("editor/hideremoved", false).toBool());
    ui->optFindRegex->setChecked(settings.value("editor/find_regex", false).toBool());
    ui->optFindCase->setChecked(settings.value("editor/find_case", false).toBool());
    ui->optPipeText->setChecked(settings.value("process/pipe_text", false).toBool());
    ui->optPipeFiles->setChecked(settings.value("process/pipe_files", false).toBool());

    ui->editGrammar->installEventFilter(this);
    ui->editGrammar->viewport()->installEventFilter(this);
    ui->editInputFiles->viewport()->installEventFilter(this);
    ui->editInputPipe->viewport()->installEventFilter(this);
    ui->editStdin->viewport()->installEventFilter(this);
    ui->editStdout->viewport()->installEventFilter(this);

    ui->editInputFiles->setWordWrapMode(QTextOption::NoWrap);
    ui->editStderr->setWordWrapMode(QTextOption::NoWrap);
    ui->editStderrPreviewInput->setWordWrapMode(QTextOption::NoWrap);
    ui->editStderrPreviewOutput->setWordWrapMode(QTextOption::NoWrap);
    on_actWrapGrammar_toggled(ui->actWrapGrammar->isChecked());
    on_actWrapIO_toggled(ui->actWrapIO->isChecked());

    ui->tableErrors->setModel(&errorEntries);

    stxInput.reset(new StreamHighlighter(ui->editStdin->document()));
    stxInputPreview.reset(new StreamHighlighter(ui->editStdinPreview->document()));
    stxOutput.reset(new StreamHighlighter(ui->editStdout->document()));
    stxGrammar.reset(new GrammarHighlighter(ui->editGrammar->document()));

    ui->frameFindReplace->hide();
    ui->btnOutputOptions->hide();
    ui->frameOutputOptions->show();

    section_jump = new QComboBox;
    ui->mainToolBar->addWidget(section_jump);
    section_jump->addItem(tr("Jump to section..."));
    section_jump->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(section_jump, SIGNAL(activated(int)), this, SLOT(sectionJump_Activated(int)));

    QShortcut *findNext = new QShortcut(QKeySequence("F3"), this);
    connect(findNext, SIGNAL(activated()), ui->actFindNext, SLOT(trigger()));

    connect(check_timer.data(), SIGNAL(timeout()), this, SLOT(checkGrammar()));
    connect(hilite_timer.data(), SIGNAL(timeout()), this, SLOT(reHilite()));
    connect(ui->editGrammar->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(scrollValue_Changed(int)));

    reOptions();
    ui->editGrammar->textChanged();
}

GrammarEditor::~GrammarEditor() {
}

void GrammarEditor::closeEvent(QCloseEvent *event) {
    if (ui->editGrammar->document()->isModified()) {
        int q = QMessageBox::question(this, tr("Discard changes?"),
                                      tr("The current document contains unsaved changes. Do you want to save them?"),
                                      QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel, QMessageBox::Save);
        if (q == QMessageBox::Save && !on_actSave_triggered()) {
            event->ignore();
            return;
        }
        else if (q == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }

    QFile(checker.txtGrammar).remove();
    QFile(checker.binGrammar).remove();
    QFile(checker.inputFile).remove();

    QSettings settings;
    settings.setValue("editor/geometry", saveGeometry());
    settings.setValue("editor/state", saveState());
    QMainWindow::closeEvent(event);
}

void GrammarEditor::on_actAbout_triggered() {
    QMessageBox::about(this, tr("About CG-3 IDE"), tr("<h1>CG-3 IDE</h1>")
                       + tr("<b>Version %1.%2.%3.%4</b>").arg(CG3IDE_VERSION_MAJOR).arg(CG3IDE_VERSION_MINOR).arg(CG3IDE_VERSION_PATCH).arg(CG3IDE_REVISION)
                       + tr("<ul><li>Developed by <a href=\"https://tinodidriksen.com/\">Tino Didriksen</a> for <a href=\"https://grammarsoft.com/\">GrammarSoft ApS</a></li><li>Development funded by <a href=\"http://www2.lael.pucsp.br/~tony/\">Tony Berber Sardinha</a>, <a href=\"http://pucsp.br/\">São Paulo Catholic University</a>, <a href=\"http://www2.lael.pucsp.br/corpora/\">CEPRIL</a>, <a href=\"http://cnpq.br/\">CNPq</a>, <a href=\"http://fapesp.br/\">FAPESP</a></li><li>Copyright 2013-2023 GrammarSoft ApS</li></ul><hr/>Report bugs and feature request to <a href=\"mailto:mail@tinodidriksen.com\">Tino Didriksen &lt;mail@tinodidriksen.com&gt;</a>"));
}

void GrammarEditor::on_actHelp_triggered() {
    QMessageBox::about(this, tr("CG-3 Resources"), tr("<h1>CG-3 Resources</h1><ul><li><a href=\"https://visl.sdu.dk/constraint_grammar.html\">VISL's Constraint Grammar Page</a></li><li><a href=\"https://visl.sdu.dk/cg3.html\">VISL's CG-3 Project Page</a></li><li><a href=\"https://groups.google.com/group/constraint-grammar\">Constraint Grammar Mailing List</a></li><li><a href=\"irc://irc.freenode.net/CG3\">IRC Freenode.net #CG3</a></li><li><a href=\"http://kevindonnelly.org.uk/2010/05/constraint-grammar-tutorial/\">Kevin Donnelly's CG Tutorial</a></li><li><a href=\"http://wiki.apertium.org/wiki/Constraint_Grammar\">Apertium's CG Page</a></li></ul><hr/>Report bugs and feature request to <a href=\"mailto:mail@tinodidriksen.com\">Tino Didriksen &lt;mail@tinodidriksen.com&gt;</a>"));
}

void GrammarEditor::on_actNew_triggered() {
    if (launchEditor()) {
        return;
    }
    GrammarEditor *n = new GrammarEditor;
    n->show();
}

void GrammarEditor::on_actClose_triggered() {
    if (ui->editGrammar->document()->isModified()) {
        int q = QMessageBox::question(this, tr("Discard changes?"),
                                      tr("The current document contains unsaved changes. Do you want to save them?"),
                                      QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel, QMessageBox::Save);
        if (q == QMessageBox::Save && !on_actSave_triggered()) {
            return;
        }
        else if (q == QMessageBox::Cancel) {
            return;
        }
    }

    ui->editGrammar->setPlainText(defGrammar);
    cur_file = QFileInfo();
    reTitle();
}

void GrammarEditor::reTitle() {
    auto filename = cur_file.fileName().isEmpty() ? tr("Untitled") : cur_file.fileName();
    auto changed = ui->editGrammar->document()->isModified() ? "*" : "";
    auto title = filename + changed;
    setWindowTitle(title + tr(" - CG-3 IDE"));
}

void GrammarEditor::reHilite() {
    stxGrammar->clear();
    while (section_jump->count() > 1) {
        section_jump->removeItem(section_jump->count()-1);
    }
    for (auto& it : stxGrammar->section_lines) {
        section_jump->addItem(tr("Line %1: %2").arg(it+1).arg(ui->editGrammar->document()->findBlockByNumber(it).text()));
    }
}

void GrammarEditor::reOptions() {
    QSettings settings;

    // ToDo: Options dialog should set a flag for whether the font was changed or not
    if (settings.contains("editor/font")) {
        QFont f = settings.value("editor/font", "Courier,10,-1,5,50,0,0,0,0,0").value<QFont>();
        ui->editGrammar->setFont(f);
        ui->editGrammar->setTabStopWidth(QFontMetrics(f).width('x')*3);

        f.setPointSize(f.pointSize()-1);
        auto tabwidth = QFontMetrics(f).width('x')*3;
        ui->editStdin->setFont(f);
        ui->editStdin->setTabStopWidth(tabwidth);
        ui->editStdinPreview->setFont(f);
        ui->editStdinPreview->setTabStopWidth(tabwidth);
        ui->editStdout->setFont(f);
        ui->editStdout->setTabStopWidth(tabwidth);
        ui->editStderr->setFont(f);
        ui->editStderr->setTabStopWidth(tabwidth);
        ui->editStderrPreviewInput->setFont(f);
        ui->editStderrPreviewInput->setTabStopWidth(tabwidth);
        ui->editStderrPreviewOutput->setFont(f);
        ui->editStderrPreviewOutput->setTabStopWidth(tabwidth);

        settingSetOrDef(settings, "editor/font", QString("Courier,10,-1,5,50,0,0,0,0,0"), ui->editGrammar->font().toString());
    }

    bool rehilite = false;
    for (int i=0 ; i<stxGrammar->fmt_desc.size() ; ++i) {
        auto& fmt = stxGrammar->fmts[i];
        auto& desc = stxGrammar->fmt_desc[i];

        QColor col = settings.value(QString("editor/highlight_%1_color").arg(desc[0]), desc[3]).value<QColor>();
        if (fmt.foreground().color() != col) {
            fmt.setForeground(col);
            rehilite = true;
        }

        int bld = settings.value(QString("editor/highlight_%1_bold").arg(desc[0]), desc[5]).toInt();
        bool bld_v = (bld == 1) ? ui->editGrammar->font().bold() : (bld == 2);
        if (fmt.font().bold() != bld_v) {
            fmt.font().setBold(bld_v);
            rehilite = true;
        }

        int ita = settings.value(QString("editor/highlight_%1_italic").arg(desc[0]), desc[4]).toInt();
        bool ita_v = (ita == 1) ? ui->editGrammar->font().italic() : (ita == 2);
        if (fmt.fontItalic() != ita_v) {
            fmt.setFontItalic(ita_v);
            rehilite = true;
        }
    }

    QFileInfo cg3bin(settings.value("cg3/binary", "").toString());
    QFileInfo convbin(cg3bin.dir().filePath("cg-conv"));
    if (!convbin.isExecutable()) {
        convbin = cg3bin.dir().filePath("cg-conv.exe");
    }
    if (ui->editInputPipe->find("{AUTO-CG-CONV}")) {
        QTextCursor tc = ui->editInputPipe->textCursor();
        tc.removeSelectedText();
        if (convbin.isExecutable()) {
            tc.insertText(convbin.filePath());
        }
    }

    /*
    if (settings.value("cg3/previewoutput", true).toBool()) {
        ui->btnRunPreviewOut->hide();
        ui->editGrammar->textChanged();
    }
    else {
        ui->btnRunPreviewOut->show();
    }
    //*/
    ui->btnRunPreviewOut->show();

    if (rehilite) {
        reHilite();
    }
}

void GrammarEditor::refreshInput() {
    if (!previewIn_dirty) {
        return;
    }
    QSettings settings;

    auto input = ui->editStdin->toPlainText();
    auto lines = input.split('\n');

    int num_lines = settings.value("cg3/maxinputlines", 1000).toInt() - lines.size();
    int num_chars = settings.value("cg3/maxinputchars", 60000).toInt() - input.size();

    auto files = ui->editInputFiles->toPlainText().split('\n');
    for (auto& file : files) {
        if (num_lines < 0 || num_chars < 0) {
            break;
        }
        file = file.trimmed();
        if (file[0] == '#') {
            continue;
        }
        if (!QFileInfo(file).isReadable()) {
            continue;
        }
        auto more_input = fileGetContents(file, num_chars);
        num_chars -= more_input.size();
        auto more_lines = more_input.split('\n');
        num_lines -= more_lines.size();
        lines += "";
        lines += "";
        lines += more_lines;
    }

    QStringList pipes;
    for (auto& prog : ui->editInputPipe->toPlainText().split('\n')) {
        prog = prog.trimmed();
        if (prog[0] == '#' || prog.isEmpty()) {
            continue;
        }
        pipes << prog;
    }

    while (!pipes.isEmpty()) {
        QProgressDialog prg(this);
        prg.setLabelText(tr("Launching pipe..."));
        prg.show();

        QProcess pipe;
        pipe.setWorkingDirectory(QDir::tempPath());
        #if defined(Q_OS_WIN)
        pipe.start("cmd", QStringList() << "/D" << "/Q" << "/C" << pipes.join(" | "));
        #else
        pipe.start("/bin/sh", QStringList() << "-c" << pipes.join(" | "));
        #endif
        if (!pipe.waitForStarted(4000)) {
            input = tr("Error in pipe launch...");
            break;
        }

        prg.setLabelText(tr("Sending input through the pipe..."));
        prg.setMinimum(0);
        prg.setMaximum(lines.size()+1);
        prg.setValue(0);
        for (auto& line : lines) {
            line += '\n';
            prg.setValue(prg.value()+1);
            pipe.write(line.toUtf8());
        }
        pipe.closeWriteChannel();

        prg.setLabelText(tr("Waiting for pipe to process..."));
        pipe.waitForFinished(4000);
        if (!pipe.waitForFinished(4000) && pipe.exitStatus() != QProcess::NormalExit) {
            input = tr("Error in pipe processing...");
            break;
        }

        prg.setLabelText(tr("Reading pipe output..."));
        ui->editStderrPreviewInput->setPlainText(pipe.readAllStandardError());
        input = pipe.readAllStandardOutput();

        pipe.close();
        prg.close();
        break;
    }

    ui->editStdinPreview->setPlainText(input);
    previewIn_dirty = false;
}

void GrammarEditor::checkGrammar() {
    QSettings settings;
    if (!settings.contains("cg3/binary")) {
        return;
    }

    QFile(checker.txtGrammar).remove();
    QFile(checker.binGrammar).remove();
    QFile(checker.inputFile).remove();

    if (filePutContents(checker.txtGrammar, ui->editGrammar->toPlainText())) {
        checker.process.reset(new QProcess);
        connect(checker.process.data(), SIGNAL(finished(int)), this, SLOT(checkGrammar_finished(int)));
        checker.process->setWorkingDirectory(cur_file.dir().path());
        checker.process->setProcessChannelMode(QProcess::MergedChannels);
        checker.process->start(settings.value("cg3/binary").toString(),
                               QStringList() << "--grammar-only" << "-v"
                               << "-g" << checker.txtGrammar
                               << "--grammar-bin" << checker.binGrammar, QIODevice::ReadOnly);
    }
}

void GrammarEditor::checkGrammar_finished(int) {
    QSettings settings;
    QTextStream log(checker.process.data());
    log.setCodec("UTF-8");
    ui->editStderr->setPlainText(log.readAll());
    auto vz = ui->tableErrors->verticalScrollBar()->value(), hz = ui->tableErrors->horizontalScrollBar()->value();

    errorSelections.clear();
    errorEntries.clear();
    errorEntries.setHorizontalHeaderLabels(QStringList() << "Line" << "Type" << "Message");
    auto warnColor = QColor(Qt::blue).lighter(190);
    auto errColor = QColor(Qt::red).lighter(190);
    auto cur = ui->editGrammar->textCursor();
    cur.clearSelection();
    cur.setPosition(0);

    QList<QRegExp> rxs;
    rxs.append(QRegExp("Line (\\d+):"));
    rxs.append(QRegExp("Lines (\\d+) and (\\d+)"));
    rxs.append(QRegExp("\\(L:(\\d+)\\)"));
    rxs.append(QRegExp("on line (\\d+)"));
    rxs.append(QRegExp("before line (\\d+)"));

    auto lines = ui->editStderr->toPlainText().split("\n");
    for (auto& line : lines) {
        for (auto& rx : rxs) {
            if (rx.indexIn(line) != -1) {
                auto caps = rx.capturedTexts();
                caps.pop_front();
                for (auto& cap : caps) {
                    QTextEdit::ExtraSelection selection;
                    QList<QStandardItem*> row;
                    row << new QStandardItem << new QStandardItem << new QStandardItem(line.section(':', 1).simplified());
                    row[0]->setData(cap.toInt(), Qt::DisplayRole);
                    if (line.contains("Warning:")) {
                        selection.format.setBackground(warnColor);
                        row[1]->setData(tr("Warning"), Qt::DisplayRole);
                        row[1]->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
                    }
                    else {
                        selection.format.setBackground(errColor);
                        row[1]->setData(tr("Error"), Qt::DisplayRole);
                        row[1]->setIcon(style()->standardIcon(QStyle::SP_MessageBoxCritical));
                    }
                    row[0]->setEditable(false);
                    row[1]->setEditable(false);
                    row[2]->setEditable(false);
                    errorEntries.appendRow(row);
                    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
                    selection.format.setToolTip(line);
                    selection.cursor = cur;
                    curGotoLine(selection.cursor, cap.toInt()-1);
                    selection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    errorSelections.append(selection);
                }
            }
        }
    }

    bool state_error = false;
    reparsed:

    for (QTextBlock block = ui->editGrammar->document()->begin() ; block.isValid() ; block = block.next()) {
        auto s = static_cast<GrammarState*>(block.userData());
        if (!s->error.isEmpty()) {
            if (!state_error) {
                state_error = true;
                reHilite();
                goto reparsed;
            }
            QTextEdit::ExtraSelection selection;
            QList<QStandardItem*> row;
            row << new QStandardItem << new QStandardItem << new QStandardItem(s->error);
            row[0]->setData(block.blockNumber()+1, Qt::DisplayRole);
            selection.format.setBackground(errColor);
            row[1]->setData(tr("Error"), Qt::DisplayRole);
            row[1]->setIcon(style()->standardIcon(QStyle::SP_MessageBoxCritical));
            row[0]->setEditable(false);
            row[1]->setEditable(false);
            row[2]->setEditable(false);
            errorEntries.appendRow(row);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.format.setToolTip(s->error);
            selection.cursor = cur;
            curGotoLine(selection.cursor, block.blockNumber());
            selection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            errorSelections.append(selection);
        }
    }
    for (auto block = ui->editGrammar->document()->begin() ; block.isValid() ; block = block.next()) {
        auto s = static_cast<GrammarState*>(block.userData());
        for (auto it = s->warnings.begin() ; it != s->warnings.end() ; ++it) {
            QTextEdit::ExtraSelection selection;
            QList<QStandardItem*> row;
            row << new QStandardItem << new QStandardItem << new QStandardItem(it.value());
            row[0]->setData(block.blockNumber()+1, Qt::DisplayRole);
            row[1]->setData(tr("Warning"), Qt::DisplayRole);
            row[1]->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
            row[0]->setEditable(false);
            row[1]->setEditable(false);
            row[2]->setEditable(false);
            errorEntries.appendRow(row);
            selection.format.setFontUnderline(true);
            selection.format.setUnderlineColor(Qt::darkRed);
            selection.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            selection.format.setToolTip(it.value());
            selection.cursor = cur;
            curGotoLine(selection.cursor, block.blockNumber());
            selection.cursor.setPosition(selection.cursor.position()+it.key().first, QTextCursor::MoveAnchor);
            selection.cursor.setPosition(selection.cursor.position()+it.key().second, QTextCursor::KeepAnchor);
            errorSelections.append(selection);
        }
    }

    errorEntries.sort(0);
    ui->tableErrors->horizontalHeader()->setStretchLastSection(true);
    ui->tableErrors->resizeColumnsToContents();
    ui->tableErrors->repaint();
    ui->tableErrors->verticalScrollBar()->setValue(vz);
    ui->tableErrors->horizontalScrollBar()->setValue(hz);
    on_editGrammar_cursorPositionChanged();

    if (settings.value("cg3/previewoutput", true).toBool() || previewOut_run) {
        previewRun();
        previewOut_run = false;
    }
}

void GrammarEditor::previewRun() {
    QSettings settings;
    if (!settings.contains("cg3/binary")) {
        return;
    }
    if (!QFile(checker.binGrammar).exists()) {
        ui->editStderrPreviewOutput->setPlainText(tr("Error in grammar..."));
        stxOutput.reset();
        return;
    }
    if (stxOutput.isNull()) {
        stxOutput.reset(new StreamHighlighter(ui->editStdout->document()));
    }
    if (settings.value("cg3/previewinput", true).toBool() || previewIn_run) {
        refreshInput();
        previewIn_run = false;
    }
    if (filePutContents(checker.txtGrammar, ui->editGrammar->toPlainText()) && filePutContents(checker.inputFile, ui->editStdinPreview->toPlainText())) {
        checker.process->disconnect();
        connect(checker.process.data(), SIGNAL(finished(int)), this, SLOT(previewOutRun_finished(int)));
        checker.process->setProcessChannelMode(QProcess::SeparateChannels);
        checker.process->start(settings.value("cg3/binary").toString(),
                               QStringList() << "-v" << "--trace"
                               << "-g" << checker.binGrammar
                               << "-I" << checker.inputFile, QIODevice::ReadOnly);
    }
}

void GrammarEditor::previewOutRun_finished(int) {
    stdout_raw = checker.process->readAllStandardOutput();
    ui->editStderrPreviewOutput->setPlainText(checker.process->readAllStandardError());
    previewOutRun_render();
}

void GrammarEditor::previewOutRun_render() {
    QString out = stdout_raw;
    QStringList olines;

    if (ui->optHideRemoved->isChecked()) {
        QStringList lines = out.split('\n');
        for (auto& line : lines) {
            if (line[0] != ';') {
                olines << line;
            }
        }
        out = olines.join("\n");
    }
    else {
        olines = out.split('\n');
    }

    if (ui->optHideTags->isChecked()) {
        QStringList otags;
        for (int i=0 ; i<olines.size() ; ++i) {
            auto& text = olines[i];
            int index = 0;
            if (rxReading.indexIn(text) != -1 && (index = rxReading2.indexIn(text)) != -1) {
                index += rxReading2.matchedLength();
                auto tags = text.mid(index).simplified().split(' ');
                text = text.left(index);
                auto oit = otags.begin();
                for (auto& tag : tags) {
                    QStringList::Iterator fit;
                    if ((fit = std::find(oit, otags.end(), tag)) != otags.end()) {
                        tag = '-';
                        oit = fit;
                    }
                    text.append(tag).append(' ');
                }
                text.replace(QRegExp("(\\s+)- (- )+"), "\\1- ");
                otags = tags;
            }
            else {
                otags.clear();
            }
        }
        out = olines.join("\n");
    }

    auto vz = ui->editStdout->verticalScrollBar()->value(), hz = ui->editStdout->horizontalScrollBar()->value();
    ui->editStdout->setPlainText(out);
    ui->editStdout->verticalScrollBar()->setValue(vz);
    ui->editStdout->horizontalScrollBar()->setValue(hz);
}

bool GrammarEditor::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::WindowActivate && !cur_file_check && !ui->editGrammar->document()->isModified()) {
        QFileInfo check(cur_file.filePath());
        if (check.exists()) {
            if (cur_file == check && (cur_file.created() != check.created() || cur_file.lastModified() != check.lastModified() || cur_file.size() != check.size())) {
                cur_file_check = true;
                int yesno = QMessageBox::question(this, tr("Reload changed file?"), tr("The file %1 has changed on disk since last save. Do you want to reload it? Any changes made here will be lost.").arg(check.filePath()), QMessageBox::Yes, QMessageBox::No);
                if (yesno == QMessageBox::Yes) {
                    auto vz = ui->editGrammar->verticalScrollBar()->value(), hz = ui->editGrammar->horizontalScrollBar()->value();
                    auto pos = ui->editGrammar->textCursor().position();
                    open(check.filePath());
                    QTextCursor tc = ui->editGrammar->textCursor();
                    tc.setPosition(pos);
                    ui->editGrammar->setTextCursor(tc);
                    ui->editGrammar->verticalScrollBar()->setValue(vz);
                    ui->editGrammar->horizontalScrollBar()->setValue(hz);
                }
                else {
                    ui->editGrammar->document()->setModified(true);
                }
                cur_file_check = false;
            }
        }
    }
    else if (watched == ui->editGrammar) {
        if (event->type() == QEvent::ToolTip) {
            auto helpEvent = static_cast<QHelpEvent*>(event);
            auto cur = ui->editGrammar->cursorForPosition(helpEvent->pos());
            QStringList tips;
            for (auto& es : errorSelections) {
                if (cur.position() >= es.cursor.selectionStart() && cur.position() <= es.cursor.selectionEnd()) {
                    tips << es.format.toolTip();
                }
            }

            auto s = static_cast<const GrammarState*>(cur.block().userData());
            auto p = cur.positionInBlock();
            auto it_e = s->tokens.upperBound(p);
            if (it_e != s->tokens.end() && it_e != s->tokens.begin()) {
                auto it_b = it_e;
                --it_b;
                QString name(cur.block().text().constData()+it_b.key(), it_e.key()-it_b.key());
                if (it_b.value() == S_SETNAME) {
                    if ((name[0] == '$' && name[1] == '$') || (name[0] == '&' && name[1] == '&')) {
                        name = name.mid(2);
                    }
                    if (stxGrammar->set_lines.contains(name)) {
                        auto line = stxGrammar->set_lines[name];
                        tips << QString("Set %1 defined on line %2:\n%3").arg(name).arg(line).arg(ui->editGrammar->document()->findBlockByNumber(line).text());
                    }
                }
                else if (it_b.value() == S_TMPLNAME) {
                    if (name[0] == 'T' && name[1] == ':') {
                        name = name.mid(2);
                    }
                    if (stxGrammar->tmpl_lines.contains(name)) {
                        auto line = stxGrammar->tmpl_lines[name];
                        tips << QString("Template %1 defined on line %2:\n%3").arg(name).arg(line).arg(ui->editGrammar->document()->findBlockByNumber(line).text());
                    }
                }
            }

            if (tips.isEmpty()) {
                QToolTip::hideText();
            }
            else {
                QToolTip::showText(helpEvent->globalPos(), tips.join("\n\n"));
            }
            helpEvent->accept();
            return true;
        }
    }
    else if (watched == ui->editStdout->viewport()) {
        if (event->type() == QEvent::MouseButtonRelease) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            auto cur = ui->editStdout->cursorForPosition(mouseEvent->pos());
            auto doc = cur.document();
            auto start = cur.position(), stop = cur.position();
            while (!doc->characterAt(start).isNull() && !doc->characterAt(start).isSpace()) {
                --start;
            }
            ++start;
            while (!doc->characterAt(start).isNull() && !doc->characterAt(stop).isSpace()) {
                ++stop;
            }
            cur.setPosition(start);
            cur.setPosition(stop, QTextCursor::KeepAnchor);
            auto tag = cur.selectedText().trimmed();
            if (rxTrace.indexIn(tag) != -1) {
                QRegExp rx(":(\\d+)\\b");
                if (rx.indexIn(tag) && rx.cap(1).toInt() != 0) {
                    editorGotoLine(ui->editGrammar, rx.cap(1).toInt()-1);
                    mouseEvent->accept();
                    return true;
                }
            }
        }
    }
    else if (event->type() == QEvent::Drop) {
        auto dropEvent = static_cast<QDropEvent*>(event);
        if (dropEvent->mimeData()->hasUrls()) {
            if (watched == ui->editGrammar->viewport()) {
                bool first = true;
                for (auto& url : dropEvent->mimeData()->urls()) {
                    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isReadable()) {
                        if (first) {
                            open(url.toLocalFile());
                            first = false;
                        }
                        else {
                            launchEditor(url.toLocalFile());
                        }
                    }
                }
                dropEvent->accept();
                return true;
            }
            else if (watched == ui->editStdin->viewport()) {
                for (auto& url : dropEvent->mimeData()->urls()) {
                    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isReadable()) {
                        QString text = fileGetContents(url.toLocalFile());
                        if (!text.isEmpty()) {
                            ui->editStdin->setPlainText(text);
                        }
                    }
                }
                dropEvent->accept();
                return true;
            }
            else if (watched == ui->editInputFiles->viewport()) {
                for (auto& url : dropEvent->mimeData()->urls()) {
                    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isReadable()) {
                        ui->editInputFiles->appendPlainText(url.toLocalFile());
                    }
                }
                dropEvent->accept();
                return true;
            }
            else if (watched == ui->editInputPipe->viewport()) {
                for (auto& url : dropEvent->mimeData()->urls()) {
                    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isExecutable()) {
                        ui->editInputPipe->appendPlainText(url.toLocalFile());
                    }
                }
                dropEvent->accept();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

bool GrammarEditor::save(const QString& filename) {
    QFileInfo check(filename);
    if (check.exists()) {
        if (check.isWritable() == false) {
            QMessageBox::information(this, tr("Not writable!"), tr("The file %1 is not writable!").arg(filename));
            return false;
        }

        if (cur_file == check && (cur_file.created() != check.created() || cur_file.lastModified() != check.lastModified() || cur_file.size() != check.size())) {
            int yesno = QMessageBox::question(this, tr("Overwrite changed file?"), tr("The file %1 has changed on disk since last save. Do you want to overwrite it with the current grammar?").arg(filename), QMessageBox::Yes, QMessageBox::No);
            if (yesno != QMessageBox::Yes) {
                return false;
            }
        }
    }

    if (!filePutContents(filename, ui->editGrammar->toPlainText())) {
        return false;
    }

    ui->editGrammar->document()->setModified(false);

    cur_file.setFile(filename);
    cur_file.refresh();
    cur_file.created();
    cur_file.lastModified();
    cur_file.size();

    QFileInfo cg3p(filename + ".cg3p");
    if (!cg3p.exists() || cg3p.isWritable()) {
        QSettings state(cg3p.filePath(), QSettings::Format::IniFormat);
        state.setValue("input_text", ui->editStdin->toPlainText());
        state.setValue("input_files", ui->editInputFiles->toPlainText());
        state.setValue("input_pipe", ui->editInputPipe->toPlainText());
    }

    reTitle();

    return true;
}

void GrammarEditor::open(const QString& filename) {
    QFileInfo check(filename);
    if (check.exists() == false) {
        QMessageBox::information(this, tr("No such file!"), tr("The file %1 does not exist!").arg(filename));
        return;
    }
    if (check.isReadable() == false) {
        QMessageBox::information(this, tr("Not readable!"), tr("The file %1 is not readable!").arg(filename));
        return;
    }

    QString text = fileGetContents(filename);
    if (text.isNull()) {
        return;
    }

    ui->editGrammar->setPlainText(text);
    ui->editGrammar->document()->setModified(false);

    cur_file.setFile(filename);
    cur_file.refresh();
    cur_file.created();
    cur_file.lastModified();
    cur_file.size();

    QFileInfo cg3p(filename + ".cg3p");
    if (cg3p.exists() && cg3p.isReadable()) {
        QSettings state(cg3p.filePath(), QSettings::Format::IniFormat);
        if (state.contains("input_text")) {
            ui->editStdin->setPlainText(state.value("input_text").toString());
        }
        if (state.contains("input_files")) {
            ui->editInputFiles->setPlainText(state.value("input_files").toString());
        }
        if (state.contains("input_pipe")) {
            ui->editInputPipe->setPlainText(state.value("input_pipe").toString());
        }
        on_editInputPipe_textChanged();
    }

    reTitle();
}

void GrammarEditor::on_actOpen_triggered() {
    if (ui->editGrammar->document()->isModified()) {
        int q = QMessageBox::question(this, tr("Discard changes?"),
                                      tr("The current document contains unsaved changes. Do you want to save them?"),
                                      QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel, QMessageBox::Save);
        if (q == QMessageBox::Save && !on_actSave_triggered()) {
            return;
        }
        else if (q == QMessageBox::Cancel) {
            return;
        }
    }

    auto filename = QFileDialog::getOpenFileName(this, tr("Open Grammar"), cur_file.path(), tr("CG-3 Grammars (*.cg3 *.cg2 *.cg *.rle);;Any File (*.*)"));
    if (filename.isEmpty()) {
        return;
    }

    open(filename);
}

bool GrammarEditor::on_actSave_triggered() {
    if (cur_file.filePath().isEmpty() || cur_file.exists() == false || cur_file.isFile() == false) {
        return on_actSaveas_triggered();
    }

    QFileInfo check(cur_file.filePath());
    if (check.isWritable() == false) {
        return on_actSaveas_triggered();
    }

    return save(cur_file.filePath());
}

bool GrammarEditor::on_actSaveas_triggered() {
    auto filename = QFileDialog::getSaveFileName(this, tr("Save Grammar"), cur_file.path(), tr("CG-3 Grammars (*.cg3 *.cg2 *.cg *.rle);;Any File (*.*)"));
    if (filename.isEmpty()) {
        return false;
    }

    return save(filename);
}

void GrammarEditor::on_actGotoLine_triggered() {
    auto line = new GotoLine(this, ui->editGrammar);
    line->show();
}

void GrammarEditor::on_actOptions_triggered() {
    auto opts = new OptionsDialog(this);
    opts->show();
}

void GrammarEditor::on_actFindHide_triggered() {
    ui->frameFindReplace->hide();
    ui->editGrammar->setFocus();
    on_editGrammar_cursorPositionChanged();
}

void GrammarEditor::on_actFindReplace_triggered() {
    ui->frameFindReplace->show();
    auto selected = ui->editGrammar->textCursor().selection().toPlainText();
    if (!selected.isEmpty()) {
        if (ui->optFindRegex->isChecked()) {
            selected = QRegExp::escape(selected);
        }
        ui->editFind->setText(selected);
    }
    ui->editFind->setFocus();
    ui->editFind->selectAll();
    on_editFind_textEdited();
}

void GrammarEditor::on_actFindNext_triggered() {
    if (!ui->frameFindReplace->isVisible()) {
        return on_actFindReplace_triggered();
    }

    QRegExp find(ui->editFind->text(),
                 ui->optFindCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                 ui->optFindRegex->isChecked() ? QRegExp::RegExp2 : QRegExp::FixedString);
    if (!find.isValid()) {
        return;
    }
    find.setMinimal(true);

    const auto& cur = ui->editGrammar->textCursor();
    auto res = ui->editGrammar->document()->find(find, cur);

    if (res.isNull()) {
        res = ui->editGrammar->document()->find(find);
    }
    if (!res.isNull()) {
        ui->editGrammar->setTextCursor(res);
        ui->editGrammar->ensureCursorVisible();
        ui->editGrammar->setFocus();
    }
    on_editFind_textEdited();
}

void GrammarEditor::on_actFindPrev_triggered() {
    if (!ui->frameFindReplace->isVisible()) {
        return on_actFindReplace_triggered();
    }

    QRegExp find(ui->editFind->text(),
                 ui->optFindCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                 ui->optFindRegex->isChecked() ? QRegExp::RegExp2 : QRegExp::FixedString);
    if (!find.isValid()) {
        return;
    }
    find.setMinimal(true);

    const auto& cur = ui->editGrammar->textCursor();
    auto res = ui->editGrammar->document()->find(find, cur, QTextDocument::FindBackward);

    if (res.isNull()) {
        res = ui->editGrammar->document()->find(find, ui->editGrammar->document()->characterCount(), QTextDocument::FindBackward);
    }
    if (!res.isNull()) {
        ui->editGrammar->setTextCursor(res);
        ui->editGrammar->ensureCursorVisible();
        ui->editGrammar->setFocus();
    }
    on_editFind_textEdited();
}

void GrammarEditor::on_actReplaceOnce_triggered() {
    if (!ui->frameFindReplace->isVisible()) {
        return on_actFindReplace_triggered();
    }

    QRegExp find(ui->editFind->text(),
                 ui->optFindCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                 ui->optFindRegex->isChecked() ? QRegExp::RegExp2 : QRegExp::FixedString);
    if (!find.isValid()) {
        return;
    }
    find.setMinimal(true);

    auto cur = ui->editGrammar->textCursor();
    cur.beginEditBlock();
    auto res = ui->editGrammar->document()->find(find, cur.selectionStart());

    if (res.isNull()) {
        res = ui->editGrammar->document()->find(find);
    }
    if (!res.isNull()) {
        QString text = res.selectedText();
        text.replace(find, ui->editReplace->text());
        res.removeSelectedText();
        cur.setPosition(res.selectionStart());
        cur.insertText(text);
        ui->editGrammar->setTextCursor(res);
        ui->editGrammar->ensureCursorVisible();
        ui->editGrammar->setFocus();
    }

    cur.endEditBlock();
    on_editFind_textEdited();
}

void GrammarEditor::on_actReplaceAll_triggered() {
    if (!ui->frameFindReplace->isVisible()) {
        return on_actFindReplace_triggered();
    }

    QRegExp find(ui->editFind->text(),
                 ui->optFindCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                 ui->optFindRegex->isChecked() ? QRegExp::RegExp2 : QRegExp::FixedString);
    if (!find.isValid()) {
        return;
    }
    find.setMinimal(true);

    auto cur = ui->editGrammar->textCursor();
    cur.beginEditBlock();
    auto res = ui->editGrammar->document()->find(find, cur.selectionStart());

    while (!res.isNull()) {
        auto text = res.selectedText();
        text.replace(find, ui->editReplace->text());
        res.removeSelectedText();
        cur.setPosition(res.selectionStart());
        cur.insertText(text);
        res = ui->editGrammar->document()->find(find, cur.selectionStart()+text.length());
    }

    cur.endEditBlock();
    on_editFind_textEdited();
}

void GrammarEditor::on_editFind_returnPressed() {
    on_actFindNext_triggered();
}

void GrammarEditor::on_editFind_textEdited() {
    if (!ui->frameFindReplace->isVisible()) {
        return;
    }
    if (ui->editFind->text().isEmpty()) {
        return;
    }
    ui->editFind->setStyleSheet("");
    ui->editFind->setToolTip("");

    findSelections.clear();
    QRegExp find(ui->editFind->text(),
                 ui->optFindCase->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive,
                 ui->optFindRegex->isChecked() ? QRegExp::RegExp2 : QRegExp::FixedString);
    if (!find.isValid()) {
        ui->editFind->setStyleSheet("background-color: #fbb");
        ui->editFind->setToolTip(tr("Regex Error: ") + find.errorString());
        return;
    }
    find.setMinimal(true);

    QTextEdit::ExtraSelection selection;
    auto lineColor = QColor(Qt::green).lighter(160);
    selection.format.setBackground(lineColor);
    selection.cursor = ui->editGrammar->textCursor();

    auto s = ui->editGrammar->cursorForPosition(QPoint(0,0)), e = ui->editGrammar->cursorForPosition(QPoint(ui->editGrammar->viewport()->width(), ui->editGrammar->viewport()->height()));

    auto text = ui->editGrammar->toPlainText().mid(s.position(), e.position() - s.position());
    int pos = 0;
    while ((pos = find.indexIn(text, pos)) != -1) {
        selection.cursor.setPosition(pos + s.position());
        selection.cursor.setPosition(pos + s.position() + find.matchedLength(), QTextCursor::KeepAnchor);
        findSelections.append(selection);
        pos += std::max(find.matchedLength(), 1);
    }

    on_editGrammar_cursorPositionChanged();
}

void GrammarEditor::scrollValue_Changed(int) {
    on_editFind_textEdited();
}

void GrammarEditor::sectionJump_Activated(int which) {
    auto it = stxGrammar->section_lines.begin();
    std::advance(it, which-1);
    editorGotoLine(ui->editGrammar, *it);
    section_jump->setCurrentIndex(0);
}

void GrammarEditor::on_btnFileInAdd_clicked(bool) {
    auto lines = ui->editInputFiles->toPlainText().split('\n');
    QString path;
    while (!lines.empty()) {
        if (!lines.back().trimmed().isEmpty() && !lines.back().trimmed().startsWith('#')) {
            path = lines.back().trimmed();
            break;
        }
        lines.pop_back();
    }

    auto filename = QFileDialog::getOpenFileName(this, tr("Add Input File"),
        path.isEmpty() ? cur_file.path() : path,
        tr("Any File (*.*)"));
    if (filename.isEmpty()) {
        return;
    }
    ui->editInputFiles->appendPlainText(filename);
}

void GrammarEditor::on_btnPipeFind_clicked(bool) {
    auto lines = ui->editInputPipe->toPlainText().split('\n');
    QString path;
    while (!lines.empty()) {
        if (!lines.back().trimmed().isEmpty() && !lines.back().trimmed().startsWith('#')) {
            path = lines.back().trimmed();
            break;
        }
        lines.pop_back();
    }

    auto filename = QFileDialog::getOpenFileName(this, tr("Add Pipe Program"),
        path.isEmpty() ? cur_file.path() : path,
        tr("Any File (*.*)"));
    if (filename.isEmpty()) {
        return;
    }
    ui->editInputPipe->appendPlainText(filename);
}

void GrammarEditor::on_btnOutputFind_clicked(bool) {
    QString filename = QFileDialog::getSaveFileName(this, tr("Output To"),
        ui->editOutputPath->text().trimmed().isEmpty() ? cur_file.path() : ui->editOutputPath->text().trimmed(),
        tr("Any File (*.*)"));
    if (filename.isEmpty()) {
        return;
    }
    ui->editOutputPath->setText(filename);
}

void GrammarEditor::on_btnRunProcess_clicked(bool) {
    QTemporaryFile tmpf(QDir(QDir::tempPath()).filePath("cg3ide-XXXXXX-") + QVariant(qrand()).toString());
    if (!tmpf.open()) {
        QMessageBox::critical(this, tr("Creating temporaries failed!"), tr("Failed to create temporary files! Make sure you have write access to the temporary folder."));
        return;
    }
    auto name = tmpf.fileName() + "-params.txt";

    QSettings settings;
    QString params;

    params += QString("binary\t") + settings.value("cg3/binary").toString() + "\n";
    params += QString("grammar\t") + checker.binGrammar + "\n";

    QStringList inputs;
    if (ui->optPipeText->isChecked()) {
        QString name = tmpf.fileName() + "-input.txt";
        tmpf.remove();
        filePutContents(name, ui->editStdin->toPlainText());
        inputs << name;
    }
    if (ui->optPipeFiles->isChecked()) {
        QStringList lines = ui->editInputFiles->toPlainText().split('\n');
        while (!lines.empty()) {
            if (!lines.front().trimmed().isEmpty() && !lines.front().trimmed().startsWith('#')) {
                inputs << lines.front().trimmed();
            }
            lines.pop_front();
        }
    }

    if (!inputs.empty()) {
        params += QString("inputs\t") + inputs.join("|") + "\n";
    }

    auto lines = ui->editInputPipe->toPlainText().split('\n');
    QStringList progs;
    while (!lines.empty()) {
        if (!lines.front().trimmed().isEmpty() && !lines.front().trimmed().startsWith('#')) {
            progs << lines.front().trimmed();
        }
        lines.pop_front();
    }
    if (!progs.empty()) {
        params += QString("pipe\t") + progs.join(" | ") + "\n";
    }

    if (!ui->editOutputPath->text().trimmed().isEmpty()) {
        params += QString("output_file\t") + ui->editOutputPath->text().trimmed() + "\n";
    }
    params += QString("output_split\t") + settings.value("process/output_split", false).toString() + "\n";

    filePutContents(name, params);
    if (!QProcess::startDetached(QDir(QCoreApplication::applicationDirPath()).filePath("cg3processor"), QStringList() << name)) {
        QMessageBox::information(this, tr("Spawning Processor failed!"), tr("Failed to spawn the Processor!"));
    }
}

void GrammarEditor::on_optPipeText_toggled(bool state) {
    QSettings settings;
    settingSetOrDef(settings, "process/pipe_text", true, state);
}

void GrammarEditor::on_optPipeFiles_toggled(bool state) {
    QSettings settings;
    settingSetOrDef(settings, "process/pipe_files", false, state);
}

void GrammarEditor::on_optOutputSplit_toggled(bool state) {
    QSettings settings;
    settingSetOrDef(settings, "process/output_split", false, state);
}

void GrammarEditor::on_actUndo_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    if (pte) {
        pte->undo();
        return;
    }
    auto le = dynamic_cast<QLineEdit*>(w);
    if (le) {
        le->undo();
    }
}

void GrammarEditor::on_actRedo_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    if (pte) {
        pte->redo();
        return;
    }
    auto le = dynamic_cast<QLineEdit*>(w);
    if (le) {
        le->redo();
    }
}

void GrammarEditor::on_actCopy_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    if (pte) {
        QTextCursor tc = pte->textCursor();
        if (tc.selectionStart() == tc.selectionEnd()) {
            tc.select(QTextCursor::BlockUnderCursor);
        }
        QApplication::clipboard()->setText(tc.selectedText());
        return;
    }
    auto le = dynamic_cast<QLineEdit*>(w);
    if (le) {
        if (le->selectionStart() == -1 && le->echoMode() == QLineEdit::Normal) {
            QApplication::clipboard()->setText(le->text());
        }
        else {
            le->copy();
        }
    }
}

void GrammarEditor::on_actCut_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    if (pte) {
        pte->cut();
        return;
    }
    auto le = dynamic_cast<QLineEdit*>(w);
    if (le) {
        le->cut();
    }
}

void GrammarEditor::on_actPaste_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    if (pte) {
        pte->paste();
        return;
    }
    auto le = dynamic_cast<QLineEdit*>(w);
    if (le) {
        le->paste();
    }
}

void GrammarEditor::on_actSelectAll_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    if (pte) {
        pte->selectAll();
        return;
    }
    auto le = dynamic_cast<QLineEdit*>(w);
    if (le) {
        le->selectAll();
    }
}

void GrammarEditor::on_actZoomIn_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    auto le = dynamic_cast<QLineEdit*>(w);
    if (pte || le) {
        QFont f = w->font();
        f.setPointSize(f.pointSize()+1);
        w->setFont(f);
        if (pte) {
            pte->setTabStopWidth(QFontMetrics(f).width('x')*3);
        }
        return;
    }
}

void GrammarEditor::on_actZoomOut_triggered() {
    auto w = QApplication::focusWidget();
    auto pte = dynamic_cast<QPlainTextEdit*>(w);
    auto le = dynamic_cast<QLineEdit*>(w);
    if (pte || le) {
        QFont f = w->font();
        f.setPointSize(std::max(f.pointSize()-1, 1));
        w->setFont(f);
        if (pte) {
            pte->setTabStopWidth(QFontMetrics(f).width('x')*3);
        }
        return;
    }
}

void GrammarEditor::on_actWrapGrammar_toggled(bool state) {
    ui->editGrammar->setWordWrapMode(state ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    QSettings settings;
    settingSetOrDef(settings, "editor/wrapgrammar", true, state);
}

void GrammarEditor::on_actWrapIO_toggled(bool state) {
    auto wmode = state ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap;
    ui->editStdin->setWordWrapMode(wmode);
    ui->editStdinPreview->setWordWrapMode(wmode);
    ui->editStdout->setWordWrapMode(wmode);
    QSettings settings;
    settingSetOrDef(settings, "editor/wrapio", false, state);
}

void GrammarEditor::on_actRestoreDocks_triggered() {
    ui->dockDebug->show();
    ui->dockInput->show();
    ui->dockOutput->show();
}

void GrammarEditor::on_btnRunPreviewIn_clicked(bool) {
    previewIn_dirty = true;
    previewIn_run = true;
    refreshInput();
}

void GrammarEditor::on_btnRunPreviewOut_clicked(bool) {
    previewOut_run = true;
    checkGrammar();
}

void GrammarEditor::on_btnOutputOptions_clicked(bool) {
    if (ui->frameOutputOptions->isVisible()) {
        ui->frameOutputOptions->hide();
    }
    else {
        ui->frameOutputOptions->show();
    }
}

void GrammarEditor::on_tableErrors_clicked(const QModelIndex& item) {
    editorGotoLine(ui->editGrammar, item.sibling(item.row(), 0).data().toInt()-1);
}

void GrammarEditor::on_optFindRegex_toggled(bool state) {
    QSettings settings;
    settingSetOrDef(settings, "editor/find_regex", false, state);
    on_editFind_textEdited();
}

void GrammarEditor::on_optFindCase_toggled(bool state) {
    QSettings settings;
    settingSetOrDef(settings, "editor/find_case", false, state);
    on_editFind_textEdited();
}

void GrammarEditor::on_optHideTags_toggled(bool state) {
    QSettings settings;
    settingSetOrDef(settings, "editor/hidetags", true, state);
    previewOutRun_render();
}

void GrammarEditor::on_optHideRemoved_toggled(bool state) {
    QSettings settings;
    settingSetOrDef(settings, "editor/hideremoved", true, state);
    previewOutRun_render();
}

void GrammarEditor::on_editStdin_textChanged() {
    previewIn_dirty = true;
    //on_editGrammar_textChanged();
}

void GrammarEditor::on_editInputFiles_textChanged() {
    on_editStdin_textChanged();
}

void GrammarEditor::on_editInputPipe_textChanged() {
    on_editStdin_textChanged();
}

void GrammarEditor::on_editGrammar_modificationChanged(bool) {
    reTitle();
}

void GrammarEditor::on_editGrammar_textChanged() {
    QSettings settings;
    auto curGrammar = ui->editGrammar->toPlainText();
    if (lastGrammar != curGrammar && (settings.value("cg3/checkgrammar", true).toBool() || settings.value("cg3/previewoutput", true).toBool())) {
        check_timer->stop();
        check_timer->setSingleShot(true);
        check_timer->start(settings.value("cg3/livedelay", 2000).toInt());
        lastGrammar = ui->editGrammar->toPlainText();
    }
}

void GrammarEditor::on_editGrammar_blockCountChanged(int) {
    errorSelections.clear();
    errorEntries.clear();
    on_editFind_textEdited();

    hilite_timer->stop();
    hilite_timer->setSingleShot(true);
    hilite_timer->start(2000);
}

void GrammarEditor::on_editGrammar_cursorPositionChanged() {
    static auto otc = ui->editGrammar->cursorForPosition(QPoint(0,0));
    const auto& ntc = ui->editGrammar->cursorForPosition(QPoint(0,0));
    if (otc != ntc) {
        otc = ntc;
        on_editFind_textEdited();
    }

    auto extraSelections = errorSelections;

    QTextEdit::ExtraSelection selection;
    auto lineColor = QColor(Qt::yellow).lighter(180);

    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = ui->editGrammar->textCursor();
    selection.cursor.clearSelection();
    selection.cursor.movePosition(QTextCursor::StartOfBlock);
    selection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    selection.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    //selection.cursor.select(QTextCursor::BlockUnderCursor); // Almost but not quite what we really want
    extraSelections.append(selection);

    if (ui->frameFindReplace->isVisible()) {
        extraSelections.append(findSelections);
    }

    ui->editGrammar->setExtraSelections(extraSelections);
    ui->statusGrammar->showMessage(
                tr("Line %1 of %2 :: Column %3 :: Character %4 of %5")
               .arg(ui->editGrammar->textCursor().blockNumber()+1)
               .arg(ui->editGrammar->blockCount())
               .arg(ui->editGrammar->textCursor().positionInBlock()+1)
               .arg(ui->editGrammar->textCursor().position()+1)
               .arg(ui->editGrammar->document()->characterCount())
               );
}

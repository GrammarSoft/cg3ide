/*
* Copyright (C) 2013, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com> for GrammarSoft ApS (http://grammarsoft.com/)
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

#pragma once
#ifndef GRAMMAREDITOR_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define GRAMMAREDITOR_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include "types.hpp"
#include "StreamHighlighter.hpp"
#include "GrammarHighlighter.hpp"
#include "OptionsDialog.hpp"
#include <QtWidgets>

namespace Ui {
class GrammarEditor;
}

class GrammarEditor : public QMainWindow {
    Q_OBJECT
    
public:
    explicit GrammarEditor(QWidget *parent = 0);
    ~GrammarEditor();

    void reTitle();
    void reOptions();
    bool save(const QString& filename);
    void open(const QString& filename);

    bool eventFilter(QObject *watched, QEvent *event);

public slots:
    void refreshInput();
    void checkGrammar();
    void previewRun();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_actAbout_triggered();
    void on_actHelp_triggered();

    void reHilite();

    void checkGrammar_finished(int);
    void previewOutRun_finished(int);
    void previewOutRun_render();
    void on_actNew_triggered();
    void on_actClose_triggered();
    void on_actOpen_triggered();
    bool on_actSave_triggered();
    bool on_actSaveas_triggered();
    void on_actGotoLine_triggered();
    void on_actOptions_triggered();

    void on_actFindHide_triggered();
    void on_actFindReplace_triggered();
    void on_actFindNext_triggered();
    void on_actFindPrev_triggered();
    void on_actReplaceOnce_triggered();
    void on_actReplaceAll_triggered();
    void on_optFindRegex_toggled(bool);
    void on_optFindCase_toggled(bool);
    void on_editFind_returnPressed();
    void on_editFind_textEdited();

    void scrollValue_Changed(int);
    void sectionJump_Activated(int);

    void on_btnFileInAdd_clicked(bool);
    void on_btnPipeFind_clicked(bool);
    void on_btnOutputFind_clicked(bool);
    void on_btnRunProcess_clicked(bool);
    void on_optPipeText_toggled(bool);
    void on_optPipeFiles_toggled(bool);
    void on_optOutputSplit_toggled(bool);

    void on_actUndo_triggered();
    void on_actRedo_triggered();
    void on_actCopy_triggered();
    void on_actCut_triggered();
    void on_actPaste_triggered();
    void on_actSelectAll_triggered();
    void on_actZoomIn_triggered();
    void on_actZoomOut_triggered();

    void on_actWrapGrammar_toggled(bool);
    void on_actWrapIO_toggled(bool);
    void on_actRestoreDocks_triggered();
    void on_btnRunPreviewIn_clicked(bool);
    void on_btnRunPreviewOut_clicked(bool);
    void on_btnOutputOptions_clicked(bool);
    void on_tableErrors_clicked(const QModelIndex&);
    void on_optHideTags_toggled(bool);
    void on_optHideRemoved_toggled(bool);
    void on_editStdin_textChanged();
    void on_editInputFiles_textChanged();
    void on_editInputPipe_textChanged();
    void on_editGrammar_modificationChanged(bool);
    void on_editGrammar_textChanged();
    void on_editGrammar_blockCountChanged(int);
    void on_editGrammar_cursorPositionChanged();

public:
    QScopedPointer<Ui::GrammarEditor> ui;
    QScopedPointer<GrammarHighlighter> stxGrammar;

private:
    QString defGrammar;
    QFileInfo cur_file;
    QScopedPointer<QTimer> check_timer;
    QScopedPointer<QTimer> hilite_timer;
    CGChecker checker;
    QList<QTextEdit::ExtraSelection> errorSelections, findSelections;
    QStandardItemModel errorEntries;
    QScopedPointer<StreamHighlighter> stxInput, stxInputPreview, stxOutput;
    QRegExp rxTrace, rxReading, rxReading2;
    QString stdout_raw;
    QComboBox *section_jump;
    bool previewIn_dirty, previewIn_run;
    bool previewOut_run;
    bool cur_file_check;
};

#endif // GRAMMAREDITOR_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

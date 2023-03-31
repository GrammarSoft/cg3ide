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

#include "GotoLine.hpp"
#include "ui_GotoLine.h"
#include "inlines.hpp"

GotoLine::GotoLine(QWidget *parent, QPlainTextEdit *editor) :
    QDialog(parent),
    ui(new Ui::GotoLine),
    editor(editor)
{
    ui->setupUi(this);
    setWindowModality(Qt::WindowModal);
    ui->editGotoLine->setText(QVariant(editor->textCursor().blockNumber()+1).toString());
    ui->editGotoLine->selectAll();
}

GotoLine::~GotoLine() {
}

void GotoLine::accept() {
    editorGotoLine(editor, ui->editGotoLine->text().toInt()-1);
    close();
}

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

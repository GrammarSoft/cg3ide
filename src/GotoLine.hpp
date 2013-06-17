#pragma once
#ifndef GOTOLINE_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define GOTOLINE_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include <QtWidgets>

namespace Ui {
class GotoLine;
}

class GotoLine : public QDialog {
    Q_OBJECT
    
public:
    explicit GotoLine(QWidget *parent = 0, QPlainTextEdit *editor = 0);
    ~GotoLine();

private slots:
    void accept();
    
private:
    QScopedPointer<Ui::GotoLine> ui;
    QPlainTextEdit *editor;
};

#endif // GOTOLINE_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
